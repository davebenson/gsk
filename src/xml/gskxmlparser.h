#ifndef __GSK_XML_PARSER_H_
#define __GSK_XML_PARSER_H_

typedef struct _GskXmlParserConfig GskXmlParserConfig;
typedef struct _GskXmlParser GskXmlParser;

typedef enum
{
  GSK_XML_PARSER_IGNORE_NS_TAGS = (1<<0),
  GSK_XML_PARSER_PASSTHROUGH_UNKNOWN_NS = (1<<1),
} GskXmlParserFlags;

#include "gskxml.h"

GskXmlParserConfig *gsk_xml_parser_config_new          (void);
GskXmlParserConfig *gsk_xml_parser_config_new_by_depth (guint depth);
GskXmlParserConfig *gsk_xml_parser_config_ref      (GskXmlParserConfig *);
void                gsk_xml_parser_config_unref    (GskXmlParserConfig *);
guint               gsk_xml_parser_config_add_path (GskXmlParserConfig *,
                                                    const char         *path);
void                gsk_xml_parser_config_add_ns   (GskXmlParserConfig *,
                                                    const char         *abbrev,
						    const char         *url);
void                gsk_xml_parser_config_set_flags(GskXmlParserConfig *config,
                                                    GskXmlParserFlags   flags);
GskXmlParserFlags   gsk_xml_parser_config_get_flags(GskXmlParserConfig *config);


GskXmlParser *gsk_xml_parser_new_take     (GskXmlParserConfig *config);
GskXmlParser *gsk_xml_parser_new          (GskXmlParserConfig *config);
GskXmlParser *gsk_xml_parser_new_by_depth (guint               depth);
GskXml       *gsk_xml_parser_dequeue      (GskXmlParser       *parser,
                                           guint               index);
gboolean      gsk_xml_parser_feed         (GskXmlParser       *parser,
                                           const guint8       *xml_data,
					   gsize               len,
					   GError            **error);
gboolean      gsk_xml_parser_feed_file    (GskXmlParser       *parser,
                                           const char         *filename,
					   GError            **error);
void          gsk_xml_parser_free         (GskXmlParser       *parser);


#endif
