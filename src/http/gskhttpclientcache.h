
typedef struct _GskHttpClientCacheSlot GskHttpClientCacheSlot;
typedef struct _GskHttpClientCacheBin GskHttpClientCacheBin;
typedef struct _GskHttpClientCacheConfig GskHttpClientCacheConfig;
typedef struct _GskHttpClientCache GskHttpClientCache;

struct _GskHttpClientCacheConfig
{
  guint max_keepalive_clients;
  guint max_idle_millis;
  guint max_connecting;
  guint max_pipelined;
  guint min_connect_interval_millis;
};

/* --- frontend API --- */
/* The API for configuring the Keepalive behavior. */
GskHttpClientCacheConfig *       gsk_http_client_cache_config_new         (void);
GskHttpClientCacheConfig *       gsk_http_client_cache_config_copy        (const GskHttpClientCacheConfig *config);
void                             gsk_http_client_cache_config_free        (GskHttpClientCacheConfig       *config);

/* hostname may be NULL to peek/set the default configuration */
const GskHttpClientCacheConfig  *gsk_http_client_cache_peek_config        (GskHttpClientCache             *cache,
                                                                           const char                     *hostname);
void                             gsk_http_client_cache_set_config         (GskHttpClientCache             *cache,
                                                                           const char                     *hostname,
                                                                           const GskHttpClientCacheConfig *config);


/* --- backend API --- */
/* How a user can get an HttpClient
 * to make a request to a host, by name.
 *
 * User (for example, GskUrlTransferHttp implementation) calls
 * gsk_http_client_cache_make_client().  Then,
 * once the user is done, they call gsk_http_client_cache_slot_done().
 * It is critical that either gsk_http_client_cache_slot_done()
 * or gsk_http_client_cache_slot_failed()
 * is called for each invocation of a GskHttpClientCacheFunc.
 *
 * It is not really important when the user calls gsk_http_client_cache_slot_done(),
 * however, for the sake of getting consistent semantics, you should
 * organize to call slot_done() once you have finished reading the stream,
 * and slot_failed() if their was no response before the client_destroy notify.
 */
typedef void (*GskHttpClientCacheFunc)      (GskHttpClientCacheSlot    *slot,
                                             GskHttpClient             *http_client,
                                             gpointer                   data);
typedef void (*GskHttpClientCacheErrorFunc) (GskHttpClientCache        *cache,
                                             const GError              *error);


/* host_address may be NULL: it can be used to bypass DNS. */
void                gsk_http_client_cache_make_client         (GskHttpClientCache         *cache,
                                                               const char                 *hostname,
                                                               GskSocketAddress           *host_address,
                                                               GskHttpClientCacheFunc      callback,
                                                               GskHttpClientCacheErrorFunc error_callback,
                                                               gpointer                    data
                                                               GDestroyNotify              destroy);

GskStream          *gsk_http_client_cache_slot_peek_transport (GskHttpClientCacheSlot     *slot);
int                 gsk_http_client_cache_slot_peek_fd        (GskHttpClientCacheSlot     *slot);
GskSocketAddress   *gsk_http_client_cache_slot_peek_address   (GskHttpClientCacheSlot     *slot);
GskHttpClientCache *gsk_http_client_cache_slot_peek_cache     (GskHttpClientCacheSlot     *slot);
void                gsk_http_client_cache_slot_done           (GskHttpClientCacheSlot     *slot);


/* --- private (exposed for debugging) --- */
struct _GskHttpClientCacheSlot
{
  gboolean use_keepalive;
  GskStreamFd *stream_fd;
  GskHttpClient *client;

  /* number of times returned to ClientReadyFunc
     minus number of times recycled via done_client */
  guint n_pipelined;
};

struct _GskHttpClientCacheBin
{
  char *hostname;
  GskNameResolverTask *dns_task;

  /* maintain dns entries so that we can
   * effectively use round-robin DNS:
   * that is, instead of connecting randomly,
   * we connect in order. */
  guint n_dns_entries;
  GskSocketAddress **dns_entries;
  guint last_used_dns_entry;
  GskSource *dns_expiration_timeout;

  /* guard against connecting too frequently */
  GskSource *connect_interval_timeout;

  /* if NULL, then this uses the default config. */
  GskHttpClientCacheConfig *config;
};

struct _GskHttpClientCache
{
  guint ref_count;

  GHashTable *hostname_to_bin;
  GskHttpClientCacheConfig default_config;
};

