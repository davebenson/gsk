#include <string.h>
#include <errno.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>              /* for rename() */

#include "gskrbtreemacros.h"
#include "gskghelpers.h"
#include "gskutils.h"
#include "gsktable.h"
#include "gsktable-file.h"
#include "gskerror.h"

typedef struct _TableUserData TableUserData;
typedef struct _MergeTask MergeTask;
typedef struct _FileInfo FileInfo;
typedef struct _TreeNode TreeNode;

#define ID_FMT  "%" G_GUINT64_FORMAT

#define JOURNAL_FILE_MAGIC              0x1143eeab

struct _TableUserData
{
  guint ref_count;
  GskTableMergeFunc merge;
  gpointer user_data;
  GDestroyNotify destroy;
};

struct _MergeTask
{
  gboolean is_started;
  FileInfo *inputs[2];

  union
  {
    struct {
      /* ratio of inputs[0]->n_data / inputs[1]->n_data * (2^16) */
      /* we want the smallest ratio possible;
         the "min_merge_ratio" specifies the limit 
         for merge-task creation. */
      guint32 input_size_ratio_b16;

      /* tree of unstarted merge tasks, sorted by input_size_ratio_b16
         then n_entries */
      MergeTask *left, *right, *parent;
      gboolean is_red;
    } unstarted;
    struct {
      GskTableFile *output;
      struct {
        GskTableReader *reader;
      } inputs[2];
    } started;
  } info;
};

struct _FileInfo
{
  GskTableFile *file;
  guint ref_count;
  guint64 first_input_entry, n_input_entries;
  MergeTask *prev_task;         /* possible merge task with prior file */
  MergeTask *next_task;         /* possible merge task with next file */
  FileInfo *prev_file, *next_file;
};

struct _TreeNode
{
  GskTableBuffer key;
  GskTableBuffer value;
  TreeNode *left, *right, *parent;
  guint is_red : 1;
};

/* always runs the table->run_list task */
typedef gboolean (*RunTaskFunc) (GskTable   *table,
                                 guint       iterations,
                                 GError    **error);
typedef struct _RunTaskFuncs RunTaskFuncs;
struct _RunTaskFuncs
{
  RunTaskFunc simplify_flush;
  RunTaskFunc simplify_noflush;
  RunTaskFunc nosimplify_flush;
  RunTaskFunc nosimplify_noflush;
};

/* optimized variants of RunTaskFuncs:
      len_memcmp_nomerge
      len_memcmp_merge
      nolen_memcmp_merge
      len_memcmp_merge
      len_nomerge
      nolen_nomerge
      nolen_merge
      len_merge      */
static RunTaskFuncs *
              table_options_get_run_funcs    (const GskTableOptions *options,
                                              gboolean        *has_len_out,
                                              GError         **error);
static GskTableFileFactory *
              table_options_get_file_factory (const GskTableOptions *,
                                              GError **error);

typedef TreeNode *(*InMemoryTreeLookupFunc) (GskTable     *table,
                                             guint         key_len,
                                             const guint8 *key_data);
typedef int       (*TreeNodeCompareFunc)    (GskTable     *table,
                                             TreeNode     *a,
                                             TreeNode     *b);
struct _GskTable
{
  char *dir;
  int lock_fd;

  /* data handling functions */
  gboolean has_len;
  union
  {
    GskTableCompareFunc with_len;
    GskTableCompareFuncNoLen no_len;
  } compare;
  union
  {
    GskTableMergeFunc with_len;
    GskTableMergeFuncNoLen no_len;
  } merge;
  union
  {
    GskTableSimplifyFunc with_len;
    GskTableSimplifyFuncNoLen no_len;
  } simplify;
  RunTaskFuncs *run_funcs;
  InMemoryTreeLookupFunc in_memory_tree_lookup;
  TreeNodeCompareFunc tree_node_compare;

  gpointer user_data;
  TableUserData *table_user_data; /* for ref-counting */

  guint64 n_input_entries;

  /* journalling */
  int journal_fd;
  char *journal_cur_fname;
  char *journal_tmp_fname;              /* actual file to write to */
  guint8 *journal_mmap;
  guint journal_len;                    /* current offset in journal */
  guint journal_size;                   /* size of journal data */
  GskTableJournalMode journal_mode;

  /* files */
  guint n_files;
  FileInfo *first_file, *last_file;

  /* old files */
  guint n_old_files;
  FileInfo **old_files;          /* as in the beginning of the journal file */

  /* tree of merge tasks, sorted by input file ratio */
  MergeTask *unstarted_merges;

  /* heap of merge tasks (sorted by n_entries ascending) */
  MergeTask *run_list;
  guint n_running_tasks;

  /* tree of in-memory entries */
  TreeNode *in_memory_tree;

  /* fixed length keys and values can be optimized by not storing the length. */
  gssize key_fixed_length;               /* or -1 */
  gssize value_fixed_length;             /* or -1 */

  /* buffers */
  GskTableBuffer result_buffers[2];
  GskTableBuffer merge_buffer;
  GskTableBuffer simplify_buffer;

  /* file factory */
  GskTableFileFactory *file_factory;

  /* used for merging the in-memory data */
  guint8 *tmp_merge_value_data;
  guint tmp_merge_value_alloced;
};

#define UNSTARTED_MERGE_TASK_IS_RED(task)         (task)->info.unstarted.is_red
#define UNSTARTED_MERGE_TASK_SET_IS_RED(task,v)   (task)->info.unstarted.is_red = v
#define COMPARE_UNSTARTED_MERGE_TASKS(a,b, rv) \
  if (a->info.unstarted.input_size_ratio_b16 < b->info.unstarted.input_size_ratio_b16) \
    rv = -1; \
  else if (a->info.unstarted.input_size_ratio_b16 > b->info.unstarted.input_size_ratio_b16) \
    rv = 1; \
  else \
    rv = (a<b) ? -1 : (a>b) ? 1 : 0;
#define GET_UNSTARTED_MERGE_TASK_TREE(table) \
  (table)->unstarted_merges, MergeTask *, \
  UNSTARTED_MERGE_TASK_IS_RED, \
  UNSTARTED_MERGE_TASK_SET_IS_RED, \
  info.unstarted.parent, \
  info.unstarted.left, \
  info.unstarted.right, \
  COMPARE_UNSTARTED_MERGE_TASKS

/* NOTE: 'table' must be a local-variable or a parameter for
   these macros to work!!! blah. */
#define TREE_NODE_IS_RED(node)         node->is_red
#define TREE_NODE_SET_IS_RED(node,v)   node->is_red = v
#define TREE_NODE_COMPARE(a,b,rv)      rv = table->tree_node_compare(table, a,b)
#define GET_IN_MEMORY_TREE(table) \
  (table)->in_memory_tree, TreeNode *, \
  TREE_NODE_IS_RED, TREE_NODE_SET_IS_RED, \
  parent, left, right, \
  TREE_NODE_COMPARE
  

static inline void
set_buffer (GskTableBuffer *buffer,
            guint           len,
            const guint8   *data)
{
  memcpy (gsk_table_buffer_set_len (buffer, len), data, len);
}
static inline void
copy_buffer (GskTableBuffer *buffer,
             const GskTableBuffer *src)
{
  set_buffer (buffer, src->len, src->data);
}

