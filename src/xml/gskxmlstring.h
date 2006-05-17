#ifndef GSK_XML_NODE_H_IN
#error "include this file by including gskxml.h"
#endif

G_BEGIN_DECLS

typedef struct _GskXmlString GskXmlString;

void gsk_xml_string_init (void);

GskXmlString *gsk_xml_string_new        (const char         *str);
GskXmlString *gsk_xml_string_new_len    (const char         *str,
                                         guint               len);
GskXmlString *gsk_xml_string_ref        (const GskXmlString *str);
void          gsk_xml_string_unref      (GskXmlString       *str);

GskXmlString *gsk_xml_strings_concat    (guint               n_strs,
                                         GskXmlString      **strs);

static inline const char *
GSK_XML_STR (const GskXmlString *str)
{
  return (const char *) str;
}

/* NOTES:
 *  - two GskXmlStrings are equal iff they are pointerwise equal
 *  - you may cast a GskXmlStrings to a 'const char *' for
 *    the purposes of printing, etc.
 */

/* strings initialized by gsk_xml_string_init() */
extern GskXmlString *gsk_xml_string__xmlns;
extern GskXmlString *gsk_xml_string__;          /* the blank string */

G_END_DECLS
