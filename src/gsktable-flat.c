
typedef struct _FlatFactory FlatFactory;
struct _FlatFactory
{
  GskTableFileFactory base_factory;
  guint bytes_per_chunk;
  guint compression_level;
};

struct _MmapFileReader
{
  gint fd;
  guint64 file_size;
  guint8 *mmapped;
  GskTableBuffer tmp_buf;               /* only if !mmapped */

  MmapUsageState *usage_state;
};

struct _FlatFileBuilder
{
  GskTableBuffer input;

  gboolean has_last_key;
  GskTableBuffer first_key;
  GskTableBuffer last_key;

  GskTableBuffer uncompressed;
  GskTableBuffer compressed;

  MmapFileWriter writers[N_FILES];
 
  z_stream compressor;
  GskMemPool compressor_allocator;
  guint8 *compressor_allocator_scratchpad;
  gsize compressor_allocator_scratchpad_len;
};

typedef enum
{
  FILE_INDEX,
  FILE_FIRSTKEYS
  FILE_DATA,
} WhichFile;
#define N_FILES 3

static const char *file_extensions[N_FILES] = { "index", "firstkeys", "data" };

typedef struct _FlatFile FlatFile;
struct _FlatFile
{
  GskTableFile base_file;
  gint         fds[N_FILES];
  FlatFileBuilder *builder;

  gboolean has_readers;         /* builder and has_readers are exclusive: they
                                   cannot be set at the same time */
  MmapFileReader readers[N_FILES];
};


/* each entry in index file is:
     4 bytes -- initial key length
     8 bytes -- initial key offset
     8 bytes -- data offset
     4 bytes -- data length
 */

/* --- mmap reading implementation --- */
static gboolean
mmap_file_reader_init (MmapFileReader *reader,
                       gint            fd,
                       GError        **error)
{
  struct stat stat_buf;
  reader->fd = fd;
  if (fstat (fd, &stat_buf) < 0)
    {
      g_set_error (error, GSK_G_ERROR_DOMAIN_QUARK,
                   GSK_ERROR_FILE_STAT,
                   "error stating fd %d: %s",
                   fd, g_strerror (errno));
      return FALSE;
    }
  reader->file_size = stat_buf.st_size;

  if (reader->file_size < max_mmap)
    {
      reader->mmapped = mmap (NULL, reader->file_size, PROT_READ, MAP_SHARED, fd, 0);
      if (reader->mmapped == NULL || reader->mmapped == MAP_FAILED)
        {
          reader->mmapped = NULL;
          g_set_error (error, GSK_G_ERROR_DOMAIN_QUARK,
                       GSK_ERROR_FILE_MMAP,
                       "error mmapping fd %d: %s",
                       fd, g_strerror (errno));
          return FALSE;
        }
    }
  else
    {
      reader->mmapped = NULL;
      gsk_table_buffer_init (&reader->tmp_buf);
    }
  return TRUE;
}

static inline void
reinit_compressor (FlatFileBuilder *builder,
                   guint            compression_level,
                   gboolean         maybe_expand_mempool)
{
  if (maybe_expand_mempool && builder->compressor_allocator->all_chunk_list != NULL)
    {
      builder->compressor_allocator_scratchpad_len *= 2;
      builder->compressor_allocator_scratchpad = g_realloc (builder->compressor_allocator_scratchpad,
                                                            builder->compressor_allocator_scratchpad_len);
    }
  gsk_mem_pool_construct_with_scratch_buf (&builder->compressor_allocator,
                                           builder->compressor_allocator_scratchpad,
                                           builder->compressor_allocator_scratchpad_len);
  memset (&builder->compressor, 0, sizeof (z_stream));
  builder->compressor.zalloc = my_mem_pool_alloc;
  builder->compressor.zfree = my_mem_pool_free;
  builder->compressor.opaque = builder;
  deflateInit (&builder->compressor, compression_level);
}

