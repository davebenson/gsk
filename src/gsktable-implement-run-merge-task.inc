/* MACROS used by this file:
     RECOMPUTE_SHOULD_TERMINATE
        update the should_terminate variable,
	based on n_written or last_feed_result.

     COMPARE_CODE
        set compare_rv based on readers[0], readers[1]

     MERGE_CODE    [optional]
        set output_buffer based on readers[0], readers[1]
 */
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
	  {RECOMPUTE_SHOULD_TERMINATE;}
	  if (should_terminate)
	    goto done;
	}
    }
  else if (merge_task->files[1]->file->eof)
    {
      ...
    }
  else
    {
      int compare_rv;
      {COMPARE_CODE;}
#ifdef MERGE_CODE
      if (compare_rv < 0)
#else
      if (compare_rv <= 0)
#endif
	{
	  /* write a */
	  ...
	}
#ifdef MERGE_CODE
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
