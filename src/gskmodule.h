#ifndef __GSK_MODULE_H_
#define __GSK_MODULE_H_

#include <glib.h>
#include <gmodule.h>

G_BEGIN_DECLS

typedef struct _GskCompileContext GskCompileContext;
typedef struct _GskModule GskModule;

GskCompileContext *gsk_compile_context_new ();
void               gsk_compile_context_add_cflags (GskCompileContext*,
                                                   const char *flags);
void               gsk_compile_context_add_ldflags(GskCompileContext*,
                                                   const char *flags);
void               gsk_compile_context_add_pkgs   (GskCompileContext*,
                                                   const char *pkgs);
void               gsk_compile_context_set_tmp_dir(GskCompileContext*,
                                                   const char *tmp_dir);
void               gsk_compile_context_set_gdb    (GskCompileContext *,
                                                   gboolean           support);
void               gsk_compile_context_set_verbose(GskCompileContext *,
                                                   gboolean           support);
void               gsk_compile_context_free       (GskCompileContext *);

/* a wrapper around GModule with ref-counting,
 * and the ability to delete itself. */

GskModule *gsk_module_compile (GskCompileContext *context,
                               guint              n_sources,
                               char             **sources,
                               GModuleFlags       flags,
                               gboolean           delete_sources,
                               char             **program_output,
                               GError           **error);
GskModule *gsk_module_open    (const char        *filename,
                               GModuleFlags       flags,
                               GError           **error);

GskModule *gsk_module_ref     (GskModule *);
void       gsk_module_unref   (GskModule *);
gpointer   gsk_module_lookup  (GskModule *,
                               const char *symbol_name);



G_END_DECLS

#endif