/* --- file-info ref-counting --- */
static inline FileInfo *
file_info_ref (FileInfo *fi)
{
  g_assert (fi->ref_count > 0);
  ++(fi->ref_count);
  return fi;
}
static inline FileInfo *
file_info_unref (FileInfo *fi, const char *dir, gboolean erase)
{
  g_assert (fi->ref_count > 0);
  if (--(fi->ref_count) == 0)
    {
      GError *error = NULL;
      if (!gsk_table_file_destroy (fi->file, dir, erase, &error))
        {
          g_warning ("gsk_table_file_destroy "ID_FMT" (erase=%u) failed: %s",
                     fi->file->id, erase, error->message);
          g_error_free (error);
        }
      g_slice_free (FileInfo, fi);
    }
  return fi;
}

/* --- journal management --- */
static gboolean read_journal  (GskTable    *table,
                               GError     **error);
static gboolean reset_journal (GskTable    *table,
                               GError     **error);

static int compare_file_id_to_pfileinfo (gconstpointer p_file_id,
                                         gconstpointer p_file_info)
{
  guint64 a = * (guint64 *) p_file_id;
  FileInfo *file_info = * (FileInfo **) p_file_info;
  if (a < file_info->file->id)
    return -1;
  else if (a > file_info->file->id)
    return 1;
  else
    return 0;
}

static inline void
kill_unstarted_merge_task (GskTable *table,
                           MergeTask *to_kill)
{
  g_assert (to_kill->inputs[0]->next_task == to_kill);
  g_assert (to_kill->inputs[1]->prev_task == to_kill);
  GSK_RBTREE_REMOVE (GET_UNSTARTED_MERGE_TASK_TREE (table), to_kill);
  to_kill->inputs[0]->next_task = NULL;
  to_kill->inputs[1]->prev_task = NULL;
  g_slice_free (MergeTask, to_kill);
}

static void
create_unstarted_merge_task (GskTable *table,
                             FileInfo *prev,
                             FileInfo *next)
{
  MergeTask *task = g_slice_new (MergeTask);
  MergeTask *unused;
  guint32 ratio_b16;
  g_assert (prev->prev_task == NULL || !prev->prev_task->is_started);
  g_assert (prev->next_task == NULL);
  g_assert (next->prev_task == NULL);
  g_assert (next->next_task == NULL || !next->next_task->is_started);
  task->is_started = FALSE;
  task->inputs[0] = prev;
  task->inputs[1] = next;
  if (prev->file->n_entries == 0 && next->file->n_entries == 0)
    ratio_b16 = 1<<16;
  else if (next->file->n_entries == 0)
    ratio_b16 = 0xffffffff;
  else
    {
      gdouble ratio_d_b16 = (double) prev->file->n_entries / next->file->n_entries
                          * (double) (1<<16);
      if (ratio_d_b16 >= 0xffffffff)
        ratio_b16 = 0xffffffff;
      else
        ratio_b16 = (guint) ratio_d_b16;
    }

  prev->next_task = next->prev_task = task;
  task->info.unstarted.input_size_ratio_b16 = ratio_b16;
  GSK_RBTREE_INSERT (GET_UNSTARTED_MERGE_TASK_TREE (table),
                     task, unused);
  g_assert (unused == NULL);
}

