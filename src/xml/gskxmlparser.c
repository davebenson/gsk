#include "gskxmlparser.h"

/* --- GskXmlParserConfig --- */
typedef struct _GskXmlParserStateTrans GskXmlParserStateTrans;
typedef struct _GskXmlParserState GskXmlParserState;
typedef struct _NsInfo NsInfo;

struct _GskXmlParserStateTrans
{
  char *name;
  GskXmlParserState *new_state;
};

struct _GskXmlParserState
{
  guint n_trans;
  GskXmlParserStateTrans *trans;

  GskXmlParserState *fallback_state;

  guint n_emit_indices;
  guint *emit_indices;
};
  
struct _NsInfo
{
  char *abbrev;
  char *url;
};

struct _GskXmlParserConfig
{
  GskXmlParserState *init;
  guint ref_count;
  guint done : 1;
  guint ignore_ns_tag : 1;
  guint passthrough_unknown_ns : 1;

  GPtrArray *paths;
  GArray *ns_array;             /* of NsInfo */
};



GskXmlParserConfig *
gsk_xml_parser_config_new (void)
{
  GskXmlParserConfig *config = g_new (GskXmlParserConfig, 1);
  config->init = NULL;
  config->ref_count = 1;
  config->done = 0;
  config->ignore_ns_tag = 0;
  config->passthrough_unknown_ns = 0;

  config->paths = g_ptr_array_new ();
  return config;
}

static GskXmlParserConfig *real_new_by_depth (guint depth)
{
  GskXmlParserConfig *n = gsk_xml_parser_config_new ();
  char *str;
  guint i;
  gsk_xml_parser_config_set_flags (n, GSK_XML_PARSER_IGNORE_NS_TAGS);
  str = g_malloc (depth * 2 + 2);
  for (i = 0; i < depth; i++)
    {
      str[2*i+0] = '*';
      str[2*i+1] = '/';
    }
  str[2*i+0] = '*';
  str[2*i+1] = 0;
  gsk_xml_parser_config_add_path (n, str);
  gsk_xml_parser_config_done (n);
  return n;
}

GskXmlParserConfig *
gsk_xml_parser_config_new_by_depth (guint depth)
{
  static GskXmlParserConfig *by_depths[32];
  if (G_LIKELY (depth < G_N_ELEMENTS (by_depths)))
    {
      if (by_depths[depth] == NULL)
        by_depths[depth] = real_new_by_depth (depth);
      return gsk_xml_parser_config_ref (by_depths[depth]);
    }
  else
    return real_new_by_depth (depth);
}

GskXmlParserConfig *
gsk_xml_parser_config_ref (GskXmlParserConfig *config)
{
  g_return_val_if_fail (config->done, NULL);
  g_return_val_if_fail (config->ref_count > 0, NULL);
  ++(config->ref_count);
  return config;
}

static void
free_state_recursive (GskXmlParserState *state)
{
  guint i;
  for (i = 0; i < state->n_trans; i++)
    {
      g_free (state->trans[i].name);
      free_state_recursive (state->trans[i].new_state);
    }
  g_free (state->trans);
  if (state->fallback_state)
    free_state_recursive (state->fallback_state);
  g_free (state);
}

void
gsk_xml_parser_config_unref    (GskXmlParserConfig *config)
{
  g_return_if_fail (config->ref_count > 0);
  if (--(config->ref_count) == 0)
    {
      free_state_recursive (config->init);
      g_ptr_array_foreach (config->paths, (GFunc) g_free, NULL);
      g_ptr_array_free (config->paths, TRUE);
      g_free (config);
    }
}


guint
gsk_xml_parser_config_add_path (GskXmlParserConfig *config,
                                const char         *path)
{
  guint rv = config->paths->len;
  g_ptr_array_add (config->paths, g_strdup (path));
  return rv;
}

