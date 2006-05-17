/* Safe shutdown handling.
 *
 * Occasionally you will have a server where a mere call to 'exit()'
 * is too harsh: for example, you may have disk data cached that needs
 * to be sync()d, or you may have a task running that isn't interruptable.
 *
 * In those cases, call gsk_shutdown_register_handler()
 * to provide a function that'll be called when gsk_shutdown() is called.
 * The shutdown will only occur when all the registered
 * handlers have indicated that they are ready by calling gsk_shutdown_handler_done().
 */

#ifndef __GSK_SHUTDOWN_H_
#define __GSK_SHUTDOWN_H_

#include "gskstream.h"

G_BEGIN_DECLS

typedef struct _GskShutdownHandler GskShutdownHandler;

typedef void (*GskShutdownFunc) (GskShutdownHandler *handler,
				 gpointer            data);
typedef void (*GskShutdownMessageFunc) (const char         *info,
				        gpointer            data);

/* registering shutdown handlers */
GskShutdownHandler *gsk_shutdown_register_handler   (GskShutdownFunc func,
                                                     gpointer  data,
						     const char *format,
						     ...) G_GNUC_PRINTF(3,4);
void                gsk_shutdown_unregister_handler (GskShutdownHandler *);

/* requesting shutdown */
void gsk_shutdown (void);

/* to be called by the GHookFunc passed to gsk_shutdown_register_handler() */
void gsk_shutdown_handler_done (GskShutdownHandler *);

/* trapping shutdown-related messages */
void gsk_shutdown_message_trap   (GskShutdownMessageFunc func,
				  gpointer               data);
void gsk_shutdown_message_untrap (GskShutdownMessageFunc func,
				  gpointer               data);

G_END_DECLS

#endif