static gboolean
read_journal (GskTable  *table,
              GError   **error)
{
  int fd = open (table->journal_cur_fname, O_RDWR);
  struct stat stat_buf;
  guint i;
  guint magic, n_files, n_merge_tasks;
  guint32 tmp32_le;
  guint64 tmp64_le;
  guint journal_size;
  guint8 *mmapped_journal;
  const guint8 *at;
  guint64 n_input_entries;
  FileInfo **file_infos;
  GskTableFileFactory *file_factory = table->file_factory;
  guint file_index;
  GskTableJournalMode old_journal_mode;
  if (fd < 0)
    {
      g_set_error (error, GSK_G_ERROR_DOMAIN, GSK_ERROR_FILE_OPEN,
                   "error opening journal file %s: %s",
                   table->journal_cur_fname,
                   g_strerror (errno));
      return FALSE;
    }
  if (fstat (fd, &stat_buf) < 0)
    {
      g_set_error (error, GSK_G_ERROR_DOMAIN, GSK_ERROR_FILE_STAT,
                   "error statting journal file %s: %s",
                   table->journal_cur_fname,
                   g_strerror (errno));
      close (fd);
      return FALSE;
    }
  journal_size = stat_buf.st_size;
  mmapped_journal = mmap (NULL, journal_size, PROT_READ|PROT_WRITE,
                          MAP_SHARED, fd, 0);
  if (mmapped_journal == MAP_FAILED || mmapped_journal == NULL)
    {
      g_set_error (error, GSK_G_ERROR_DOMAIN, GSK_ERROR_FILE_MMAP,
                   "error mmapping journal file %s: %s",
                   table->journal_cur_fname,
                   g_strerror (errno));
      close (fd);
      return FALSE;
    }

  /* parse the journal */
  at = mmapped_journal;

  tmp32_le = * (guint32 *) at;
  magic = GUINT32_FROM_LE (tmp32_le);
  if (magic != JOURNAL_FILE_MAGIC)
    {
      g_set_error (error, GSK_G_ERROR_DOMAIN, GSK_ERROR_PARSE,
                   "invalid magic on journal file (0x%08x, not 0x%08x)",
                   magic, JOURNAL_FILE_MAGIC);
      goto error_cleanup;
    }
  at += 4;

  tmp32_le = * (guint32 *) at;
  n_files = GUINT32_FROM_LE (tmp32_le);
  at += 4;

  tmp32_le = * (guint32 *) at;
  n_merge_tasks = GUINT32_FROM_LE (tmp32_le);
  at += 4;

  tmp32_le = * (guint32 *) at;
  at += 4;
  if (tmp32_le != 0)
    {
      g_set_error (error, GSK_G_ERROR_DOMAIN, GSK_ERROR_PARSE,
                   "reserved word in journal nonzero");
      goto error_cleanup;
    }

  tmp64_le = * (guint64 *) at;
  at += 8;
  n_input_entries = GUINT64_FROM_LE (tmp64_le);

  file_infos = g_new0 (FileInfo *, n_files);

  /* parse files */
  for (i = 0; i < n_files; i++)
    {
      guint64 file_id, first_input_entry, n_input_entries, n_entries;
      FileInfo *file_info;

      memcpy (&tmp64_le, at, 8);
      file_id = GUINT64_FROM_LE (tmp64_le);
      at += 8;

      memcpy (&tmp64_le, at, 8);
      first_input_entry = GUINT64_FROM_LE (tmp64_le);
      at += 8;

      memcpy (&tmp64_le, at, 8);
      n_input_entries = GUINT64_FROM_LE (tmp64_le);
      at += 8;

      memcpy (&tmp64_le, at, 8);
      n_entries = GUINT64_FROM_LE (tmp64_le);
      at += 8;

      file_info = g_slice_new0 (FileInfo);
      file_info->first_input_entry = first_input_entry;
      file_info->n_input_entries = n_input_entries;

      file_info->file
        = gsk_table_file_factory_open_file (file_factory, table->dir,
                                            file_id, error);
      if (file_info->file == NULL)
        goto error_cleanup;
      file_info->file->n_entries = n_entries;
      g_assert (file_info->file);
      file_info->ref_count = 1;
      file_infos[i] = file_info;

      if (i > 0)
        {
          FileInfo *prev = file_infos[i-1];
          guint64 prev_end = prev->first_input_entry + prev->n_input_entries;
          if (prev_end != file_info->first_input_entry)
            {
              g_set_error (error, GSK_G_ERROR_DOMAIN, GSK_ERROR_PARSE,
                           "inconsistency: files "ID_FMT" and "ID_FMT" are not continguous (prev_end=%"G_GUINT64_FORMAT"; cur_start=%"G_GUINT64_FORMAT")",
                           prev->file->id, file_info->file->id, prev_end, file_info->first_input_entry);
              goto error_cleanup;
            }
          file_info->prev_file = prev;
          prev->next_file = file_info;
        }
      else
        file_info->prev_file = NULL;
    }
  if (n_files > 0)
    file_infos[n_files-1]->next_file = NULL;

  /* parse merge tasks */
  file_index = 0;
  for (i = 0; i < n_merge_tasks; i++)
    {
      /* format:
           - for each input:
             - uint64 for the file-id
             - reader state
           - output file-id
           - output file build state
       */
      struct {
        guint64 file_id;
        guint reader_state_len;
        const guint8 *reader_state;
        FileInfo *file_info;
      } inputs[2];
      guint64 output_file_id;
      guint build_state_len;
      const guint8 *build_state;
      MergeTask *merge_task;
      guint j;

      for (j = 0; j < 2; j++)
        {
          memcpy (&tmp64_le, at, 8);
          at += 8;
          inputs[j].file_id = GUINT64_FROM_LE (tmp64_le);

          memcpy (&tmp32_le, at, 8);
          at += 4;
          inputs[j].reader_state_len = GUINT32_FROM_LE (tmp32_le);

          inputs[j].reader_state = at;
          at += inputs[j].reader_state_len;
        }

      memcpy (&tmp64_le, at, 8);
      at += 8;
      output_file_id = GUINT64_FROM_LE (tmp64_le);

      memcpy (&tmp32_le, at, 8);
      at += 4;
      build_state_len = GUINT32_FROM_LE (tmp32_le);

      build_state = at;
      at += build_state_len;

      /* link merge data into file infos */
      while (file_index + 1 < n_files
          && file_infos[file_index]->file->id != inputs[0].file_id)
        file_index++;
      if (file_index + 1 == n_files)
        {
          g_set_error (error, GSK_G_ERROR_DOMAIN, GSK_ERROR_PARSE,
                       "merge task's input[0] refers to input file "ID_FMT" which was not found, in the non-tail portion of the files list",
                       inputs[0].file_id);
          goto error_cleanup;
        }
      inputs[0].file_info = file_infos[file_index];
      inputs[1].file_info = file_infos[file_index+1];
      g_assert (inputs[0].file_info->file->id == inputs[0].file_id);
      if (inputs[1].file_info->file->id != inputs[1].file_id)
        {
          g_set_error (error, GSK_G_ERROR_DOMAIN, GSK_ERROR_PARSE,
                       "second input to merge task nonconsecutive");
          goto error_cleanup;
        }
      g_assert (inputs[0].file_info->next_task == NULL);
      g_assert (inputs[1].file_info->prev_task == NULL);

      merge_task = g_slice_new (MergeTask);
      merge_task->is_started = TRUE;
      merge_task->inputs[0] = inputs[0].file_info;
      merge_task->inputs[1] = inputs[1].file_info;

      for (j = 0; j < 2; j++)
        {
          /* restore reader */
          GskTableReader *reader;
          reader = gsk_table_file_recreate_reader (inputs[j].file_info->file,
                                                   table->dir,
                                                   inputs[j].reader_state_len,
                                                   inputs[j].reader_state,
                                                   error);
          if (reader == NULL)
            {
              if (j == 1)
                gsk_table_reader_destroy (merge_task->info.started.inputs[0].reader);
              g_slice_free (MergeTask, merge_task);
              goto error_cleanup;
            }
          merge_task->info.started.inputs[j].reader = reader;
        }

      merge_task->info.started.output
        = gsk_table_file_factory_open_building_file (file_factory,
                                                     table->dir, output_file_id,
                                                     build_state_len,
                                                     build_state, error);
      if (merge_task->info.started.output == NULL)
        {
          gsk_g_error_add_prefix (error,
                                "instantiating merge-task between files "ID_FMT" and "ID_FMT,
                                inputs[0].file_id,
                                inputs[1].file_id);
          gsk_table_reader_destroy (merge_task->info.started.inputs[0].reader);
          gsk_table_reader_destroy (merge_task->info.started.inputs[1].reader);
          g_slice_free (MergeTask, merge_task);
          goto error_cleanup;
        }
      inputs[0].file_info->next_task = merge_task;
      inputs[1].file_info->prev_task = merge_task;

      /* neither of these inputs can be involved in another merge-task,
         so advance the pointer to prevent reuse. */
      file_index++;
    }

  /* --- do consistency checks --- */

  /* TODO: other checks not already done during parsing??? */

  /* --- process existing journal entries --- */

  /* setup various bits of the table;
     after doing this, do not goto error_cleanup,
     since it will free some of this data */
  table->journal_mmap = mmapped_journal;
  table->journal_size = journal_size;
  table->n_files = n_files;
  if (n_files > 0)
    {
      table->first_file = file_infos[0];
      table->last_file = file_infos[n_files-1];
    }
  else
    {
      /* since the GskTable object is zeroed, the list is already empty */
    }
  table->n_old_files = n_files;
  table->old_files = file_infos;
  for (i = 0; i < n_files; i++)
    table->old_files[i] = file_info_ref (file_infos[i]);

  /* create unstarted merge tasks */
  for (i = 0; i < n_files - 1; i++)
    if (file_infos[i]->next_task == NULL
     && (file_infos[i]->prev_task == NULL
         || !file_infos[i]->prev_task->is_started)
     && (file_infos[i+1]->next_task == NULL
         || !file_infos[i+1]->next_task->is_started))
      {
        g_assert (file_infos[i+1]->prev_task == NULL);
        create_unstarted_merge_task (table, file_infos[i], file_infos[i+1]);
      }

  /* disable journalling and replay the journal */
  old_journal_mode = table->journal_mode;
  table->journal_mode = GSK_TABLE_JOURNAL_NONE;
  for (;;)
    {
      guint align_offset = ((gsize)at) & 3;
      guint32 key_len, value_len;
      if (align_offset)
        at += 4 - align_offset;
      tmp32_le = ((guint32 *) at)[0];
      key_len = GUINT32_FROM_LE (tmp32_le);
      if (key_len == 0)
        break;
      key_len--;            /* journal key lengths are offset by 1 */
      tmp32_le = ((guint32 *) at)[1];
      value_len = GUINT32_FROM_LE (tmp32_le);
      at += 8;
      if (!gsk_table_add (table, key_len, at, value_len, at + key_len, error))
        {
          gsk_g_error_add_prefix (error, "error replaying journal");
          table->journal_mode = old_journal_mode;
          /* do not use error_cleanup, let gsk_table_destroy() do the work */
          return FALSE;
        }
      at += key_len + value_len;
    }
  table->journal_mode = old_journal_mode;
  table->journal_len = at - mmapped_journal;
  table->n_input_entries = n_input_entries;

  return TRUE;

error_cleanup:
  if (file_infos)
    {
      for (i = 0; i < n_files && file_infos[i] != NULL; i++)
        file_info_unref (file_infos[i], table->dir, FALSE);
      g_free (file_infos);
    }
  munmap (mmapped_journal, journal_size);
  return FALSE;
}