void
gsk_xml_parser_config_add_ns   (GskXmlParserConfig *config,
                                const char         *abbrev,
                                const char         *url)
{
  NsInfo ns_info;
  g_return_if_fail (!config->done);
  ns_info.abbrev = g_strdup (abbrev);
  ns_info.url = g_strdup (url);
  g_array_append_val (config->ns_array, ns_info);
}

void
gsk_xml_parser_config_set_flags(GskXmlParserConfig *config,
                                GskXmlParserFlags   flags)
{
  g_return_if_fail (!config->done);
  ...
}

GskXmlParserFlags
gsk_xml_parser_config_get_flags(GskXmlParserConfig *config)
{
  ...
}

struct _PathIndex
{
  guint path_len;
  const char *path_start;
  guint orig_index;
};

static void
branch_states_recursive (GskXmlParserState *state,
                         guint              n_paths,
                         char             **paths)
{
  char **next_paths =  g_newa (char *, n_paths);
  PathIndex *indices = g_newa (PathIndex, n_paths);
  guint i;
  guint n_indices = 0;
  guint n_trans;
  for (i = 0; i < n_paths; i++)
    {
      if (paths[i] == NULL)
        next_paths[i] = NULL;
      else if ((end=strchr (paths[i], '/')) != NULL)
        {
          indices[n_indices].path_start = paths[i];
          indices[n_indices].path_len = end - paths[i];
          indices[n_indices].orig_index = i;
          n_indices++;
          got_trans = TRUE;          // needed?
          next_paths[i] = end + 1;
          if (end == paths[i] + 1 && paths[i][0] == '*')
            n_star_trans++;
        }
      else
        {
          indices[n_indices].path_start = paths[i];
          indices[n_indices].path_len = strlen (paths[i]);
          indices[n_indices].orig_index = i;
          n_indices++;
          got_output = TRUE;          // needed?
          next_paths[i] = NULL;
          if (strcmp (paths[i], "*") == 0)
            n_star_outputs++;
        }
    }
  if (n_indices == 0)
    {
      state->n_trans = 0;
      state->trans = NULL;
      return;
    }

  qsort (indices, n_indices, sizeof (PathIndex), compare_path_index_by_str);
  n_trans = 0;
  for (i = 0; i < n_indices; i++)
    if (i == 0
      || indices[i-1].path_len != indices[i].path_len
      || memcmp (indices[i-1].path, indices[i].path, indices[i].path_len) != 0)
      {
        indices[i].is_start = TRUE;
        if (indices[i].path_len == 1 && indices[i].path[0] == '*')
          star_trans_start = i;
        else
          n_trans++;
      }
    else
      {
        indices[i].is_start = FALSE;
      }

  state->trans = g_new (GskXmlParserStateTrans, n_trans);
  for (i = 0; i < n_indices; )
    {
      guint end;
      guint n_outputs = n_star_outputs;
      if (next_paths[indices[i].orig_index])
        n_outputs++;
      for (end = i + 1; end < n_indices && !indices[end].is_start; end++)
        if (next_paths[indices[end].orig_index])
          n_outputs++;

      /* create state->trans[i] */
      ...

      /* write state->trans[i].state->outputs */
      ...

      /* create state_next_paths by copying next_paths
         and zeroing unmatching paths */
      ...

      /* initialize fallback state */
      if (n_star_trans)
        {
          ...
        }

      /* recurse on state */
      ...
    }
}

void
gsk_xml_parser_config_done(GskXmlParserConfig *config)
{
  g_return_if_fail (config != NULL);
  g_return_if_fail (!config->done);
  g_return_if_fail (config->ref_count > 0);
  config->done = 1;
  branch_states_recursive (config->state,
                           config->paths->len,
                           (char **) config->paths->pdata);
}


/* --- GskXmlParser --- */
struct _ParseLevel
{
  GskXmlParserState *state;     /* NULL if past known state */
  GPtrArray *children;          /* NULL if discardable */
  guint n_ns;
  char **ns_map;                /* prefix to canon prefix */
  ParseLevel *up;
};
struct _GskXmlParser
{
  GMarkupParseContext *parse_context;
  ParseLevel *level;
  GskXmlParserConfig *config;
};

