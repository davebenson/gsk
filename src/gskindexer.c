#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include "gskqsortmacro.h"
#include "gskindexer.h"

#define MAX_INDEXER_FILES    64
#define MAX_IN_MEMORY        2048
#define MAX_DIR_RETRIES      100

typedef struct _InMemoryData InMemoryData;
struct _InMemoryData
{
  unsigned orig_index;
  GByteArray *array;
};
struct _GskIndexer
{
  char *dir;
  guint64 files[MAX_INDEXER_FILES];

  unsigned n_in_memory;
  InMemoryData in_memory_data[MAX_IN_MEMORY];

  guint64 next_file_id;

  GskIndexerCompareFunc compare;
  GskIndexerMergeFunc merge;
  gpointer user_data;

  GByteArray *tmp_pad;
};
  

GskIndexer       *gsk_indexer_new         (GskIndexerCompareFunc compare,
                                           GskIndexerMergeFunc   merge,
			                   void                 *user_data)
{
  /* make tmp dir */
  char buf[1024];
  unsigned ct = 1;
  unsigned i;
  GskIndexer *indexer;
  while (ct < MAX_DIR_RETRIES)
    {
      g_snprintf (buf, sizeof (buf), "/tmp/gskidx-%u-%05u",
                  (unsigned)time(NULL), ct++);
      if (mkdir (buf, 0755) == 0)
        break;
    }
  if (ct == MAX_DIR_RETRIES)
    return NULL;

  indexer = g_new (GskIndexer, 1);
  indexer->dir = g_strdup (buf);
  for (i = 0; i < MAX_INDEXER_FILES; i++)
    indexer->files[i] = 0;
  indexer->n_in_memory = 0;
  for (i = 0; i < MAX_IN_MEMORY; i++)
    indexer->in_memory_data[i].array = g_byte_array_new ();
  indexer->next_file_id = 1;
  indexer->tmp_pad = g_byte_array_new ();
  return indexer;
}

static guint64
merge_files (GskIndexer *indexer,
             guint64     old,
             guint64     new)
{
  /* open readers */
  mk_filename (buf, indexer, old);
  reader_a = fopen (buf, "rb");
  if (reader_a == NULL)
    g_error ("error opening %s: %s", buf, g_strerror (errno));
  mk_filename (buf, indexer, new);
  reader_b = fopen (buf, "rb");
  if (reader_b == NULL)
    g_error ("error opening %s: %s", buf, g_strerror (errno));
  ...

  /* open writer */
  id = indexer->next_file_id++;
  mk_filename (buf, indexer, id);
  ...

  /* merge */
  ...

  fclose (reader_a);
  fclose (reader_b);
  fclose (writer);
  return id;
}

static void
flush_in_memory_data (GskIndexer *indexer)
{
  guint64 fno;
  FILE *fp;
  char buf[1024];
  InMemoryData *imd = indexer->in_memory_data;
  unsigned n_imd = indexer->n_in_memory;
  if (indexer->n_in_memory == 0)
    return;
  
  /* qsort */
#define COMPARE_IN_MEMORY_DATA(a,b, rv)                 \
  rv = indexer->compare (a.array->len, a.array->data, \
                         b.array->len, b.array->data, \
                         indexer->user_data);           \
  if (rv == 0)                                          \
    {                                                   \
      if (a.orig_index < b.orig_index) rv = -1;       \
      else if (a.orig_index > b.orig_index) rv = 1;   \
    }
  GSK_QSORT (imd, InMemoryData, n_imd, COMPARE_IN_MEMORY_DATA);
#undef COMPARE_IN_MEMORY_DATA

  if (indexer->merge)
    {
      /* merge */
      unsigned n_output = 1;
      unsigned i;
      for (i = 1; i < n_imd; i++)
        {
          if (n_output > 0
           && indexer->compare (imd[n_output-1].array->len,
                                imd[n_output-1].array->data,
                                imd[i].array->len,
                                imd[i].array->data,
                                indexer->user_data) == 0)
            {
              switch (indexer->merge (imd[n_output-1].array->len,
                                      imd[n_output-1].array->data,
                                      imd[i].array->len,
                                      imd[i].array->data,
                                      indexer->tmp_pad,
                                      indexer->user_data))
                {
                case GSK_INDEXER_MERGE_RETURN_A:
                  break;
                case GSK_INDEXER_MERGE_RETURN_B:
                  {
                    GByteArray *tmp = imd[n_output-1].array;
                    imd[n_output-1].array = imd[i].array;
                    imd[i].array = tmp;
                  }
                  break;
                case GSK_INDEXER_MERGE_IN_PAD:
                  {
                    GByteArray *tmp = imd[n_output-1].array;
                    imd[n_output-1].array = indexer->tmp_pad;
                    indexer->tmp_pad = tmp;
                    break;
                  }
                case GSK_INDEXER_MERGE_DISCARD:
                  n_output--;
                  break;
                }
            }
          else
            imd[n_output++] = imd[i];
        }

      /* no point to write out 0 length file */
      if (n_output == 0)
        {
          indexer->n_in_memory = 0;
          return;
        }
    }
  fno = indexer->next_file_id++;
  g_snprintf (buf, sizeof (buf), "%s/%llx", indexer->dir, fno);
  fp = fopen (buf, "wb");
  if (fp == NULL)
    g_error ("error creating %s: %s", buf, g_strerror (errno));
  unsigned i;
  for (i = 0; i < indexer->n_in_memory; i++)
    {
      guint32 length = indexer->in_memory_data[i].array->len;
      if (fwrite (&length, 4, 1, fp) != 1
       || fwrite (indexer->in_memory_data[i].array->data,
                  indexer->in_memory_data[i].array->len,
		  1, fp) != 1)
        g_error ("error writing file: %s", g_strerror (errno));
    }
  fclose (fp);

  i = 0;
  for (i = 0; i < MAX_INDEXER_FILES && indexer->files[i] != 0; i++)
    {
      /* merge indexer->files[i] (older) and fno */
      guint64 old_id = indexer->files[i];
      guint64 id = merge_files (indexer, old_id, fno);
      indexer->files[i] = 0;
      unlink_file (indexer, old_id);
      unlink_file (indexer, fno);
      fno = id;
    }
  g_assert (i < MAX_INDEXER_FILES);
  indexer->files[i] = fno;
  indexer->n_in_memory = 0;
}