static gboolean
resize_journal (gint       journal_fd,
                guint8   **journal_mmap_inout,
                guint     *journal_size_inout,
                guint      new_min_size,
                GError   **error)
{
  guint new_size;
  guint8 *tmp;
  if (new_min_size <= *journal_size_inout)
    return TRUE;
  new_size = *journal_size_inout;
  while (new_size < new_min_size)
    new_size += new_size;

  /* un-mmap file */
  munmap (*journal_mmap_inout, *journal_size_inout);

  /* resize */
  if (ftruncate (journal_fd, new_size) < 0)
    {
      g_set_error (error, GSK_G_ERROR_DOMAIN, GSK_ERROR_FILE_TRUNCATE,
                   "error resizing journal: %s",
                   g_strerror (errno));
      return FALSE;
    }

  /* mmap file */
  tmp = mmap (NULL, new_size, PROT_READ|PROT_WRITE, MAP_SHARED, journal_fd, 0);
  if (tmp == NULL || tmp == MAP_FAILED)
    {
      g_set_error (error, GSK_G_ERROR_DOMAIN, GSK_ERROR_FILE_MMAP,
                   "error mmapping resized journal");
      return FALSE;
    }
  *journal_size_inout = new_size;
  *journal_mmap_inout = tmp;
  return TRUE;
}

static gboolean
reset_journal (GskTable   *table,
               GError    **error)
{
  guint i;
  FileInfo *fi;
  int journal_fd;
  guint8 *journal_mmap;
  guint at;
  guint n_merge_tasks_written;

  g_assert (table->in_memory_tree == NULL);

  /* write temporary new journal which
     is the state dumped out,
     and no journalled adds. */
  {
    journal_fd = open (table->journal_tmp_fname, O_CREAT|O_RDWR|O_TRUNC, 0644);
    if (journal_fd < 0)
      {
        g_set_error (error, GSK_G_ERROR_DOMAIN, GSK_ERROR_FILE_OPEN,
                     "error creating new journal file %s: %s",
                     table->journal_tmp_fname, g_strerror (errno));
        return FALSE;
      }
    if (ftruncate (journal_fd, table->journal_size) < 0)
      {
        g_set_error (error, GSK_G_ERROR_DOMAIN, GSK_ERROR_FILE_TRUNCATE,
                     "error sizing journal file: %s",
                     g_strerror (errno));
        goto failed_writing_journal;
      }
    journal_mmap = mmap (NULL, table->journal_size, PROT_READ|PROT_WRITE,
                         MAP_SHARED, journal_fd, 0);
    if (journal_mmap == NULL || journal_mmap == MAP_FAILED)
      {
        g_set_error (error, GSK_G_ERROR_DOMAIN, GSK_ERROR_FILE_TRUNCATE,
                     "mmap failed on tmp journal: %s",
                     g_strerror (errno));
        close (journal_fd);
        unlink (table->journal_tmp_fname);
        return FALSE;
      }
  }

  {
    guint32 header[6];
    guint32 tmp;
    header[0] = GUINT32_TO_LE (JOURNAL_FILE_MAGIC);
    header[1] = GUINT32_TO_LE (table->n_files);
    header[2] = GUINT32_TO_LE (table->n_running_tasks);
    header[3] = 0;              /* reserved */
    tmp = table->n_input_entries;
    header[4] = GUINT32_TO_LE (tmp);
    tmp = table->n_input_entries>>32;
    header[5] = GUINT32_TO_LE (tmp);
    memcpy (journal_mmap, header, 24);
  }
  at = 24;
  for (fi = table->first_file; fi; fi = fi->next_file)
    {
      guint64 file_header[4];
      file_header[0] = GUINT64_TO_LE (fi->file->id);
      file_header[1] = GUINT64_TO_LE (fi->first_input_entry);
      file_header[2] = GUINT64_TO_LE (fi->n_input_entries);
      file_header[3] = GUINT64_TO_LE (fi->file->n_entries);
      if (at + sizeof (file_header) > table->journal_size)
        {
          if (!resize_journal (journal_fd,
                               &journal_mmap, &table->journal_size,
                               at + sizeof (file_header),
                               error))
            return FALSE;
        }
      memcpy (journal_mmap + at, file_header, sizeof (file_header));
      at += sizeof (file_header);
    }
  n_merge_tasks_written = 0;
  for (fi = table->first_file; fi; fi = fi->next_file)
    if (fi->next_task != NULL && fi->next_task->is_started)
      {
        MergeTask *task = fi->next_task;
        guint reader_state_lens[2];
        guint8 *reader_states[2];
        guint build_state_len;
        guint8 *build_state;
        guint input;
        for (input = 0; input < 2; input++)
          {
            if (!gsk_table_file_get_reader_state (task->inputs[input]->file,
                                                  task->info.started.inputs[input].reader,
                                                  &reader_state_lens[input],
                                                  &reader_states[input],
                                                  error))
              {
                gsk_g_error_add_prefix (error, "reset_journal: input %u", input);
                goto failed_writing_journal;
              }
          }
        if (!gsk_table_file_get_build_state (task->info.started.output,
                                             &build_state_len,
                                             &build_state,
                                             error))
          {
            gsk_g_error_add_prefix (error, "reset_journal: build state");
            goto failed_writing_journal;
          }

        guint total_len;
        total_len = (8 + 4 + reader_state_lens[0])
                  + (8 + 4 + reader_state_lens[1])
                  + (8 + 4 + build_state_len);
        if (at + total_len > table->journal_size)
          {
            if (!resize_journal (journal_fd,
                                 &journal_mmap, &table->journal_size,
                                 at + total_len,
                                 error))
              return FALSE;
          }
        for (input = 0; input < 2; input++)
          {
            guint64 id = task->inputs[input]->file->id;
            guint32 len = reader_state_lens[input];
            guint64 id_le = GUINT64_TO_LE (id);
            guint32 len_le = GUINT32_TO_LE (len);
            memcpy (journal_mmap + at, &id_le, 8);
            at += 8;
            memcpy (journal_mmap + at, &len_le, 4);
            at += 4;
            memcpy (journal_mmap + at, reader_states[input], len);
            at += len;
            g_free (reader_states[input]);
          }
        {
          guint64 id = task->info.started.output->id;
          guint32 len = build_state_len;
          guint64 id_le = GUINT64_TO_LE (id);
          guint32 len_le = GUINT32_TO_LE (len);
          memcpy (journal_mmap + at, &id_le, 8);
          at += 8;
          memcpy (journal_mmap + at, &len_le, 4);
          at += 4;
          memcpy (journal_mmap + at, build_state, len);
          at += len;
          g_free (build_state);
        }

        n_merge_tasks_written++;
      }
  g_assert (n_merge_tasks_written == table->n_running_tasks);

  /* move the journal into place */
  if (rename (table->journal_tmp_fname, table->journal_cur_fname) < 0)
    {
      g_set_error (error, GSK_G_ERROR_DOMAIN, GSK_ERROR_FILE_RENAME,
                   "error moving journal into place: %s",
                   g_strerror (errno));
      goto failed_writing_journal;
    }

  table->journal_len = at;

  /* blow away old files */
  for (i = 0; i < table->n_old_files; i++)
    file_info_unref (table->old_files[i], table->dir, TRUE);
  g_free (table->old_files);

  /* preserve old files */
  table->old_files = g_new (FileInfo *, table->n_files);
  i = 0;
  table->n_old_files = table->n_files;
  for (fi = table->first_file; fi; fi = fi->next_file)
    table->old_files[i++] = file_info_ref (fi);

  return TRUE;

failed_writing_journal:
  close (journal_fd);
  unlink (table->journal_tmp_fname);
  return FALSE;
}


