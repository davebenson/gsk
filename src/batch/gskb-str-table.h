#ifndef __GSKB_STR_TABLE_H_
#define __GSKB_STR_TABLE_H_

typedef struct _GskbStrTable GskbStrTable;

#include "../gskbuffer.h"

struct _GskbStrTable
{
  /*< private >*/
  guint type;
  guint table_size;
  gpointer table_data;
  gsize sizeof_entry_data;
  gsize sizeof_entry;
  gsize entry_data_offset;
  gboolean is_global;
  gboolean is_ptr;
  char *str_slab;
  guint str_slab_size;
};

typedef struct _GskbStrTableEntry GskbStrTableEntry;
struct _GskbStrTableEntry
{
  const char *str;
  void *entry_data;
};

typedef void (*GskbStrTableEntryOutputFunc) (gconstpointer entry_data,
                                             GskBuffer    *dest);

/* if this returns NULL, it is because of duplicate entries */
GskbStrTable *gskb_str_table_new    (gsize         sizeof_entry_data,
                                     gsize         alignof_entry_data,
                                     guint         n_entries,
				     const GskbStrTableEntry *entry);
GskbStrTable *gskb_str_table_new_ptr(guint         n_entries,
				     const GskbStrTableEntry *entry);
void          gskb_str_table_print_compilable_deps
                                    (GskbStrTable *table,
				     const char   *table_name,
                                     const char   *entry_type_name,
                                     GskbStrTableEntryOutputFunc output_func,
				     GskBuffer    *output);
void          gskb_str_table_print_compilable_object
                                    (GskbStrTable *table,
				     const char   *table_name,
                                     const char   *sizeof_entry_data_str,
                                     const char   *alignof_entry_data_str,
				     GskBuffer    *output);
/* helper function which calls the above to functions to declare the table staticly. */
void          gskb_str_table_print  (GskbStrTable *table,
                                     gboolean      is_global,
				     const char   *table_name,
                                     const char   *entry_type_name,
                                     GskbStrTableEntryOutputFunc output_func,
                                     const char   *sizeof_entry_data_str,
                                     const char   *alignof_entry_data_str,
				     GskBuffer    *output);
const void   *gskb_str_table_lookup (GskbStrTable *table,
                                     const char   *str);
void          gskb_str_table_free   (GskbStrTable *table);


#endif
