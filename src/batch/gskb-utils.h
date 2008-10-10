

typedef struct _GskbStrTable GskbStrTable;
struct _GskbStrTable
{
  guint type;
  void *table_data;
  gsize sizeof_entry_data;
  gboolean is_global;
  const char *str_slab;
};

typedef struct _GskbStrTableEntry GskbStrTableEntry;
struct _GskbStrTableEntry
{
  const char *str;
  void *entry_data;
};

#include "../gskbuffer.h"

typedef void (*GskbTableEntryOutputFunc) (gconstpointer entry_data,
                                          GskBuffer    *dest);

GskbStrTable *gskb_str_table_new    (gsize     sizeof_entry_data,
                                     guint     n_entries,
				     const GskbStrTableEntry *entry);
void          gskb_str_table_print_compilable_deps
                                    (GskbStrTable *table,
				     const char   *table_name,
                                     GskbTableEntryOutputFunc output_func,
				     GskBuffer    *output);
void          gskb_str_table_print_compilable_object
                                    (GskbStrTable *table,
				     const char   *table_name,
                                     const char   *sizeof_entry_data_str,
				     GskBuffer    *output);
/* helper function which calls the above to functions to declare the table staticly. */
void          gskb_str_table_print_static
                                    (GskbStrTable *table,
				     const char   *table_name,
                                     GskbTableEntryOutputFunc output_func,
                                     const char   *sizeof_entry_data_str,
				     GskBuffer    *output);
const void   *gskb_str_table_lookup (GskbStrTable *table,
                                     const char   *str);
void          gskb_str_table_free   (GskbStrTable *table);


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

