#include "gskxmlparser.h"

/* --- GskXmlParserConfig --- */
typedef struct _GskXmlParserStateTrans GskXmlParserStateTrans;

struct _GskXmlParserStateTrans
{
  char *name;
  GskXmlParserStateTrans *new_state;
};

struct _GskXmlParserState
{
  guint n_trans;
  GskXmlParserStateTrans *trans;

  GskXmlParserStateTrans *fallback_trans;

  gint emit_index;              /* or -1 */
};
  

struct _GskXmlParserConfig
{
  GskXmlParserState *init;
  guint ref_count;
  guint done : 1;
  guint ignore_ns_tag : 1;
  guint passthrough_unknown_ns : 1;
};



GskXmlParserConfig *
gsk_xml_parser_config_new (void)
{
  ...
}

static GskXmlParserConfig *real_new_by_depth (guint depth)
{
  GskXmlParserConfig *n = gsk_xml_parser_config_new ();
  char *str;
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

void
gsk_xml_parser_config_unref    (GskXmlParserConfig *config)
{
  g_return_if_fail (config->ref_count > 0);
  if (--(config->ref_count) == 0)
    {
      ...
    }
}

guint
gsk_xml_parser_config_add_path (GskXmlParserConfig *config,
                                const char         *path);
void
gsk_xml_parser_config_add_ns   (GskXmlParserConfig *config,
                                const char         *abbrev,
                                const char         *url);
void
gsk_xml_parser_config_set_flags(GskXmlParserConfig *config,
                                GskXmlParserFlags   flags)
{
  ...
}

GskXmlParserFlags
gsk_xml_parser_config_get_flags(GskXmlParserConfig *config)
{
  ...
}

void
gsk_xml_parser_config_done(GskXmlParserConfig *config)
{
  g_return_if_fail (config != NULL);
  g_return_if_fail (!config->done);
  g_return_if_fail (config->ref_count > 0);
  config->done = 1;
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


/* Called for character data */
/* text is not nul-terminated */
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


/* Called for character data */
/* text is not nul-terminated */
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
                             gsize               len,
                             GError            **error)
{
  ...
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

