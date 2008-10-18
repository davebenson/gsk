#ifndef __GSKB_UINT_TABLE__H_
#define __GSKB_UINT_TABLE__H_

#include "../gskbuffer.h"

typedef void (*GskbUIntTableEntryOutputFunc) (gconstpointer entry_data,
                                              GskBuffer    *dest);

typedef struct _GskbUIntTable GskbUIntTable;
struct _GskbUIntTable
{
  guint type;
  guint table_size;
  void *table_data;
  void *entry_data;             /* used for ranges, at least */
  gsize sizeof_entry_data;
  guint32 max_value;            /* must be less than G_MAXUINT32 */
  gboolean is_global;
};

typedef struct _GskbUIntTableEntry GskbUIntTableEntry;
struct _GskbUIntTableEntry
{
  guint32 value;
  void *entry_data;
};

GskbUIntTable *gskb_uint_table_new   (gsize          sizeof_entry_data,
                                      gsize          alignof_entry_data,
                                      guint          n_entries,
				      const GskbUIntTableEntry *entry);
void           gskb_uint_table_print_compilable_deps
                                     (GskbUIntTable *table,
	         		      const char    *table_name,
                                      GskbUIntTableEntryOutputFunc output_func,
	         		      GskBuffer     *output);
void           gskb_uint_table_print_compilable_object
                                     (GskbUIntTable *table,
	         		      const char    *table_name,
                                      const char    *sizeof_entry_data_str,
                                      const char    *alignof_entry_data_str,
	         		      GskBuffer     *output);
void           gskb_uint_table_print_static
                                     (GskbUIntTable *table,
	         		      const char    *table_name,
                                      const char    *type_name,
                                      GskbUIntTableEntryOutputFunc output_func,
                                      const char    *sizeof_entry_data_str,
                                      const char    *alignof_entry_data_str,
	         		      GskBuffer     *output);
const void   * gskb_uint_table_lookup(GskbUIntTable *table,
                                      guint32        value);
void           gskb_uint_table_free  (GskbUIntTable *table);


#endif
