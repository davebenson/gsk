#ifndef __GSK_XML_NODE_H_
#define __GSK_XML_NODE_H_

#include "../gskerror.h"

#define GSK_XML_NODE_H_IN
#include "gskxmlstring.h"
#include "gskxmlnode.h"
#include "gskxmlparser.h"
#include "gskxmlbuilder.h"
#include "gskxmlcontext.h"
#undef GSK_XML_NODE_H_IN

G_BEGIN_DECLS

GskXmlNode *gsk_xml_parse_file      (const char       *filename,
                                     GskXmlParseFlags  flags,
                                     GError          **error);
GskXmlNode *gsk_xml_parse_data      (char             *buf,
                                     gssize            buf_len,
                                     GskXmlParseFlags  flags,
                                     GError          **error);
gboolean    gsk_xml_write_file      (GskXmlNode       *doc,
                                     const char       *filename,
                                     gboolean          format,
                                     GError          **error);
char *      gsk_xml_to_string       (GskXmlNode       *doc,
                                     gboolean          format);
GskXmlNode *gsk_xml_trim_whitespace (GskXmlNode       *node);

G_END_DECLS

#endif
