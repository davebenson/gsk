#include "gskb-format.h"
#include "gskb-codegen-config.h"
#include "gskb-format-codegen.h"

GskbCodegenConfig *
gskb_codegen_config_new            (const char      *type_prefix,
                                           const char      *func_prefix)
{
  GskbCodegenConfig *config = g_slice_new0 (GskbCodegenConfig);
  config->type_prefix = g_strdup (type_prefix);
  config->func_prefix = g_strdup (func_prefix);
  return config;
}
void
gskb_codegen_config_set_all_static (GskbCodegenConfig *config,
                                           gboolean         all_static)
{
  config->all_static = all_static;
}
void
gskb_codegen_config_free (GskbCodegenConfig *config)
{
  g_free (config->type_prefix);
  g_free (config->func_prefix);
  g_slice_free (GskbCodegenConfig, config);
}

static gboolean
gskb_format_codegen__emit_typedefs     (GskbFormat *format,
                                        GskbCodegenSection section,
                                        const GskbCodegenConfig *config,
                                        GskBuffer *output,
                                        GError **error)
{
  g_assert (section == GSKB_CODEGEN_SECTION_TYPEDEFS);
  switch (format->type)
    {
    case GSKB_FORMAT_TYPE_INT:
      {
        const char *src_type;
        static struct {
          GskbFormatIntType int_type;
          const char *c_type;
        } int_c_types[] = {             /* NOTE: must match order of enum */
          { GSKB_FORMAT_INT_INT8,           "gint8" },
          { GSKB_FORMAT_INT_INT16,          "gint16" },
          { GSKB_FORMAT_INT_INT32,          "gint32" },
          { GSKB_FORMAT_INT_INT64,          "gint64" },
          { GSKB_FORMAT_INT_UINT8,          "guint8" },
          { GSKB_FORMAT_INT_UINT16,         "guint16" },
          { GSKB_FORMAT_INT_UINT32,         "guint32" },
          { GSKB_FORMAT_INT_UINT64,         "guint64" },
          { GSKB_FORMAT_INT_INT,            "gint32" },
          { GSKB_FORMAT_INT_UINT,           "guint32" },
          { GSKB_FORMAT_INT_LONG,           "gint64" },
          { GSKB_FORMAT_INT_ULONG,          "guint64" },
          { GSKB_FORMAT_INT_BIT,            "gboolean" }
        };
        
        /* validate that the table is ok.  (sigh) */
        g_assert (format->v_int.int_type < G_N_ELEMENTS (int_c_types));
        g_assert (int_c_types[format->v_int.int_type].int_type
                  == format->v_int.int_type);

        gsk_buffer_printf (output,
                           "typedef %s %s%s;\n",
                           int_c_types[format->v_int.int_type].c_type,
                           config->type_prefix, format->any.TypeName);
      }
      break;
    case GSKB_FORMAT_TYPE_FLOAT:
      {
        const char *src_type;
        static struct {
          GskbFormatFloatType float_type;
          const char *c_type;
        } float_c_types[] = {             /* NOTE: must match order of enum */
          { GSKB_FORMAT_FLOAT_SINGLE,       "gfloat" },
          { GSKB_FORMAT_FLOAT_DOUBLE,       "gdouble" },
        };
        
        /* validate that the table is ok.  (sigh) */
        g_assert (format->v_float.float_type < G_N_ELEMENTS (float_c_types));
        g_assert (float_c_types[format->v_float.float_type].float_type
                  == format->v_float.float_type);

        gsk_buffer_printf (output,
                           "typedef %s %s%s;\n",
                           float_c_types[format->v_float.float_type].c_type,
                           config->type_prefix, format->any.TypeName);
      }
      break;
    case GSKB_FORMAT_TYPE_ENUM:
      /* nothing to do */
      break;
    case GSKB_FORMAT_TYPE_ALIAS:
      gsk_buffer_printf (output,
                         "typedef %s%s %s%s;\n",
                         config->type_prefix, format->v_alias.format->any.TypeName,
                         config->type_prefix, format->any.TypeName);

    default:
      gsk_buffer_printf (output,
                         "typedef struct _%s%s %s%s;\n",
                         config->type_prefix, format->any.TypeName,
                         config->type_prefix, format->any.TypeName);
      break;
    }
  return TRUE;
}

static gboolean
gskb_format_codegen__emit_structures   (GskbFormat *format,
                                        GskbCodegenSection phase,
                                        const GskbCodegenConfig *config,
                                        GskBuffer *output,
                                        GError **error)
{
  guint i;
  switch (format->type)
    {
    case GSKB_FORMAT_TYPE_ALIAS:
    case GSKB_FORMAT_TYPE_INT:
    case GSKB_FORMAT_TYPE_FLOAT:
    case GSKB_FORMAT_TYPE_STRING:
      break;
    case GSKB_FORMAT_TYPE_ENUM:
      {
        char *uc_name;
        guint expected = 0;
        uc_name = g_ascii_strup (format->any.lc_name, -1);
        gsk_buffer_append_str (output, "typedef enum {\n");
        for (i = 0; i < format->v_enum.n_values; i++)
          {
            char *enum_uc_name = g_ascii_strup (format->v_enum.values[i].name, -1);
            if (i > 0)
              gsk_buffer_append_str (output, ",\n");
            if (format->v_enum.values[i].value == expected)
              gsk_buffer_printf (output,
                                 "  %s__%s", uc_name, enum_uc_name);
            else
              gsk_buffer_printf (output,
                                 "  %s__%s = %u", uc_name, enum_uc_name,
                                 format->v_enum.values[i].value);
            expected = format->v_enum.values[i].value + 1;
            g_free (enum_uc_name);
          }
        gsk_buffer_append_str (output, "} %s%s;\n\n",
                               config->type_prefix,
                               format->any.TypeName);
        g_free (uc_name);
      }
      break;
    case GSKB_FORMAT_TYPE_FIXED_ARRAY:
      gsk_buffer_printf (output,
                         "struct _%s%s\n"
                         "{\n"
                         "  %s%s data[%u];\n"
                         "};\n\n",
                         config->type_prefix, format->any.TypeName,
                         config->type_prefix, format->v_fixed_array.element_format->any.TypeName,
                         format->v_fixed_array.length);
      break;
    case GSKB_FORMAT_TYPE_LENGTH_PREFIXED_ARRAY:
      gsk_buffer_printf (output,
                         "struct _%s%s\n"
                         "{\n"
                         "  guint length;\n"
                         "  %s%s *data;\n"
                         "};\n\n",
                         config->type_prefix, format->any.TypeName,
                         config->type_prefix, format->any.TypeName);
      break;
    case GSKB_FORMAT_TYPE_STRUCT:
      gsk_buffer_printf (output,
                         "struct _%s%s\n"
                         "{\n",
                         config->type_prefix, format->any.TypeName);
      for (i = 0; i < format->v_struct.n_members; i++)
        {
          GskbFormatStructMember *member = format->v_struct.members + i;
          gsk_buffer_printf (output,
                             "  %s%s %s;\n",
                             config->type_prefix, member->format->any.TypeName,
                             member->name);
        }
      gsk_buffer_printf (output, "};\n\n");
      break;
    case GSKB_FORMAT_TYPE_UNION:
      gsk_buffer_printf (output,
                         "struct _%s%s\n"
                         "{\n"
                         "  %s%s_Type type;\n"
                         "  struct {\n" ,
                         config->type_prefix, format->any.TypeName,
                         config->type_prefix, format->any.TypeName);
      for (i = 0; i < format->v_union.n_cases; i++)
        {
          GskbFormatUnionCase *c = format->v_union.cases + i;
          gsk_buffer_printf (output,
                             "    %s%s %s;\n",
                             config->type_prefix, c->format->any.TypeName,
                             c->name);
        }
      gsk_buffer_printf (output, "  } info;\n"
                                 "};\n\n");

      break;
    case GSKB_FORMAT_TYPE_EXTENSIBLE:
      gsk_buffer_printf (output,
                         "struct _%s%s\n"
                         "{\n"
                         "  GskbExtensible base_instance;\n",
                         config->type_prefix, format->any.TypeName);
      for (i = 0; i < format->v_extensible.n_members; i++)
        gsk_buffer_printf (output,
                           "  guint8 has_%s : 1;\n",
                           format->v_extensible.members[i].name);
      for (i = 0; i < format->v_extensible.n_members; i++)
        {
          GskbFormatExtensibleMember *m = format->v_extensible.members + i;
          gsk_buffer_printf (output,
                             "  %s%s %s;\n",
                             config->type_prefix, m->format->any.TypeName,
                             m->name);
        }
      break;
    default:
      g_assert_not_reached ();
    }
  return TRUE;
}

