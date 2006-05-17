#include "gskzlibinflator.h"
#include <zlib.h>
#include "gskzlib.h"

/* TODO: add raw_read_buffer! */

static GObjectClass *parent_class = NULL;

#define MAX_BUFFER_SIZE	4096

static guint
gsk_zlib_inflator_raw_read      (GskStream     *stream,
			 	 gpointer       data,
			 	 guint          length,
			 	 GError       **error)
{
  GskZlibInflator *zlib_inflator = GSK_ZLIB_INFLATOR (stream);
  guint rv = gsk_buffer_read (&zlib_inflator->decompressed, data, length);

  if (!gsk_io_get_is_writable (zlib_inflator))
    {
       if (zlib_inflator->decompressed.size == 0)
	 gsk_io_notify_read_shutdown (zlib_inflator);
    }
  else
    {
      if (zlib_inflator->decompressed.size < MAX_BUFFER_SIZE)
	gsk_io_mark_idle_notify_write (zlib_inflator);
      if (zlib_inflator->decompressed.size == 0)
	gsk_io_clear_idle_notify_read (zlib_inflator);
    }

  return rv;
}

static gboolean
do_sync (GskZlibInflator *zlib_inflator, GError **error)
{
  z_stream *zst = zlib_inflator->private_stream;
  guint8 buf[4096];
  int rv;
  if (zst == NULL)
    return TRUE;

  zst->next_in = NULL;
  zst->avail_in = 0;
  do
    {
      /* Set up output location */
      zst->next_out = buf;
      zst->avail_out = sizeof (buf);

      /* Decompress */
      rv = inflate (zst, Z_SYNC_FLUSH);
      if (rv == Z_OK || rv == Z_STREAM_END)
	gsk_buffer_append (&zlib_inflator->decompressed, buf, zst->next_out - buf);
    }
  while (rv == Z_OK && zst->avail_out == 0);
  if (rv != Z_OK && rv != Z_STREAM_END)
    {
      GskErrorCode zerror_code = gsk_zlib_error_to_gsk_error (rv);
      const char *zmsg = gsk_zlib_error_to_message (rv);
      g_set_error (error, GSK_G_ERROR_DOMAIN, zerror_code,
		   "could not inflate: %s", zmsg);
      return FALSE;
    }
  return TRUE;
}

static gboolean
gsk_zlib_inflator_shutdown_write (GskIO      *io,
				  GError    **error)
{
  GskZlibInflator *zlib_inflator = GSK_ZLIB_INFLATOR (io);
  if (! do_sync (GSK_ZLIB_INFLATOR (io), error))
    return FALSE;
  if (zlib_inflator->decompressed.size == 0)
    gsk_io_notify_read_shutdown (io);
  else
    gsk_io_mark_idle_notify_write (io);
  return TRUE;
}

static guint
gsk_zlib_inflator_raw_write     (GskStream     *stream,
			 	 gconstpointer  data,
			 	 guint          length,
			 	 GError       **error)
{
  GskZlibInflator *zlib_inflator = GSK_ZLIB_INFLATOR (stream);
  z_stream *zst;
  guint8 buf[4096];
  int rv;
  if (zlib_inflator->private_stream == NULL)
    {
      zst = g_new (z_stream, 1);
      zlib_inflator->private_stream = zst;
      zst->next_in = (gpointer) data;
      zst->avail_in = length;
      zst->zalloc = NULL;
      zst->zfree = NULL;
      zst->opaque = NULL;
      inflateInit (zst);
    }
  else
    {
      zst = zlib_inflator->private_stream;
      zst->next_in = (gpointer) data;
      zst->avail_in = length;
    }

  do
    {
      /* Set up output location */
      zst->next_out = buf;
      zst->avail_out = sizeof (buf);

      /* Decompress */
      rv = inflate (zst, Z_NO_FLUSH);
      if (rv == Z_OK || rv == Z_STREAM_END)
	gsk_buffer_append (&zlib_inflator->decompressed, buf, zst->next_out - buf);
    }
  while (rv == Z_OK && zst->avail_in > 0);
  if (rv != Z_OK && rv != Z_STREAM_END)
    {
      GskErrorCode zerror_code = gsk_zlib_error_to_gsk_error (rv);
      const char *zmsg = gsk_zlib_error_to_message (rv);
      g_set_error (error, GSK_G_ERROR_DOMAIN, zerror_code,
		   "could not inflate: %s", zmsg);
    }
  if (zlib_inflator->decompressed.size > MAX_BUFFER_SIZE)
    gsk_io_clear_idle_notify_write (zlib_inflator);
  if (zlib_inflator->decompressed.size > 0)
    gsk_io_mark_idle_notify_read (zlib_inflator);

  return length - zst->avail_in;
}

static void
gsk_zlib_inflator_finalize     (GObject *object)
{
  GskZlibInflator *inflator = GSK_ZLIB_INFLATOR (object);
  if (inflator->private_stream)
    {
      inflateEnd (inflator->private_stream);
      g_free (inflator->private_stream);
    }
  gsk_buffer_destruct (&inflator->decompressed);
  (*parent_class->finalize) (object);
}

/* --- functions --- */
static void
gsk_zlib_inflator_init (GskZlibInflator *zlib_inflator)
{
  gsk_io_mark_is_readable (zlib_inflator);
  gsk_io_mark_is_writable (zlib_inflator);
}

static void
gsk_zlib_inflator_class_init (GskZlibInflatorClass *class)
{
  GskIOClass *io_class = GSK_IO_CLASS (class);
  GskStreamClass *stream_class = GSK_STREAM_CLASS (class);
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  parent_class = g_type_class_peek_parent (class);
  stream_class->raw_read = gsk_zlib_inflator_raw_read;
  stream_class->raw_write = gsk_zlib_inflator_raw_write;
  io_class->shutdown_write = gsk_zlib_inflator_shutdown_write;
  object_class->finalize = gsk_zlib_inflator_finalize;
}

GType gsk_zlib_inflator_get_type()
{
  static GType zlib_inflator_type = 0;
  if (!zlib_inflator_type)
    {
      static const GTypeInfo zlib_inflator_info =
      {
	sizeof(GskZlibInflatorClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gsk_zlib_inflator_class_init,
	NULL,		/* class_finalize */
	NULL,		/* class_data */
	sizeof (GskZlibInflator),
	0,		/* n_preallocs */
	(GInstanceInitFunc) gsk_zlib_inflator_init,
	NULL		/* value_table */
      };
      zlib_inflator_type = g_type_register_static (GSK_TYPE_STREAM,
                                                  "GskZlibInflator",
						  &zlib_inflator_info, 0);
    }
  return zlib_inflator_type;
}

/**
 * gsk_zlib_inflator_new:
 *
 * Create a new zlib inflator: this takes deflated (compressed) input
 * which is written into it, and uncompressed data can be read from it.
 *
 * returns: the newly allocated stream.
 */
GskStream *
gsk_zlib_inflator_new (void)
{
  return g_object_new (GSK_TYPE_ZLIB_INFLATOR, NULL);
}
