#include <stdio.h>
#include <string.h>
#include "gskxml.h"

typedef struct _ParseData ParseData;
struct _ParseData
{
  GskXmlNode *doc;
  gboolean multiple_documents;
};

#define PARSE_DATA_INIT { NULL, FALSE }

static GskXmlNode *
finish_parse_impl (ParseData    *parse_data,
                   GskXmlParser *parser,
                   GError      **error)
{
  gsk_xml_parser_free (parser);
  if (parse_data->multiple_documents)
    {
      if (parse_data->doc)
        gsk_xml_node_unref (parse_data->doc);
      g_set_error (error, GSK_G_ERROR_DOMAIN,
                   GSK_ERROR_MULTIPLE_ROOTS,
                   "multiple documents in xml file");
      return NULL;
    }
  if (parse_data->doc == NULL)
    {
      g_set_error (error, GSK_G_ERROR_DOMAIN,
                   GSK_ERROR_NO_DOCUMENT,
                   "no documents in xml file");
      return NULL;
    }
  return parse_data->doc;
}

static void
handle_root_node (GskXmlNode *node,
                  gpointer    data)
{
  ParseData *parse_data = data;
  if (parse_data->doc == NULL)
    parse_data->doc = gsk_xml_node_ref (node);
  else
    parse_data->multiple_documents = TRUE;
}

GskXmlNode *gsk_xml_parse_file  (const char   *filename,
                                 GskXmlParseFlags flags,
                                 GError      **error)
{
  ParseData parse_data = PARSE_DATA_INIT;
  GskXmlParser *parser = gsk_xml_parser_new (flags,
                                             handle_root_node,
                                             &parse_data,
                                             NULL);
  if (!gsk_xml_parser_feed_file (parser, filename, error)
   || !gsk_xml_parser_finish (parser, error))
    {
      if (parse_data.doc)
        gsk_xml_node_unref (parse_data.doc);
      gsk_xml_parser_free (parser);
      return NULL;
    }
  return finish_parse_impl (&parse_data, parser, error);
}

GskXmlNode *gsk_xml_parse_data  (char         *buf,
                                 gssize        buf_len,
                                 GskXmlParseFlags flags,
                                 GError      **error)
{
  ParseData parse_data = PARSE_DATA_INIT;
  GskXmlParser *parser = gsk_xml_parser_new (flags,
                                             handle_root_node,
                                             &parse_data,
                                             NULL);
  if (!gsk_xml_parser_feed (parser, buf, buf_len, error)
   || !gsk_xml_parser_finish (parser, error))
    {
      if (parse_data.doc)
        gsk_xml_node_unref (parse_data.doc);
      gsk_xml_parser_free (parser);
      return NULL;
    }
  return finish_parse_impl (&parse_data, parser, error);
}

typedef struct _Sink Sink;
struct _Sink
{
  void (*printf) (gpointer data, const char *format, va_list args);
  gpointer data;
};
static void
sink_printf (Sink *sink,
             const char *format,
             ...) G_GNUC_PRINTF(2,3);
static void
sink_printf (Sink *sink,
             const char *format,
             ...)
{
  va_list args;
  va_start (args, format);
  sink->printf (sink->data, format, args);
  va_end (args);
}

static void
sink_print_spaces (Sink *sink, guint n_spaces)
{
  sink_printf (sink, "%.*s", n_spaces, "");
}

static void
write_sink_raw       (GskXmlNode *doc,
                      Sink       *sink)
{
  if (doc->type == GSK_XML_NODE_TYPE_TEXT)
    {
      char *fmt;
      GskXmlNodeText *text = GSK_XML_NODE_TEXT (doc);
      fmt = g_markup_escape_text ((const char*)(text->content), -1);
      sink_printf (sink, "%s", fmt);
      g_free (fmt);
    }
  else
    {
      guint i;
      GskXmlNodeElement *element = GSK_XML_NODE_ELEMENT (doc);
      sink_printf (sink, "<%s", (char*)(element->name));
      for (i = 0; i < element->n_attributes; i++)
        {
          GskXmlAttribute *attr = element->attributes + i;
          if (attr->ns)
            sink_printf (sink, " %s=\"%s\"", (char*)(attr->name), (char*)(attr->value));
          else
            sink_printf (sink, " %s:%s=\"%s\"", (char*)(attr->ns->abbrev), (char*)(attr->name), (char*)(attr->value));
        }
      if (element->n_children == 0)
        sink_printf (sink, " />");
      else
        {
          sink_printf (sink, ">");
          for (i = 0; i < element->n_children; i++)
            write_sink_raw (element->children[i], sink);
          sink_printf (sink, "</%s>", (char*)(element->name));
        }
    }
}

