#include "gskmodule.h"

struct _GskModule
{
  GModule *the_module;

  guint ref_count;

  /* an array of files to delete when ref-count reached 0;
   * typically NULL if not gdb'ing */
  char **gdb_files_to_rm;
};

GskModule *
gsk_module_compile (char **sources,
                    gboolean delete_sources,
                    GError **error)
{
  GskModule *rv;
  guint n_sources = g_strv_length (sources);

  for (i = 0; i < n_sources; i++)
    {
      /* compile to same file, with .o extension */
      char *output = g_strdup_printf ("%s.o", sources[i]);

      /* assemble command */
      ...
      
      /* run command */
      ...
    }

  /* assemble linker command */
  ...

  rv = g_new (GskModule, 1);
  rv->module = module;
  rv->ref_count = 1;

  if (support_gdb)
    {
      rv->gdb_files_to_rm = ...;
    }
  else
    {
      /* delete files immediately */
      ...
    }
  return rv;
}

GskModule *
gsk_module_open (const char *filename,
                 GError    **error)
{
  ...
}

GskModule *
gsk_module_ref (GskModule *module)
{
  g_return_val_if_fail (module->ref_count > 0, module);
  ++(module->ref_count);
  return module;
}
void
gsk_module_unref (GskModule *module)
{
  g_return_if_fail (module->ref_count > 0);
  if (--(module->ref_count) == 0)
    {
      if (module->gdb_files_to_rm)
        {
          char **at;
          for (at = module->gdb_files_to_rm; *at; at++)
            unlink (*at);
          g_strfreev (module->gdb_files_to_rm);
        }
      g_module_close (module->the_module);
      g_free (module);
    }
}

gpointer
gsk_module_lookup (GskModule  *module,
                   const char *symbol_name)
{
  gpointer rv;
  if (g_module_symbol (module->the_module, symbol_name, &rv))
    return rv;
  else
    return NULL;
}
