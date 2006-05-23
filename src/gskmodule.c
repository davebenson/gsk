#include <unistd.h>             /* getpid() */
#include <stdio.h>

#include "gskerror.h"
#include "gskmodule.h"
#include "compile-info.h"

struct _GskCompileContext
{
  char *tmp_dir;
  char *cc, *ld;
  GString *cflags;
  GString *ldflags;
  GPtrArray *packages;
  char *package_cflags, *package_ldflags;
  gboolean gdb_support;
  gboolean verbose;
};


GskCompileContext *gsk_compile_context_new ()
{
  GskCompileContext *rv = g_new (GskCompileContext, 1);
  rv->tmp_dir = NULL;
  rv->cc = g_strdup (GSK_CC);
  rv->ld = g_strdup (GSK_LD_SHLIB);
  rv->cflags = g_string_new (GSK_CFLAGS " " GSK_COMPILE_ONLY_FLAG);
  rv->ldflags = g_string_new (GSK_LD_SHLIB_FLAGS);
  rv->packages = g_ptr_array_new ();
  rv->package_cflags = NULL;
  rv->package_ldflags = NULL;
  rv->gdb_support = FALSE;
  return rv;
}
void               gsk_compile_context_add_cflags (GskCompileContext*context,
                                                   const char *flags);
void               gsk_compile_context_add_ldflags(GskCompileContext*context,
                                                   const char *flags);
void               gsk_compile_context_add_pkgs   (GskCompileContext*context,
                                                   const char *pkgs);
void               gsk_compile_context_set_tmp_dir(GskCompileContext*context,
                                                   const char *tmp_dir);
void               gsk_compile_context_set_gdb    (GskCompileContext *context,
                                                   gboolean           support);
void               gsk_compile_context_set_verbose(GskCompileContext *context,
                                                   gboolean           support)
{
  context->verbose = support;
}

void               gsk_compile_context_free       (GskCompileContext *);

struct _GskModule
{
  GModule *module;
  guint ref_count;
  char **files_to_kill;
};

static void
run_pkg_config       (GskCompileContext *context,
                      const char        *prg_option,
                      char             **flags_out)
{
  GString *cmd_str;
  GString *str;
  guint i;
  FILE *fp;
  char buf[4096];

  cmd_str = g_string_new (GSK_PKGCONFIG);
  g_string_append_printf (cmd_str, " --cflags");
  for (i = 0; i < context->packages->len; i++)
    g_string_append_printf (cmd_str, " %s",
                            (char*)(context->packages->pdata[i]));
  str = g_string_new ("");
  fp = popen (cmd_str->str, "r");
  while (fgets (buf, sizeof (buf), fp) != NULL)
    g_string_append (str, buf);
  if (pclose (fp) < 0)
    g_error ("error running pkg-config");
  g_strstrip (str->str);
  *flags_out = g_strdup (str->str);
  g_string_free (str, TRUE);
  g_string_free (cmd_str, TRUE);
}

static void
ensure_pkg_info_ok (GskCompileContext *context)
{
  if (context->package_ldflags == NULL)
    {
      if (context->packages->len == 0)
        {
          context->package_cflags = g_strdup ("");
          context->package_ldflags = g_strdup ("");
        }
      else
        {
          run_pkg_config (context, "--cflags", &context->package_cflags);
          run_pkg_config (context, "--libs", &context->package_ldflags);
        }
    }
}