static FlatFileBuilder *
flat_file_builder_new (guint compression_level)
{
  FlatFileBuilder *builder = g_slice_new (FlatFileBuilder);
  gsk_table_buffer_init (&builder->input);
  gsk_table_buffer_init (&builder->first_key);
  gsk_table_buffer_init (&builder->last_key);
  gsk_table_buffer_init (&builder->compressed);
  gsk_table_buffer_init (&builder->uncompressed);
  builder->compressor_allocator_scratchpad_len = 1024;
  builder->compressor_allocator_scratchpad = g_malloc (builder->compressor_allocator_scratchpad_len);
  reinit_compressor (builder, compression_level, FALSE);
  return builder;
}

typedef enum
{
  OPEN_MODE_CREATE,
  OPEN_MODE_CONTINUE_CREATE,
  OPEN_MODE_READONLY
} OpenMode;

static gboolean
open_3_files (FlatFile                 *file,
              const char               *dir,
              guint64                   id,
              OpenMode                  open_mode,
              GError                  **error)
{
  char fname_buf[GSK_TABLE_MAX_PATH];
  guint open_flags;
  const char *participle;
  guint f;
  switch (open_mode)
    {
    case OPEN_MODE_CREATE:
      open_flags = O_RDWR | O_CREAT | O_TRUNC;
      participle = "creating";
      break;
    case OPEN_MODE_CONTINUE_CREATE:
      open_flags = O_RDWR;
      participle = "opening for writing";
      break;
    case OPEN_MODE_READONLY:
      open_flags = O_RDONLY;
      participle = "opening for reading";
      break;
    default:
      g_assert_not_reached ();
    }

  for (f = 0; f < N_FILES; f++)
    {
      gsk_table_mk_fname (fname_buf, dir, id, file_extensions[f]);
      file->fds[f] = open (fname_buf, open_flags, 0644);
      if (file->index_fd < 0)
        {
          guint tmp_f;
          for (tmp_f = 0; tmp_f < f; tmp_f++)
            close (file->fds[tmp_f]);
          RAISE_OPEN_ERROR ();
          g_set_error (error, GSK_G_ERROR_DOMAIN_QUARK,
                       GSK_ERROR_FILE_CREATE,
                       "error %s %s: %s",
                       participle, fname_buf, g_strerror (errno));
          return FALSE;
        }
    }
  return TRUE;
}

static GskTableFile *
flat__create_file      (GskTableFileFactory      *factory,
                        const char               *dir,
                        guint64                   id,
                        const GskTableFileHints  *hints,
                        GError                  **error)
{
  FlatFactory *ffactory = (FlatFactory *) factory;
  FlatFile *rv = g_slice_new (FlatFile);

  if (!open_3_files (rv, dir, id, OPEN_MODE_CREATE, error))
    {
      g_slice_free (FlatFile, rv);
      return NULL;
    }

  rv->builder = flat_file_builder_new (ffactory->compression_level);
  rv->has_readers = FALSE;
  return &rv->base_file;
}

static GskTableFile *
flat__open_building_file(GskTableFileFactory     *factory,
                         const char               *dir,
                         guint64                   id,
                         guint                     state_len,
                         const guint8             *state_data,
                         GError                  **error)
{
  FlatFactory *ffactory = (FlatFactory *) factory;
  FlatFile *rv = g_slice_new (FlatFile);
  if (!open_3_files (rv, dir, id, OPEN_MODE_CONTINUE_CREATE, error))
    {
      g_slice_free (FlatFile, rv);
      return NULL;
    }

  rv->builder = flat_file_builder_new (ffactory->compression_level);

  /* seek according to 'state_data' */
  g_assert (state_len == 24);
  {
    guint64 offsets_le[3];
    memcpy (offsets_le, state_data, 24);
    for (f = 0; f < N_FILES; f++)
      {
        guint64 offset_le;
        guint64 offset;
        memcpy (&offset_le, state_data + 8 * f, 8);
        offset = GUINT64_FROM_LE (offset_le);
        if (!gsk_table_mmap_writer_init_at (&rv->builder->writers[f], rv->fds[f], offset, error))
          {
            for (tmp_f = 0; tmp_f < f; tmp_f++)
              gsk_table_mmap_writer_unmap (&rv->builder->writers[tmp_f]);
            for (tmp_f = 0; tmp_f < N_FILES; tmp_f++)
              close (rv->fds[tmp_f]);
            builder_recycle (ffactory, rv->builder);
            g_slice_free (FlatFile, rv);
            return NULL;
          }
      }
  }
  rv->has_readers = FALSE;

  return &rv->base_file;
}