/* --- without namespace support --- */
static void 
without_ns__start_element  (GMarkupParseContext *context,
                            const gchar         *element_name,
                            const gchar        **attribute_names,
                            const gchar        **attribute_values,
                            gpointer             user_data,
                            GError             **error)
{
  ...
}

static void 
without_ns__end_element(GMarkupParseContext *context,
                        const gchar         *element_name,
                        gpointer             user_data,
                        GError             **error)
{
  ...
}

static void 
without_ns__text       (GMarkupParseContext *context,
                        const gchar         *text,
                        gsize                text_len,  
                        gpointer             user_data,
                        GError             **error)
{
  ...
}


static void 
without_ns__passthrough(GMarkupParseContext *context,
                        const gchar         *passthrough_text,
                        gsize                text_len,  
                        gpointer             user_data,
                        GError             **error)
{
  ...
}

/* --- with namespace support --- */
static void 
with_ns__start_element  (GMarkupParseContext *context,
                         const gchar         *element_name,
                         const gchar        **attribute_names,
                         const gchar        **attribute_values,
                         gpointer             user_data,
                         GError             **error)
{
  ...
}

static void 
with_ns__end_element(GMarkupParseContext *context,
                     const gchar         *element_name,
                     gpointer             user_data,
                     GError             **error)
{
  ...
}

static void 
with_ns__text       (GMarkupParseContext *context,
                     const gchar         *text,
                     gsize                text_len,  
                     gpointer             user_data,
                     GError             **error)
{
  ...
}


static void 
with_ns__passthrough(GMarkupParseContext *context,
                     const gchar         *passthrough_text,
                     gsize                text_len,  
                     gpointer             user_data,
                     GError             **error)
{
  ...
}

static GMarkupParser without_ns_parser =
{
  without_ns__start_element,
  without_ns__end_element,
  without_ns__text,
  without_ns__passthrough,
  NULL                          /* no error handler */
};
static GMarkupParser with_ns_parser =
{
  with_ns__start_element,
  with_ns__end_element,
  with_ns__text,
  with_ns__passthrough,
  NULL                          /* no error handler */
};

GskXmlParser *
gsk_xml_parser_new_take (GskXmlParserConfig *config)
{
  GskXmlParser *parser;
  GMarkupParser *p;
  g_return_val_if_fail (config->done, NULL);
  parser = g_slice_new (GskXmlParser);
  p = config->ignore_ns_tag ? &without_ns_parser : &with_ns_parser;
  parser->parse_context = g_markup_parse_context_new (p, 0, parser, NULL);
  parser->level = NULL;
  parser->config = config;
  return parser;
}

GskXmlParser *
gsk_xml_parser_new (GskXmlParserConfig *config)
{
  return gsk_xml_parser_new_take (gsk_xml_parser_config_ref (config));
}


GskXmlParser *
gsk_xml_parser_new_by_depth (guint               depth)
{
  GskXmlParserConfig *config = gsk_xml_parser_config_new_by_depth (depth);
  return gsk_xml_parser_new_take (config);
}

GskXml *
gsk_xml_parser_dequeue      (GskXmlParser       *parser,
                             guint               index)
{
  ...
}

gboolean
gsk_xml_parser_feed         (GskXmlParser       *parser,
                             const guint8       *xml_data,
                             gssize              len,
                             GError            **error)
{
  return g_markup_parse_context_parse (parser->parse_context,
                                       (const char *) xml_data, len, error);
}

gboolean
gsk_xml_parser_feed_file    (GskXmlParser       *parser,
                             const char         *filename,
                             GError            **error)
{
  ...
}

void
gsk_xml_parser_free         (GskXmlParser       *parser)
{
  ...

  gsk_xml_parser_config_unref (parser->config);
}