void              gsk_indexer_add         (GskIndexer           *indexer,
                                           unsigned              length,
					   const guint8         *data)
{
  unsigned old_n = indexer->n_in_memory;
  GByteArray *a = indexer->in_memory_data[old_n].array;
  g_byte_array_set_size (a, length);
  memcpy (a->data, data, length);
  indexer->in_memory_data[old_n].orig_index = old_n;
  if (++(indexer->n_in_memory) == MAX_IN_MEMORY)
    {
      /* flush file */
      flush_in_memory_data (indexer);
    }
} 

struct _GskIndexerReader
{
  FILE *fp;
  GByteArray *data;
};

GskIndexerReader *gsk_indexer_make_reader (GskIndexer           *indexer)
{
  flush_in_memory_data (indexer);

  for (i = 0; i < MAX_INDEXER_FILES; i++)
    {
      if (indexer->files[i])
        {
          if (last_file_index >= 0)
            {
              /* merge */
              guint64 id = merge_files (indexer,
                                        indexer->files[i],
                                        indexer->files[last_file_index]);
              unlink_file (indexer, indexer->files[i]);
              unlink_file (indexer, indexer->files[last_file_index]);
              indexer->files[last_file_index] = 0;
              indexer->files[i] = id;
              last_file_index = i;
            }
          else
            last_file_index = i;
        }
    }
  pad = g_byte_array_new ();
  if (last_file_index == -1)
    {
      fp = fopen ("/dev/null", "rb");
    }
  else
    {
      char buf[1024];
      g_snprintf (buf, sizeof (buf), "%s/%llx",
                  indexer->dir,
                  indexer->files[last_file_index]);
      fp = fopen (buf, "rb");
      if (fp == NULL)
        g_error ("error creating indexer reader: %s", g_strerror (errno));

      /* peek record */
      if (fread (&len, 4, 1, fp) == 1)
        {
          g_byte_array_set_size (pad, len);
          if (fread (pad->data, pad->len, 1, fp) != 1)
            {
              ...
            }
        }
      else if (feof (fp))
        {
          fclose (fp);
          fp = NULL;
        }
      else if (ferror (fp))
        {
          g_error ("error reading file");
        }
      else
        g_assert_not_reached ();
    }
  reader = g_new (GskIndexerReader, 1);
  reader->fp = fp;
  reader->pad = pad;
  return reader;
}
  
  /* merge all existing pieces together */
void              gsk_indexer_destroy     (GskIndexer           *indexer);

gboolean      gsk_indexer_reader_has_data (GskIndexerReader *reader)
{
  return reader->fp != NULL;
}
gboolean      gsk_indexer_reader_peek_data(GskIndexerReader *reader,
                                           unsigned         *len_out,
                                           const guint8    **data_out)
{
  if (reader->fp == NULL)
    return FALSE;
  *len_out = reader->pad->len;
  *data_out = reader->pad->data;
  return TRUE;
}
gboolean      gsk_indexer_reader_advance  (GskIndexerReader *reader)
{
  if (reader->fp == NULL)
    return FALSE;
  if (fread (&len, 4, 1, reader->fp) == 1)
    {
      g_byte_array_set_size (reader->pad, len);
      if (fread (reader->pad->data, reader->pad->len, 1, reader->fp) != 1)
        {
          g_error ("partial record");
        }
      return TRUE;
    }
  else if (feof (reader->fp))
    {
      fclose (reader->fp);
      reader->fp = NULL;
      return FALSE;
    }
  else if (ferror (reader->fp))
    g_error ("error reading file");
  else
    g_assert_not_reached ();
}


void          gsk_indexer_reader_destroy  (GskIndexerReader *reader)
{
  if (reader->fp)
    fclose (reader->fp);
  g_byte_array_free (reader->pad, TRUE);
  g_free (reader);
}