GskTableFile *
flat__open_file        (GskTableFileFactory      *factory,
                        const char               *dir,
                        guint64                   id,
                        GError                  **error)
{
  FlatFactory *ffactory = (FlatFactory *) factory;
  FlatFile *rv = g_slice_new (FlatFile);
  if (!open_3_files (rv, dir, id, OPEN_MODE_READONLY, error))
    {
      g_slice_free (FlatFile, rv);
      return NULL;
    }
  rv->builder = NULL;

  /* mmap small files for reading */
  for (f = 0; f < N_FILES; f++)
    {
      if (!mmap_file_reader_init (&ffile->readers[f], ffile->fds[f], error))
        {
          guint tmp_f;
          for (tmp_f = 0; tmp_f < f; tmp_f++)
            mmap_file_reader_clear (&ffile->readers[tmp_f]);
          for (f = 0; f < N_FILES; f++)
            close (ffile->fds[f]);
          g_slice_free (FlatFile, rv);
          return NULL;
        }
    }
  rv->has_readers = TRUE;

  return &rv->base_file;
}

static inline void
do_compress (FlatFileBuilder *builder,
             guint            len,
             const guint8    *data)
{
  ...
}

static guint
uint32_vli_encode (guint32 to_encode,
                   guint8 *buf)         /* min length 5 */
{
  if (to_encode < 0x80)
    {
      buf[0] = to_encode;
      return 1;
    }
  else if (to_encode < (1<<14))
    {
      buf[0] = 0x80 | (to_encode >> 7);
      buf[1] = to_encode & 0x7f;
      return 2;
    }
  else if (to_encode < (1<<21))
    {
      buf[0] = 0x80 | (to_encode >> 14);
      buf[1] = 0x80 | (to_encode >> 7);
      buf[2] = to_encode & 0x7f;
      return 3;
    }
  else if (to_encode < (1<<28))
    {
      buf[0] = 0x80 | (to_encode >> 21);
      buf[1] = 0x80 | (to_encode >> 14);
      buf[2] = 0x80 | (to_encode >> 7);
      buf[3] = to_encode & 0x7f;
      return 4;
    }
  else
    {
      buf[0] = 0x80 | (to_encode >> 28);
      buf[1] = 0x80 | (to_encode >> 21);
      buf[2] = 0x80 | (to_encode >> 14);
      buf[3] = 0x80 | (to_encode >> 7);
      buf[4] = to_encode & 0x7f;
      return 5;
    }
}
static guint
uint32_vli_decode (const guint8 *input,
                   guint32      *decoded)
{
  guint32 val = input[0] & 0x7f;
  if (input[0] & 0x80)
    {
      guint used = 1;
      do
        {
          val <<= 7;
          val |= (input[used] & 0x7f);
        }
      while ((input[used++] & 0x80) != 0);
      *decoded = val;
      return used;
    }
  else
    {
      *decoded = val;
      return 1;
    }
}

static gboolean
flush_to_files (FlatFileBuilder *builder,
                GError **error)
{
  /* emit index, keyfile and data file stuff */
  guint8 header[2*12];

  /* flush compressor */
  do_compress_flush (builder);

  /* encode index entry */
  encode_length_offset (header, builder->first_key.len, gsk_table_mmap_writer_offset (builder->firstkeys_writer));
  encode_length_offset (header + 12, builder->compressed.len, gsk_table_mmap_writer_offset (builder->data_writer));

  /* write data to files */
  if (!gsk_table_mmap_writer_write (builder->index_writer, sizeof(header), header, error)
   || !gsk_table_mmap_writer_write (builder->firstkeys_writer, builder->first_key.len, builder->first_key.data, error))
   || !gsk_table_mmap_writer_write (builder->data_writer, builder->first_key.len, builder->first_key.data, error))
    return FALSE;
  return TRUE;
}

