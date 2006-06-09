#ifndef __GSK_ERROR_H_
#define __GSK_ERROR_H_

#include <glib-object.h>

G_BEGIN_DECLS

/* --- the error domain for internal gsk errors (private) --- */
extern GQuark gsk_g_error_domain_quark;
extern GType gsk_error_code_type;

/* public macros for accessing the above */
#define GSK_G_ERROR_DOMAIN	(gsk_g_error_domain_quark)
#define GSK_TYPE_ERROR_CODE	(gsk_error_code_type)

/* --- types of errors that can occur in gsk --- */
typedef enum
{
  GSK_ERROR_NONE,

  GSK_ERROR_ALREADY_ATTACHED,
  GSK_ERROR_BUFFER_UNBUFFER_MIXED,
  GSK_ERROR_STREAM_ATTACH_NOT_READABLE,
  GSK_ERROR_STREAM_ATTACH_NOT_WRITABLE,
  GSK_ERROR_READ_POLL_FAILED,
  GSK_ERROR_WRITE_POLL_FAILED,
  GSK_ERROR_SHUTDOWN_READ_FAILED,
  GSK_ERROR_SHUTDOWN_WRITE_FAILED,
  GSK_ERROR_OPEN_FAILED,
  GSK_ERROR_UNEXPECTED_PARTIAL_WRITE,
  GSK_ERROR_UNEXPECTED_PARTIAL_READ,
  GSK_ERROR_FOREIGN_ADDRESS,
  GSK_ERROR_ACCEPTED_SOCKET_FAILED,
  GSK_ERROR_BIND_FAILED,
  GSK_ERROR_BIND_UNAVAILABLE,
  GSK_ERROR_END_OF_FILE,
  GSK_ERROR_NOT_READY,
  GSK_ERROR_IO,
  GSK_ERROR_UNKNOWN,
  GSK_ERROR_FULL,
  GSK_ERROR_VERSION,
  GSK_ERROR_INTERNAL,
  GSK_ERROR_INVALID_STATE,
  GSK_ERROR_LINGERING_DATA,		/* close lost data */
  GSK_ERROR_MULTIPLE_PROBLEMS,

  /* resolver errors */
  GSK_ERROR_RESOLVER_NOT_FOUND,
  GSK_ERROR_RESOLVER_NO_NAME_SERVERS,
  GSK_ERROR_RESOLVER_TOO_MANY_FAILURES,
  GSK_ERROR_RESOLVER_NO_DATA,
  GSK_ERROR_RESOLVER_ACCESS,
  GSK_ERROR_RESOLVER_SOCKET_DIED,
  GSK_ERROR_RESOLVER_FORMAT,
  GSK_ERROR_RESOLVER_SERVER_PROBLEM,

  /* http errors */
  GSK_ERROR_HTTP_PARSE,
  GSK_ERROR_HTTP_NOT_FOUND,

  /* regex errors */
  GSK_ERROR_REGEX_SYNTAX,

  /* errno imports */
  GSK_ERROR_TOO_MANY_SYSTEM_FDS,
  GSK_ERROR_TOO_MANY_PROCESS_FDS,
  GSK_ERROR_ADDRESS_ALREADY_IN_USE,
  GSK_ERROR_ADDRESS_FAMILY_NOT_SUPPORTED_BY_PROTOCOL_FAMILY,
  GSK_ERROR_ARG_LIST_TOO_LONG,
  GSK_ERROR_BAD_ADDRESS,
  GSK_ERROR_BAD_FD,
  GSK_ERROR_BAD_FORMAT,
  GSK_ERROR_BAD_PROCEDURE_FOR_PROGRAM,
  GSK_ERROR_BLOCK_DEVICE_REQUIRED,
  GSK_ERROR_BROKEN_PIPE,
  GSK_ERROR_CANNOT_ASSIGN_REQUESTED_ADDRESS,
  GSK_ERROR_CANNOT_SEND_AFTER_SOCKET_SHUTDOWN,
  GSK_ERROR_CONNECTION_REFUSED,
  GSK_ERROR_CONNECTION_RESET_BY_PEER,
  GSK_ERROR_CROSS_DEVICE_LINK,
  GSK_ERROR_DESTINATION_ADDRESS_REQUIRED,
  GSK_ERROR_DEVICE_BUSY,
  GSK_ERROR_DEVICE_FAILED,
  GSK_ERROR_DEVICE_NOT_CONFIGURED,
  GSK_ERROR_DIRECTORY_NOT_EMPTY,
  GSK_ERROR_DISC_QUOTA_EXCEEDED,
  GSK_ERROR_FILE_EXISTS,
  GSK_ERROR_FILE_NAME_TOO_LONG,
  GSK_ERROR_FILE_NOT_FOUND,
  GSK_ERROR_FILE_TOO_LARGE,
  GSK_ERROR_FUNCTION_NOT_IMPLEMENTED,
  GSK_ERROR_HOST_IS_DOWN,
  GSK_ERROR_ILLEGAL_SEEK,
  GSK_ERROR_INAPPROPRIATE_IOCTL_FOR_DEVICE,
  GSK_ERROR_INTERRUPTED_SYSTEM_CALL,
  GSK_ERROR_INVALID_ARGUMENT,
  GSK_ERROR_IS_A_DIRECTORY,
  GSK_ERROR_MESSAGE_TOO_LONG,
  GSK_ERROR_NETWORK_DROPPED_CONNECTION_ON_RESET,
  GSK_ERROR_NETWORK_IS_DOWN,
  GSK_ERROR_NETWORK_IS_UNREACHABLE,
  GSK_ERROR_NOT_A_DIRECTORY,
  GSK_ERROR_NO_BUFFER_SPACE_AVAILABLE,
  GSK_ERROR_NO_CHILD_PROCESSES,
  GSK_ERROR_NO_LOCKS_AVAILABLE,
  GSK_ERROR_NO_ROUTE_TO_HOST,
  GSK_ERROR_NO_SPACE_LEFT_ON_DEVICE,
  GSK_ERROR_NUMERICAL_ARGUMENT_OUT_OF_DOMAIN,
  GSK_ERROR_OPERATION_ALREADY_IN_PROGRESS,
  GSK_ERROR_OPERATION_NOT_SUPPORTED,
  GSK_ERROR_OPERATION_NOW_IN_PROGRESS,
  GSK_ERROR_OPERATION_TIMED_OUT,
  GSK_ERROR_OUT_OF_MEMORY,
  GSK_ERROR_PERMISSION_DENIED,
  GSK_ERROR_PROCESS_NOT_FOUND,
  GSK_ERROR_PROGRAM_VERSION_WRONG,
  GSK_ERROR_PROTOCOL_FAMILY_NOT_SUPPORTED,
  GSK_ERROR_PROTOCOL_NOT_AVAILABLE,
  GSK_ERROR_PROTOCOL_NOT_SUPPORTED,
  GSK_ERROR_PROTOCOL_WRONG_TYPE_FOR_SOCKET,
  GSK_ERROR_READ_ONLY_FILE_SYSTEM,
  GSK_ERROR_RESOURCE_DEADLOCK,
  GSK_ERROR_RESOURCE_TEMPORARILY_UNAVAILABLE,
  GSK_ERROR_RESULT_TOO_LARGE,
  GSK_ERROR_RPC_PROG_NOT_AVAIL,
  GSK_ERROR_RPC_STRUCT_IS_BAD,
  GSK_ERROR_RPC_VERSION_WRONG,
  GSK_ERROR_SOCKET_IS_ALREADY_CONNECTED,
  GSK_ERROR_SOCKET_IS_NOT_CONNECTED,
  GSK_ERROR_SOCKET_OPERATION_ON_NON_SOCKET,
  GSK_ERROR_SOCKET_TYPE_NOT_SUPPORTED,
  GSK_ERROR_SOFTWARE_CAUSED_CONNECTION_ABORT,
  GSK_ERROR_STALE_NFS_FILE_HANDLE,
  GSK_ERROR_TEXT_FILE_BUSY,
  GSK_ERROR_TOO_MANY_LEVELS_OF_REMOTE_IN_PATH,
  GSK_ERROR_TOO_MANY_LEVELS_OF_SYMBOLIC_LINKS,
  GSK_ERROR_TOO_MANY_LINKS,
  GSK_ERROR_TOO_MANY_OPEN_FILES,
  GSK_ERROR_TOO_MANY_OPEN_FILES_IN_SYSTEM,
  GSK_ERROR_TOO_MANY_PROCESSES,
  GSK_ERROR_TOO_MANY_REFERENCES_CANNOT_SPLICE,
  GSK_ERROR_TOO_MANY_USERS,

  /* xml related errors */
  GSK_ERROR_MULTIPLE_ROOTS,
  GSK_ERROR_NO_DOCUMENT,

  GSK_ERROR_COMPILE,
  GSK_ERROR_LINK,
  GSK_ERROR_OPEN_MODULE,
  GSK_ERROR_CIRCULAR
} GskErrorCode;


GskErrorCode gsk_error_code_from_errno (int errno_number);

/* private */
void _gsk_error_init (void);

G_END_DECLS

#endif
