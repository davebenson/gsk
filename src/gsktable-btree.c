/* Implementation of gsk_table_file_factory_new_btree() */
#include "gsktable-mmap.h"

struct _BtreeFactory
{
  GskTableFileFactory base_instance;
  guint branches_per_level;
  guint values_per_leaf;
};


typedef struct _BtreeFile BtreeFile;
struct _BtreeFile
{
  GskTableFile base_instance;
  const char *dir;
  guint64 id;

  guint height;
  guint cur_level;

  guint64 cur_level_start_offset;

  guint *level_indices;         /* only used during leaf level processing */

  /* for non-leaf levels, this is the offset of the last
     child btree node, relative to the starting offset for that level */
  guint64 last_child_node_offset;

  /* Output the finished tree.
     We will have to go back and setup some metadata. */
  GskTableMmapWriter writer;

  /* Commands are written here, and eventually read out again. */
  GskTableMmapPipe buffer;

  /* we must maintain a little buffer while writing the leaf
     nodes, since we need to have some warning before the end-of-file */
  guint first_buffered_data;
  guint n_buffered_data;                /* max buffered is 'height' */
  MyData *buffered_data;
};

enum
{
  MSG__CHILD_NODE=42,   /* params: length as uint32le */
  MSG__BRANCH_VALUE,    /* params: level as byte, value as remainder of data */
  MSG__BRANCH_ENDED,    /* params: level as byte */
  MSG__LEVEL_ENDED      /* params: none */
} MsgType;


static GskTableFile *
btree__create_file      (GskTableFileFactory      *factory,
                         const char               *dir,
                         guint64                   id,
                         const GskTableFileHints  *hints,
                         GError                  **error)
{
  BtreeFile *rv = g_new (BtreeFile, 1);
  BtreeFileComputeStatsInfo *info = &rv->info.compute_stats;
  guint64 cur_file_size;
  char fname_buf[GSK_TABLE_MAX_PATH];

  rv->dir = GSK_TABLE_FILE_DIR_MAYBE_STRDUP (dir);
  rv->id = id;
  gsk_table_mk_fname (fname_buf, dir, id, "btree");
  fd = open (fname_buf, O_RDWR|O_CREAT|O_TRUNC, 0644);
  if (hints->allocate_disk_space_based_on_max_sizes)
    {
      cur_file_size = (hints->max_entries << 3)
                    + hints->max_key_bytes
                    + hints->max_value_bytes;
      ftruncate (fd, n_bytes);
    }
  else
    cur_file_size = 0;
  gsk_table_mmap_writer_init (&info->writer, fd, cur_file_size);

  gsk_table_mk_fname (fname_buf, dir, id, "buffer");
  fd = open (fname_buf, O_RDWR|O_CREAT|O_TRUNC, 0644);
  gsk_table_mmap_pipe_init (&info->buffer, fd, 0, 0);

  rv->factory = factory;
  return &rv->base_instance;
}

static GskTableFile *
btree__open_building_file (GskTableFileFactory     *factory,
                           const char               *dir,
                           guint64                   id,
                           guint                     state_len,
                           const guint8             *state_data,
                           GError                  **error)
{
  ...
}

static GskTableFile *
btree__open_file          (GskTableFileFactory      *factory,
                           const char               *dir,
                           guint64                   id,
                           GError                  **error)
{
  ...
}


