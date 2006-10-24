/* a throttling transparent proxy. */

typedef struct _GskThrottleProxyConnection GskThrottleProxyConnection;
typedef struct _Side Side;

/* configuration */
guint upload_per_second_base = 10*1024;
guint download_per_second_base = 100*1024;
guint upload_per_second_noise = 1*1024;
guint download_per_second_noise = 10*1024;

struct _Side
{
  GskStream *read_side;		/* client for upload, server for download */
  GskStream *write_side;	/* client for upload, server for download */
  gboolean read_side_blocked;
  gboolean write_side_blocked;

  /* sides are in this list if their xferred_in_last_second==max
     but buffer.size < max_buffer */
  Side *next_throttled, *prev_throttled;
  gboolean throttled;

  guint max_xfer_per_second;
  gulong last_xfer_second;
  guint xferred_in_last_second;

  GskBuffer buffer;

  guint max_buffer;/* should be set to max_xfer_per_second or a bit more */
};

struct _GskThrottleProxyConnection
{
  GskStream *server;
  GskStream *client;
  GskBuffer  upload;	/* client => server data */
  GskBuffer  download;	/* server => client data */

  guint      max_upload_per_second;
  gulong     last_upload_second;
  guint      uploaded_in_last_second;
  guint      max_download_per_second;
  gulong     last_download_second;
  guint      downloaded_in_last_second;

  gboolean   server_write_block, server_read_block;
  gboolean   client_write_block, client_read_block;
};

#define CURRENT_SECOND() (gsk_main_loop_default ()->current_time.tv_sec)

/* must be called whenever side->buffer changes "emptiness" */
static inline void
update_write_block (Side   *side)
{
  gboolean old_val = side->write_side_blocked;
  gboolean val = (side->buffer.size == 0);
  side->write_side_blocked = val;

  if (old_val && !val)
    gsk_io_block_writable (side->write_side);
  else if (!old_val && val)
    gsk_io_unblock_writable (side->write_side);
}

/* must be called whenever side->buffer changes "emptiness" */
static inline void
update_read_block (Side   *side)
{
  gboolean was_throttled = side->throttled;
  gboolean old_val = side->write_side_blocked;
  gboolean xfer_blocked = side->xferred_in_last_second >= side->max_xfer_per_second;
  gboolean buf_blocked = side->buffer.size < side->max_buffer;
  gboolean val = xfer_blocked || buf_blocked;
  side->throttled = xfer_blocked && !buf_blocked;
  side->write_side_blocked = val;

  if (side->throttled && !was_throttled)
    {
      /* put in throttled list */
      ...
    }
  else if (!side->throttled && was_throttled)
    {
      /* remove from throttled list */
      ...
    }

  if (old_val && !val)
    gsk_io_block_writable (side->write_side);
  else if (!old_val && val)
    gsk_io_unblock_writable (side->write_side);


}

static gboolean
handle_side_writable (GskStream *stream,
                      gpointer data)
{
  Side *side = data;
  GError *error = NULL;
  gsk_stream_write_buffer (stream, &conn->buffer, &error);
  if (error)
    {
      g_warning ("error writing to stream %p: %s",
                 stream, error->message);
      g_error_free (error);
    }
  update_write_block (side);
  return TRUE;
}

static gboolean
handle_side_readable (GskStream *stream,
                      gpointer data)
{
  Side *side = data;
  gulong cur_sec = CURRENT_SECOND ();
  GError *error = NULL;
  if (cur_sec == side->last_xfer_second)
    {
      max_read = side->max_xfer_per_second - side->xferred_in_last_second;
    }
  else
    {
      side->xferred_in_last_second = 0;
      side->last_xfer_second = cur_sec;
      max_read = side->max_xfer_per_second;
    }

  nread = gsk_stream_read_buffer (stream, &side->buffer, &error);
  if (error == NULL)
    {
      g_warning ("error reading from stream %p: %s",
		 stream, error->message);
      g_error_free (error);
    }

  side->xferred_in_last_second += nread;
  g_assert (side->xferred_in_last_second <= side->max_xfer_per_second);
  update_write_block (side);

  if (side->xferred_in_last_second == side->max_xfer_per_second)
    {
      g_assert (!side->read_side_blocked);
      gsk_io_block_readable (side->read_side);
      side->read_side_blocked = TRUE;
      read_blocked_until_next_second 
        = g_slist_prepend (read_blocked_until_next_second, side);
    }
}


/* --- handle a new stream --- */
static gboolean
handle_accept  (GskStream    *stream,
                gpointer      data,
                GError      **error);


/* --- unblock throttled streams every second --- */
static gboolean
unblock_timer_func (gpointer data)
{
  Side *at = first_throttled;
  gulong sec = CURRENT_SECOND ();
  while (at)
    {
      Side *next = at->next_throttled;
      g_assert (side->throttled);
      g_assert (side->read_side_blocked);
      if (sec > side->last_xfer_second)
        {
	  side->last_xfer_second = sec;
	  side->xferred_in_last_second = 0;
          update_read_block (at);
        }
      at = next;
    }

  /* schedule next timeout */
  ...

  return FALSE;
}

/* --- main --- */
int main(int argc, char **argv)
{
  ...
}
