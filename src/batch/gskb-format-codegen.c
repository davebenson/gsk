
typedef enum
{
  GSKB_FORMAT_CODEGEN_EMIT_TYPEDEFS,
  GSKB_FORMAT_CODEGEN_EMIT_STRUCTURES,
  GSKB_FORMAT_CODEGEN_EMIT_FUNCTION_PROTOTYPES,
  GSKB_FORMAT_CODEGEN_EMIT_FUNCTION_IMPLS
} GskbFormatCodegenEmit;

typedef struct _GskbFormatCodegenConfig GskbFormatCodegenConfig;
struct _GskbFormatCodegenConfig
{
  char *type_prefix, *func_prefix;
  gboolean all_static;
};
GskbFormatCodegenConfig *
gskb_format_codegen_config_new            (const char      *type_prefix,
                                           const char      *func_prefix)
{
  GskbFormatCodegenConfig *config = g_slice_new0 (GskbFormatCodegenConfig);
  config->type_prefix = g_strdup (type_prefix);
  config->func_prefix = g_strdup (func_prefix);
  return config;
}
void
gskb_format_codegen_config_set_all_static (GskbFormatCodegenConfig *config,
                                           gboolean         all_static)
{
  config->all_static = all_static;
}
void
gskb_format_codegen_config_free (GskbFormatCodegenConfig *config)
{
  g_free (config->type_prefix);
  g_free (config->func_prefix);
  g_slice_free (config);
  return config;
}

static gboolean
gskb_format_codegen__emit_typedefs     (GskbFormat *format,
                                        GskbFormatCodegenEmit phase,
                                        const GskbFormatCodegenConfig *config,
                                        GskBuffer *output,
                                        GError **error)
{
  g_assert (phase == GSKB_FORMAT_CODEGEN_EMIT_TYPEDEFS);
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
          { GSKB_FORMAT_INT_VARLEN_INT32,   "gint32" },
          { GSKB_FORMAT_INT_VARLEN_INT64,   "gint64" },
          { GSKB_FORMAT_INT_VARLEN_UINT32,  "guint32" },
          { GSKB_FORMAT_INT_VARLEN_UINT64,  "guint64" }
          { GSKB_FORMAT_INT_BIT,            "gboolean" }
        };
        
        /* validate that the table is ok.  (sigh) */
        g_assert (format->v_int.int_type < G_N_ELEMENTS (int_c_types));
        g_assert (int_c_types[format->v_int.int_type].int_type
                  == format->v_int.int_type);

        gsk_buffer_printf (output,
                           "typedef %s %s%s;\n"
                           int_c_types[format->v_int.int_type].c_type,
                           format->type_prefix, format->TypeName);
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
                           "typedef %s %s%s;\n"
                           float_c_types[format->v_float.float_type].c_type,
                           format->type_prefix, format->TypeName);
      }
      break;
    case GSKB_FORMAT_TYPE_ENUM:
      /* nothing to do */
      break;
    case GSKB_FORMAT_TYPE_ALIAS:
      gsk_buffer_printf (output,
                         "typedef %s%s %s%s;\n",
                         config->type_prefix, format->v_alias.format->any.TypeName);
                         config->type_prefix, format->any.TypeName,

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
                                        GskbFormatCodegenEmit phase,
                                        const GskbFormatCodegenConfig *config,
                                        GskBuffer *output,
                                        GError **error)
{
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
        uc_name = mixed_to_uc (format->any.TypeName);
        gsk_buffer_append_str (output, "typedef enum {\n");
        for (i = 0; i < format->v_enum.n_values; i++)
          {
            char *enum_uc_name = lower_to_upper (format->v_enum.values[i].name);
            if (i > 0)
              gsk_buffer_append_str (output, ",\n");
            if (format->v_enum.values[i].value == expected)
              gsk_buffer_printf (buffer,
                                 "  %s__%s", uc_name, enum_uc_name);
            else
              gsk_buffer_printf (buffer,
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
                         config->type_prefix, format->any.TypeName,
                         format->v_fixed_array.element_format->v_fixed_array.len);
      break;
    case GSKB_FORMAT_TYPE_LENGTH_PREFIXED_ARRAY:
      gsk_buffer_printf (output,
                         "struct _%s%s\n"
                         "{\n"
                         "  guint length;\n"
                         "  %s%s *data;\n"
                         "};\n\n",
                         config->type_prefix, format->any.TypeName,
                         config->type_prefix, format->any.TypeName,
                         format->v_fixed_array.element_format->v_fixed_array.len);
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
                             config->type_prefix, member->format->any.TypeName,
                             member->name);
        }
      gsk_buffer_printf (output, "  } info;\n"
                                 "};\n\n");

      break;
    default:
      g_assert_not_reached ();
    }
  return TRUE;
}

