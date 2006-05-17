#ifndef __GSK_STREAM_CONNECTION_H_
#define __GSK_STREAM_CONNECTION_H_

#include "gskstream.h"

G_BEGIN_DECLS

typedef struct _GskStreamConnection GskStreamConnection;

GType gsk_stream_connection_get_type(void) G_GNUC_CONST;
#define GSK_TYPE_STREAM_CONNECTION	(gsk_stream_connection_get_type ())
#define GSK_STREAM_CONNECTION(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), GSK_TYPE_STREAM_CONNECTION, GskStreamConnection))
#define GSK_IS_STREAM_CONNECTION(obj)   (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GSK_TYPE_STREAM_CONNECTION))

GskStreamConnection *
       gsk_stream_connection_new              (GskStream        *input_stream,
                                               GskStream        *output_stream,
              		                       GError          **error);
void   gsk_stream_connection_detach           (GskStreamConnection *connection);
void   gsk_stream_connection_shutdown         (GskStreamConnection *connection);
void   gsk_stream_connection_set_max_buffered (GskStreamConnection *connection,
              		                       guint                max_buffered);
guint  gsk_stream_connection_get_max_buffered (GskStreamConnection *connection);
void   gsk_stream_connection_set_atomic_read_size(GskStreamConnection *connection,
              		                       guint                atomic_read_size);
guint  gsk_stream_connection_get_atomic_read_size(GskStreamConnection *connection);

G_END_DECLS

#endif