/* methods for a file which is being built */
static gboolean
btree__feed_entry       (GskTableFile             *file,
                         guint                     real_key_len,
                         const guint8             *real_key_data,
                         guint                     real_value_len,
                         const guint8             *real_value_data,
                         GError                  **error)
{
  BtreeFile *f = (BtreeFile *) file;

  /* NOTE: we don't generally want to actually process the "real"
     data, instead we put it in our circular buffer,
     after processing data that is exactly "height" elements old.
     The extra elements are used to finish up the tree. */

  if (f->n_buffered_data == f->height)
    {
      guint level;
      MyData *d = &f->buffered_data[f->first_buffered_data];
      for (level = 0; level + 1 < file->height; level++)
        if (f->level_indices[level] % 2 == 0)
          break;
      if (level + 1 < file->height)
        {
          /* defer this data for the next pass */
          emit_branch_value (f, level, d);
        }
      else
        {
          /* add this data to current leaf node */
          add_data_to_child_node (f, d);
        }

      if (level + 1 == file->height)
        {
          if (f->level_indices[level] == f->values_per_leaf)
            {
              /* emit child node */
              emit_child_node (f);

              f->level_indices[level] = 0;
              level--;
            }
          else
            {
              f->level_indices[level] += 1;
              goto done_incrementing_array;
            }
        }
      for (;;)
        {
          f->level_indices[level] += 1;
          if (level == 0)
            break;
          if (f->level_indices[level] == f->branches_per_level * 2 + 1)
            {
              /* emit branch-end message for level */
              emit_branch_ended (f, level);

              f->level_indices[level] = 0;
              level--;
            }
        }

      /* replace oldest element with real data */
      if (G_UNLIKELY (d->alloced < real_key_len + real_value_len))
        {
          guint new_alloced = d->alloced * 2;
          while (new_alloced < real_key_len + real_value_len)
            new_alloced *= 2;
          g_free (d->data);
          d->data = g_malloc (new_alloced);
        }
      d->key_len = real_key_len;
      d->value_len = real_value_len;
      memcpy (d->data, real_key_data, real_key_len);
      memcpy (d->data + real_key_len, real_value_data, real_value_len);

      f->first_buffered_data += 1;
      if (f->first_buffered_data == f->height)
        f->first_buffered_data = 0;
    }
  else
    {
      /* add new data into buffer */
      MyData *d = &f->buffered_data[f->n_buffered_data];
      guint alloced = 16;
      while (alloced < real_key_len + real_value_len)
        alloced *= 2;

      d->key_len = real_key_len;
      d->value_len = real_value_len;
      d->alloced = alloced;
      d->data = g_malloc (alloced);
      memcpy (d->data, real_key_data, real_key_len);
      memcpy (d->data + real_key_len, real_value_data, real_value_len);

      f->n_buffered_data++;
    }
}

static void
update_level_header (BtreeFile *f)
{
  guint64 off = f->cur_level_start_offset;
  guint64 len = gsk_table_mmap_writer_offset (&f->writer) - off;
  guint64 loc_le[2];
  loc_le[0] = GUINT64_TO_LE (off);
  loc_le[1] = GUINT64_TO_LE (len);
  g_assert (BTREE_HEADER_LEVEL_SIZE == sizeof (loc_le));
  gsk_table_mmap_writer_pwrite (f->writer,
                                BTREE_HEADER_SIZE
                                + BTREE_HEADER_LEVEL_SIZE * f->cur_level,
                                BTREE_HEADER_LEVEL_SIZE,
                                (const guint8 *) loc_le);
}
static gboolean
btree__done_feeding     (GskTableFile             *file,
                         gboolean                 *ready_out,
                         GError                  **error)
{
  BtreeFile *f = (BtreeFile *) file;
  guint i;
  if (f->level_indices[0] == 0)
    {
      /* a short tree */
      ...
      *ready_out = TRUE;

      /* free buffer */
      for (i = 0; i < f->n_buffered_data; i++)
        g_free (f->buffered_data[i].data);
      g_free (f->buffered_data);
      f->buffered_data = NULL;

      return TRUE;
    }
  else
    {
      guint level = 0;
      while (level < f->height - 1)
        if (f->level_indices[level] % 2 == 0)
          break;
      if (G_LIKELY (level == f->height - 1))
        {
          /* add one element to leaf node */
          add_data_to_child_node (f, f->buffered_data + f->first_buffered_data);

          /* emit child node */
          emit_child_node (f);

          /* update header to reflect level's offset and length */
          update_level_header (f);

          f->first_buffered_data++;
          if (f->first_buffered_data == f->height)
            f = 0;
          f->n_buffered_data--;
          level--;
        }
      while (level > 0)
        {
          MyData *d = &f->buffered_data[f->first_buffered_data];
          emit_branch_value (f, level, d);
          emit_branch_ended (f, level);

          f->first_buffered_data++;
          if (f->first_buffered_data == f->height)
            f = 0;
          g_assert (f->n_buffered_data > 0);
          f->n_buffered_data--;
          level--;
        }

      /* toss extra elements into level 0 */
      while (f->n_buffered_data > 0)
        {
          ...
        }

      if (f->cur_level == 0)
        {
          *ready_out = TRUE;

          /* finishing touches? */
          ...
        }
      else
        {
          emit_level_ended (f);
          f->cur_level--;
        }
    }

  for (i = 0; i < f->height; i++)
    g_free (f->buffered_data[i].data);
  g_free (f->buffered_data);
  f->buffered_data = NULL;

  return TRUE;
}