typedef enum
{
  EMISSION_PACK,
  EMISSION_GET_PACKED_SIZE,
  EMISSION_PACK_SLAB,
  EMISSION_UNPACK,
  EMISSION_UNPACK_PARTIAL,
  EMISSION_DESTRUCT
} EmissionFunction;

typedef gboolean (*EmissionFunctionImplementor)   (GskbFormat *format,
                                                 const GskbFormatCodegenConfig *config,
                                                 GskBuffer *output,
                                                 GError **error);


/* helper functions, used by implementor functions */
static void start_function (const char *qualifiers,
                            GskFormat *format,
                            const GskbFormatCodegenConfig *config,
                            EmissionFunction function,
                            GskBuffer *buffer);




/* pack */
#define return_value__pack       guint
static const char *type_name_pairs__pack[] = {
  "$maybe_const $type_name $maybe_ptr", "value",
  "GskbAppendFunc", "append_func",
  "gpointer", "append_func_data"
};
static gboolean 
implement__pack(GskbFormat *format,
                const GskbFormatCodegenConfig *config,
                GskBuffer *output,
                GError **error)
{
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
                                 config->func_prefix, sub->lc_name,
                                 sub->always_by_pointer ? "&" : "", i);
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
                               config->func_prefix, sub->lc_name,
                               sub->always_by_pointer ? "&" : "");
          }
        break;
      }
    case GSKB_FORMAT_TYPE_LENGTH_PREFIXED_ARRAY:
      {
        ...
      }
    case GSKB_FORMAT_TYPE_STRUCT:
      {
        ...
      }
    case GSKB_FORMAT_TYPE_UNION:
      {
        ...
      }
    case GSKB_FORMAT_TYPE_ENUM:
      {
        ...
      }
    }
}


typedef struct {
  EmissionFunction ef;
  const char *name;
  const char *ret_value;
  const char *const *type_name_pairs_templates;
  EmissionFunctionImplementor implementor;
} emission_function_info[] = {
#define ENTRY(UCNAME, lcname) \
  { EMISSION_ ## UCNAME, \
    #lcname, \
    return_value__##lcname, \
    type_name_pairs__##lcname, \
    implement__##lcname }
  ENTRY (PACK, pack),
  ENTRY (GET_PACKED_SIZE, get_packed_size),
  ENTRY (PACK_SLAB, pack_slab),
  ENTRY (UNPACK, unpack),
  ENTRY (UNPACK_PARTIAL, unpack_partial),
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
                const GskbFormatCodegenConfig *config,
                EmissionFunction function,
                GskBuffer *buffer);
{
  char *func_name = g_strdup_printf ("%s%s_%s",
                                     config->func_prefix,
                                     format->any.lc_name,
                                     emission_function_info[function].name);
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

gboolean
gskb_format_codegen_emit_function      (GskbFormat *format,
                                        EmissionFunction function,
                                        const char *qualifiers,
                                        gboolean emit_implementation,
                                        const GskbFormatCodegenConfig *config,
                                        GskBuffer *output,
                                        GError **error)
{
  const char *name = emission_function_info[function].name;
  g_assert (emission_function_info[function].ef == function);
  if (format->type == GSKB_FORMAT_TYPE_INT
   || format->type == GSKB_FORMAT_TYPE_FLOAT
   || format->type == GSKB_FORMAT_TYPE_STRING)
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
                                        GskbFormatCodegenEmit phase,
                                        const GskbFormatCodegenConfig *config,
                                        GskBuffer *output,
                                        GError **error)
{
  EmissionFunction i;
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
                         GskbFormatCodegenEmit phase,
                         const GskbFormatCodegenConfig *config,
                         GskBuffer *output,
                         GError **error);
/* must match order of GskbFormatCodegenEmit */
static Emitter emitters[] = {
  gskb_format_codegen__emit_typedefs,
  gskb_format_codegen__emit_structures,
  gskb_format_codegen__emit_function,
  gskb_format_codegen__emit_function
};
gboolean    gskb_format_codegen        (GskbFormat *format,
                                        GskbFormatCodegenEmit phase,
                                        const GskbFormatCodegenConfig *config,
                                        GskBuffer *output,
                                        GError **error)
{
  return emitters[phase] (format, phase, config, output, error);
}

