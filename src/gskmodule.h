#ifndef __GSK_MODULE_H_
#define __GSK_MODULE_H_

#include <glib.h>

G_BEGIN_DECLS

typedef struct _GskModule GskModule;

/* a wrapper around GModule with ref-counting,
 * and the ability to delete itself. */

GskModule *gsk_module_compile (char **sources,
                               gboolean delete_sources,
                               GError **error);
GskModule *gsk_module_open    (const char *filename,
                               GError **error);

GskModule *gsk_module_ref     (GskModule *);
void       gsk_module_unref   (GskModule *);
gpointer   gsk_module_lookup  (GskModule *,
                               const char *symbol_name);

G_END_DECLS

#endif
