#include "gskxml.h"
#include "../gskerror.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

/* TODO: proper error handling */

struct _GskXmlParser
{
  GskXmlBuilder *builder;
  GskXmlParserCallback callback;
  gpointer data;
  GDestroyNotify destroy;
  GMarkupParseContext *parse_context;
};
static void
xml_parser_start_element (GMarkupParseContext *context,
                          const gchar         *element_name,
                          const gchar        **attribute_names,
                          const gchar        **attribute_values,
                          gpointer             user_data,
                          GError             **error)
{
  GskXmlParser *parser = user_data;
  GskXmlString *elt_name = gsk_xml_string_new (element_name);
  guint n_attrs = 0;
  guint i;
  GskXmlString **attrs;
  while (attribute_names[n_attrs] != NULL)
    n_attrs++;
  attrs = g_newa (GskXmlString *, n_attrs * 2);
  for (i = 0; i < n_attrs; i++)
    {
      attrs[2*i+0] = gsk_xml_string_new (attribute_names[i]);
      attrs[2*i+1] = gsk_xml_string_new (attribute_values[i]);
    }
  gsk_xml_builder_start (parser->builder, elt_name, n_attrs, attrs);
  for (i = 0; i < n_attrs * 2; i++)
    gsk_xml_string_unref (attrs[i]);
  gsk_xml_string_unref (elt_name);
}

static void
xml_parser_end_element   (GMarkupParseContext *context,
                          const gchar         *element_name,
                          gpointer             user_data,
                          GError             **error)
{
  GskXmlParser *parser = user_data;
  GskXmlString *name = gsk_xml_string_new (element_name);
  GskXmlNode *node;
  gsk_xml_builder_end (parser->builder, name);
  gsk_xml_string_unref (name);
  node = gsk_xml_builder_get_doc (parser->builder);
  if (node != NULL)
    {
      parser->callback (node, parser->data);
      gsk_xml_node_unref (node);
    }
}

static void
xml_parser_text  (GMarkupParseContext *context,
                  const gchar         *text,
                  gsize                text_len,  
                  gpointer             user_data,
                  GError             **error)
{
  if (text_len > 0)
    {
      GskXmlParser *parser = user_data;
      GskXmlString *content = gsk_xml_string_new_len (text, text_len);
      gsk_xml_builder_text (parser->builder, content);
      gsk_xml_string_unref (content);
    }
}

static void
xml_parser_error  (GMarkupParseContext *context,
                   GError              *error,
                   gpointer             user_data)
{
  g_warning ("xml_parser_error: %s", error->message);
}


static GMarkupParser parser_funcs =
{
  xml_parser_start_element,
  xml_parser_end_element,
  xml_parser_text,
  NULL,                         /* no passthrough() implementation */
  xml_parser_error
};

GskXmlParser *
gsk_xml_parser_new        (GskXmlParseFlags     flags,
                           GskXmlParserCallback callback,
                           gpointer             data,
                           GDestroyNotify       destroy)
{
  GskXmlParser *parser = g_new (GskXmlParser, 1);
  parser->builder = gsk_xml_builder_new (flags);
  parser->callback = callback;
  parser->data = data;
  parser->destroy = destroy;
  parser->parse_context = g_markup_parse_context_new (&parser_funcs,
                                                      0,
                                                      parser, NULL);
  return parser;
}

gboolean
gsk_xml_parser_feed       (GskXmlParser        *parser,
                           const char          *data,
                           gssize               len,
                           GError             **error)
{
  return g_markup_parse_context_parse (parser->parse_context,
                                       data, len, error);
}

gboolean
gsk_xml_parser_finish     (GskXmlParser        *parser,
                           GError             **error)
{
  return g_markup_parse_context_end_parse (parser->parse_context, error);
}

gboolean
gsk_xml_parser_feed_file  (GskXmlParser        *parser,
                           const char          *filename,
                           GError             **error)
{
  int fd = open (filename, O_RDONLY);
  char buf[4096];
  gssize nread;
  if (fd < 0)
    {
      int e = errno;
      g_set_error (error, GSK_G_ERROR_DOMAIN,
                   gsk_error_code_from_errno (e),
                   "error opening %s: %s", filename, g_strerror (e));
      return FALSE;
    }
  while ((nread=read (fd, buf, sizeof (buf))) > 0
     || (nread < 0 && errno == EINTR))
    {
      if (nread < 0)
        continue;
      if (!gsk_xml_parser_feed (parser, buf, nread, error))
        {
          close (fd);
          return FALSE;
        }
    }
  if (nread < 0)
    {
      int e = errno;
      g_set_error (error, GSK_G_ERROR_DOMAIN,
                   gsk_error_code_from_errno (e),
                   "error reading %s: %s", filename, g_strerror (e));
      close (fd);
      return FALSE;
    }
  close (fd);
  return TRUE;
}

void
gsk_xml_parser_free       (GskXmlParser        *parser)
{
  g_markup_parse_context_free (parser->parse_context);
  if (parser->destroy)
    parser->destroy (parser->data);
  gsk_xml_builder_free (parser->builder);
  g_free (parser);
}
