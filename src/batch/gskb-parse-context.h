
typedef struct _GskbParseContext GskbParseContext;
typedef struct _GskbParseToken GskbParseToken;

struct _GskbParseContext
{
  GskbContext *context;
  GskbNamespace *cur_namespace;
  GPtrArray *errors;
};

struct _GskbParseToken
{
  const char *filename;
  guint line_no;
  guint type;
  char *str;
  guint i;
};