/* methods for a file which is being built */
static GskTableFeedEntryResult
flat__feed_entry      (GskTableFile             *file,
                       guint                     key_len,
                       const guint8             *key_data,
                       guint                     value_len,
                       const guint8             *value_data,
                       GError                  **error)
{
  FlatFile *ffile = (FlatFile *) file;
  FlatFactory *ffactory = (FlatFactory *) file->factory;
  FlatFileBuilder *builder = ffile->builder;
  guint8 enc_buf[5+5];

  g_assert (builder != NULL);

  if (builder->has_last_key)
    {
      /* compute prefix length */
      guint prefix_len = 0;
      guint max_prefix_len = MIN (key_len, builder->last_key.len);
      guint encoded_len, tmp;
      while (prefix_len < max_prefix_len
          && key_data[prefix_len] == builder->last_key.data[prefix_len])
        prefix_len++;

      /* encode prefix_length, and (key_len-prefix_length) */
      encoded_len = uint32_vli_encode (prefix_len, enc_buf);
      tmp = uint32_vli_encode (key_len - prefix_len, enc_buf + encoded_len);
      encoded_len += tmp;
      memcpy (gsk_table_buffer_set_len (builder->uncompressed, encoded_len)
              enc_buf, encoded_len);

      /* copy non-prefix portion of key */
      memcpy (gsk_table_buffer_append (builder->uncompressed, key_len - prefix_len),
              key_data + prefix_len, key_len - prefix_len);
    }
  else
    {
      /* the key's length will be in the index file
         (no prefix-compression possible on the first key);
         the key's data will be in the firstkeys file */
      builder->has_last_key = TRUE;
      memcpy (gsk_table_buffer_set_len (&builder->first_key,
                                        key_len), key_data, key_len);
    }

  /* encode value length */
  encoded_len = uint32_vli_encode (value_len, enc_buf);
  memcpy (gsk_table_buffer_append (builder->uncompressed, encoded_len)
          enc_buf, encoded_len);

  /* compress the non-value portion */
  do_compress (builder, builder->uncompressed.len, builder->uncompressed.data);

  /* compress the value portion */
  do_compress (builder, value_len, value_data);

  if (builder->compressed.len >= ffactory->bytes_per_chunk)
    {
      if (!flush_to_files (builder, error))
        return GSK_TABLE_FEED_ENTRY_ERROR;

      reinit_compressor (builder, ffactory->compression_level, TRUE);
      builder->has_last_key = FALSE;
      gsk_table_buffer_set_len (&builder->compressed, 0);
    }
  else
    {
      builder->has_last_key = TRUE;
      memcpy (gsk_table_buffer_set_len (&builder->last_key,
                                        key_len), key_data, key_len);
    }
  return builder->has_last_key ? GSK_TABLE_FEED_ENTRY_WANT_MORE
                               : GSK_TABLE_FEED_ENTRY_SUCCESS;
}

static gboolean 
flat__done_feeding     (GskTableFile             *file,
                        gboolean                 *ready_out,
                        GError                  **error)
{
  FlatFile *ffile = (FlatFile *) file;
  FlatFactory *ffactory = (FlatFactory *) file->factory;
  FlatFileBuilder *builder = ffile->builder;
  guint f;
  if (builder->has_last_key)
    {
      if (!flush_to_files (builder, error))
        return FALSE;
    }

  /* unmap and ftruncate all files */
  for (f = 0; f < N_FILES; f++)
    {
      guint64 offset = mmap_writer_offset (&builder->writers[f]);
      mmap_writer_clear (&builder->writers[f]);
      if (ftruncate (ffile->fds[f], offset) < 0)
        {
          g_set_error (error, GSK_G_ERROR_DOMAIN_QUARK,
                       GSK_ERROR_FILE_TRUNCATE,
                       "error truncating %s file: %s",
                       file_extensions[f], g_strerror (errno));
          return FALSE;
        }
    }

  /* mmap for reading small files */
  for (f = 0; f < N_FILES; f++)
    if (!mmap_file_reader_init (&ffile->readers[f], ffile->fds[f], error))
      {
        for (tmp_f = 0; tmp_f < f; tmp_f++)
          mmap_file_reader_clear (&ffile->readers[tmp_f]);
        return FALSE;
      }

  /* recycle/free the builder object */
  ffile->builder = NULL;
  builder_recycle (ffactory, builder);

  return TRUE;
}

