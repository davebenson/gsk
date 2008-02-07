/* GskTable.
 *
 * A efficient on-disk table, meaning a mapping from an
 * uninterpreted piece of binary-data to a value.
 * When multiple values are added with the same
 * key, they are merged, using user-definable semantics.
 */

typedef struct
{
  guint len;
  guint8 *data;
  guint alloced;
} GskTableBuffer;

G_INLINE_FUNC void    gsk_table_buffer_init    (GskTableBuffer *buffer);
G_INLINE_FUNC guint8 *gsk_table_buffer_set_len (GskTableBuffer *buffer,
                                                guint           len);
G_INLINE_FUNC void    gsk_table_buffer_clear   (GskTableBuffer *buffer);

typedef enum
{
  GSK_TABLE_MERGE_RETURN_A,
  GSK_TABLE_MERGE_RETURN_A_DONE,
  GSK_TABLE_MERGE_RETURN_B,
  GSK_TABLE_MERGE_RETURN_B_DONE,
  GSK_TABLE_MERGE_SUCCESS,
  GSK_TABLE_MERGE_SUCCESS_DONE,
  GSK_TABLE_MERGE_DROP,
} GskTableMergeResult;
typedef GskTableMergeResult (*GskTableMergeFunc) (guint         key_len,
                                                  const guint8 *key_data,
                                                  guint         a_len,
                                                  const guint8 *a_data,
                                                  guint         b_len,
                                                  const guint8 *b_data,
                                                  GskTableBuffer *output,
                                                  gpointer      user_data);
typedef GskTableMergeResult (*GskTableMergeFuncNoLen) (const guint8 *key_data,
                                                       const guint8 *a_data,
                                                       const guint8 *b_data,
                                                       GByteArray   *pad,
                                                       gpointer      user_data);

struct _GskTableOptions
{
  GskTableMergeFunc merge;
  gpointer user_data;
  GDestroyNotify destroy_user_data;

  /* You may supply the size range for optimizations.
     you must the same parameters between invocations. */
  guint min_key_size, max_key_size;
  guint min_value_size, max_value_size;

  /* tunables */
  gsize max_in_memory_entries;
  gsize max_in_memory_bytes;
};

GskTableOptions     *gsk_table_options_new    (void);
void                 gsk_table_options_destroy(GskTableOptions *options);

GskTable *  gsk_table_new         (const char            *dir,
                                   const GskTableOptions *options,
	          	           GError               **error);
void        gsk_table_add         (GskTable              *table,
                                   guint                  key_len,
	          	           const guint8          *key_data,
                                   guint                  value_len,
	          	           const guint8          *value_data);
gboolean    gsk_table_query       (GskTable              *table,
                                   guint                  key_len,
			           const guint8          *key_data,
			           guint                 *value_len_out,
			           guint8               **value_data_out);
const char *gsk_table_peek_dir    (GskTable              *table);
void        gsk_table_destroy     (GskTable              *table);

/* TODO: iterator */
