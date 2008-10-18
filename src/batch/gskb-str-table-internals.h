
typedef enum
{
  GSKB_STR_TABLE_BSEARCH,
  GSKB_STR_TABLE_ABLZ,		/* hash-table */
  GSKB_STR_TABLE_5003_33        /* hash-table */
} GskbStrTableType;

typedef struct _GskbStrTableBSearchEntry GskbStrTableBSearchEntry;
struct _GskbStrTableBSearchEntry
{
  guint32 str_slab_offset;
  /* data follows */
};
#define GSKB_STR_TABLE_BSEARCH_ENTRY__ALIGNOF  GSKB_ALIGNOF_UINT32

typedef struct _GskbStrTableHashEntry GskbStrTableHashEntry;
struct _GskbStrTableHashEntry
{
  guint32 hash_code;
  guint32 str_slab_offset;
  /* data follows */
};
#define GSKB_STR_TABLE_HASH_ENTRY__ALIGNOF  GSKB_ALIGNOF_UINT32
