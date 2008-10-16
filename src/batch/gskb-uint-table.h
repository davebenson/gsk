

typedef struct _GskbUIntTable GskbUIntTable;
struct _GskbUIntTable
{
  guint type;
  void *table_data;
  gsize sizeof_entry_data;
  gboolean is_global;
};

typedef struct _GskbUIntTableEntry GskbUIntTableEntry;
struct _GskbUIntTableEntry
{
  guint32 value;
  void *entry_data;
};

#include "../gskbuffer.h"

GskbUIntTable *gskb_uint_table_new   (gsize     sizeof_entry_data,
                                     guint     n_entries,
				     const GskbUIntTableEntry *entry);
void           gskb_uint_table_print_compilable_deps
                                     (GskbUIntTable *table,
	         		      const char   *table_name,
                                      GskbTableEntryOutputFunc output_func,
	         		      GskBuffer    *output);
void           gskb_uint_table_print_compilable_object
                                     (GskbUIntTable *table,
	         		      const char   *table_name,
                                      const char   *sizeof_entry_data_str,
	         		      GskBuffer    *output);
void           gskb_uint_table_print_static
                                     (GskbUIntTable *table,
	         		      const char   *table_name,
                                      GskbTableEntryOutputFunc output_func,
                                      const char   *sizeof_entry_data_str,
	         		      GskBuffer    *output);
const void   * gskb_uint_table_lookup(GskbUIntTable *table,
                                      guint32       value);
void           gskb_uint_table_free  (GskbUIntTable *table);