static guint
get_length_with_max (GskXmlNode *node,
                     guint       max_len)
{
  switch (node->type)
    {
    case GSK_XML_NODE_TYPE_TEXT:
      /* HACK */
      return strlen (GSK_XML_STR (GSK_XML_NODE_TEXT (node)->content)) * 3 / 2;
    case GSK_XML_NODE_TYPE_ELEMENT:
      {
        GskXmlNodeElement *element = GSK_XML_NODE_ELEMENT (node);
        guint elt_name_len = strlen (GSK_XML_STR (element->name));
        guint rv = 2 + elt_name_len;
        guint i;
        if (element->n_children)
          rv += 4 + elt_name_len;
        for (i = 0; rv < max_len && i < element->n_children; i++)
          rv += get_length_with_max (element->children[i], max_len - rv);
        return rv;
      }
    }
  g_return_val_if_reached (0);
}
static gboolean
fits_in (GskXmlNode *node, guint max_len)
{
  return get_length_with_max (node, max_len) < max_len;
}

static void
write_sink_formatted (GskXmlNode *doc,
                      Sink       *sink,
                      guint       indent,
                      guint       line_length)
{
  if (doc->type == GSK_XML_NODE_TYPE_TEXT
   || indent >= line_length
   || fits_in (doc, line_length - indent))
    {
      sink_print_spaces (sink, indent);
      write_sink_raw (doc, sink);
      sink_printf (sink, "\n");
    }
  else /* we have a structured tag that doesn't fit on one line */
    {
      /* emit open tag on one line */
      guint subindent;
      guint i;
      GskXmlNodeElement *element = GSK_XML_NODE_ELEMENT (doc);
      sink_print_spaces (sink, indent);
      sink_printf (sink, "<%s>\n", (const char *) element->name);

      /* emit contents indented */
      subindent = indent + 2;
      if (subindent > line_length * 2 / 3)
        subindent = 0;
      for (i = 0; i < element->n_children; i++)
        {
          /* TODO: should trim whitespace? */

          write_sink_formatted (doc, sink, subindent, line_length);
        }

      /* emit close tag on one line */
      sink_print_spaces (sink, indent);
      sink_printf (sink, "</%s>\n", (const char *) element->name);
    }
}

static inline void
xml_write_sink (GskXmlNode   *doc,
                Sink         *sink,
                gboolean      format)
{
  if (format)
    write_sink_formatted (doc, sink, 0, 80);
  else
    {
      write_sink_raw (doc, sink);
      sink_printf (sink, "\n");
    }
}

static void
my_vfprintf (void *data, const char *format, va_list args)
{
  vfprintf (data, format, args);
}

gboolean    gsk_xml_write_file  (GskXmlNode   *doc,
                                 const char   *filename,
                                 gboolean      format,
                                 GError      **error)
{
  FILE *fp = fopen (filename, "w");
  Sink sink;
  int err;
  sink.printf = my_vfprintf;
  sink.data = fp;
  xml_write_sink (doc, &sink, format);
  err = ferror (fp);
  fclose (fp);
  return err == 0;
}

static void
my_string_append_vprintf (gpointer data,
                          const char *format,
                          va_list args)
{
  guint bound = g_printf_string_upper_bound (format, args);
  char *to_free = NULL;
  char *buf;
  GString *rv = data;
  if (bound < 1024)
    buf = g_alloca (bound + 1);
  else
    buf = to_free = g_malloc (bound + 1);
  g_vsnprintf (buf, bound, format, args);
  g_string_append (rv, buf);
}

char *
gsk_xml_to_string   (GskXmlNode   *doc,
                     gboolean      format)
{
  GString *rv = g_string_new ("");
  Sink sink;
  sink.printf = my_string_append_vprintf;
  sink.data = rv;
  xml_write_sink (doc, &sink, format);
  return g_string_free (rv, FALSE);
}

static GskXmlString *
concat_text_nodes_to_string (guint          n_text_nodes,
                             GskXmlNode   **text_nodes)
{
  guint i;
  if (n_text_nodes > 64)
    {
      GskXmlString **pieces = g_new (GskXmlString *, n_text_nodes);
      GskXmlString *rv;
      for (i = 0; i < n_text_nodes; i++)
        pieces[i] = GSK_XML_NODE_TEXT (text_nodes[i])->content;
      rv = gsk_xml_strings_concat (n_text_nodes, pieces);
      g_free (pieces);
      return rv;
    }
  else
    {
      GskXmlString **pieces = g_newa (GskXmlString *, n_text_nodes);
      for (i = 0; i < n_text_nodes; i++)
        pieces[i] = GSK_XML_NODE_TEXT (text_nodes[i])->content;
      return gsk_xml_strings_concat (n_text_nodes, pieces);
    }
}

