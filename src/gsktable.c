#include "gsktable.h"

/* Overall structure.
 *
 * This database maps a key to a value,
 * with merging semantics provided
 * by a user-configurable function-pointer if
 * the key already exists.
 *
 * Support for deletion is provided by a "FinalMerge" function
 * which can delete entries which match some criteria.
 */


struct _TableUserData
{
  guint ref_count;
  GskTableMergeFunc merge;
  gpointer user_data;
  GDestroyNotify destroy;
};

struct _Prefix
{
  guint prefix_len;
  guint8 *prefix;
  guint64 index_byte_offset;
  guint64 index_entry_count;
  guint index_data_length;
  /* size of each index entry is
   *      heap_offset_size
   *    + key_size_size
   *    + value_size_size
   *    + index_data_length
   */
};

struct _MergeTask
{
  gboolean is_started;
  FileInfo *inputs[2];

  guint64 n_entries;    /* sum of inputs[i]->n_entries */

  union
  {
    struct {
      /* ratio of inputs[0]->n_data / inputs[1]->n_data * (2^16) */
      /* we want the smallest ratio possible;
         the "min_merge_ratio" specifies the limit 
         for merge-job creation. */
      guint32 input_size_ratio_b16;

      /* tree of unstarted merge jobs, sorted by input_size_ratio_b16
         then n_entries */
      MergeTask *left, *right, *parent;
      gboolean is_red;
    } unstarted;
    struct {
      GskTableFile *output;
    } started;
  } info;
};

struct _FileInfo
{
  guint64 start_record, n_records;
  guint64 file_id;

  guint8 heap_offset_size;
  guint8 key_size_size;         /* 0 for fixed-length keys */
  guint8 value_size_size;       /* 0 for fixed-length values */

  guint n_prefixes;
  Prefix *prefixes;
  guint8 *index_file;
  gint index_fd;
  guint8 *heap_file;
  gint heap_fd;

  /* point to the jobs ahead and behind;
     merges[0] will be NULL if this is the first 
     file, or if merges[1]->started.
     merges[1] will be NULL if this is the last
     file, or if merges[0]->started. */
  MergeTask *merge_tasks[2];
};

struct _TreeNode
{
  GskTableBuffer key;
  GskTableBuffer value;
  TreeNode *left, *right, *parent;
  guint is_red : 1;
};

struct _GskTable
{
  char *dir;
  int lock_fd;

  /* merge functions */
  GskTableMergeFunc merge;
  gpointer user_data;
  TableUserData *table_user_data; /* for ref-counting */

  /* journalling */
  int journal_fd;
  char *journal_new_fname;
  char *journal_old_fname;
  guint8 *journal_mmap;

  /* files */
  guint n_files;
  FileInfo **files;

  /* old files */
  guint n_old_files;
  FileInfo **old_files;          /* as in the beginning of the journal file */

  /* tree of merge tasks, sorted by input file ratio */
  MergeTask *unstarted_merges;

  /* heap of merge tasks (sorted by n_entries ascending) */
  MergeTask *started_merges[MAX_RUNNING_MERGES];

  /* fixed length keys and values can be optimized by not storing the length. */
  gssize key_fixed_length;               /* or -1 */
  gssize value_fixed_length;             /* or -1 */

  /* used for merging the in-memory data */
  guint8 *tmp_merge_value_data;
  guint tmp_merge_value_alloced;
};


GskTable *
gsk_table_new         (const char            *dir,
                       const GskTableOptions *options,
                       GError               **error)
{
  gboolean did_mkdir;
  GskTable *table;
  if (g_file_test (dir, G_FILE_TEST_IS_DIR))
    {
      did_mkdir = FALSE;
    }
  else
    {
      did_mkdir = TRUE;
      if (!gsk_mkdir_p (dir, 0755, error))
        return FALSE;
    }

  lock_fd = gsk_lock_dir (dir, FALSE, error);
  if (lock_fd < 0)
    return FALSE;

  table = g_new0 (GskTable, 1);
  table->dir = g_strdup (dir);
  table->lock_fd = lock_fd;

  if (did_mkdir)
    {
      /* prepare a new table:
       * nothing to do, yet */
    }
  else
    {
      /* load existing table */
      if (!read_journal (table, error))
        {
          GError *e = NULL;
          g_free (table->dir);
          if (!gsk_unlock_dir (table->lock_fd, &e))
            {
              g_warning ("error unlocking dir on failure reading journal: %s",
                         e->message);
              g_clear_error (&e);
            }
          g_free (table);
          return NULL;
        }

      /* move aside unused files */
      ...
    }

  return table;
}

/**
 * gsk_table_add:
 * @table: the table to add data to.
 * @key_len:
 * @key_data:
 * @value_len:
 * @value_data:
 *
 * 
 */