/* --- in-memory tree lookup (implementations) --- */
static inline gint compare_memory (guint a_len, const guint8 *a_data,
                                   guint b_len, const guint8 *b_data)
{
  int rv;
  if (a_len < b_len)
    {
      rv = memcmp (a_data, b_data, a_len);
      if (rv == 0)
        rv = 1;
    }
  else if (a_len > b_len)
    {
      rv = memcmp (a_data, b_data, b_len);
      if (rv == 0)
        rv = -1;
    }
  else
    rv = memcmp (a_data, b_data, a_len);
  return rv;
}

static TreeNode *
in_memory_tree_lookup_memcmp (GskTable     *table,
                              guint         key_len,
                              const guint8 *key_data)
{
  TreeNode *found = NULL;
#define TMP_KEY_COMPARATOR(unused, at, rv) \
  rv = compare_memory (key_len, key_data, at->key.len, at->key.data)
  GSK_RBTREE_LOOKUP_COMPARATOR (GET_IN_MEMORY_TREE (table),
                                unused, TMP_KEY_COMPARATOR, found);
#undef TMP_KEY_COMPARATOR
  return found;
}
static TreeNode *
in_memory_tree_lookup_with_len (GskTable     *table,
                                guint         key_len,
                                const guint8 *key_data)
{
  TreeNode *found = NULL;
#define TMP_KEY_COMPARATOR(unused, at, rv) \
  rv = table->compare.with_len (key_len, key_data, \
                                at->key.len, at->key.data, \
                                table->user_data)
  GSK_RBTREE_LOOKUP_COMPARATOR (GET_IN_MEMORY_TREE (table),
                                unused, TMP_KEY_COMPARATOR, found);
#undef TMP_KEY_COMPARATOR
  return found;
}

static TreeNode *
in_memory_tree_lookup_no_len (GskTable     *table,
                              guint         key_len,
                              const guint8 *key_data)
{
  TreeNode *found = NULL;
#define TMP_KEY_COMPARATOR(unused, at, rv) \
  rv = table->compare.no_len (key_data, at->key.data, table->user_data)
  GSK_RBTREE_LOOKUP_COMPARATOR (GET_IN_MEMORY_TREE (table),
                                unused, TMP_KEY_COMPARATOR, found);
#undef TMP_KEY_COMPARATOR
  return found;
}

static int tree_node_compare_memcmp (GskTable *table,
                                     TreeNode *a,
                                     TreeNode *b)
{
  return compare_memory (a->key.len, a->key.data, b->key.len, b->key.data);
}
static int tree_node_compare_with_len (GskTable *table,
                                       TreeNode *a,
                                       TreeNode *b)
{
  return table->compare.with_len (a->key.len, a->key.data,
                                  b->key.len, b->key.data,
                                  table->user_data);
}
static int tree_node_compare_no_len (GskTable *table,
                                     TreeNode *a,
                                     TreeNode *b)
{
  return table->compare.no_len (a->key.data, b->key.data, table->user_data);
}

static guint
uint64_hash (gconstpointer a)
{
  guint64 ai = * (guint64 *) a;
  guint ai_high = ai>>32;
  guint ai_low = ai;
  return ai_low * 33 + ai_high;
}
static gboolean
uint64_equal (gconstpointer a, gconstpointer b)
{
  return (* (guint64 *) a) == (* (guint64 *) b);
}

/** 
 * gsk_table_new:
 * @dir: the directory where the table will store its data.
 * @options: configuration and optimization hints.
 * @new_flags: whether to create or open an existing table.
 * @error: place to put the error if something goes wrong.
 *
 * Create a new GskTable object.
 * Only one table may use a directory at a time.
 *
 * @options gives both compare, merge and delete
 * semantics, and hints on the sizes of the data.
 *
 * @new_flags determines whether we are allowed
 * to create a new table or open an existing table.
 * If @new_flags is 0, we will permit either,
 * equivalent to GSK_TABLE_MAY_CREATE|GSK_TABLE_MAY_EXIST.
 *
 * returns: the newly created table object, or NULL on error.
 */
