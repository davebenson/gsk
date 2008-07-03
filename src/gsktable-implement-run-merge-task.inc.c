#define INCL_FLAG_DO_SIMPLIFY    1
#define INCL_FLAG_DO_FLUSH       2
#define INCL_FLAG_HAS_LEN        4
#define INCL_FLAG_MEMCMP         8
#define INCL_FLAG_HAS_MERGE      16

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

#define FUNC_NAME run_merge_task__##FUNC_NAME_P0##FUNC_NAME_P2##FUNC_NAME_P3##FUNC_NAME_P4##FUNC_NAME_P5

#undef FUNC_NAME_P0
#undef FUNC_NAME_P1
#undef FUNC_NAME_P2
#undef FUNC_NAME_P3
#undef FUNC_NAME_P4
#undef INCL_FLAG


static gboolean
FUNC_NAME  (GskTable      *table,
            guint          iterations,
	    GError       **error)
{
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
#if !USE_MEMCMP || HAS_LEN
  gpointer user_data = table->user_data;
#endif

restart_testing_eof:

  if (readers[0]->eof)
    {
      if (readers[1]->eof)
	{
	  is_done = TRUE;
	  goto done;
	}
      for (;;)
	{
	  ...
	  n_written++;
#if !DO_FLUSH
          if (n_written >= iterations)
	    goto done;
#endif
	}
    }
  else if (merge_task->files[1]->file->eof)
    {
      ...
    }
  else
    {
      int compare_rv;
#if USE_MEMCMP
      if (readers[0]->key_len < readers[1]->key_len[1])
        {
	  compare_rv = memcmp (readers[0]->key_data, readers[1]->key_data,
	                       readers[0]->key_len);
          if (compare_rv == 0)
	    compare_rv = 1;
        }
      else if (readers[0]->key_len > readers[1]->key_len[1])
        {
	  compare_rv = memcmp (readers[0]->key_data, readers[1]->key_data,
	                       readers[1]->key_len);
          if (compare_rv == 0)
	    compare_rv = -1;
        }
      else
	compare_rv = memcmp (readers[0]->key_data, readers[1]->key_data,
			     readers[0]->key_len);
#elif HAS_LEN
      compare_rv = table->compare.with_len (reader[0]->key_len,
                                            reader[0]->key_data,
                                            reader[1]->key_len,
                                            reader[1]->key_data,
					    user_data);
#else
      compare_rv = table->compare.with_len (reader[0]->key_data,
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
#if DO_SIMPLIFY
	  ...
#else
	  ...
#endif
	}
#if HAS_MERGE
      else if (compare_rv == 0)
	{
	  /* merge and write merged key/value */
	  ...
	}
#endif
      else
	{
	  /* write b */
	  ...
	}
    }
  return TRUE;
}