void
gsk_table_add         (GskTable              *table,
                       guint                  key_len,
                       const guint8          *key_data,
                       guint                  value_len,
                       const guint8          *value_data);
{
  TreeNode *found;
  gboolean must_write_journal = table->journal_fd >= 0;
  g_assert (table->key_fixed_length < 0
         || table->key_fixed_length == key_len);
  g_assert (table->value_fixed_length < 0
         || table->value_fixed_length == value_len);

#define TMP_KEY_COMPARATOR(unused, at, rv) \
  rv = compare_memory (key_len, key_data, (at)->key_len, (at)->key_data)
  GSK_RBTREE_LOOKUP_COMPARATOR (GET_IN_MEMORY_TREE (table),
                                unused, TMP_KEY_COMPARATOR, found);
#undef TMP_KEY_COMPARATOR

  if (found)
    {
      /* Merge the old data with the new data. */
      switch (table->merge (key_len, key_data,
                            found->value_len, found->value_data,
                            value_len, value_data,
                            &table->tmp_buffer, table->user_data))
        {
        case GSK_TABLE_MERGE_RETURN_A:
        case GSK_TABLE_MERGE_RETURN_A_DONE:
          /* nothing to do */
          break;
        case GSK_TABLE_MERGE_RETURN_B:
        case GSK_TABLE_MERGE_RETURN_B_DONE:
          table->in_memory_bytes -= found->value.len;
          table->in_memory_bytes += value_len;
          gsk_table_buffer_set_len (&found->value, value_len);
          memcpy (found->value.data, value_data, value_len);
          break;
        case GSK_TABLE_MERGE_SUCCESS:
        case GSK_TABLE_MERGE_SUCCESS_DONE:
          {
            GskTableBuffer swap = table->tmp_buffer;
            table->in_memory_bytes -= found->value.len;
            table->tmp_buffer = found->value;
            found->value = swap;
            table->in_memory_bytes += found->value.len;
          }
          break;
        case GSK_TABLE_MERGE_DROP:
          GSK_RBTREE_REMOVE (GET_IN_MEMORY_TREE (table), found);
          break;
        }
    }
  else
    {
      TreeNode *new = table->in_memory_nodes + table->n_memory_added;

      /* write key and value into the new node */
      resize_buffer (key_len,
                     &new->key_len,
                     &new->key_data,
                     &new->key_alloced);
      resize_buffer (value_len,
                     &new->value_len,
                     &new->value_data,
                     &new->value_alloced);
      memcpy (new->key_data, key_data, key_len);
      memcpy (new->value_data, value_data, value_len);

      /* TODO: this repeats the work of the lookup.
         maybe we need a GSK_RBTREE_INSERT_NO_REPLACE()
         which can be used at the outset instead of LOOKUP_COMPARATOR? */
      GSK_RBTREE_INSERT (GET_IN_MEMORY_TREE (table), new, found);
      g_assert (found == NULL);

      table->in_memory_bytes += key_len + value_len;
    }

  table->in_memory_entry_count++;
  if (table->in_memory_entry_count == table->max_in_memory_entries
   || table->in_memory_bytes >= table->max_in_memory_bytes)
    {
      /* flush the tree */
      guint new_file_index = flush_tree (table);

      /* create MergeTasks */
      guint mt;
      for (mt = 0; mt < 2; mt++)
        {
          MergeTask *m;
          if (mt == 0 && new_file_index == 0)
            continue;
          if (mt == 1 && new_file_index == table->n_files - 1)
            continue;
          m = allocate_or_recycle_merge_task (table);
          g_assert (!m->started);
          m->is_started = FALSE;
          m->inputs[0] = table->files[new_file_index - mt];
          m->inputs[1] = table->files[new_file_index - mt + 1];
          m->n_entries = m->inputs[0]->n_entries + m->inputs[1]->n_entries;
          m->info.unstarted.input_size_ratio_b16 = ...;
          GSK_RBTREE_INSERT (GET_UNSTARTED_MERGE_TASK_TREE (table),
                             m, unused);
          g_assert (unused == NULL);
          ...
        }

      /* maybe flush journal */
      if (table->journalling
       && (++(table->journal_index) == table->journal_period))
        {
          /* write new journal */
          ...

          /* move it into place */
          ...

          must_write_journal = FALSE;
        }
    }

  if (must_write_journal)
    {
      ...
    }
}

gboolean
gsk_table_query       (GskTable              *table,
                       guint                  key_len,
                       const guint8          *key_data,
                       guint                 *value_len_out,
                       guint8               **value_data_out)
{
  ...
}

const char *
gsk_table_peek_dir    (GskTable              *table)
{
  return table->dir;
}

void
gsk_table_destroy     (GskTable              *table)
{
  ...
}