static void
implement_format_any (GskbFormat *format,
                      const GskbCodegenConfig *config,
                      GskBuffer *output)
{

  gsk_buffer_printf (output,
                         "{\n"
                         "  %s,         /* type */\n"
                         "  %s,         /* ctype */\n"
                         "  1,          /* ref_count */\n"
                         "  \"%s\", \"%s\", \"%s\",  /* various names */\n"
                         "  sizeof(%s%s), alignof(%s%s),\n"
                         "  %u,       /* always by pointer */\n"
                         "  %u,       /* requires_destruct */\n"
                         "  %u,       /* fixed_length */\n"
                         "  %s%s__n_c_members,\n"
                         "  %s%s__c_members\n"
                         "},\n",
                         gskb_format_type_enum_name (format->type),
                         gskb_format_ctype_enum_name (format->ctype),
                         format->any.name,
                         format->any.TypeName,
                         format->any.lc_name,
                         config->type_prefix, format->any.TypeName,
                         config->type_prefix, format->any.TypeName,
                         format->any.always_by_pointer,
                         format->any.requires_destruct,
                         format->any.fixed_length,
                         config->func_prefix, format->any.lc_name,
                         config->func_prefix, format->any.lc_name);
}

static void
define_static_format_from_instance  (GskbFormat *format,
                                     const GskbCodegenConfig *config,
                                     GskBuffer *output)
{
  gsk_buffer_printf (output,
                 "#define %s%s__static_format  ((GskbFormat *)(&%s%s__static_format_instance))\n",
                 config->func_prefix, format->any.lc_name,
                 config->func_prefix, format->any.lc_name);
}
static gboolean
gskb_format_codegen__emit_static_formats (GskbFormat *format,
                                          GskbCodegenSection phase,
                                          const GskbCodegenConfig *config,
                                          GskBuffer *output,
                                          GError **error)
{
  switch (format->type)
    {
    case GSKB_FORMAT_TYPE_INT:
    case GSKB_FORMAT_TYPE_FLOAT:
    case GSKB_FORMAT_TYPE_STRING:
      {
        gsk_buffer_printf (output,
                           "#define %s%s__n_c_members     0\n"
                           "#define %s%s__c_members       NULL\n",
                           config->func_prefix, format->any.name,
                           config->func_prefix, format->any.name);
        gsk_buffer_printf ("#define %s%s__static_format   gskb_%s_format\n",
                           config->func_prefix, format->any.name, format->any.name);
        return TRUE;
      }
    case GSKB_FORMAT_TYPE_FIXED_ARRAY:
      {
      gsk_buffer_printf (output,
                         "#define %s%s__n_c_members  %u\n"
                         "static GskbFormatCMember %s%s__c_members[%u] = {\n",
                         config->func_prefix, format->any.lc_name, format->v_fixed_array.length,
                         config->func_prefix, format->any.lc_name, format->v_fixed_array.length);
      for (i = 0; i < format->v_fixed_array.length; i++)
        {
          gsk_buffer_printf (output,
                             "  { \"%u\", %s, %s%s__static_format, sizeof (%s%s) * %u },\n",
                             i, gskb_format_ctype_enum_name (format->v_fixed_array.element_format->any.ctype),
                             config->func_prefix, format->v_fixed_array.element_format->any.lc_name,
                             config->type_prefix, format->v_fixed_array.element_format->any.TypeName,
                             i);
        }
      gsk_buffer_printf (output, "};\n");

      gsk_buffer_printf (output,
                         "static GskbFormatFixedArray %s%s__static_format_instance =\n"
                         "{\n",
                         config->func_prefix, format->any.lc_name);
      implement_format_any (format, config, output);
      gsk_buffer_printf (output,
                         "  %u,\n"
                         "  %s%s__static_format\n"
                         "};\n",
                         format->v_fixed_array.length,
                         config->func_prefix, format->any.lc_name,
                         config->func_prefix, format->v_fixed_array.element_format->any.lc_name);
      define_static_format_from_instance (format, config, output);
      return TRUE;
      }

    case GSKB_FORMAT_TYPE_LENGTH_PREFIXED_ARRAY:
      {
        gsk_buffer_printf (output,
                           "#define %s%s__n_c_members  2\n"
                           "static GskbFormatCMember %s%s__c_members[2] = {\n"
                           "  { \"length\", GSKB_FORMAT_CTYPE_UINT32, gskb_format_uint, offsetof (%s%s, length) },\n"
                           "  { \"data\", GSKB_FORMAT_CTYPE_ARRAY_PTR, NULL, offsetof (%s%s, data) }\n"
                           "};\n",
                         config->func_prefix, format->any.lc_name,
                         config->func_prefix, format->any.lc_name,
                         config->type_prefix, format->any.TypeName,
                         config->type_prefix, format->any.TypeName);
      gsk_buffer_printf (output,
                         "static GskbFormatLengthPrefixedArray %s%s__static_format_instance =\n"
                         "{\n",
                         config->func_prefix, format->any.lc_name);
      implement_format_any (format, config, output);
      gsk_buffer_printf (output,
                         "  %s%s__static_format\n"
                         "};\n",
                         config->func_prefix, format->v_length_prefixed_array.element_format->any.lc_name);
      define_static_format_from_instance (format, config, output);
      return TRUE;

    case GSKB_FORMAT_TYPE_STRUCT:
      {
        gsk_buffer_printf (output,
                           "#define %s%s__n_c_members  %u\n"
                           "static GskbFormatCMember %s%s__c_members[%u] = {\n"
                           config->func_prefix, format->any.lc_name, format->any.v_struct.n_members,
                           config->func_prefix, format->any.lc_name, format->any.v_struct.n_members);
        for (i = 0; i < format->v_struct.n_members; i++)
          {
            ...
          }
        gsk_buffer_printf (output,
                           "};\n");

        gsk_buffer_printf (output,
                           "static GskbFormatStructMember %s%s__members[%u] = {\n");
        for (i = 0; i < format->v_struct.n_members; i++)
          {
            ...
          }
        gsk_buffer_printf (output,
                           "};\n");
        gsk_buffer_printf (output,
                           "static GskbFormatStruct %s%s__static_format_instance =\n"
                           "{\n",
                           config->func_prefix, format->any.lc_name);
        implement_format_any (format, config, output);
        gsk_buffer_printf (output,
                           "  %u, %s%s__members
                           "};\n",
                           config->func_prefix, format->v_length_prefixed_array.element_format->any.lc_name);
        define_static_format_from_instance (format, config, output);
        return TRUE;

      ...
    case GSKB_FORMAT_TYPE_UNION:
      ...
    case GSKB_FORMAT_TYPE_ENUM:
      ...
    case GSKB_FORMAT_TYPE_ALIAS:
      ...
    case GSKB_FORMAT_TYPE_EXTENSIBLE:
      ...
    default:
      g_assert_not_reached ();
    }
}



typedef gboolean (*OutputFunctionImplementor)   (GskbFormat *format,
                                                 const GskbCodegenConfig *config,
                                                 GskBuffer *output,
                                                 GError **error);


/* helper functions, used by implementor functions */
static void start_function (const char *qualifiers,
                            GskbFormat *format,
                            const GskbCodegenConfig *config,
                            GskbCodegenOutputFunction function,
                            GskBuffer *buffer);




/* pack */
#define return_value__pack       "void"
static const char *type_name_pairs__pack[] = {
  "$maybe_const $type_name $maybe_ptr", "value",
  "GskbAppendFunc", "append_func",
  "gpointer", "append_func_data",
  NULL
};
static gboolean 
implement__pack(GskbFormat *format,
                const GskbCodegenConfig *config,
                GskBuffer *output,
                GError **error)
{
  guint i;
  switch (format->type)
    {
    case GSKB_FORMAT_TYPE_FIXED_ARRAY:
      {
        GskbFormat *sub = format->v_fixed_array.element_format;
        if (format->v_fixed_array.length < 5)
          {
            for (i = 0; i < format->v_fixed_array.length; i++)
              gsk_buffer_printf (output,
             "  %s%s_pack (%svalue->data[%u], append_func, append_func_data);\n",
                                 config->func_prefix, sub->any.lc_name,
                                 sub->any.always_by_pointer ? "&" : "", i);
          }
        else
          {
            gsk_buffer_printf (output,
                               "  guint i;\n"
                               "  for (i = 0; i < %u; i++)\n"
                               "    {\n"
                               "      %s%s_pack (%svalue->data[i], append_func, append_func_data);\n"
                               "    }\n",
                               format->v_fixed_array.length,
                               config->func_prefix, sub->any.lc_name,
                               sub->any.always_by_pointer ? "&" : "");
          }
        break;
      }
    case GSKB_FORMAT_TYPE_LENGTH_PREFIXED_ARRAY:
      {
        GskbFormat *sub = format->v_length_prefixed_array.element_format;
        gsk_buffer_printf (output,
                           "  guint i;\n"
                           "  gskb_uint_pack (value->length, append_func, append_func_data);\n"
                           "  for (i = 0; i < value->length; i++)\n"
                           "    %s%s_pack (%svalue->data[i], append_func, append_func_data);\n",
                               config->func_prefix, sub->any.lc_name,
                               sub->any.always_by_pointer ? "&" : "");

        break;
      }
    case GSKB_FORMAT_TYPE_STRUCT:
      for (i = 0; i < format->v_struct.n_members; i++)
        {
          GskbFormatStructMember *member = format->v_struct.members + i;
          gsk_buffer_printf (output,
                             "  %s%s_pack (%svalue->%s, append_func, append_func_data);\n",
                             config->func_prefix, member->format->any.lc_name,
                             member->format->any.always_by_pointer,
                             member->name);
        }
      break;
    case GSKB_FORMAT_TYPE_UNION:
      {
        char *ucname = g_ascii_strup (format->any.lc_name, -1);
        gsk_buffer_printf (output,
                           "  gskb_uint_pack (value->type, append_func, append_func_data);\n"
                           "  switch (value->type)\n"
                           "    {\n");
        for (i = 0; i < format->v_union.n_cases; i++)
          {
            GskbFormatUnionCase *c = format->v_union.cases + i;
            char *uccasename = g_ascii_strup (c->name, -1);
            gsk_buffer_printf (output,
                               "    case %s__%s:\n"
                               "      %s%s_pack (%svalue->info.%s, append_func, append_func_data);\n"
                               "      break;\n",
                               ucname, uccasename,
                               config->func_prefix, c->format->any.lc_name,
                               c->name);
            g_free (uccasename);
          }
        g_free (ucname);

        gsk_buffer_printf (output,
                           "    default:\n"
                           "      g_assert_not_reached ();\n"
                           "    }\n");
        break;
      }
    case GSKB_FORMAT_TYPE_ENUM:
      gsk_buffer_printf (output,
                         "  gskb_uint_pack (value, append_func, append_func_data);\n");
      break;
    case GSKB_FORMAT_TYPE_EXTENSIBLE:
      gsk_buffer_printf (output,
                         "  gskb_format_pack (%s%s_peek_format (),\n"
                         "                    value,\n"
                         "                    append_func, append_func_data);\n",
                         config->func_prefix, format->any.lc_name);
      break;
    }
  return TRUE;
}

/* get_packed_size */
#define return_value__get_packed_size       "guint"
static const char *type_name_pairs__get_packed_size[] = {
  "$maybe_const $type_name $maybe_ptr", "value",
  NULL
};
static gboolean 
implement__get_packed_size(GskbFormat *format,
                           const GskbCodegenConfig *config,
                           GskBuffer *output,
                           GError **error)
{
  guint i;
  switch (format->type)
    {
    case GSKB_FORMAT_TYPE_FIXED_ARRAY:
      {
        GskbFormat *sub = format->v_fixed_array.element_format;
        if (format->v_fixed_array.length < 5)
          {
            gsk_buffer_printf (output, "  guint rv = 0;\n");
            for (i = 0; i < format->v_fixed_array.length; i++)
              gsk_buffer_printf (output,
                                 "  rv += %s%s_get_packed_size (%svalue->data[%u]);\n",
                                 config->func_prefix, sub->any.lc_name,
                                 sub->any.always_by_pointer ? "&" : "", i);
          }
        else
          {
            gsk_buffer_printf (output,
                               "  guint i, rv = 0;\n"
                               "  for (i = 0; i < %u; i++)\n"
                               "    rv += %s%s_get_packed_size (%svalue->data[i]);\n",
                                 format->v_fixed_array.length,
                                 config->func_prefix, sub->any.lc_name,
                                 sub->any.always_by_pointer ? "&" : "");
          }
        gsk_buffer_printf (output, "  return rv;\n");
        break;
      }
    case GSKB_FORMAT_TYPE_LENGTH_PREFIXED_ARRAY:
      {
        GskbFormat *sub = format->v_length_prefixed_array.element_format;
        gsk_buffer_printf (output,
                           "  guint rv = gskb_uint_get_packed_size (value->length);\n");
        if (sub->any.is_fixed_length)
          gsk_buffer_printf (output, "  rv += value->length * %u;\n", sub->any.fixed_length);
        else
          {
            gsk_buffer_printf (output,
                               "  guint i;\n"
                               "  for (i = 0; i < %u; i++)\n"
                               "    rv += %s%s_get_packed_size (%svalue->data[i]);\n",
                                 format->v_fixed_array.length,
                                 config->func_prefix, sub->any.lc_name,
                                 sub->any.always_by_pointer ? "&" : "");
          }
        gsk_buffer_printf (output, "  return rv;\n");
        break;
      }
    case GSKB_FORMAT_TYPE_STRUCT:
      {
        gsk_buffer_printf (output, "  guint rv = 0;\n");
        for (i = 0; i < format->v_struct.n_members; i++)
          {
            GskbFormatStructMember *member = format->v_struct.members + i;
            gsk_buffer_printf (output,
                               "  rv += %s%s_get_packed_size (%svalue->%s);\n",
                               config->func_prefix, member->format->any.lc_name,
                               member->format->any.always_by_pointer, member->name);
          }
        gsk_buffer_printf (output, "  return rv;\n");
        break;
      }
    case GSKB_FORMAT_TYPE_UNION:
      {
        char *ucname = g_ascii_strup (format->any.lc_name, -1);
        gsk_buffer_printf (output, "  switch (value->type)\n    {\n");
        for (i = 0; i < format->v_union.n_cases; i++)
          {
            GskbFormatUnionCase *c = format->v_union.cases + i;
            char *uccasename = g_ascii_strup (c->name, -1);
            gsk_buffer_printf (output, "    case %s__%s:\n",
                               ucname, uccasename);
            if (c->format == NULL)
              gsk_buffer_printf (output, "      return gskb_uint_get_packed_size (%s__%s);\n",
                                 ucname, uccasename);
            else
              gsk_buffer_printf (output, "      return gskb_uint_get_packed_size (%s__%s) + %s%s_get_packed_size (%svalue->info.%s);\n",
                                 ucname, uccasename, config->func_prefix, c->name);
            g_free (uccasename);
          }
        g_free (ucname);
        gsk_buffer_printf (output, "    default:\n"
                                   "      g_return_val_if_reached (0);\n"
                                   "    }\n");
        break;
      }
    case GSKB_FORMAT_TYPE_ENUM:
      /* note: never happens, as this is implemented with a macro instead */
      gsk_buffer_printf (output, "  return gskb_uint_get_packed_size (value);\n");
      break;
    case GSKB_FORMAT_TYPE_EXTENSIBLE:
      gsk_buffer_printf (output, "  return gskb_format_get_packed_size (%s%s_peek_format (), value);\n",
                         config->func_prefix, format->any.lc_name);
      break;
    default:
      g_return_val_if_reached (FALSE);
    }
  return TRUE;
}

/* pack_slab */
#define return_value__pack_slab       "guint"
static const char *type_name_pairs__pack_slab[] = {
  "$maybe_const $type_name $maybe_ptr", "value",
  "guint8 *", "out",
  NULL
};
static gboolean 
implement__pack_slab      (GskbFormat *format,
                           const GskbCodegenConfig *config,
                           GskBuffer *output,
                           GError **error)
{
  guint i;
  switch (format->type)
    {
    case GSKB_FORMAT_TYPE_FIXED_ARRAY:
      {
        GskbFormat *sub = format->v_fixed_array.element_format;
        if (format->v_fixed_array.length < 5)
          {
            gsk_buffer_printf (output, "  guint rv = 0;\n");
            for (i = 0; i < format->v_fixed_array.length; i++)
              gsk_buffer_printf (output,
                                 "  rv += %s%s_pack_slab (%svalue->data[%u], out + rv);\n",
                                 config->func_prefix, sub->any.lc_name,
                                 sub->any.always_by_pointer ? "&" : "", i);
          }
        else
          {
            gsk_buffer_printf (output,
                               "  guint i, rv = 0;\n"
                               "  for (i = 0; i < %u; i++)\n"
                               "    rv += %s%s_pack_slab (%svalue->data[i], out + rv);\n",
                                 format->v_fixed_array.length,
                                 config->func_prefix, sub->any.lc_name,
                                 sub->any.always_by_pointer ? "&" : "");
          }
        gsk_buffer_printf (output, "  return rv;\n");
        break;
      }
    case GSKB_FORMAT_TYPE_LENGTH_PREFIXED_ARRAY:
      {
        GskbFormat *sub = format->v_length_prefixed_array.element_format;
        gsk_buffer_printf (output,
                           "  guint rv = gskb_uint_pack_slab (value->length, out);\n");
      {
        gsk_buffer_printf (output,
                           "  guint i;\n"
                           "  for (i = 0; i < %u; i++)\n"
                           "    rv += %s%s_pack_slab (%svalue->data[i], out + rv);\n",
                             format->v_fixed_array.length,
                             config->func_prefix, sub->any.lc_name,
                             sub->any.always_by_pointer ? "&" : "");
      }
        gsk_buffer_printf (output, "  return rv;\n");
        break;
      }
    case GSKB_FORMAT_TYPE_STRUCT:
      {
        gsk_buffer_printf (output, "  guint rv = 0;\n");
        for (i = 0; i < format->v_struct.n_members; i++)
          {
            GskbFormatStructMember *member = format->v_struct.members + i;
            gsk_buffer_printf (output,
                               "  rv += %s%s_pack_slab (%svalue->%s, out + rv);\n",
                               config->func_prefix, member->format->any.lc_name,
                               member->format->any.always_by_pointer, member->name);
          }
        gsk_buffer_printf (output, "  return rv;\n");
        break;
      }
    case GSKB_FORMAT_TYPE_UNION:
      {
        char *ucname = g_ascii_strup (format->any.lc_name, -1);
        gsk_buffer_printf (output, "  switch (value->type)\n    {\n");
        for (i = 0; i < format->v_union.n_cases; i++)
          {
            GskbFormatUnionCase *c = format->v_union.cases + i;
            char *uccasename = g_ascii_strup (c->name, -1);
            gsk_buffer_printf (output, "    case %s__%s:\n",
                               ucname, uccasename);
            if (c->format == NULL)
              gsk_buffer_printf (output, "      return gskb_uint_pack_slab (%s__%s, out);\n",
                                 ucname, uccasename);
            else
              gsk_buffer_printf (output, "      {\n"
                                         "         guint size1 = gskb_uint_pack_slab (%s__%s, out);\n"
                                         "         return size1 + %s%s_pack_slab (%svalue->info.%s, out + size1);\n"
                                         "      }\n",
                                         ucname, uccasename, config->func_prefix, c->name);
            g_free (uccasename);
          }
        g_free (ucname);
        gsk_buffer_printf (output, "    default:\n"
                                   "      g_return_val_if_reached (0);\n"
                                   "    }\n");
        break;
      }
    case GSKB_FORMAT_TYPE_ENUM:
      /* note: never happens, as this is implemented with a macro instead */
      gsk_buffer_printf (output, "  return gskb_uint_get_packed_size (value);\n");
      break;
    case GSKB_FORMAT_TYPE_EXTENSIBLE:
      gsk_buffer_printf (output, "  return gskb_format_get_packed_size (%s%s_peek_format (), value);\n",
                         config->func_prefix, format->any.lc_name);
      break;
    default:
      g_return_val_if_reached (FALSE);
    }
}

/* validate */
#define return_value__validate     "guint"
static const char *type_name_pairs__validate[] = {
  "guint", "length",
  "const guint8 *", "data",
  "GError **", "error",
  NULL
};

static gboolean 
implement__validate       (GskbFormat *format,
                           const GskbCodegenConfig *config,
                           GskBuffer *output,
                           GError **error)
{
  guint i;
  switch (format->type)
    {
    case GSKB_FORMAT_TYPE_FIXED_ARRAY:
      {
        GskbFormat *sub = format->v_fixed_array.element_format;
        gsk_buffer_printf (output, "  guint rv = 0, sub_used;\n");
        if (format->v_fixed_array.length < 5)
          {
            for (i = 0; i < format->v_fixed_array.length; i++)
              gsk_buffer_printf (output,
                                 "  if (!%s%s_validate (len - rv, data + rv, &sub_used, error))\n"
                                 "    {\n"
                                 "      gsk_g_error_add_prefix (error, \"validating element #%%u of %%u\", %u, %u);\n"
                                 "      return FALSE;\n"
                                 "    }\n"
                                 "  rv += sub_used;\n",
                                 config->func_prefix, sub->any.lc_name,
                                 i, format->v_fixed_array.length);
          }
        else
          {
            gsk_buffer_printf (output,
                               "  guint i, rv = 0;\n"
                               "  for (i = 0; i < %u; i++)\n"
                               "    if (!%s%s_validate (len - rv, out + rv, &sub_used, error))\n"
                               "      {\n"
                                 "      gsk_g_error_add_prefix (error, \"validating element #%%u of %%u\", i, %u);\n"
                                 "      return FALSE;\n"
                                 "    }\n"
                                 "  rv += sub_used;\n",
                                 format->v_fixed_array.length,
                                 config->func_prefix, sub->any.lc_name,
                                 format->v_fixed_array.length);
          }
        gsk_buffer_printf (output, "  *n_used_out = rv;\n"
                                   "  return TRUE;\n");
        break;
      }
    case GSKB_FORMAT_TYPE_LENGTH_PREFIXED_ARRAY:
      {
        GskbFormat *sub = format->v_length_prefixed_array.element_format;
        gsk_buffer_printf (output,
                           "  guint rv, sub_used, i;\n"
                           "  guint32 n;\n"
                           "  if (!gskb_uint_validate_unpack (len, data, &sub_used, &n, error))\n"
                           "    {\n"
                           "      gsk_g_error_add_prefix (error, \"parsing length-prefix\");\n"
                           "      return FALSE;\n"
                           "    }\n"
                           "  rv = sub_used;\n");
        gsk_buffer_printf (output,
                           "  for (i = 0; i < n; i++)\n"
                           "    {\n"
                           "      if (!%s%s_validate (len - rv, out + rv, &sub_used, error))\n"
                           "        {\n"
                           "          gsk_g_error_add_prefix (error, \"validating element #%%u of %%u\", i, n);\n"
                           "          return FALSE;\n"
                           "        }\n"
                           "      rv += sub_used;\n"
                           "    }\n",
                             config->func_prefix, sub->any.lc_name);
        gsk_buffer_printf (output, "  *n_used_out = rv;\n"
                                   "  return TRUE;\n");
        break;
      }
    case GSKB_FORMAT_TYPE_STRUCT:
      {
        gsk_buffer_printf (output, "  guint rv = 0;\n");
        for (i = 0; i < format->v_struct.n_members; i++)
          {
            GskbFormatStructMember *member = format->v_struct.members + i;
            gsk_buffer_printf (output,
                               "  if (!%s%s_validate (len - rv, out + rv, &sub_used, error))\n"
                               "    {\n"
                               "      gsk_g_error_add_prefix (error, \"validating member '%s' of %s\", \"%s\", \"%s\");\n"
                               "      return FALSE;\n"
                               "    }\n",
                               config->func_prefix, member->format->any.lc_name,
                               member->name, format->any.name);
          }
        gsk_buffer_printf (output, "  *n_used_out = rv;\n"
                                   "  return TRUE;\n");
        break;
      }
    case GSKB_FORMAT_TYPE_UNION:
      {
        char *ucname = g_ascii_strup (format->any.lc_name, -1);
        gsk_buffer_printf (output,
                           "  guint rv;\n"
                           "  guint32 type;\n"
                           "  if (!gskb_uint_validate_unpack (len, data, &sub_used, &type, error))\n"
                           "    {\n"
                           "      gsk_g_error_add_prefix (error, \"error parsing union tag\");\n"
                           "      return FALSE;\n"
                           "    }\n");
        gsk_buffer_printf (output, "  switch (type)\n    {\n");
        for (i = 0; i < format->v_union.n_cases; i++)
          {
            GskbFormatUnionCase *c = format->v_union.cases + i;
            char *uccasename = g_ascii_strup (c->name, -1);
            gsk_buffer_printf (output, "    case %s__%s:\n",
                               ucname, uccasename);
            if (c->format == NULL)
              gsk_buffer_printf (output, "      *n_used_out = sub_used;\n"
                                         "      return TRUE;\n");
            else
              gsk_buffer_printf (output, "      rv = sub_used;\n"
                                         "      if (!%s%s_validate (len - rv, data + rv, &sub_used, error))\n"
                                         "        {\n"
                                         "           gsk_g_error_add_prefix (error, \"validating case '%%s' of '%%s'\", \"%s\", \"%s\");\n"
                                         "           return FALSE;\n"
                                         "        }\n"
                                         "      *n_used_out = rv + sub_used;\n"
                                         "      return TRUE;\n",
                                         ucname, uccasename, c->name, format->any.name);

            g_free (uccasename);
          }
        g_free (ucname);
        gsk_buffer_printf (output, "    default:\n"
                                   "      g_set_error (error, GSK_G_ERROR_DOMAIN_QUARK,\n"
                                   "                   GSK_ERROR_BAD_FORMAT,\n"
                                   "                   \"invalid tag %u for '%%s'\", type, \"%s\");\n"
                                   "      return FALSE;\n"
                                   "    }\n", format->any.name);
        break;
      }
    case GSKB_FORMAT_TYPE_ENUM:
      {
        char *ucname = g_ascii_strup (format->any.lc_name, -1);
        gsk_buffer_printf (output,
                           "  if (!gskb_uint_validate_unpack (len, data, n_used_out, &val, error))\n"
                           "    return FALSE;\n"
                           "  switch (val)\n"
                           "  {\n");
        for (i = 0; i < format->v_enum.n_values; i++)
          {
            GskbFormatEnumValue *e = format->v_enum.values + i;
            char *uccasename = g_ascii_strup (e->name, -1);
            gsk_buffer_printf (output,
                               "  case %s__%s: return TRUE;\n", ucname, uccasename);
            g_free (uccasename);
          }
        gsk_buffer_printf (output,
                           "  default:\n"
                           "    g_set_error (error, GSK_G_ERROR_DOMAIN_QUARK,\n"
                           "                 GSK_ERROR_BAD_FORMAT,\n"
                           "                 \"invalid value %%u for enum %%s\", val, \"%s\");\n"
                           "    return FALSE;\n"
                           "  }\n",
                           format->any.name);

        g_free (ucname);
      }
      break;
    case GSKB_FORMAT_TYPE_EXTENSIBLE:
      gsk_buffer_printf (output, "  return gskb_format_validate (%s%s_peek_format (), len, data, n_used_out, error);\n",
                         config->func_prefix, format->any.lc_name);
      break;
    default:
      g_return_val_if_reached (FALSE);
    }
}


/* unpack */
#define return_value__unpack       "guint"
static const char *type_name_pairs__unpack[] = {
  "const guint8 *", "in",
  "{type_name} *", "value_out",
  NULL
};
static gboolean 
implement__unpack         (GskbFormat *format,
                           const GskbCodegenConfig *config,
                           GskBuffer *output,
                           GError **error)
{
  guint i;
  switch (format->type)
    {
    case GSKB_FORMAT_TYPE_FIXED_ARRAY:
      {
        GskbFormat *sub = format->v_fixed_array.element_format;
        gsk_buffer_printf (output, "  guint rv = 0;\n");
        if (format->v_fixed_array.length < 5)
          {
            for (i = 0; i < format->v_fixed_array.length; i++)
              gsk_buffer_printf (output,
                                 "  rv += %s%s_unpack (in + rv, &value_out->data[%u]);\n",
                                 config->func_prefix, sub->any.lc_name, i);
          }
        else
          {
            gsk_buffer_printf (output,
                               "  guint i;\n"
                               "  for (i = 0; i < %u; i++)\n"
                               "    rv += %s%s_unpack (in + rv, &value_out->data[i]);\n",
                               format->v_fixed_array.length,
                               config->func_prefix, sub->any.lc_name);
          }
        gsk_buffer_printf (output, "  return rv;\n");
        break;
      }
    case GSKB_FORMAT_TYPE_LENGTH_PREFIXED_ARRAY:
      {
        GskbFormat *sub = format->v_length_prefixed_array.element_format;
        gsk_buffer_printf (output,
                           "  guint rv, i;\n"
                           "  guint32 n;\n"
                           "  rv = gskb_uint_unpack (in, &n);\n"
                           "  value_out->length = n;\n"
                           "  value_out->data = g_new (%s%s, n);\n",
                           config->type_prefix, sub->any.TypeName);
        gsk_buffer_printf (output,
                           "  for (i = 0; i < n; i++)\n"
                           "    rv += %s%s_unpack (in + rv, value_out->data[i]);\n",
                           config->func_prefix, sub->any.lc_name);
        gsk_buffer_printf (output, "  return rv;\n");
        break;
      }
    case GSKB_FORMAT_TYPE_STRUCT:
      {
        gsk_buffer_printf (output, "  guint rv = 0;\n");
        for (i = 0; i < format->v_struct.n_members; i++)
          {
            GskbFormatStructMember *member = format->v_struct.members + i;
            gsk_buffer_printf (output,
                               "  rv += %s%s_unpack (in + rv, &value_out->%s);\n",
                               config->func_prefix, member->format->any.lc_name,
                               member->name);
          }
        gsk_buffer_printf (output, "  return rv;\n");
        break;
      }
    case GSKB_FORMAT_TYPE_UNION:
      {
        char *ucname = g_ascii_strup (format->any.lc_name, -1);
        gsk_buffer_printf (output,
                           "  guint rv;\n"
                           "  guint32 type;\n"
                           "  rv = gskb_uint_unpack (data, &type);\n"
                           "  value_out->type = type;\n");
        gsk_buffer_printf (output, "  switch (type)\n    {\n");
        for (i = 0; i < format->v_union.n_cases; i++)
          {
            GskbFormatUnionCase *c = format->v_union.cases + i;
            char *uccasename = g_ascii_strup (c->name, -1);
            gsk_buffer_printf (output, "    case %s__%s:\n",
                               ucname, uccasename);
            if (c->format == NULL)
              gsk_buffer_printf (output, "      return rv;\n");
            else
              gsk_buffer_printf (output, "      return rv + %s%s_unpack (in + rv, &value_out->info.%s);\n",
                                         config->func_prefix, c->format->any.lc_name, c->name);

            g_free (uccasename);
          }
        g_free (ucname);
        gsk_buffer_printf (output, "    default:\n"
                                   "      g_return_val_if_reached (0);\n"
                                   "    }\n");
        break;
      }
    case GSKB_FORMAT_TYPE_ENUM:
      {
        gsk_buffer_printf (output,
                           "  guint rv;\n"
                           "  guint32 value;\n"
                           "  rv = gskb_uint_unpack (data, &value);\n"
                           "  *value_out = value;\n"
                           "  return rv;\n");
        break;
      }
      break;
    case GSKB_FORMAT_TYPE_EXTENSIBLE:
      gsk_buffer_printf (output, "  return gskb_format_unpack (%s%s_peek_format (), in, value_out);\n",
                         config->func_prefix, format->any.lc_name);
      break;
    default:
      g_return_val_if_reached (FALSE);
    }
}

/* unpack_mempool */
#define return_value__unpack_mempool       "guint"
static const char *type_name_pairs__unpack_mempool[] = {
  "const guint8 *", "in",
  "{type_name} *", "value_out",
  "GskMemPool *", "mem_pool",
  NULL
};
static gboolean 
implement__unpack_mempool (GskbFormat *format,
                           const GskbCodegenConfig *config,
                           GskBuffer *output,
                           GError **error)
{
  guint i;
  switch (format->type)
    {
    case GSKB_FORMAT_TYPE_FIXED_ARRAY:
      {
        GskbFormat *sub = format->v_fixed_array.element_format;
        gsk_buffer_printf (output, "  guint rv = 0;\n");
        if (format->v_fixed_array.length < 5)
          {
            for (i = 0; i < format->v_fixed_array.length; i++)
              gsk_buffer_printf (output,
                                 "  rv += %s%s_unpack_mempool (in + rv, &value_out->data[%u], mem_pool);\n",
                                 config->func_prefix, sub->any.lc_name, i);
          }
        else
          {
            gsk_buffer_printf (output,
                               "  guint i;\n"
                               "  for (i = 0; i < %u; i++)\n"
                               "    rv += %s%s_unpack_mempool (in + rv, &value_out->data[i], mem_pool);\n",
                               format->v_fixed_array.length,
                               config->func_prefix, sub->any.lc_name);
          }
        gsk_buffer_printf (output, "  return rv;\n");
        break;
      }
    case GSKB_FORMAT_TYPE_LENGTH_PREFIXED_ARRAY:
      {
        GskbFormat *sub = format->v_length_prefixed_array.element_format;
        gsk_buffer_printf (output,
                           "  guint rv, i;\n"
                           "  guint32 n;\n"
                           "  rv = gskb_uint_unpack (in, &n);\n"
                           "  value_out->length = n;\n"
                           "  value_out->data = g_new (%s%s, n);\n",
                           config->type_prefix, sub->any.TypeName);
        gsk_buffer_printf (output,
                           "  for (i = 0; i < n; i++)\n"
                           "    rv += %s%s_unpack_mempool (in + rv, value_out->data[i], mem_pool);\n",
                           config->func_prefix, sub->any.lc_name);
        gsk_buffer_printf (output, "  return rv;\n");
        break;
      }
    case GSKB_FORMAT_TYPE_STRUCT:
      {
        gsk_buffer_printf (output, "  guint rv = 0;\n");
        for (i = 0; i < format->v_struct.n_members; i++)
          {
            GskbFormatStructMember *member = format->v_struct.members + i;
            gsk_buffer_printf (output,
                               "  rv += %s%s_unpack_mempool (in + rv, &value_out->%s, mem_pool);\n",
                               config->func_prefix, member->format->any.lc_name,
                               member->name);
          }
        gsk_buffer_printf (output, "  return rv;\n");
        break;
      }
    case GSKB_FORMAT_TYPE_UNION:
      {
        char *ucname = g_ascii_strup (format->any.lc_name, -1);
        gsk_buffer_printf (output,
                           "  guint rv;\n"
                           "  guint32 type;\n"
                           "  rv = gskb_uint_unpack (data, &type);\n"
                           "  value_out->type = type;\n");
        gsk_buffer_printf (output, "  switch (type)\n    {\n");
        for (i = 0; i < format->v_union.n_cases; i++)
          {
            GskbFormatUnionCase *c = format->v_union.cases + i;
            char *uccasename = g_ascii_strup (c->name, -1);
            gsk_buffer_printf (output, "    case %s__%s:\n",
                               ucname, uccasename);
            if (c->format == NULL)
              gsk_buffer_printf (output, "      return rv;\n");
            else
              gsk_buffer_printf (output, "      return rv + %s%s_unpack_mempool (in + rv, &value_out->info.%s, mem_pool);\n",
                                         config->func_prefix, c->format->any.lc_name, c->name);

            g_free (uccasename);
          }
        g_free (ucname);
        gsk_buffer_printf (output, "    default:\n"
                                   "      g_return_val_if_reached (0);\n"
                                   "    }\n");
        break;
      }
    case GSKB_FORMAT_TYPE_ENUM:
      {
        gsk_buffer_printf (output,
                           "  guint rv;\n"
                           "  guint32 value;\n"
                           "  rv = gskb_uint_unpack (data, &value);\n"
                           "  *value_out = value;\n"
                           "  return rv;\n");
        break;
      }
      break;
    case GSKB_FORMAT_TYPE_EXTENSIBLE:
      gsk_buffer_printf (output, "  return gskb_format_unpack_mempool (%s%s_peek_format (), in, value_out, mem_pool);\n",
                         config->func_prefix, format->any.lc_name);
      break;
    default:
      g_return_val_if_reached (FALSE);
    }
}

/* destruct */
#define return_value__destruct       "void"
static const char *type_name_pairs__destruct[] = {
  "{type_name} *", "value",
  NULL
};
static gboolean 
implement__destruct (GskbFormat *format,
                     const GskbCodegenConfig *config,
                     GskBuffer *output,
                     GError **error)
{
  guint i;
  switch (format->type)
    {
    case GSKB_FORMAT_TYPE_FIXED_ARRAY:
      {
        GskbFormat *sub = format->v_fixed_array.element_format;
        if (format->v_fixed_array.length < 5)
          {
            for (i = 0; i < format->v_fixed_array.length; i++)
              gsk_buffer_printf (output,
                                 "  rv += %s%s_destruct (&value->data[%u]);\n",
                                 config->func_prefix, sub->any.lc_name, i);
          }
        else
          {
            gsk_buffer_printf (output,
                               "  guint i;\n"
                               "  for (i = 0; i < %u; i++)\n"
                               "    rv += %s%s_destruct (&value->data[i]);\n",
                               format->v_fixed_array.length,
                               config->func_prefix, sub->any.lc_name);
          }
        gsk_buffer_printf (output, "  return rv;\n");
        break;
      }
    case GSKB_FORMAT_TYPE_LENGTH_PREFIXED_ARRAY:
      {
        GskbFormat *sub = format->v_length_prefixed_array.element_format;
        if (sub->any.requires_destruct)
          gsk_buffer_printf (output,
                             "  guint i;\n"
                             "  for (i = 0; i < value->length; i++)\n"
                             "    %s%s_destruct (value->data[i]);\n",
                             config->func_prefix, sub->any.lc_name);
        gsk_buffer_printf (output, "  g_free (value->data);\n");
        break;
      }
    case GSKB_FORMAT_TYPE_STRUCT:
      {
        for (i = 0; i < format->v_struct.n_members; i++)
          {
            GskbFormatStructMember *member = format->v_struct.members + i;
            gsk_buffer_printf (output,
                               "  %s%s_destruct (&value->%s);\n",
                               config->func_prefix, member->format->any.lc_name,
                               member->name);
          }
        break;
      }
    case GSKB_FORMAT_TYPE_UNION:
      {
        char *ucname = g_ascii_strup (format->any.lc_name, -1);
        gsk_buffer_printf (output, "  switch (value->type)\n    {\n");
        for (i = 0; i < format->v_union.n_cases; i++)
          {
            GskbFormatUnionCase *c = format->v_union.cases + i;
            char *uccasename = g_ascii_strup (c->name, -1);
            gsk_buffer_printf (output, "    case %s__%s:\n",
                               ucname, uccasename);
            if (c->format == NULL)
              gsk_buffer_printf (output, "      break;\n");
            else
              gsk_buffer_printf (output, "      %s%s_destruct (&value->info.%s);\n"
                                         "      break;\n",
                                         config->func_prefix, c->format->any.lc_name, c->name);

            g_free (uccasename);
          }
        g_free (ucname);
        gsk_buffer_printf (output, "    default:\n"
                                   "      g_return_if_reached ();\n"
                                   "    }\n");
        break;
      }
    case GSKB_FORMAT_TYPE_EXTENSIBLE:
      gsk_buffer_printf (output,
                         "  gskb_format_clear_value (%s%s_peek_format (), value);\n",
                         config->func_prefix, format->any.lc_name);
      break;
    default:
      g_return_val_if_reached (FALSE);
    }
  return TRUE;
}

/* peek_format */
#define return_value__peek_format       "GskbFormat *"
static const char *type_name_pairs__peek_format[] = {
  NULL
};
static gboolean 
implement__peek_format (GskbFormat *format,
                        const GskbCodegenConfig *config,
                        GskBuffer *output,
                        GError **error)
{
  gsk_buffer_printf (output, "  return %s%s__static_format;\n",
                     config->func_prefix, format->any.lc_name);
  return TRUE;
}

static struct {
  GskbCodegenOutputFunction output_function;
  const char *name;
  const char *ret_value;
  const char *const *type_name_pairs_templates;
  OutputFunctionImplementor implementor;
} output_function_info[] = {
#define ENTRY(UCNAME, lcname) \
  { GSKB_CODEGEN_OUTPUT_ ## UCNAME, \
    #lcname, \
    return_value__##lcname, \
    type_name_pairs__##lcname, \
    implement__##lcname }
  ENTRY (PACK, pack),
  ENTRY (GET_PACKED_SIZE, get_packed_size),
  ENTRY (PACK_SLAB, pack_slab),
  ENTRY (VALIDATE, validate),
  ENTRY (UNPACK, unpack),
  ENTRY (UNPACK_MEMPOOL, unpack_mempool),
  ENTRY (DESTRUCT, destruct),
  ENTRY (PEEK_FORMAT, peek_format),
};

static void
generic_start_function (GskBuffer *buffer,
                        const char *qualifiers,
                        const char *ret_value,
                        const char *func_name,
                        guint n_args,
                        char **args_type_name_pairs)
{
  ..
}
static void
start_function (const char *qualifiers,
                GskFormat *format,
                const GskbCodegenConfig *config,
                GskbCodegenOutputFunction function,
                GskBuffer *buffer);
{
  char *func_name = g_strdup_printf ("%s%s_%s",
                                     config->func_prefix,
                                     format->any.lc_name,
                                     output_function_info[function].name);
  const char *const*raw_args = emission_function_info[function].type_name_pair_templates;
  char **substituted_names;
  guint n_args = g_strv_length ((char**)raw_args) / 2;
  substituted_names = g_new (char *, n_args * 2 + 1);
  for (i = 0; i < n_args; i++)
    {
      ...
    }
  substituted_names[2*n_args] = NULL;
  generic_start_function (buffer, qualifiers,
                          emission_function_info[function].ret_value,
                          func_name,
                          n_args,
                          substituted_names);
  g_free (func_name);
  g_strfreev (substituted_names);
}

static gboolean
is_fundamental (GskbFormat *format)
{
  return format->type == GSKB_FORMAT_TYPE_INT
      || format->type == GSKB_FORMAT_TYPE_FLOAT
      || format->type == GSKB_FORMAT_TYPE_STRING;
}

gboolean
gskb_format_codegen_emit_function      (GskbFormat *format,
                                        GskbCodegenOutputFunction function,
                                        const char *qualifiers,
                                        gboolean emit_implementation,
                                        const GskbCodegenConfig *config,
                                        GskBuffer *output,
                                        GError **error)
{
  const char *name = emission_function_info[function].name;
  g_assert (emission_function_info[function].output_function == function);
  if (is_fundamental (format))
    {
      if (emit_implementation)
        return TRUE;
      gsk_buffer_printf (output,
                         "#define %s%s_%s gskb_%s_%s\n",
                         config->func_prefix, format->lc_name, name,
                         format->lc_name, name);
      return TRUE;
    }
  if (format->type == GSKB_FORMAT_TYPE_ALIAS)
    {
      gsk_buffer_printf (output,
                         "#define %s%s_%s %s%s_%s\n",
                         config->func_prefix, format->lc_name, name,
                         config->func_prefix, format->v_alias.format->lc_name, name,
                         format->lc_name, name);
      return TRUE;
    }

  /* misc hacks. */
  switch (function)
    {
    case GSKB_CODEGEN_OUTPUT_GET_PACKED_SIZE:
      if (format->is_fixed_length)
        {
          gsk_buffer_printf (output,
                             "#define %s%s_get_packed_size(value) %u\n",
                             config->func_prefix, format->any.lc_name,
                             format->any.fixed_length);
          return TRUE;
        }
      if (format->type == GSKB_FORMAT_TYPE_ENUM)
        {
          gsk_buffer_printf (output,
                             "#define %s%s_get_packed_size(value) gskb_uint_get_packed_size(value)\n",
                             config->func_prefix, format->any.lc_name);
          return TRUE;
        }
      break;
    }

  start_function (qualifiers, format, config, function, output);
  gsk_buffer_append_string (output, "\n{\n");
  if (!emission_function_info[function].implementor (format, 
                                                     config,
                                                     output,
                                                     error))
    return FALSE;
  gsk_buffer_append_string (output, "}\n\n");
  return TRUE;
}

static gboolean
gskb_format_codegen__emit_function   
                                       (GskbFormat *format,
                                        GskbCodegenSection phase,
                                        const GskbCodegenConfig *config,
                                        GskBuffer *output,
                                        GError **error)
{
  GskbCodegenOutputFunction i;
  const char *qualifiers;
  if (config->all_static)
    qualifiers = "static ";
  else
    qualifiers = "";
  for (i = 0; i < N_EMISSION_FUNCTIONS; i++)
    {
      if (!gskb_format_codegen_emit_function (format, i, qualifiers,
                                              phase == GSKB_FORMAT_CODEGEN_EMIT_FUNCTION_IMPLS, config, buffer, error))
        return FALSE;
    }
  return TRUE;
}

typedef gboolean (*Emitter) (GskbFormat *format,
                         GskbCodegenSection phase,
                         const GskbCodegenConfig *config,
                         GskBuffer *output,
                         GError **error);
/* must match order of GskbCodegenSection */
static Emitter emitters[] = {
  gskb_format_codegen__emit_typedefs,
  gskb_format_codegen__emit_structures,
  gskb_format_codegen__emit_function,
  gskb_format_codegen__emit_function
};
gboolean    gskb_format_codegen        (GskbFormat *format,
                                        GskbCodegenSection phase,
                                        const GskbCodegenConfig *config,
                                        GskBuffer *output,
                                        GError **error)
{
  return emitters[phase] (format, phase, config, output, error);
}

