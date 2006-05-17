#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "gskerrno.h"

/**
 * gsk_errno_is_ignorable:
 * @errno_value: errno value.
 *
 * Determine whether an errno code is insignificant.
 *
 * returns: whether the error is a transient ignorable error.
 */
gboolean
gsk_errno_is_ignorable (int errno_value)
{
#ifdef EWOULDBLOCK
  if (errno_value == EWOULDBLOCK)
    return TRUE;
#endif
  return errno_value == EINTR || errno_value == EAGAIN;
}

/**
 * gsk_errno_from_fd:
 * @fd: a file descriptor.
 *
 * Query the file descriptor for the last error which occurred on it.
 *
 * returns: the errno code for the last error on the file descriptor.
 */
int
gsk_errno_from_fd (int fd)
{
  socklen_t size_int = sizeof (int);
  int value = EINVAL;
  if (getsockopt (fd, SOL_SOCKET, SO_ERROR, &value, &size_int) < 0)
    {
      /* Note: this behavior is vaguely hypothetically broken,
       *       in terms of ignoring getsockopt's error;
       *       however, this shouldn't happen, and EINVAL is ok if it does.
       *       Furthermore some broken OS's return an error code when
       *       fetching SO_ERROR!
       */
      return value;
    }

  return value;
}