GskModule *
gsk_module_compile  (GskCompileContext *context,
                     guint              n_sources,
                     char             **sources,
                     GModuleFlags       flags,
                     gboolean           delete_sources,
                     char             **program_output,
                     GError           **error)
{
  GModule *module;
  GskModule *rv;
  GString *linker_cmd;
  char *output_fname;
  char **to_kill_files;
  guint i;
  GString *output;
  char buf[4096];

  /* pick unused filename for module */
  {
    static guint seq = 0;
    for (;;)
      {
        output_fname = g_strdup_printf ("%s/mod-%u-%u.so",
                                        context->tmp_dir ? context->tmp_dir
                                                         : g_get_tmp_dir (),
                                        (guint) getpid (),
                                        seq++);
        if (!g_file_test (output_fname, G_FILE_TEST_EXISTS))
          break;
        g_free (output_fname);
      }
  }

  ensure_pkg_info_ok (context);

  linker_cmd = g_string_new (context->ld);
  g_string_append_printf (linker_cmd, " %s %s -o '%s'",
                          context->ldflags->str,
                          context->package_ldflags,
                          output_fname);

  output = g_string_new ("");
  for (i = 0; i < n_sources; i++)
    {
      char *command = g_strdup_printf ("%s %s %s -o '%s.o' '%s' 2>&1",
                                       context->cc,
                                       context->cflags->str,
                                       context->package_cflags,
                                       sources[i], sources[i]);

      FILE *fp;
      if (context->verbose)
        g_printerr ("compiling: %s\n", command);
      fp = popen (command, "r");
      while (fgets (buf, sizeof (buf), fp) != NULL)
        g_string_append (output, buf);
      if (pclose (fp) < 0)
        {
          g_set_error (error,
                       GSK_G_ERROR_DOMAIN,
                       GSK_ERROR_COMPILE,
                       "error compiling shlib");
          if (program_output)
            *program_output = g_string_free (output, FALSE);
          else
            g_string_free (output, TRUE);
          return NULL;
        }
      g_string_append_printf (linker_cmd, " '%s.o'", sources[i]);
    }

  /* assemble linker command */
  {
    FILE *fp;
    if (context->verbose)
      g_printerr ("linking: %s\n", linker_cmd->str);
    fp = popen (linker_cmd->str, "r");
    while (fgets (buf, sizeof (buf), fp) != NULL)
      g_string_append (output, buf);
    if (pclose (fp) < 0)
      {
        g_set_error (error,
                     GSK_G_ERROR_DOMAIN,
                     GSK_ERROR_COMPILE,
                     "error linking shlib");
        if (program_output)
          *program_output = g_string_free (output, FALSE);
        else
          g_string_free (output, TRUE);
        return NULL;
      }
  }

  module = g_module_open (output_fname, flags);
  if (module == NULL)
    {
      g_set_error (error,
                   GSK_G_ERROR_DOMAIN,
                   GSK_ERROR_OPEN_MODULE,
                   "error opening creating module %s: %s",
                   output_fname, g_module_error ());
      return NULL;
    }

  rv = g_new (GskModule, 1);
  rv->module = module;
  rv->ref_count = 1;

  /* make the list of files to unlink. */
  {
    GPtrArray *to_kill = g_ptr_array_new ();
    if (delete_sources)
      {
        for (i = 0; i < n_sources; i++)
          g_ptr_array_add (to_kill, g_strdup (sources[i]));
      }
    for (i = 0; i < n_sources; i++)
      g_ptr_array_add (to_kill, g_strdup_printf ("%s.o", sources[i]));
    g_ptr_array_add (to_kill, output_fname);
    g_ptr_array_add (to_kill, NULL);
    to_kill_files = (char **) g_ptr_array_free (to_kill, FALSE);
  }

  if (context->gdb_support)
    {
      rv->files_to_kill = to_kill_files;
    }
  else
    {
      /* delete files immediately */
      char **at;
      for (at = to_kill_files; *at; at++)
        unlink (*at);
      g_strfreev (to_kill_files);
      rv->files_to_kill = NULL;
    }

  if (program_output)
    *program_output = g_string_free (output, FALSE);
  else
    g_string_free (output, TRUE);

  return rv;
}

GskModule *
gsk_module_open (const char *filename,
                 GModuleFlags flags,
                 GError    **error)
{
  GModule *module = g_module_open (filename, flags);
  GskModule *rv;
  if (module == NULL)
    {
      g_set_error (error,
                   GSK_G_ERROR_DOMAIN,
                   GSK_ERROR_OPEN_MODULE,
                   "error opening module %s: %s",
                   filename, g_module_error ());
      return NULL;
    }
  rv = g_new (GskModule, 1);
  rv->ref_count = 1;
  rv->files_to_kill = NULL;
  rv->module = module;
  return rv;
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
      if (module->files_to_kill)
        {
          char **at;
          for (at = module->files_to_kill; *at; at++)
            unlink (*at);
          g_strfreev (module->files_to_kill);
        }
      g_module_close (module->module);
      g_free (module);
    }
}

gpointer
gsk_module_lookup (GskModule  *module,
                   const char *symbol_name)
{
  gpointer rv;
  if (g_module_symbol (module->module, symbol_name, &rv))
    return rv;
  else
    return NULL;
}
