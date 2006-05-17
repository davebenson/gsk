#include "gskshutdown.h"
#include "gskmainloop.h"
#include "gskmain.h"

struct _GskShutdownHandler
{
  char *name;
  GskShutdownFunc func;
  gpointer data;
  gboolean done;
  GskShutdownHandler *prev;
  GskShutdownHandler *next;
};

typedef struct _MessageHandler MessageHandler;
struct _MessageHandler
{
  GskShutdownMessageFunc func;
  gpointer               data;
  MessageHandler        *next;

  guint                  notifying : 1;
  guint                  removed_while_notifying : 1;
};

static GskShutdownHandler *first_handler = NULL, *last_handler = NULL;
static MessageHandler *first_message_handler = NULL, *last_message_handler = NULL;
static gboolean is_shutting_down = FALSE;
static guint handler_wait_count = 0;

static GskShutdownHandler *
register_handler_valist (GskShutdownFunc func,
			 gpointer  data,
			 const char *format,
			 va_list args)
{
  GskShutdownHandler *rv;
  g_return_val_if_fail (!is_shutting_down, NULL);
  rv = g_new (GskShutdownHandler, 1);
  rv->func = func;
  rv->data = data;
  rv->done = FALSE;
  rv->name = g_strdup_vprintf (format, args);
  rv->next = NULL;
  rv->prev = last_handler;
  if (last_handler)
    last_handler->next = rv;
  else
    first_handler = rv;
  last_handler = rv;
  return rv;
}

/* registering shutdown handlers */
GskShutdownHandler *gsk_shutdown_register_handler   (GskShutdownFunc func,
                                                     gpointer  data,
						     const char *format,
						     ...)
{
  va_list args;
  GskShutdownHandler *rv;
  va_start (args, format);
  rv = register_handler_valist (func, data, format, args);
  va_end (args);
  return rv;
}

void gsk_shutdown_unregister_handler (GskShutdownHandler *handler)
{
  g_return_if_fail (!is_shutting_down);
  if (handler->prev)
    handler->prev->next = handler->next;
  else
    first_handler = handler->next;
  if (handler->next)
    handler->next->prev = handler->prev;
  else
    last_handler = handler->prev;
  g_free (handler->name);
  g_free (handler);
}

static void
send_message (const char *str)
{
  MessageHandler *at;
  if (first_message_handler == NULL)
    return;
  for (at = first_message_handler; at; )
    {
      g_return_if_fail (!at->notifying);
      at->notifying = 1;
      at->func (str, at->data);
      at->notifying = 0;
      if (!at->removed_while_notifying)
	{
	  at = at->next;
	}
      else
	{
	  MessageHandler *next = at->next;
	  g_free (at);
	  at = next;
	}
    }
}

static gboolean
check_which_handlers (gpointer data)
{
  GString *message = g_string_new ("Unfinished shutdown handlers:");
  GskShutdownHandler *at;
  for (at = first_handler; at; at = at->next)
    if (!at->done)
      g_string_append_printf (message, " [%s]", at->name);
  send_message (message->str);
  g_string_free (message, TRUE);
  return TRUE;
}

void gsk_shutdown (void)
{
  GskShutdownHandler *at;
  g_return_if_fail (!is_shutting_down);
  is_shutting_down = TRUE;

  /* we hold a count here until we are done running all the shutdown handlers;
     otherwise we might run gsk_main_quit() before it is time */
  handler_wait_count = 1;

  for (at = first_handler; at; at = at->next)
    {
      ++handler_wait_count;
      at->func (at, at->data);
    }

  /* print a little shutdown message every so often */
  gsk_main_loop_add_timer (gsk_main_loop_default (), check_which_handlers, NULL, NULL, 3000, 3000);

  g_return_if_fail (handler_wait_count > 0);
  if (--handler_wait_count == 0)
    {
      send_message ("done shutting down");
      gsk_main_quit ();
    }
}

void gsk_shutdown_handler_done (GskShutdownHandler *handler)
{
  g_return_if_fail (handler_wait_count > 0);
  g_return_if_fail (!handler->done);
  g_return_if_fail (is_shutting_down);
  handler->done = TRUE;
  if (--handler_wait_count == 0)
    {
      send_message ("done shutting down");
      gsk_main_quit ();
    }
}

void gsk_shutdown_message_trap   (GskShutdownMessageFunc func,
				  gpointer               data)
{
  MessageHandler *mh = g_new (MessageHandler, 1);
  mh->func =func;
  mh->data = data;
  mh->next = NULL;
  mh->notifying = mh->removed_while_notifying = 0;
  if (last_message_handler)
    last_message_handler->next = mh;
  else
    first_message_handler = mh;
  last_message_handler = mh;
}

void gsk_shutdown_message_untrap (GskShutdownMessageFunc func,
				  gpointer               data)
{
  MessageHandler *prev = NULL;
  MessageHandler *at;
  for (at = first_message_handler; at; at = at->next)
    {
      if (at->func == func && at->data == data)
	{
	  if (at->notifying)
	    {
	      at->removed_while_notifying = 1;
	      return;
	    }
	  if (prev)
	    prev->next = at->next;
	  else
	    first_message_handler = at->next;
	  if (at == last_message_handler)
	    last_message_handler = prev;
	  g_free (at);
	  return;
	}
      prev = at;
    }
  g_return_if_reached ();
}

