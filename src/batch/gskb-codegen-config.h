

typedef enum
{
  GSKB_CODEGEN_SECTION_TYPEDEFS,
  GSKB_CODEGEN_SECTION_STRUCTURES,
  GSKB_CODEGEN_SECTION_FUNCTION_PROTOTYPES,
  GSKB_CODEGEN_SECTION_FUNCTION_IMPLS
} GskbCodegenSection;

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


