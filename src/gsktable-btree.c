/* Implementation of gsk_table_file_factory_new_btree() */

struct _BtreeFactory
{
  GskTableFileFactory base_instance;
  guint branches_per_level;
  guint swell_factor;
};

typedef enum
{
  /* if the hints don't allow us to start building the tree immediately,
     we first just store all entries to disk to compute statistics */
  BTREE_PHASE_COMPUTE_STATS,

  /* small btrees are constructed in-memory;
     therefore, their "state" consists of all their entries */
  BTREE_PHASE_IN_MEMORY_BUILD,

  /* for building a large btree, the first level is somewhat different
     than subsequent levels. */
  BTREE_PHASE_BUILDING_BOTTOM_LEVEL,
  BTREE_PHASE_BUILDING_NONBOTTOM_LEVEL,

} BtreePhase;

struct _BtreeFileEntryMemory
{
  guint key_len;
  guint8 *key_data;
  guint value_len;
  guint8 *value_data;
};

typedef struct _BtreeFileInMemoryBuildInfo BtreeFileInMemoryBuildInfo;
struct _BtreeFileInMemoryBuildInfo
{
  guint max_entries;
  BtreeFileEntryMemory *entries;
  guint n_entries;
  GskMemPool mem_pool;
};
typedef struct _BtreeFileComputeStatsInfo BtreeFileComputeStatsInfo;
struct _BtreeFileComputeStatsInfo
{
  GskTableMmapWriter writer;

  /* statistics */
  guint64 n_entries;
  guint64 n_key_bytes;
  guint64 n_value_bytes;
  guint min_key_size;
  guint max_key_size;
  guint min_value_size;
  guint max_value_size;
};
struct _BtreeFileBuildingBottomLevelInfo
{
  guint height;
  GskTableMmapReader reader;

  /* Output the bottom level of the tree; we know everything about it
     since it has no children, so we won't need to pass through this data
     again (until we start reading from it) */
  GskTableMmapWriter final_writer;

  GskTableMmapPipe buffer;
};
struct _BtreeFileBuildingNonbottomLevelInfo
{
  guint height;
  guint cur_level;                     /* height-2 or less */

  /* Output the bottom level of the tree; we know everything about it
     since it has no children, so we won't need to pass through this data
     again (until we start reading from it) */
  GskTableMmapWriter final_writer;

  GskTableMmapPipe buffer;
};


typedef struct _BtreeFile BtreeFile;
struct _BtreeFile
{
  GskTableFile base_instance;
  BtreePhase phase;
  union
  {
    BtreeFileInMemoryBuildInfo in_memory_build;
    BtreeFileComputeStatsInfo compute_stats;
    BtreeFileBuildingBottomLevelInfo building_bottom_level;
    BtreeFileBuildingNonbottomLevelInfo building_nonbottom_level;

    struct {
    } building_nonbottom_level;
  } info;
};


/* once we know the exact number of entries,
   we construct the tree with the children given immediately before their
   parents.  then we write out the data, with the leaves
   of the tree first, and the root last.  this is easier because we that's how we have to compress.

   Algorithm.

     Step 1.  Determine the height of the tree.
       The height is the smallest tree that will accomodate,
       given the current branches_per_level and swell_factor.

       If S=branches_per_level and W=swell_factor,
       then a tree of height 1 has at most (S+1)*W
       nodes.  A tree of height H has (S^(H-1) + S^(H-2) + ... + 1) * (S + 1) * W.

     Step 2.  Scan data emitting the bottom level of the tree.
       The bottom level will never be too small (if the swell factor
       is two, then at least half the elements will be in the bottom level).

       The upper level information must be written to a tmp file
       so that we don't have to rescan the data.

     Step 3.  (Repeat for each level of the tree)
 */



static GskTableFile *
btree__create_file      (GskTableFileFactory      *factory,
                         const char               *dir,
                         guint64                   id,
                         const GskTableFileHints  *hints,
                         GError                  **error)
{
  BtreeFile *rv = g_new (BtreeFile, 1);
  if (hints->in_memory)
    {
      BtreeFileInMemoryBuildInfo *info = &rv->info.in_memory_build;
      rv->state = BTREE_PHASE_IN_MEMORY_BUILD;
      if (hints->max_entries < 16)
        info->max_entries = hints->max_entries;
      else
        info->max_entries = 16;
      info->entries = g_new (BtreeFileEntryMemory, info->max_entries);
      info->n_entries = 0;
      gsk_mem_pool_construct (&info->mem_pool);
    }
  else
    {
      BtreeFileComputeStatsInfo *info = &rv->info.compute_stats;
      rv->state = BTREE_PHASE_COMPUTE_STATS;
      fd = ...;
      gsk_table_mmap_writer_init (&info->writer, fd);

      info->n_entries = 0;
      info->n_key_bytes = 0;
      info->n_value_bytes = 0;
      info->min_key_size = G_MAXUINT;
      info->max_key_size = 0;
      info->min_value_size = G_MAXUINT;
      info->max_value_size = 0;
    }
  rv->factory = factory;
  return &rv->base_instance;
}

