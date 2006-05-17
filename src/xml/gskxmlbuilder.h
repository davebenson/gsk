/* GskXmlBuilder: incremental xml structure building,
   useful for implementing parser-callbacks */

#ifndef GSK_XML_NODE_H_IN
#error "include this file by including gskxml.h"
#endif

G_BEGIN_DECLS

typedef struct _GskXmlBuilder GskXmlBuilder;
GskXmlBuilder *gsk_xml_builder_new      (GskXmlParseFlags flags);
void           gsk_xml_builder_free     (GskXmlBuilder *builder);

void           gsk_xml_builder_start    (GskXmlBuilder *builder,
                                         GskXmlString  *name,
                                         guint          n_attrs,
                                         GskXmlString**attrs);
void           gsk_xml_builder_start_ns (GskXmlBuilder *builder,
                                         GskXmlString  *name_ns,
                                         GskXmlString  *name,
                                         guint          n_attrs,
                                         GskXmlRawAttribute *attrs);
void           gsk_xml_builder_text     (GskXmlBuilder *builder,
                                         GskXmlString  *content);
void           gsk_xml_builder_add_node (GskXmlBuilder *builder,
                                         GskXmlNode    *node);
void           gsk_xml_builder_start_c  (GskXmlBuilder *builder,
                                         const char    *name,
                                         guint          n_attrs,
                                         char        **attrs);
void           gsk_xml_builder_text_c   (GskXmlBuilder *builder,
                                         const char    *content);

/* NOTE: name is optional
   NOTE: return-value is not a new reference, but a peeked version
   of an internal structure.  you should 'ref' it to hold onto it.
 */
GskXmlNode    *gsk_xml_builder_end   (GskXmlBuilder *builder,
                                      GskXmlString  *name);


/* will return NULL until there is a document to return;
   you must unref a non-NULL return value (ie you take ownership) */
GskXmlNode    *gsk_xml_builder_get_doc(GskXmlBuilder *builder);

G_END_DECLS
