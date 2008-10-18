
typedef enum
{
  GSKB_UINT_TABLE_EMPTY,
  GSKB_UINT_TABLE_DIRECT,
  GSKB_UINT_TABLE_BSEARCH,
  GSKB_UINT_TABLE_RANGES
  //GSKB_UINT_TABLE_HASH_DIRECT
} GskbUIntTableType;

typedef struct _GskbUIntTableRange GskbUIntTableRange;
struct _GskbUIntTableRange
{
  guint32 start, count;
  guint entry_data_offset;
};
