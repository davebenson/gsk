//#define INCL_FLAG_DO_SIMPLIFY    1
//#define INCL_FLAG_DO_FLUSH       2
//#define INCL_FLAG_HAS_LEN        4
//#define INCL_FLAG_MEMCMP         8
//#define INCL_FLAG_HAS_MERGE      16

#undef DO_SIMPLIFY
#if (INCL_FLAG & INCL_FLAG_DO_SIMPLIFY)
# define DO_SIMPLIFY   1
# define FUNC_NAME_P0  simplify_
#else
# define DO_SIMPLIFY   0
# define FUNC_NAME_P0  nosimplify_
#endif

#undef DO_FLUSH
#if (INCL_FLAG & INCL_FLAG_DO_FLUSH)
# define DO_FLUSH      1
# define FUNC_NAME_P1  flush_
#else
# define DO_SIMPLIFY   0
# define FUNC_NAME_P1  noflush_
#endif

#undef HAS_LEN
#if (INCL_FLAG & INCL_FLAG_HAS_LEN)
# define HAS_LEN       1
# define FUNC_NAME_P2  len_
#else
# define HAS_LEN       0
# define FUNC_NAME_P2  nolen_
#endif

#undef USE_MEMCMP
#if (INCL_FLAG & INCL_FLAG_MEMCMP)
# define USE_MEMCMP       1
# define FUNC_NAME_P3  memcmp_
#else
# define USE_MEMCMP       0
# define FUNC_NAME_P3  compare_
#endif

#undef HAS_MERGE
#if (INCL_FLAG & INCL_FLAG_HAS_MERGE)
# define HAS_MERGE       1
# define FUNC_NAME_P4  merge
#else
# define HAS_MERGE       0
# define FUNC_NAME_P4  nomerge
#endif

#define FUNC_SUFFIX  FUNC_NAME_P0##FUNC_NAME_P2##FUNC_NAME_P3##FUNC_NAME_P4##FUNC_NAME_P5
#define FUNC_NAME run_merge_task__##FUNC_SUFFIX
#define COPY_FILE_READER copy_file_reader__##FUNC_SUFFIX

#if HAS_LEN
# define MAYBE_LEN(opt_len)   opt_len,
#else
# define MAYBE_LEN(opt_len)
#endif

static gboolean
COPY_FILE_READER (GskTable  *table,
                  MergeTask *task,
                  guint      input_index,
                  gboolean  *is_done_out,
                  GError   **error)
{
  GskTableReader *reader = task->info.started.inputs[input_index].reader;
  GskTableFile *output = merge_task->info.started.output;
  for (;;)
    {
      switch (gsk_table_file_feed_entry (output,
                                         reader->key_len, reader->key_data,
                                         reader->value_len, reader->value_data,
                                         error))
        {
        case GSK_TABLE_FEED_ENTRY_WANT_MORE:
          ...
          break;
        case GSK_TABLE_FEED_ENTRY_SUCCESS:
          ...
          break;
        case GSK_TABLE_FEED_ENTRY_ERROR:
          ...
          break;
        default:
          g_assert_not_reached ();
        }
      n_written++;
#if !DO_FLUSH
      if (n_written >= iterations)
        {
          *is_done_out = TRUE;
          return TRUE;
        }
#endif
    }
}