static gboolean 
flat__get_build_state  (GskTableFile             *file,
                        guint                    *state_len_out,
                        guint8                  **state_data_out,
                        GError                  **error)
{
  guint f;
  *state_len_out = 1 + 3 * 8;
  *state_data_out = g_malloc (*state_len_out);
  *state_data_out[0] = 0;               /* phase 0; reserved to allow multiphase processing in future */
  for (f = 0; f < N_FILES; f++)
    {
      guint64 offset = gsk_table_mmap_writer_offset (builder->writers + f);
      guint64 offset_le = GUINT64_TO_BE (offset);
      memcpy (*state_data_out + 8 * f + 1, &offset_le, 8);
    }
  return TRUE;
}

static gboolean 
flat__build_file       (GskTableFile             *file,
                        gboolean                 *ready_out,
                        GError                  **error)
{
  *ready_out = TRUE;
  return TRUE;
}

static void     
flat__release_build_data(GskTableFile            *file)
{
  /* nothing to do, since we finish building immediately */
}

/* --- query api --- */
static gboolean 
flat__query_file       (GskTableFile             *file,
                        GskTableFileQuery        *query_inout,
                        GError                  **error)
{
  FlatFile *ffile = (FlatFile *) file;
  guint64 n_index_records, first, n;
  if (ffile->builder != NULL)
    n_index_records = mmap_writer_offset (ffile->builder->writers[FILE_INDEX])
                    / SIZEOF_INDEX_ENTRY;
  else if (ffile->has_readers)
    n_index_records = ffile->readers[FILE_INDEX].file_size
                    / SIZEOF_INDEX_ENTRY;
  else
    {
      g_set_error (error, GSK_G_ERROR_DOMAIN_QUARK,
                   GSK_ERROR_INVALID_STATE,
                   "flat file in error state");
      return FALSE;
    }

  if (n_index_records == 0)
    {
      query_inout->found = FALSE;
      return TRUE;
    }
  first = 0;
  n = n_index_records;
  while (n > 1)
    {
      guint64 mid = first + n / 2;

      /* read index entry */
      ...

      /* read key */
      ...

      /* invoke comparator */
      ...

      if (compare_rv < 0)
        {
          n = mid - first;
        }
      else if (compare_rv > 0)
        {
          n = first + n - mid;
          first = mid;
        }
      else
        {
          CacheEntryRecord *record;
          cache_entry = cache_entry_force (ffile, mid, error);
          if (cache_entry == NULL)
            return FALSE;
          record = cache_entry->records + 0;
          memcpy (gsk_table_buffer_set_len (&query_inout->value, record->value_len),
                  record->value_data, record->value_len);
          query_inout->found = TRUE;
          return TRUE;
        }
    }

  /* uncompress block, cache */
  cache_entry = cache_entry_force (ffile, first, error);
  if (cache_entry == NULL)
    return FALSE;

  /* bsearch the uncompressed block */
  {
    guint first = 0;
    guint n = cache_entry->n_records;
    while (n > 1)
      {
        guint mid = first + n / 2;
        CacheEntryRecord *record = cache_entry->records + mid;
        int compare_rv = query_inout->compare (record->key_len, record->key_data,
                                               query_inout->compare_data);
        if (compare_rv < 0)
          {
            n = mid - first;
          }
        else if (compare_rv > 0)
          {
            n = first + n - mid;
            first = mid;
          }
        else
          {
            memcpy (gsk_table_buffer_set_len (&query_inout->value, record->value_len),
                    record->value_data, record->value_len);
            query_inout->found = TRUE;
            return;
          }
      }
    if (n == 1 && first < cache_entry->n_records)
      {
        CacheEntryRecord *record = cache_entry->records + first;
        int compare_rv = query_inout->compare (record->key_len, record->key_data,
                                               query_inout->compare_data);
        if (compare_rv == 0)
          {
            memcpy (gsk_table_buffer_set_len (&query_inout->value, record->value_len),
                    record->value_data, record->value_len);
            query_inout->found = TRUE;
            return;
          }
      }
  }
  query_inout->found = FALSE;
  return TRUE;
}

