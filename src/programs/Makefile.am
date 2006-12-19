INCLUDES = @GLIB_CFLAGS@ @GSK_DEBUG_CFLAGS@

bin_PROGRAMS = gsk-control-client gsk-netcat gsk-wget gsk-debug-alloc-tool \
	gsk-throttle-proxy gsk-webserver

gsk_control_client_SOURCES = gsk-control-client-main.c
gsk_control_client_LDADD = ../libgsk-1.0.la @READLINE_LIBS@ @GLIB_EXTRA_LDFLAGS@

gsk_netcat_SOURCES = gsk-netcat.c
gsk_netcat_LDADD = ../libgsk-1.0.la @GLIB_EXTRA_LDFLAGS@

gsk_wget_SOURCES = gsk-wget.c
gsk_wget_LDADD = ../libgsk-1.0.la @GLIB_EXTRA_LDFLAGS@

gsk_webserver_SOURCES = gsk-webserver.c
gsk_webserver_LDADD = ../libgsk-1.0.la @GLIB_EXTRA_LDFLAGS@

gsk_throttle_proxy_SOURCES = gsk-throttle-proxy.c
gsk_throttle_proxy_LDADD = ../libgsk-1.0.la @GLIB_EXTRA_LDFLAGS@

gsk_debug_alloc_tool_SOURCES = gsk-debug-alloc-tool.c gskdebuglog.c gskdebuglog.h
gsk_debug_alloc_tool_LDADD = ../libgsk-1.0.la @GLIB_EXTRA_LDFLAGS@

noinst_PROGRAMS = example-server
example_server_SOURCES = example-server.c
example_server_LDADD = ../libgsk-1.0.la @GLIB_EXTRA_LDFLAGS@