static gboolean
FUNC_NAME  (GskTable      *table,
            guint          iterations,
	    GError       **error)
{
  MergeTask *task = table->run_list;
  g_assert (task->is_started);

#if !USE_MEMCMP
# if HAS_LEN
  GskTableCompareFunc compare = table->compare.with_len;
# else
  GskTableCompareFuncNoLen compare = table->compare.no_len;
# endif
#endif

#if HAS_MERGE
# if HAS_LEN
  GskTableMergeFunc merge = table->merge.with_len;
# else
  GskTableMergeFuncNoLen merge = table->merge.no_len;
# endif
#endif

#if DO_SIMPLIFY
# if HAS_LEN
  GskTableSimplifyFunc = table->simplify.with_len;
# else
  GskTableSimplifyFuncNoLen = table->simplify.no_len;
# endif
  GskTableBuffer *simp_buffer = &table->simplify_buffer;
#endif

#if !USE_MEMCMP || HAS_LEN || DO_SIMPLIFY
  gpointer user_data = table->user_data;
#endif
  GskTableFile *output = task->info.started.output;

restart_testing_eof:

  if (readers[0]->eof)
    {
      if (readers[1]->eof)
	{
	  is_done = TRUE;
	  goto done;
	}
      if (!COPY_FILE_READER (table, merge_task, 1, &is_done, error))
        return FALSE;
      if (is_done)
        goto done;
      return TRUE;
    }
  else if (readers[1]->eof)
    {
      if (!COPY_FILE_READER (table, merge_task, 0, &is_done, error))
        return FALSE;
      if (is_done)
        goto done;
      return TRUE;
    }
  else
    {
      int compare_rv;
#if USE_MEMCMP
      compare_rv = compare_memory (readers[0]->key_len,
                                   readers[0]->key_data,
                                   readers[1]->key_len,
                                   readers[1]->key_data);
#else
      compare_rv = compare (MAYBE_LEN (reader[0]->key_len)
                            reader[0]->key_data,
                            MAYBE_LEN (reader[1]->key_len)
                            reader[1]->key_data,
                            user_data);
#endif

#if HAS_MERGE
      if (compare_rv < 0)
#else
      if (compare_rv <= 0)
#endif
	{
	  /* write a */
          guint value_len;
          const guint8 *value_data;
#if DO_SIMPLIFY
	  switch (simplify (MAYBE_LEN (reader[0]->key_len)
                            reader[0]->key_data,
	                    MAYBE_LEN (reader[0]->value_len)
                            reader[0]->value_data,
                            simp_buffer,
                            user_data))
            {
            case GSK_TABLE_SIMPLIFY_IDENTITY:
              value_len = reader[0]->value_len;
              value_data = reader[0]->value_data;
              break;
            case GSK_TABLE_SIMPLIFY_SUCCESS:
              value_len = simp_buffer->len;
              value_data = simp_buffer->data;
              break;
            case GSK_TABLE_SIMPLIFY_DELETE:
              goto do_advance_a;
            default:
              g_assert_not_reached ();
            }
#else
          value_len = reader[0]->value_len;
          value_data = reader[0]->value_data;
#endif
          switch (gsk_table_file_feed_entry (output,
                                             reader[0]->key_len,
                                             reader[0]->key_data,
                                             value_len,
                                             value_data,
                                             error))
            {
            case GSK_TABLE_FEED_ENTRY_WANT_MORE:
              ...
              break;
            case GSK_TABLE_FEED_ENTRY_SUCCESS:
              ...
              break;
            case GSK_TABLE_FEED_ENTRY_ERROR:
              ...
              break;
            default:
              g_assert_not_reached ();
            }

do_advance_a:
          /* advance a, possibly becoming eof or error */
          gsk_table_reader_advance (reader[0]);
          if (reader[0]->error)
            {
              g_set_error (error, reader[0]->error->domain,
                           reader[0]->error->code,
                           "run_merge_task: error reading: %s",
                           reader[0]->error->message);
              return FALSE;
            }
          if (reader[0]->eof)
            goto restart_testing_eof;
	}
#if HAS_MERGE
      else if (compare_rv == 0)
	{
	  /* merge and write merged key/value */
          guint value_len;
          const guint8 *value_data;
          switch (merge (MAYBE_LEN (reader[0]->key_len)
                         reader[0]->key_data,
                         MAYBE_LEN (reader[0]->value_len)
                         reader[0]->value_data,
                         MAYBE_LEN (reader[1]->value_len)
                         reader[1]->value_data,
                         merge_buf,
                         user_data))
            {
            case GSK_TABLE_MERGE_RETURN_A:
              value_len = reader[0]->value_len;
              value_data = reader[0]->value_data;
              break;
            case GSK_TABLE_MERGE_RETURN_B:
              value_len = reader[1]->value_len;
              value_data = reader[1]->value_data;
              break;
            case GSK_TABLE_MERGE_SUCCESS:
              value_len = merge_buf->len;
              value_data = merge_buf->data;
              break;
            case GSK_TABLE_MERGE_DROP:
              goto do_advance_a_and_b;
            default:
              g_assert_not_reached ();
            }

#if DO_SIMPLIFY
          switch (simplify (MAYBE_LEN (reader[0]->key_len)
                            reader[0]->key_data,
	                    MAYBE_LEN (value_len)
                            value_data,
                            simp_buffer,
                            user_data))
            {
            case GSK_TABLE_SIMPLIFY_IDENTITY:
              break;
            case GSK_TABLE_SIMPLIFY_SUCCESS:
              value_len = simp_buffer->len;
              value_data = simp_buffer->data;
              break;
            case GSK_TABLE_SIMPLIFY_DELETE:
              goto do_advance_a_and_b;
            default:
              g_assert_not_reached ();
            }
#endif
          switch (gsk_table_file_feed_entry (output,
                                             reader[0]->key_len,
                                             reader[0]->key_data,
                                             value_len,
                                             value_data,
                                             error))
            {
            case GSK_TABLE_FEED_ENTRY_WANT_MORE:
              ...
              break;
            case GSK_TABLE_FEED_ENTRY_SUCCESS:
              ...
              break;
            case GSK_TABLE_FEED_ENTRY_ERROR:
              ...
              break;
            default:
              g_assert_not_reached ();
            }

do_advance_a_and_b:
          /* advance a and b, possibly becoming eof or error */
          gsk_table_reader_advance (reader[0]);
          gsk_table_reader_advance (reader[1]);
          if (reader[0]->error)
            {
              g_set_error (error, reader[0]->error->domain,
                           reader[0]->error->code,
                           "run_merge_task: error reading: %s",
                           reader[0]->error->message);
              return FALSE;
            }
          if (reader[1]->error)
            {
              g_set_error (error, reader[1]->error->domain,
                           reader[1]->error->code,
                           "run_merge_task: error reading: %s",
                           reader[1]->error->message);
              return FALSE;
            }
          if (reader[0]->eof || reader[1]->eof)
            goto restart_testing_eof;
	}
#endif
      else
	{
	  /* write b */
          guint value_len;
          const guint8 *value_data;
#if DO_SIMPLIFY
	  switch (simplify (MAYBE_LEN (reader[1]->key_len)
                            reader[1]->key_data,
	                    MAYBE_LEN (reader[1]->value_len)
                            reader[1]->value_data,
                            simp_buffer,
                            user_data))
            {
            case GSK_TABLE_SIMPLIFY_IDENTITY:
              value_len = reader[1]->value_len;
              value_data = reader[1]->value_data;
              break;
            case GSK_TABLE_SIMPLIFY_SUCCESS:
              value_len = simp_buffer->len;
              value_data = simp_buffer->data;
              break;
            case GSK_TABLE_SIMPLIFY_DELETE:
              goto do_advance_a;
            default:
              g_assert_not_reached ();
            }
#else
          value_len = reader[1]->value_len;
          value_data = reader[1]->value_data;
#endif
          switch (gsk_table_file_feed_entry (output,
                                             reader[1]->key_len,
                                             reader[1]->key_data,
                                             value_len,
                                             value_data,
                                             error))
            {
            case GSK_TABLE_FEED_ENTRY_WANT_MORE:
              ...
              break;
            case GSK_TABLE_FEED_ENTRY_SUCCESS:
              ...
              break;
            case GSK_TABLE_FEED_ENTRY_ERROR:
              ...
              break;
            default:
              g_assert_not_reached ();
            }

do_advance_b:
          /* advance b, possibly becoming eof or error */
          gsk_table_reader_advance (reader[1]);
          if (reader[1]->error)
            {
              g_set_error (error, reader[1]->error->domain,
                           reader[1]->error->code,
                           "run_merge_task: error reading: %s",
                           reader[1]->error->message);
              return FALSE;
            }
          if (reader[1]->eof)
            goto restart_testing_eof;
	}
    }
  return TRUE;
}

#undef FUNC_NAME_P0
#undef FUNC_NAME_P1
#undef FUNC_NAME_P2
#undef FUNC_NAME_P3
#undef FUNC_NAME_P4
#undef INCL_FLAG