/* --- reader api --- */
static FlatFileReader *
reader_open_fps (GskTableFile *file,
                 const char   *dir,
                 GError      **error)
{
  FlatFileReader *freader = g_slice_new (FlatFileReader);
  freader->base_reader.eof = FALSE;
  freader->base_reader.error = NULL;
  for (f = 0; f < N_FILES; f++)
    {
      freader->fps[f] = ...;
      if (freader->fps[f] == NULL)
        {
          ...
          g_slice_free (FlatFileReader, freader);
          return NULL;
        }
    }
  freader->base_reader.advance = reader_advance;
  freader->base_reader.destroy = reader_destroy;
  return freader;
}
static FlatFileReader *
reader_open_eof (void)
{
  FlatFileReader *freader = g_slice_new (FlatFileReader);
  freader->base_reader.eof = TRUE;
  freader->base_reader.error = NULL;
  for (f = 0; f < N_FILES; f++)
    freader->fps[f] = NULL;
  freader->base_reader.advance = reader_advance;
  freader->base_reader.destroy = reader_destroy;
  return freader;
}

/* only returns FALSE on error -- eof just raises the flag */
static void
read_and_uncompress_chunk (FlatFileReader *freader)
{
  /* read index fp record, or set eof flag or error */
  guint8 index_data[SIZEOF_INDEX_ENTRY];
  IndexEntry index_entry;
  guint8 *firstkey;
  if (FREAD (index_data, SIZEOF_INDEX_ENTRY, 1, freader->fps[FILE_INDEX]) != 1)
    {
      freader->eof = 1;
      return;
    }
  index_entry_deserialize (index_data, &index_entry);

  /* allocate buffers in one piece */
  firstkey = g_malloc (index_entry.firstkey_len + index_entry.compressed_data_len);
  compressed_data = firstkey + index_entry.firstkey_len;

  /* read firstkey */
  if (FREAD (firstkey, index_entry.firstkey_len, 1, freader->fps[FILE_FIRSTKEYS]) != 1)
    {
      freader->error = g_error_new (GSK_G_ERROR_DOMAIN_QUARK,
                                    GSK_ERROR_PREMATURE_EOF,
                                    "premature eof in firstkey file");
      g_free (firstkey);
      return;
    }

  /* read data */
  compressed_data = g_malloc (index_entry.compressed_data_len);
  if (FREAD (compressed_data, index_entry.compressed_data_len, 1, freader->fps[FILE_FIRSTKEYS]) != 1)
    {
      freader->error = g_error_new (GSK_G_ERROR_DOMAIN_QUARK,
                                    GSK_ERROR_PREMATURE_EOF,
                                    "premature eof in compressed-data file");
      g_free (firstkey);
      return;
    }

  /* do the actual un-gzipping and scanning */
  freader->cache_entry = cache_entry_deserialize (index_entry.firstkey_len, firstkey,
                                                  index_entry.compressed_data_len, compressed_data,
                                                  &freader->base_reader.error);
  g_free (firstkey);
}

static inline void
init_base_reader_record (FlatFileReader *freader)
{
  CacheEntryRecord *record = freader->cache_entry->records + freader->record_index;
  freader->base_reader.key_len = record->key_len;
  freader->base_reader.key_data = record->key_data;
  freader->base_reader.value_len = record->value_len;
  freader->base_reader.value_data = record->value_data;
}

static void
reader_advance (GskTableReader *reader)
{
  FlatFileReader *freader = (FlatFileReader *) reader;
  if (freader->eof || freader->error)
    return;
  if (++freader->record_index == freader->cache_entry->n_records)
    {
      g_free (freader->cache_entry);
      read_and_uncompress_chunk (freader);
      if (reader->eof || reader->error != NULL)
        return;
      freader->record_index = 0;
    }
  init_base_reader_record (freader);
}

