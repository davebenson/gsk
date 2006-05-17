#include "gsktimegm.h"
#include "config.h"

/* If threadsafe versions of gmtime and localtime
   are not available, just hack it, and define
   lookalike macros. */
#if !HAVE_GMTIME_R
#define gmtime_r(t, tm)  G_STMT_START{ (*tm) = (*gmtime(t)); } G_STMT_END
#endif
#if !HAVE_LOCALTIME_R
#define localtime_r(t, tm)  G_STMT_START{ (*tm) = (*localtime(t)); } G_STMT_END
#endif

/* Helper function:
      int subtract_nearby_struct_tm(a, b);
   Compute *a - *b, in seconds, where a,b differ by less than a day. */
#if !HAVE_TIMEGM
static int
subtract_nearby_struct_tm (const struct tm *a, const struct tm *b)
{
  /* TO CONSIDER: should this incorporate sanity checks? */
  int hour_delta = a->tm_hour - b->tm_hour;
  int minute_delta = a->tm_min - b->tm_min;
  int second_delta = a->tm_sec - b->tm_sec;
  int day_delta = a->tm_mday - b->tm_mday;
  int minutes_diff;
  if (day_delta >= 27)
    day_delta = -1;
  else if (day_delta <= -27)
    day_delta = +1;
  minutes_diff = ((day_delta * 24 + hour_delta) * 60 + minute_delta);
  return minutes_diff * 60 + second_delta;
}
#endif

/**
 * gsk_timegm:
 * @t: the time to treat as UTC time.
 *
 * Convert a broken down time representation
 * in UTC (== Grenwich Mean Time, GMT)
 * to the number of seconds since the Great Epoch.
 *
 * returns: the number of seconds since the beginning of 1970
 * in Grenwich Mean Time.  (This is also none as unix time)
 */
time_t
gsk_timegm(const struct tm *t)
{
  struct tm copy = *t;
#if HAVE_TIMEGM
  /* Note: we must copy 't', because the GNU implementation of
     timegm actually modifies its input. */
  return timegm (&copy);
#else
  time_t mktime_result = mktime (&copy);
  struct tm gm_tmp;
  struct tm local_tmp;
  int local_to_gm;
  gmtime_r (&mktime_result, &gm_tmp);
  localtime_r (&mktime_result, &local_tmp);
  local_to_gm = subtract_nearby_struct_tm (&local_tmp, &gm_tmp);
  return mktime_result + local_to_gm;
#endif	/* !HAVE_TIMEGM */
}
