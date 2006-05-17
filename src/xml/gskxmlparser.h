#ifndef GSK_XML_NODE_H_IN
#error "include this file by including gskxml.h"
#endif

G_BEGIN_DECLS

typedef void (*GskXmlParserCallback) (GskXmlNode   *document,
                                      gpointer      data);

typedef enum
{
  /* by default, "xmlns" and "xmlns:*" attribytes
     are interpreted, and affect the 'ns'
     and 'ns_by_abbrev' members.  also 'x:y' element names
     are replaced with 'y' and the appropriate namespace
     for 'x', if there is one.  if no namespace with
     abbreviation 'x' exists, the 'x:y' is retained
     as the element name. */
  GSK_XML_PARSE_WITHOUT_NAMESPACE_SUPPORT = (1<<0)

} GskXmlParseFlags;


typedef struct _GskXmlParser GskXmlParser;

GskXmlParser *gsk_xml_parser_new        (GskXmlParseFlags     flags,
                                         GskXmlParserCallback callback,
                                         gpointer             data,
                                         GDestroyNotify       destroy);
gboolean      gsk_xml_parser_feed       (GskXmlParser        *parser,
                                         const char          *data,
                                         gssize               len,
                                         GError             **error);
gboolean      gsk_xml_parser_feed_file  (GskXmlParser        *parser,
                                         const char          *filename,
                                         GError             **error);
gboolean      gsk_xml_parser_finish     (GskXmlParser        *parser,
                                         GError             **error);
void          gsk_xml_parser_free       (GskXmlParser        *parser);

G_END_DECLS