static GskTableReader *
flat__create_reader    (GskTableFile             *file,
                        const char               *dir,
                        GError                  **error)
{
  FlatFileReader *freader = reader_open_fps (file, dir, error);
  if (freader == NULL)
    return NULL;

  read_and_uncompress_chunk (freader);
  if (!freader->base_reader.eof && freader->base_reader.error == NULL)
    {
      freader->record_index = 0;
      init_base_reader_record (freader);
    }

  return &freader->base_reader;
}

/* you must always be able to get reader state */
static gboolean 
flat__get_reader_state (GskTableFile             *file,
                        GskTableReader           *reader,
                        guint                    *state_len_out,
                        guint8                  **state_data_out,
                        GError                  **error)
{
  /* state is:
       1 byte state -- 0=in progress;  1=eof
     if state==0:
       8 bytes index file offset
       8 bytes firstkeys file offset
       8 bytes data offset
       4 bytes index into the compressed byte to return next
     if state==1:
       no other data
   */
  g_assert (file->error == NULL);
  if (file->eof)
    {
      *state_len_out = 1;
      *state_data_out = g_malloc (1);
      (*state_data_out)[0] = 1;
      return TRUE;
    }
  ...
}

static GskTableReader *
flat__recreate_reader  (GskTableFile             *file,
                        const char               *dir,
                        guint                     state_len,
                        const guint8             *state_data,
                        GError                  **error)
{
  FlatFileReader *freader;
  if (freader == NULL)
    return NULL;
  switch (state_data[0])
    {
    case 0:             /* in progress */
      freader = reader_open_fps (file, dir, error);

      /* seek */
      ...

      uncompress_chunk (freader);
      freader->record_index = ...;
      init_base_reader_record (freader);

      if (freader->base_reader.eof
       || freader->base_reader.error != NULL)
        {
          if (freader->base_reader.error)
            {
              ...
            }
          else
            g_set_error (error, GSK_G_ERROR_DOMAIN_QUARK,
                         GSK_ERROR_PREMATURE_EOF,
                         "unexpected eof restoring file reader");
          for (f = 0; f < N_FILES; f++)
            fclose (freader->fps[f]);
          g_slice_free (FlatFileReader, freader);
          return NULL;
        }
      break;
    case 1:             /* eof */
      g_assert (state_len == 1);
      freader = reader_open_eof ();
      break;
    default:
      g_set_error (error, GSK_G_ERROR_DOMAIN_QUARK,
                   GSK_ERROR_PARSE,
                   "unknown state for reader");
      return NULL;
    }
  return &freader->base_reader;
}


/* destroying files and factories */
static gboolean
flat__destroy_file     (GskTableFile             *file,
                        const char               *dir,
                        gboolean                  erase,
                        GError                  **error)
{
  FlatFile *ffile = (FlatFile *) file;
  FlatFileBuilder *builder = ffile->builder;
  guint f;
  if (builder != NULL)
    {
      for (f = 0; f < N_FILES; f++)
        mmap_writer_clear (builder->writers + f);
      builder_recycle (builder);
    }
  else if (ffile->has_readers)
    {
      for (f = 0; f < N_FILES; f++)
        mmap_reader_clear (builder->readers + f);
    }
  for (f = 0; f < N_FILES; f++)
    close (ffile->fds[f]);
  if (erase)
    {
      for (f = 0; f < N_FILES; f++)
        {
          char fname_buf[GSK_TABLE_MAX_PATH];
          gsk_table_mk_fname (fname_buf, dir, file->id, file_extensions[f]);
          unlink (fname);
        }
    }
  g_slice_free (FlatFile, ffile);
  return TRUE;
}

static void
flat__destroy_factory  (GskTableFileFactory      *factory)
{
  /* static factory */
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
      16384,
      3                         /* zlib compression level */
    };

  return &the_factory.base_instance;
}