static gboolean
btree__get_build_state  (GskTableFile             *file,
                         guint                    *state_len_out,
                         guint8                  **state_data_out,
                         GError                  **error)
{
  ...
}

static gboolean
btree__build_file       (GskTableFile             *file,
                         gboolean                 *ready_out,
                         GError                  **error)
{
  guint i;
  for (i = 0; i < N_MESSAGES_PER_BUILD_FILE; i++)
    {
      guint32 msg_le;
      const guint8 *msg_data;
      if (G_UNLIKELY (!gsk_table_mmap_pipe_read (&f->pipe, 1, &msg_data)))
        {
          g_set_error (...);
          return FALSE;
        }
      switch (msg_data[0])
        {
        case MSG__CHILD_NODE:
          /* write subtree reference into branch */
          ...
          break;

        case MSG__BRANCH_VALUE:
          ...
          break;
        case MSG__BRANCH_ENDED:
          ...
          break;
        case MSG__LEVEL_ENDED:
          ...
          if (level == 0)
            goto finished_level0;
          break;

        default:
          g_set_error (...);
          return FALSE;
        }
    }
  *ready_out = FALSE;
  return TRUE;

finished_level0:

  /* convert writer to reader; close pipe */
  ...

  *ready_out = TRUE;
  return TRUE;
}

static void
btree__release_build_data(GskTableFile            *file)
{
  BtreeFile *f = (BtreeFile *) file;
  char fname_buf[GSK_TABLE_MAX_PATH];

  /* unlink pipe */
  gsk_table_mk_fname (fname_buf, f->dir, f->id, "buffer");
  unlink (fname_buf);
}

/* methods for a file which has been constructed */
static gboolean
btree__query_file       (GskTableFile             *file,
                         GskTableFileQuery        *query_inout,
                         GError                  **error)
{
  ...
}

static GskTableReader *
btree__create_reader    (GskTableFile             *file,
                         GError                  **error)
{
  ...
}

static gboolean
btree__get_reader_state (GskTableFile             *file,
                         GskTableReader           *reader,
                         guint                    *state_len_out,
                         guint8                  **state_data_out,
                         GError                  **error)
{
  ...
}

static GskTableReader *
btree__recreate_reader  (GskTableFile             *file,
                         guint                     state_len,
                         const guint8             *state_data,
                         GError                  **error)
{
  ...
}

/* destroying files and factories */
static gboolean
btree__destroy_file     (GskTableFile             *file,
                         gboolean                  erase,
                         GError                  **error)
{
  BtreeFile *f = (BtreeFile *) file;
  ...
  GSK_TABLE_FILE_DIR_MAYBE_FREE (f->dir);
}

static void
btree__destroy_factory  (GskTableFileFactory      *factory)
{
  /* no-op for static factory */
}


/* for now, return a static factory object */
GskTableFileFactory *gsk_table_file_factory_new_btree (void)
{
  static BtreeFactory the_factory =
    {
      {
        btree__create_file,
        btree__open_building_file,
        btree__open_file,
        btree__feed_entry,
        btree__done_feeding,
        btree__get_build_state,
        btree__build_file,
        btree__release_build_data,
        btree__query_file,
        btree__create_reader,
        btree__get_reader_state,
        btree__recreate_reader,
        btree__destroy_file,
        btree__destroy_factory
      },
      16,               /* branches_per_level */
      32                /* values_per_leaf */
    };

  return &the_factory.base_instance;
}