static GskTableFile *
btree__open_building_file (GskTableFileFactory     *factory,
                           const char               *dir,
                           guint64                   id,
                           guint                     state_len,
                           const guint8             *state_data,
                           GError                  **error)
{
  ...
}

static GskTableFile *
btree__open_file          (GskTableFileFactory      *factory,
                           const char               *dir,
                           guint64                   id,
                           GError                  **error)
{
  ...
}


/* methods for a file which is being built */
static gboolean
btree__feed_entry       (GskTableFile             *file,
                         guint                     key_len,
                         const guint8             *key_data,
                         guint                     value_len,
                         const guint8             *value_data,
                         GError                  **error)
{
  BtreeFile *f = (BtreeFile *) file;
  if (f->phase == BTREE_PHASE_IN_MEMORY_BUILD)
    {
      BtreeFileInMemoryBuildInfo *info = &file->info.in_memory_build;
      BtreeFileEntryMemory *entry;
      if (G_UNLIKELY (info->n_entries == info->max_entries))
        {
          if (G_UNLIKELY (info->max_entries == 0))
            /* shouldn't happen if the hinting was correct... */
            info->max_entries = 16;
          else
            info->max_entries *= 2;
          info->entries = g_renew (BtreeFileEntryMemory, info->entries, info->max_entries);
        }

      /* carve off a new entry */
      entry = info->entries + info->n_entries++;

      /* initialize it */
      entry->key_len = key_len;
      entry->key_data = gsk_mem_pool_alloc_unaligned (key_len);
      memcpy (entry->key_data, key_data, key_len);
      entry->value_len = value_len;
      entry->value_data = gsk_mem_pool_alloc_unaligned (value_len);
      memcpy (entry->value_data, value_data, value_len);
    }
  else if (f->phase == BTREE_PHASE_COMPUTE_STATS)
    {
      ...
    }
  else
    {
      g_assert_not_reached ();
    }
}

static gboolean
btree__done_feeding     (GskTableFile             *file,
                         gboolean                 *ready_out,
                         GError                  **error)
{
  BtreeFile *f = (BtreeFile *) file;
  if (f->phase == BTREE_PHASE_IN_MEMORY_BUILD)
    {
      ...
    }
  else if (f->phase == BTREE_PHASE_COMPUTE_STATS)
    {
      ...
    }
  else
    {
      g_assert_not_reached ();
    }
}

static gboolean
btree__get_build_state  (GskTableFile             *file,
                         guint                    *state_len_out,
                         guint8                  **state_data_out,
                         GError                  **error)
{
  ...
}

static gboolean
btree__build_file       (GskTableFile             *file,
                         gboolean                 *ready_out,
                         GError                  **error)
{
  ...
}

static void
btree__release_build_data(GskTableFile            *file)
{
  ...
}

/* methods for a file which has been constructed */
static gboolean
btree__query_file       (GskTableFile             *file,
                         GskTableFileQuery        *query_inout,
                         GError                  **error)
{
  ...
}

static GskTableReader *
btree__create_reader    (GskTableFile             *file,
                         GError                  **error)
{
  ...
}

static gboolean
btree__get_reader_state (GskTableFile             *file,
                         GskTableReader           *reader,
                         guint                    *state_len_out,
                         guint8                  **state_data_out,
                         GError                  **error)
{
  ...
}

static GskTableReader *
btree__recreate_reader  (GskTableFile             *file,
                         guint                     state_len,
                         const guint8             *state_data,
                         GError                  **error)
{
  ...
}

/* destroying files and factories */
static gboolean
btree__destroy_file     (GskTableFile             *file,
                         gboolean                  erase,
                         GError                  **error)
{
  ...
}

static void
btree__destroy_factory  (GskTableFileFactory      *factory)
{
  /* no-op for static factory */
}


/* for now, return a static factory object */
GskTableFileFactory *gsk_table_file_factory_new_btree (void)
{
  static GskTableFileFactory the_factory =
    {
      btree__create_file,
      btree__open_building_file,
      btree__open_file,
      btree__feed_entry,
      btree__done_feeding,
      btree__get_build_state,
      btree__build_file,
      btree__release_build_data,
      btree__query_file,
      btree__create_reader,
      btree__get_reader_state,
      btree__recreate_reader,
      btree__destroy_file,
      btree__destroy_factory
    };
  return &the_factory;
}
