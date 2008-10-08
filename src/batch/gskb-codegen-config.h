

typedef enum
{
  GSKB_CODEGEN_SECTION_TYPEDEFS,
  GSKB_CODEGEN_SECTION_STRUCTURES,
  GSKB_CODEGEN_SECTION_FORMAT_DECLS,
  GSKB_CODEGEN_SECTION_FORMAT_PRIVATE_DECLS,
  GSKB_CODEGEN_SECTION_FORMAT_IMPLS,
  GSKB_CODEGEN_SECTION_FUNCTION_PROTOTYPES,
  GSKB_CODEGEN_SECTION_FUNCTION_IMPLS
} GskbCodegenSection;

#define GSKB_N_CODEGEN_SECTIONS (GSKB_CODEGEN_SECTION_FUNCTION_IMPLS+1)

typedef enum
{
  /* if format->always_by_pointer,
         void {lctype}_pack      (const {type}  *value,
                                  GskbAppendFunc append,
                                  gpointer       append_data);
     else
         void {lctype}_pack      ({type}         value,
                                  GskbAppendFunc append,
                                  gpointer       append_data); */
  GSKB_CODEGEN_OUTPUT_PACK,


  /* if format->always_by_pointer,
         guint {lctype}_get_packed_size (const {type}  *value);
     else
         guint {lctype}_get_packed_size ({type}         value); */
  GSKB_CODEGEN_OUTPUT_GET_PACKED_SIZE,

  /* if format->always_by_pointer,
         guint {lctype}_pack_slab (const {type}  *value,
                                   guint8        *out);
     else
         guint {lctype}_pack_slab ({type}         value,
                                   guint8        *out);  */
  GSKB_CODEGEN_OUTPUT_PACK_SLAB,


  /*     guint    {lctype}_validate_partial  (guint          len,
                                              const guint8  *data,
                                              GError       **error);    */
  GSKB_CODEGEN_OUTPUT_VALIDATE_PARTIAL,

  /*     guint {lctype}_unpack     (const guint8  *data,
                                    {type}        *value_out);
      will use glib's malloc as needed (for each string or array)
   */
  GSKB_CODEGEN_OUTPUT_UNPACK,

  /*     guint {lctype}_unpack_mempool    (const guint8  *data,
                                           {type}        *value_out,
                                           GskMemPool    *mem_pool);
   */
  GSKB_CODEGEN_OUTPUT_UNPACK_MEMPOOL,

  /* destruct an object.  this method will be define to nothing to
     types that do not require destruction:
         void {lctype}_destruct   ({type}         *value,
                                   GskbUnpackFlags flags,
                                   GskbAllocator  *allocator);   */
  GSKB_CODEGEN_OUTPUT_DESTRUCT,

} GskbCodegenOutputFunction;

#define GSKB_N_CODEGEN_OUTPUT_FUNCTIONS  (GSKB_CODEGEN_OUTPUT_DESTRUCT+1)


typedef struct _GskbCodegenConfig GskbCodegenConfig;
struct _GskbCodegenConfig
{
  char *type_prefix, *func_prefix;
  gboolean all_static;
};

GskbCodegenConfig *
     gskb_codegen_config_new            (const char        *type_prefix,
                                         const char        *func_prefix);
void gskb_codegen_config_set_all_static (GskbCodegenConfig *config,
                                         gboolean           all_static);
void gskb_codegen_config_free           (GskbCodegenConfig *config);



/* internal */
#include "../gskbuffer.h"
gboolean    gskb_format_codegen        (GskbFormat *format,
                                        GskbCodegenSection section,
                                        const GskbCodegenConfig *config,
                                        GskBuffer *output,
                                        GError   **error);
