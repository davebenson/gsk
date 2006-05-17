task_lib_h_files = \
gsktask.h \
gsktaskfactory.h \
gsktasknamespace.h \
gsktaskcompiler.h

task_lib_c_files = \
gsktask.c \
gsktaskfactory.c \
gsktasknamespace.c \
gsktaskcompiler.c

#gsktaskperl_lo_CFLAGS = @PERL_CFLAGS@

task_lib_files = $(task_lib_c_files) $(task_lib_h_files)

taskdir = $(pkgincludedir)/task
task_HEADERS = $(task_lib_h_files)

# HACK: only gsktaskperl.c requires PERL_CFLAGS.
INCLUDES = @GSK_CFLAGS@ @GSK_LIB_CFLAGS@ @PERL_CFLAGS@

if GSK_NO_CHECKS_LIBRARY
lib_LTLIBRARIES = libgsk-task-ndebug.la
libgsk_task_ndebug_la_SOURCES=$(task_lib_files)
libgsk_task_ndebug_la_LIBADD=@PERL_LIBS@
else
if GSK_DEBUG_LIBRARY
lib_LTLIBRARIES = libgsk-task-debug.la
libgsk_task_debug_la_SOURCES=$(task_lib_files)
libgsk_task_debug_la_LIBADD=@PERL_LIBS@
else
lib_LTLIBRARIES = libgsk-task.la
libgsk_task_la_SOURCES=$(task_lib_files)
libgsk_task_la_LIBADD=@PERL_LIBS@
endif
endif
