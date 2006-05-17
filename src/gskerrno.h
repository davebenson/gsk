#ifndef __GSK_ERRNO_H_
#define __GSK_ERRNO_H_

#include <glib.h>

G_BEGIN_DECLS

gboolean gsk_errno_is_ignorable (int errno_value);
int gsk_errno_from_fd (int fd);

G_END_DECLS

#endif
