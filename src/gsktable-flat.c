
typedef struct _FlatFactory FlatFactory;
struct _FlatFactory
{
  GskTableFileFactory base_factory;
};

static GskTableFile *
flat__create_file      (GskTableFileFactory      *factory,
                        const char               *dir,
                        guint64                   id,
                        const GskTableFileHints  *hints,
                        GError                  **error)
{
  ...
}

static GskTableFile *
flat__open_building_file(GskTableFileFactory     *factory,
                         const char               *dir,
                         guint64                   id,
                         guint                     state_len,
                         const guint8             *state_data,
                         GError                  **error)
{
  ...
}

GskTableFile *
flat__open_file        (GskTableFileFactory      *factory,
                        const char               *dir,
                        guint64                   id,
                        GError                  **error)
{
  ...
}


/* methods for a file which is being built */
static gboolean 
flat__feed_entry      (GskTableFile             *file,
                       guint                     key_len,
                       const guint8             *key_data,
                       guint                     value_len,
                       const guint8             *value_data,
                       GError                  **error)
{
  ...
}

static gboolean 
flat__done_feeding     (GskTableFile             *file,
                        gboolean                 *ready_out,
                        GError                  **error)
{
  ...
}

/* Is the file in a state where it can be checkpointed?
If you cannot get the build state, feed_entry() until you can. */
static gboolean 
flat__can_get_build_state(GskTableFile           *file)
{
  ...
}

static gboolean 
flat__get_build_state  (GskTableFile             *file,
                        guint                    *state_len_out,
                        guint8                  **state_data_out,
                        GError                  **error)
{
  ...
}

static gboolean 
flat__build_file       (GskTableFile             *file,
                        gboolean                 *ready_out,
                        GError                  **error)
{
  ...
}

static void     
flat__release_build_data(GskTableFile            *file)
{
  ...
}


/* methods for a file which has been constructed */
static gboolean 
flat__query_file       (GskTableFile             *file,
                        GskTableFileQuery        *query_inout,
                        GError                  **error)
{
  ...
}

static GskTableReader *
flat__create_reader    (GskTableFile             *file,
                        GError                  **error)
{
  ...
}

/* you must always be able to get reader state */
static gboolean 
flat__get_reader_state (GskTableFile             *file,
                        GskTableReader           *reader,
                        guint                    *state_len_out,
                        guint8                  **state_data_out,
                        GError                  **error)
{
  ...
}

static GskTableReader *
flat__recreate_reader  (GskTableFile             *file,
                        guint                     state_len,
                        const guint8             *state_data,
                        GError                  **error)
{
  ...
}


/* destroying files and factories */
static gboolean
flat__destroy_file     (GskTableFile             *file,
                        gboolean                  erase,
                        GError                  **error)
{
  ...
}

static void
flat__destroy_factory  (GskTableFileFactory      *factory)
{
  ...
}


/* for now, return a static factory object */
GskTableFileFactory *gsk_table_file_factory_new_flat (void)
{
  static FlatFactory the_factory =
    {
      {
        flat__create_file,
        flat__open_building_file,
        flat__open_file,
        flat__feed_entry,
        flat__done_feeding,
        flat__can_get_build_state,
        flat__get_build_state,
        flat__build_file,
        flat__release_build_data,
        flat__query_file,
        flat__create_reader,
        flat__get_reader_state,
        flat__recreate_reader,
        flat__destroy_file,
        flat__destroy_factory
      },
    };

  return &the_factory.base_instance;
}
