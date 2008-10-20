#include "gskb-str-table.h"
#include <errno.h>
#include "../gskerrno.h"


static const char *symbols[] = {
"int8",
"int16",
"int32",
"int64",
"uint8",
"uint16",
"uint32",
"uint64",
"int",
"long",
"uint",
"ulong",
"float32",
"float64",
"string",
"extensible",
"struct",
"union",
"bitfields",
"enum",
"alias",
};

static void
render_index_as_parser_token (gconstpointer entry_data,
                              GskBuffer *output)
{
  guint32 index = * (const guint32 *) entry_data;
  char *uc = g_ascii_strup (symbols[index], -1);
  gsk_buffer_printf (output, "GSKB_TOKEN_TYPE_%s", uc);
  g_free (uc);
}

int main()
{
  guint32 *indices = g_new (guint32, G_N_ELEMENTS (symbols));
  GskbStrTableEntry *entries = g_new (GskbStrTableEntry, G_N_ELEMENTS (symbols));
  guint i;
  GskBuffer buffer = GSK_BUFFER_STATIC_INIT;
  GskbStrTable *table;
  for (i = 0; i < G_N_ELEMENTS (symbols); i++)
    {
      indices[i] = i;
      entries[i].str = symbols[i];
      entries[i].entry_data = indices + i;
    }
  table = gskb_str_table_new (4, 4, G_N_ELEMENTS (symbols), entries);
  g_assert (table != NULL);
  gskb_str_table_print (table, FALSE, "gskb_parser_symbol_table", "guint32",
                        render_index_as_parser_token,
                        "sizeof(guint32)", "GSKB_ALIGNOF_UINT32", &buffer);
  while (buffer.size > 0)
    {
      if (gsk_buffer_writev (&buffer, 1) < 0)
        {
          if (!gsk_errno_is_ignorable (errno))
            g_error ("gsk_buffer_writev: %s", g_strerror (errno));
        }
    }
  gskb_str_table_free (table);
  g_free (entries);
  g_free (indices);
  return 0;
}