GskXmlNode *
gsk_xml_trim_whitespace (GskXmlNode *node)
{
  switch (node->type)
    {
    case GSK_XML_NODE_TYPE_TEXT:
      {
        const char *str = GSK_XML_STR (GSK_XML_NODE_TEXT (node)->content);
        gboolean has_leading_whitespace, has_trailing_whitespace;
        gunichar u;
        const char *end;
        if (*str == 0)
          return gsk_xml_node_ref (node);
        u = g_utf8_get_char (str);
        has_leading_whitespace = g_unichar_isspace (u);
        end = strchr (str, 0);
        end = g_utf8_find_prev_char (str, end);
        u = g_utf8_get_char (end);
        has_trailing_whitespace = g_unichar_isspace (u);

        if (has_leading_whitespace || has_trailing_whitespace)
          {
            GskXmlString *trimmed;
            const char *trimmed_start = str;
            const char *trimmed_end = end;
            if (has_leading_whitespace)
              {
                trimmed_start += g_utf8_skip[*(guchar*)trimmed_start];
                do
                  {
                    u = g_utf8_get_char (trimmed_start);
                    if (!g_unichar_isspace (u))
                      break;
                    trimmed_start += g_utf8_skip[*(guchar*)trimmed_start];
                  }
                while (trimmed_start < trimmed_end);
              }
            if (has_trailing_whitespace && trimmed_start < trimmed_end)
              {
                trimmed_end = g_utf8_find_prev_char (trimmed_start,
                                                     trimmed_end);
                while (trimmed_start < trimmed_end)
                  {
                    const char *prev;
                    prev = g_utf8_find_prev_char (trimmed_start, trimmed_end);
                    if (!g_unichar_isspace (g_utf8_get_char (prev)))
                      break;
                    trimmed_end = prev;
                  }
              }
            trimmed = gsk_xml_string_new_len (trimmed_start,
                                              trimmed_end - trimmed_start);
            node = gsk_xml_node_new_text (trimmed);
            gsk_xml_string_unref (trimmed);
            return node;

          }
        else
          return gsk_xml_node_ref (node);
      }

    case GSK_XML_NODE_TYPE_ELEMENT:
      {
        GskXmlNode **new_subnodes;
        GskXmlNode **to_free;
        GskXmlNodeElement *element = GSK_XML_NODE_ELEMENT (node);
        guint n_children = element->n_children;
        GskXmlNode **old_subnodes = element->children;
        GskXmlNode *rv;
        guint i, o;
        gboolean did_something = FALSE;
        if (element->n_children > 16)
          {
            new_subnodes = g_new (GskXmlNode *, n_children);
            to_free = new_subnodes;
          }
        else
          {
            new_subnodes = g_newa (GskXmlNode *, n_children);
            to_free = NULL;
          }

        for (i = 0; i < n_children; i++)
          {
            new_subnodes[i] = gsk_xml_trim_whitespace (old_subnodes[i]);
            did_something = did_something
                          || (new_subnodes[i] != old_subnodes[i]);
          }
        o = 0;
        for (i = 0; i < n_children; )
          if (new_subnodes[i]->type == GSK_XML_NODE_TYPE_ELEMENT)
            {
              new_subnodes[o++] = new_subnodes[i++];
            }
          else
            {
              guint past_text;
              for (past_text = i + 1; i < n_children; past_text++)
                if (new_subnodes[i]->type != GSK_XML_NODE_TYPE_TEXT)
                  break;
              if (past_text == i + 1)
                {
                  if (GSK_XML_STR (GSK_XML_NODE_TEXT (new_subnodes[i])->content)[0] == '\0')
                    {
                      gsk_xml_node_unref (new_subnodes[i++]);
                      did_something = TRUE;
                    }
                  else
                    new_subnodes[o++] = new_subnodes[i++];
                }
              else
                {
                  /* concatenate text nodes */
                  GskXmlString *concat;
                  concat = concat_text_nodes_to_string (past_text - i, new_subnodes + i);

                  /* free old nodes */
                  while (i < past_text)
                    gsk_xml_node_unref (new_subnodes[i++]);

                  /* only store it if it is nonempty */
                  if (GSK_XML_STR (concat)[0] != '\0')
                    new_subnodes[o++] = gsk_xml_node_new_text (concat);
                  gsk_xml_string_unref (concat);
                  did_something = TRUE;
                }
            }
        if (did_something)
          /* create new xmlnode */
          rv = gsk_xml_node_new_from_element_with_new_children (node, o, new_subnodes);
        else
          rv = gsk_xml_node_ref (node);
        for (i = 0; i < o; i++)
          gsk_xml_node_unref (new_subnodes[i]);
        if (to_free)
          g_free (to_free);
        return rv;
      }

    default:
      g_assert_not_reached ();
    }
}
