
#ifndef GSK_XML_NODE_H_IN
#error "include this file by including gskxml.h"
#endif

G_BEGIN_DECLS

typedef struct _GskXmlNodeBase GskXmlNodeBase;
typedef struct _GskXmlNodeText GskXmlNodeText;
typedef struct _GskXmlNodeElement GskXmlNodeElement;
typedef union _GskXmlNode GskXmlNode;
typedef struct _GskXmlNamespace GskXmlNamespace;

struct _GskXmlNamespace
{
  guint ref_count;
  GskXmlString *abbrev;
  GskXmlString *uri;
};

GskXmlNamespace *gsk_xml_namespace_new   (GskXmlString *abbrev, /* may be NULL */
                                          GskXmlString *uri);
GskXmlNamespace *gsk_xml_namespace_ref   (GskXmlNamespace *ns);
void             gsk_xml_namespace_unref (GskXmlNamespace *ns);


/* XXX: are these values better as GSK_XML_{ELEMENT,TEXT}??? */
typedef enum
{
  GSK_XML_NODE_TYPE_ELEMENT,
  GSK_XML_NODE_TYPE_TEXT
} GskXmlNodeType;


/* members exhibited by all xmlnodes */
struct _GskXmlNodeBase
{
  guint8 type;
  guint16 magic;
  guint ref_count;
};

/* raw text content node */
struct _GskXmlNodeText
{
  GskXmlNodeBase base;
  GskXmlString *content;
};

typedef struct _GskXmlAttribute GskXmlAttribute;
struct _GskXmlAttribute
{
  GskXmlNamespace *ns;              /* may be NULL */
  GskXmlString *name;
  GskXmlString *value;
};
typedef struct _GskXmlRawAttribute GskXmlRawAttribute;
struct _GskXmlRawAttribute
{
  GskXmlString *ns_abbrev;              /* may be NULL */
  GskXmlString *name;
  GskXmlString *value;
};

/* xml element node */
struct _GskXmlNodeElement
{
  GskXmlNodeBase base;
  GskXmlNamespace *ns;              /* may be NULL */
  GskXmlString *name;

  guint n_children;
  GskXmlNode **children;
  guint n_element_children;
  GskXmlNode **element_children;

  /* attributes are sorted by namespace-uri/name-pointer */
  guint n_attributes;
  GskXmlAttribute *attributes;
};

/* casting macros */
#define GSK_XML_NODE_TEXT(node)      _GSK_XML_NODE_CAST(node, GskXmlNodeText, GSK_XML_NODE_TYPE_TEXT)
#define GSK_XML_NODE_ELEMENT(node)   _GSK_XML_NODE_CAST(node, GskXmlNodeElement, GSK_XML_NODE_TYPE_ELEMENT)
#define GSK_XML_NODE(node)           _GSK_XML_NODE_CAST_ANY(node)

/* a node in the xml tree.
   immutable.
   does not have parent pointer
   (may have multiple or no parents) */
union _GskXmlNode
{
  guint8 type;
  GskXmlNodeBase base;
  GskXmlNodeElement v_element;
  GskXmlNodeText v_text;
};

GskXmlNode *gsk_xml_node_new_text    (GskXmlString *text);
GskXmlNode *gsk_xml_node_new_text_c  (const char   *text);

/* create an element, completely unaware of namespaces */
GskXmlNode *gsk_xml_node_new_element (GskXmlNamespace *ns,
                                      GskXmlString *name,
                                      guint         n_attrs,
                                      GskXmlAttribute *attrs,
                                      guint         n_children,
                                      GskXmlNode  **children);


GskXmlNode *gsk_xml_node_ref         (GskXmlNode   *node);
void        gsk_xml_node_unref       (GskXmlNode   *node);
GskXmlNode *gsk_xml_node_find_child  (GskXmlNode   *node,
                                      GskXmlString *ns_uri,
                                      GskXmlString *child_name,
                                      guint         instance);
GskXmlString*gsk_xml_node_find_property(GskXmlNode   *node,
                                        GskXmlString *ns_uri, /* may be NULL */
                                        GskXmlString *child_name,
                                        guint         instance);

GskXmlString *gsk_xml_node_get_content(GskXmlNode *node);

/* random */
GskXmlNode *gsk_xml_node_new_from_element_with_new_children
                                     (GskXmlNode   *node,
                                      guint         n_children,
                                      GskXmlNode  **children);

/* casting macro implementations */
gpointer gsk_xml_node_cast_check (gpointer node, GskXmlNodeType node_type);
gpointer gsk_xml_node_cast_check_any_type (gpointer node);

#ifdef G_DISABLE_CAST_CHECKS
/* debugging disabled */
#define _GSK_XML_NODE_CAST_ANY(node)                 ((GskXmlNode*)(node))
#define _GSK_XML_NODE_CAST(node, c_type, enum_value) ((c_type *)(node))
#else
/* debugging enabled */
#define _GSK_XML_NODE_CAST_ANY(node)                 ((GskXmlNode*)gsk_xml_node_cast_check_any_type(node))
#define _GSK_XML_NODE_CAST(node, c_type, enum_value) ((c_type *) gsk_xml_node_cast_check(node, enum_value))
#endif


G_END_DECLS