GskTable *
gsk_table_new         (const char            *dir,
                       const GskTableOptions *options,
                       GskTableNewFlags       new_flags,
                       GError               **error)
{
  gboolean did_mkdir;
  GskTable *table;
  RunTaskFuncs *run_funcs;
  gboolean has_len;
  int lock_fd;
  GskTableFileFactory *factory;

  run_funcs = table_options_get_run_funcs (options, &has_len, error);
  if (run_funcs == NULL)
    return NULL;
  factory = table_options_get_file_factory (options, error);
  if (factory == NULL)
    return NULL;

  if (g_file_test (dir, G_FILE_TEST_IS_DIR))
    {
      if ((new_flags & GSK_TABLE_MAY_EXIST) == 0)
        {
          g_set_error (error, GSK_G_ERROR_DOMAIN, GSK_ERROR_EXISTS,
                       "table dir %s already exists", dir);
          return NULL;
        }
      did_mkdir = FALSE;
    }
  else
    {
      if ((new_flags & GSK_TABLE_MAY_CREATE) == 0)
        {
          g_set_error (error, GSK_G_ERROR_DOMAIN, GSK_ERROR_EXISTS,
                       "table dir %s already exists", dir);
          return NULL;
        }
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
  table->run_funcs = run_funcs;
  table->has_len = has_len;
  table->in_memory_tree_lookup
    = table->compare.no_len == NULL ? in_memory_tree_lookup_memcmp
             : has_len ? in_memory_tree_lookup_with_len
             : in_memory_tree_lookup_no_len;
  table->tree_node_compare
    = table->compare.no_len == NULL ? tree_node_compare_memcmp
             : has_len ? tree_node_compare_with_len
             : tree_node_compare_no_len;
  table->journal_mode = options->journal_mode;

  if (did_mkdir)
    {
      /* make an empty journal file */
      if (!reset_journal (table, error))
        {
          gsk_table_destroy (table);
          gsk_rm_rf (table->dir, NULL);
          return NULL;
        }
    }
  else
    {
      GDir *dirlist;
      GHashTable *known_ids;
      GPtrArray *to_delete;
      FileInfo *fi;
      const char *name;
      const char *at;

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
      dirlist = g_dir_open (dir, 0, error);
      if (dirlist == NULL)
        {
          g_warning ("g_dir_open failed on existing db dir?: %s", dir);
          /* TODO: cleanup (or maybe just return table anyway?) */
          g_free (table);
          return NULL;
        }
      known_ids = g_hash_table_new (uint64_hash, uint64_equal);
      for (fi = table->first_file; fi; fi = fi->next_file)
        {
          g_hash_table_insert (known_ids, &fi->file->id, fi->file);
          if (fi->next_task->is_started)
            {
              GskTableFile *output = fi->next_task->info.started.output;
              g_hash_table_insert (known_ids, &output->id, output);
            }
        }
      to_delete = g_ptr_array_new ();
      while ((name=g_dir_read_name (dirlist)) != NULL)
        {
          guint64 id;
          if (name[0] == '.'
           && ((strcmp (name, ".") == 0 || strcmp (name, "..") == 0)))
            continue;           /* ignore . and .. */

          if ('A' <= name[0] && name[0] <= 'Z')
            continue;           /* ignore user files */

          if (strcmp (name, table->journal_tmp_fname) == 0
           || strcmp (name, table->journal_cur_fname) == 0)
            continue;           /* ignore journal files */

          /* find file-id and extension, since it's possible we want to
             delete this file. */
          for (at = name; g_ascii_isxdigit (*at); at++)
            ;
          if (at == name || *at != '.')
            {
              g_warning ("unrecognized file '%s' in dir.. skipping", name);
              continue;
            }
          id = g_ascii_strtoull (at, NULL, 16);
          /* TODO: verify we know the extension? */
          if (g_hash_table_lookup (known_ids, &id) == NULL)
            {
              g_message ("unknown id for file %s: deleting it", name);
              g_ptr_array_add (to_delete, g_strdup_printf ("%s/%s", dir, name));
            }
        }
      g_hash_table_destroy (known_ids);
      g_ptr_array_foreach (to_delete, (GFunc) unlink, NULL);    /* eep! */
      g_ptr_array_foreach (to_delete, (GFunc) g_free, NULL);
      g_ptr_array_free (to_delete, TRUE);
      g_dir_close (dirlist);
    }

  return table;
}


/* --- starting a merge-task */
static gboolean
start_merge_task (GskTable   *table,
                  MergeTask  *merge_task,
                  GError    **error)
{
  FileInfo *prev = merge_task->inputs[0];
  FileInfo *next = merge_task->inputs[1];
  g_assert (!merge_task->is_started);
  g_assert (prev->prev_task == NULL || !prev->prev_task->is_started);
  g_assert (prev->next_task == NULL);
  g_assert (next->prev_task == NULL);
  g_assert (next->next_task == NULL || !next->next_task->is_started);
  if (prev->prev_task)
    {
      MergeTask *to_kill = prev->prev_task;
      g_assert (!to_kill->is_started);
      g_assert (to_kill->inputs[1] == prev);
      kill_unstarted_merge_task (table, to_kill);
    }
  if (next->next_task)
    {
      MergeTask *to_kill = next->next_task;
      g_assert (!to_kill->is_started);
      g_assert (to_kill->inputs[0] == next);
      kill_unstarted_merge_task (table, to_kill);
    }

  GSK_RBTREE_REMOVE (GET_UNSTARTED_MERGE_TASK_TREE (table), merge_task);

  merge_task->is_started = TRUE;
  merge_task->info.started.output = output;
  merge_task->info.started.inputs[0].reader = readers[0];
  merge_task->info.started.inputs[1].reader = readers[1];

  /* insert into run-list */
  MergeTask **p_next = &table->run_list;
  for (;;)
    {
      MergeTask *next = *p_next;
      guint64 next_n_input_entries;
      if (next == NULL)
        break;
      next_n_input_entries = next->inputs[0]->file->n_entries
                           + next->inputs[1]->file->n_entries;
      if (next_n_input_entries > n_input_entries)
        break;

      p_next = &next->info.started.next_run;
    }
  merge_task->info.started.next_run = *p_next;
  *p_next = merge_task;
  table->n_running_tasks++;
}

static gboolean
maybe_start_tasks (GskTable *table)
{
  while (table->n_running_tasks < table->max_running_tasks
      && table->unstarted_merges != NULL)
    {
      MergeTask *bottom_heaviest = GSK_RBTREE_FIRST (GET_UNSTARTED_MERGE_TASK_TREE (table));
      if (bottom_heaviest > MAX_MERGE_RATIO)
        break;
      if (!start_merge_task (table, bottom_heaviest, error))
        return FALSE;
    }
  return TRUE;
}

static inline gboolean
run_merge_task (GskTable   *table,
                guint       count,
                gboolean    flush,
                GError    **error)
{
  MergeTask *merge_task = table->run_list;
  RunTaskFuncs *run_funcs = table->run_funcs;
  RunTaskFunc func;
  gboolean use_simplify;
  g_assert (merge_task != NULL);

  use_simplify = merge_task->inputs[0]->first_input_entry == 0
              && table->simplify.no_len != NULL;
  func = use_simplify ? (flush ? run_funcs->simplify_flush
                               : run_funcs->simplify_noflush)
                      : (flush ? run_funcs->nosimplify_flush
                               : run_funcs->nosimplify_noflush);
  return (*func) (table, count, error);
}

/**
 * gsk_table_add:
 * @table: the table to add data to.
 * @key_len:
 * @key_data:
 * @value_len:
 * @value_data:
 * @error: place to put the error if something goes wrong.
 *
 * Add a new key/value pair to a GskTable.
 * If the key already exists, the semantics are dependent
 * on the merge function; if no merge function is given,
 * then both rows will exist in the table.
 *
 * returns: whether the addition was successful.
 */
gboolean
gsk_table_add         (GskTable              *table,
                       guint                  key_len,
                       const guint8          *key_data,
                       guint                  value_len,
                       const guint8          *value_data,
                       GError               **error)
{
  TreeNode *found;
  gboolean must_write_journal = table->journal_fd >= 0;
  g_assert (table->key_fixed_length < 0
         || table->key_fixed_length == key_len);
  g_assert (table->value_fixed_length < 0
         || table->value_fixed_length == value_len);

  found = table->in_memory_tree_lookup (table, key_len, key_data);
  table->n_input_entries++;

  if (found)
    {
      /* Merge the old data with the new data. */
      switch (table->merge (key_len, key_data,
                            found->value_len, found->value_data,
                            value_len, value_data,
                            &table->tmp_buffer, table->user_data))
        {
        case GSK_TABLE_MERGE_RETURN_A:
          /* nothing to do */
          break;
        case GSK_TABLE_MERGE_RETURN_B:
          table->in_memory_bytes -= found->value.len;
          table->in_memory_bytes += value_len;
          set_buffer (&found->value, value_len, value_data);
          break;
        case GSK_TABLE_MERGE_SUCCESS:
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
      set_buffer (&new->key, key_len, key_data);
      set_buffer (&new->value, value_len, value_data);

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
      FileInfo *new_file = flush_tree (table);

      /* create MergeTasks */
      if (new_file->prev_file != NULL)
        create_unstarted_merge_task (table, new_file->prev_file, new_file);

      /* maybe flush journal */
      if (table->journalling
       && (++(table->journal_index) == table->journal_period))
        {
          /* write new journal */
          if (!reset_journal (table, error))
            {
              gsk_g_error_add_prefix (error, "error flushing journal");
              return FALSE;
            }

          must_write_journal = FALSE;
        }

      if (!maybe_start_tasks (table, error))
        return FALSE;
    }

  if (table->run_list != NULL)
    {
      if (!run_merge_task (table, 32, FALSE, error))
        return FALSE;
    }

  if (must_write_journal)
    {
      guint new_journal_len = 4 + key_len + 4 + value_len + table->journal_len;
      if (new_journal_len % 4 != 0)
        new_journal_len += (4 - new_journal_len % 4);
      if (new_journal_len + 4 > table->journal_size)
        {
          if (!resize_journal (&table->journal_fd,
                               &table->journal_mmap,
                               &table->journal_size,
                               new_journal_len + 4,
                               error))
            {
              gsk_g_error_add_prefix (error, "expanding journal");
              return FALSE;
            }
        }
      memset (table->journal_mmap + new_journal_len, 0, 4);
      memcpy (table->journal_mmap + 8 + table->journal_len,
              key_data, key_len);
      memcpy (table->journal_mmap + 8 + table->journal_len + key_len,
              value_data, value_len);
      ((guint32*)(table->journal_mmap + table->journal_len))[1]
         = GUINT32_TO_LE (value_len);
      GSK_MEMORY_BARRIER ();
      ((guint32*)(table->journal_mmap + table->journal_len))[0]
         = GUINT32_TO_LE (key_len + 1);
    }
  return TRUE;
}

gboolean
gsk_table_query       (GskTable              *table,
                       guint                  key_len,
                       const guint8          *key_data,
                       gboolean              *found_value_out,
                       guint                 *value_len_out,
                       guint8               **value_data_out,
                       GError               **error)
{
  gboolean reverse = table->query_reverse_chronologically;
  gboolean has_result = FALSE;
  GskTableBuffer *result_buffers = table->result_buffers;       /* [2] */
  GskTableBuffer *result = result_buffers;
  GskTableBuffer *other_result = result_buffers+1;
  GskTableFileQuery *query = table->file_query;

  /* first query rbtree (if in reverse-chronological mode (default)) */
  if (reverse)
    {
      TreeNode *node = table->in_memory_tree_lookup (table, key_len, key_data);
      if (node != NULL)
        {
          has_result = TRUE;
          copy_buffer (&result, &node->value.len);

          /* are we done? */
          if (table->is_stable_func != NULL
           && table->is_stable_func (key_len, key_data,
                                     result.len, result.data,
                                     table->user_data))
            goto done_querying_copy_result;
        }
    }

  /* walk through files, using merge-jobs as appropriate */
  for (fi = reverse ? table->last_file : table->first_file;
       fi != NULL;
       fi = reverse ? fi->prev_file : fi->next_file)
    {
      MergeTask *mt = reverse ? fi->prev_task : fi->next_task;
      gboolean used_merge_output = FALSE;
      if (use_merge_tasks && mt != NULL && mt->is_started)
        {
          if (key before or equal to last writer sync point)
            {
              if (!gsk_table_file_query (mt->info.started.output,
                                         query, error))
                {
                  gsk_g_error_add_prefix (error, "querying merge-task output");
                  goto failed;
                }
              used_merge_output = TRUE;
              goto handle_file_query_result;
            }
        }

      /* query file */
      if (!gsk_table_file_query (fi->file, query, error))
        {
          gsk_g_error_add_prefix (error, "querying merge-task output");
          goto failed;
        }

handle_file_query_result:
      if (query->found)
        {
          if (has_result)
            {
              /* merge values */
              GskTableMergeResult merge_result;
              if (reverse)
                merge_result
                  = table->has_len ?
                       table->merge.with_len (key_len, key_data,
                                              query.result.len,
                                              query.result.data,
                                              result->len, result->data,
                                              other_result,
                                              user_data)
                     : table->merge.no_len (key_data,
                                            query.result.data,
                                            result->data,
                                            other_result,
                                            user_data)
              else
                merge_result
                  = table->has_len ?
                       table->merge.with_len (key_len, key_data,
                                              result->len, result->data,
                                              query.result.len,
                                              query.result.data,
                                              other_result,
                                              user_data)
                     : table->merge.no_len (key_data,
                                            result->data,
                                            query.result.data,
                                            other_result,
                                            user_data)
              switch (merge_result)
                {
                case GSK_TABLE_MERGE_RETURN_A:
                  if (reverse)
                    copy_buffer (result, &query.result);
                  else
                    ; /* do nothing: result is already correct */
                  break;
                case GSK_TABLE_MERGE_RETURN_B:
                  if (!reverse)
                    copy_buffer (result, &query.result);
                  else
                    ; /* do nothing: result is already correct */
                  break;
                case GSK_TABLE_MERGE_SUCCESS:
                  {
                    GskTableBuffer *tmp = result;
                    result = other_result;
                    other_result = tmp;
                    break;
                  }
                case GSK_TABLE_MERGE_DROP:
                  has_result = FALSE;
                  break;
                default:
                  g_assert_not_reached ();
                }
            }
          else if (merge == NULL)
            {
              *found_value_out = TRUE;
              copy_buffer (&result, &query->result);
              goto done_querying_copy_result;
            }
          else
            {
              has_result = TRUE;
              copy_buffer (&result, &query->result);
            }

          /* are we done? */
          if (table->is_stable_func != NULL
           && table->is_stable_func (key_len, key_data,
                                     result.len, result.data,
                                     table->user_data))
            goto done_querying_copy_result;
        }

      /* skip one extra file */
      if (used_merge_output)
        fi = reverse ? fi->prev_file : fi->next_file;
    }

  /* last query rbtree (if in chronological mode) */
  if (!reverse)
    {
      TreeNode *node = table->in_memory_tree_lookup (table, key_len, key_data);
      if (node != NULL)
        {
          if (has_result)
            {
              /* merge */
              GskTableMergeResult merge_result;
              merge_result
                = table->has_len
                     ? table->merge.with_len (key_len, key_data,
                                              result->len, result->data,
                                              node->value.len, node->value.data,
                                              other_result,
                                              user_data)
                     : table->merge.no_len (key_data,
                                            result->data,
                                            node->value.data,
                                            other_result,
                                            user_data);
              switch (merge_result)
                {
                case GSK_TABLE_MERGE_RETURN_A:
                  break;
                case GSK_TABLE_MERGE_RETURN_B:
                  copy_buffer (&result, &node->value);
                  break;
                case GSK_TABLE_MERGE_SUCCESS:
                  {
                    GskTableBuffer *tmp = result;
                    result = other_result;
                    other_result = tmp;
                    break;
                  }
                case GSK_TABLE_MERGE_DROP:
                  has_result = FALSE;
                  break;
                default:
                  g_assert_not_reached ();
                }
            }
          else
            {
              has_result = TRUE;
              copy_buffer (&result, &node->value);
            }

          /* note: no need to check stability,
             since there's nothing more to do anyways. */
        }
    }

done_querying_copy_result:
  *found_value_out = has_result;
  if (has_result)
    {
      *value_len_out = result.len;
      *value_data_out = g_memdup (result.data, result.len);
    }
  /* fall through */

done_querying:
  gsk_table_buffer_clear (&result);
  return TRUE;

failed:
  gsk_table_buffer_clear (&result);
  return FALSE;
}

const char *
gsk_table_peek_dir    (GskTable              *table)
{
  return table->dir;
}

void
gsk_table_destroy     (GskTable              *table)
{
  guint i;
  FileInfo *fi, *next=NULL;
  for (fi = table->first_file; fi != NULL; fi = next)
    {
      next = fi->next_file;
      file_info_unref (fi, table->dir, TRUE);           /* may free fi */
    }
  for (i = 0; i < table->n_old_files; i++)
    file_info_unref (table->old_files[i], table->dir, FALSE);
  g_free (table->files);
  g_free (table->old_files);
  g_free (table->journal_cur_fname);
  g_free (table->journal_tmp_fname);
  munmap (table->journal_mmap, table->journal_size);
  gsk_table_buffer_clear (&table->result_buffers[0]);
  gsk_table_buffer_clear (&table->result_buffers[1]);
  gsk_table_buffer_clear (&table->merge_buffer);
  gsk_table_buffer_clear (&table->simplify_buffer);
  g_slice_free (GskTable, table);
}

/* --- optimizing run_merge_task variants --- */
#define INCL_FLAG_DO_SIMPLIFY    1
#define INCL_FLAG_DO_FLUSH       2
#define INCL_FLAG_HAS_LEN        4
#define INCL_FLAG_MEMCMP         8
#define INCL_FLAG_HAS_MERGE      16

#define INCL_FLAG 0
#include "gsktable-implement-run-merge-task.inc.c"
#define INCL_FLAG 1
#include "gsktable-implement-run-merge-task.inc.c"
#define INCL_FLAG 2
#include "gsktable-implement-run-merge-task.inc.c"
#define INCL_FLAG 3
#include "gsktable-implement-run-merge-task.inc.c"
#define INCL_FLAG 4
#include "gsktable-implement-run-merge-task.inc.c"
#define INCL_FLAG 5
#include "gsktable-implement-run-merge-task.inc.c"
#define INCL_FLAG 6
#include "gsktable-implement-run-merge-task.inc.c"
#define INCL_FLAG 7
#include "gsktable-implement-run-merge-task.inc.c"
#define INCL_FLAG 8
#include "gsktable-implement-run-merge-task.inc.c"
#define INCL_FLAG 9
#include "gsktable-implement-run-merge-task.inc.c"
#define INCL_FLAG 10
#include "gsktable-implement-run-merge-task.inc.c"
#define INCL_FLAG 11
#include "gsktable-implement-run-merge-task.inc.c"
#define INCL_FLAG 12
#include "gsktable-implement-run-merge-task.inc.c"
#define INCL_FLAG 13
#include "gsktable-implement-run-merge-task.inc.c"
#define INCL_FLAG 14
#include "gsktable-implement-run-merge-task.inc.c"
#define INCL_FLAG 15
#include "gsktable-implement-run-merge-task.inc.c"
#define INCL_FLAG 16
#include "gsktable-implement-run-merge-task.inc.c"
#define INCL_FLAG 17
#include "gsktable-implement-run-merge-task.inc.c"
#define INCL_FLAG 18
#include "gsktable-implement-run-merge-task.inc.c"
#define INCL_FLAG 19
#include "gsktable-implement-run-merge-task.inc.c"
#define INCL_FLAG 20
#include "gsktable-implement-run-merge-task.inc.c"
#define INCL_FLAG 21
#include "gsktable-implement-run-merge-task.inc.c"
#define INCL_FLAG 22
#include "gsktable-implement-run-merge-task.inc.c"
#define INCL_FLAG 23
#include "gsktable-implement-run-merge-task.inc.c"
#define INCL_FLAG 24
#include "gsktable-implement-run-merge-task.inc.c"
#define INCL_FLAG 25
#include "gsktable-implement-run-merge-task.inc.c"
#define INCL_FLAG 26
#include "gsktable-implement-run-merge-task.inc.c"
#define INCL_FLAG 27
#include "gsktable-implement-run-merge-task.inc.c"
#define INCL_FLAG 28
#include "gsktable-implement-run-merge-task.inc.c"
#define INCL_FLAG 29
#include "gsktable-implement-run-merge-task.inc.c"
#define INCL_FLAG 30
#include "gsktable-implement-run-merge-task.inc.c"
#define INCL_FLAG 31
#include "gsktable-implement-run-merge-task.inc.c"

#define DEFINE_RUN_TASK_FUNCS(suffix) \
{ run_merge_task__simplify_flush_##suffix, \
  run_merge_task__simplify_noflush_##suffix, \
  run_merge_task__nosimplify_flush_##suffix, \
  run_merge_task__nosimplify_noflush_##suffix }
static RunTaskFuncs all_run_task_funcs[2][2][2] = /* has_len, has_compare, has_merge */
{ { { DEFINE_RUN_TASK_FUNCS(nolen_memcmp_nomerge),
      DEFINE_RUN_TASK_FUNCS(nolen_memcmp_merge) },
    { DEFINE_RUN_TASK_FUNCS(nolen_compare_nomerge),
      DEFINE_RUN_TASK_FUNCS(nolen_compare_merge) } },
  { { DEFINE_RUN_TASK_FUNCS(len_memcmp_nomerge),
      DEFINE_RUN_TASK_FUNCS(len_memcmp_merge) },
    { DEFINE_RUN_TASK_FUNCS(len_compare_nomerge),
      DEFINE_RUN_TASK_FUNCS(len_compare_merge) } } };
#undef DEFINE_RUN_TASK_FUNCS

static RunTaskFuncs *table_options_get_run_funcs (GskTableOptions *options,
                                                  gboolean        *has_len_out,
                                                  GError         **error)
{
  gboolean has_len     = options->compare != NULL
                      || options->merge != NULL
                      || options->simplify != NULL;
  gboolean has_compare = options->compare != NULL
                      || options->compare_no_len != NULL;
  gboolean has_merge   = options->merge != NULL
                      || options->merge_no_len != NULL;
#define TEST_MEMBER_NULL(member) \
  if (options->member != NULL) \
    { \
      g_set_error (error, GSK_G_ERROR_DOMAIN, GSK_ERROR_INVALID_ARG, \
                   "length and no-length function pointers mixed:  did not expect %s to be non-NULL", \
                   #member); \
      return NULL; \
    }
  if (has_len)
    {
      TEST_MEMBER_NULL (compare_no_len);
      TEST_MEMBER_NULL (merge_no_len);
      TEST_MEMBER_NULL (simplify_no_len);
    }
  else
    {
      TEST_MEMBER_NULL (compare);
      TEST_MEMBER_NULL (merge);
      TEST_MEMBER_NULL (simplify);
    }
  *has_len_out = has_len;
  return &all_run_task_funcs[has_len?1:0][has_compare?1:0][has_merge?1:0];
}
