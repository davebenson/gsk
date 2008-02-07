/* GskTableFile:
 *  A mechanism for creating searchable files from a sorted stream of entrys.
 */

typedef struct _GskTableFileFactory GskTableFileFactory;
typedef struct _GskTableFile GskTableFile;
typedef struct _GskTableFileHints GskTableFileHints;

struct _GskTableFileHints
{
  guint64 max_entries;
  guint64 max_bytes;
  guint min_key_size, max_key_size;
  guint min_value_size, max_value_size;
  gboolean in_memory;
};

struct _GskTableFile
{
  GskTableFileFactory *factory;
};

struct _GskTableFileFactory
{
  /* creating files (in initial, mid-build and queryable states) */
  GskTableFile     *(*create_file)      (GskTableFileFactory      *factory,
					 const char               *dir,
					 guint64                   id,
				         const GskTableFileHints  *hints,
			                 GError                  **error);
  GskTableFile     *(*open_building_file)(GskTableFileFactory     *factory,
					 const char               *dir,
					 guint64                   id,
                                         guint                     state_len,
                                         const guint8             *state_data,
			                 GError                  **error);
  GskTableFile     *(*open_file)        (GskTableFileFactory      *factory,
					 const char               *dir,
					 guint64                   id,
			                 GError                  **error);

  /* methods for a file which is being built */
  gboolean          (*feed_entry)      (GskTableFile             *file,
                                         guint                     key_len,
                                         const guint8             *key_data,
                                         guint                     value_len,
                                         const guint8             *value_data,
					 GError                  **error);
  gboolean          (*done_feeding)     (GskTableFile             *file,
                                         gboolean                 *ready_out,
					 GError                  **error);
  gboolean          (*get_build_state)  (GskTableFile             *file,
                                         guint                    *state_len_out,
                                         guint8                  **state_data_out,
					 GError                  **error);
  gboolean          (*build_file)       (GskTableFile             *file,
                                         gboolean                 *ready_out,
					 GError                  **error);
  void              (*release_build_data)(GskTableFile            *file);

  /* methods for a file which has been constructed */
  gboolean          (*query_file)       (GskTableFile             *file,
                                         GskTableFileQuery        *query_inout,
					 GError                  **error);
  GskTableReader   *(*create_reader)    (GskTableFile             *file,
                                         GError                  **error);
  gboolean          (*get_reader_state) (GskTableFile             *file,
                                         GskTableReader           *reader,
                                         guint                    *state_len_out,
                                         guint8                  **state_data_out,
                                         GError                  **error);
  GskTableReader   *(*recreate_reader)  (GskTableFile             *file,
                                         guint                     state_len,
                                         const guint8             *state_data,
                                         GError                  **error);

  /* destroying files and factories */
  gboolean          (*destroy_file)     (GskTableFile             *file,
                                         gboolean                  erase,
					 GError                  **error);
  void              (*destroy_factory)  (GskTableFileFactory      *factory);
};

/* optimized for situations where writes predominate */
GskTableFileFactory *gsk_table_file_factory_new_flat (void);

/* optimized for situations where reads are common */
GskTableFileFactory *gsk_table_file_factory_new_btree (void);

#define gsk_table_file_factory_create_file(factory, dir, id, hints, error) \
  ((factory)->create_file ((factory), (dir), (id), (hints), (error)))
#define gsk_table_file_factory_open_file(factory, dir, id, error) \
  ((factory)->open_file ((factory), (dir), (id), (error)))
#define gsk_table_file_factory_feed_entry(factory, key_len, key_data, value_len, value_data, error) \
  ((factory)->feed_entry ((factory), key_len, key_data, value_len, value_data, error))
#define gsk_table_file_factory_done_feeding(factory, ready_out, error) \
  ((factory)->done_feeding ((factory), (ready_out), (error))
#define gsk_table_file_factory_build_file(factory, ready_out, error) \
  ((factory)->build_file ((factory), (ready_out), (error))
#define gsk_table_file_factory_query_file(factory, query_inout, error) \
  ((factory)->query_file ((factory), (query_inout), (error))
#define gsk_table_file_factory_destroy_file(factory, erase, error) \
  ((factory)->destroy_file ((factory), (erase), (error))
#define gsk_table_file_factory_destroy(factory) \
  (factory)->destroy_factory (factory)

