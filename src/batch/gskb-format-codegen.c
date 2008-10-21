/*
    GSKB - a batch processing framework

    Copyright (C) 2008 Dave Benson

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA

    Contact:
        daveb@ffem.org <Dave Benson>
*/

#include <string.h>
#include "gskb-format.h"
#include "gskb-codegen-config.h"
#include "gskb-str-table.h"
#include "gskb-uint-table.h"

GskbCodegenConfig *
gskb_codegen_config_new            (void)
{
  GskbCodegenConfig *config = g_slice_new0 (GskbCodegenConfig);
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
  g_slice_free (GskbCodegenConfig, config);
}

static void
gskb_format_codegen__emit_typedefs     (GskbFormat *format,
                                        const GskbCodegenConfig *config,
                                        GskBuffer *output)
{
  switch (format->type)
    {
    case GSKB_FORMAT_TYPE_INT:
      g_assert_not_reached ();
    case GSKB_FORMAT_TYPE_FLOAT:
      g_assert_not_reached ();
      break;
    case GSKB_FORMAT_TYPE_ENUM:
      /* nothing to do */
      break;
    case GSKB_FORMAT_TYPE_ALIAS:
      gsk_buffer_printf (output,
                         "typedef %s %s;\n",
                         format->v_alias.format->any.c_type_name,
                         format->any.c_type_name);

    default:
      gsk_buffer_printf (output,
                         "typedef struct _%s %s;\n",
                         format->any.c_type_name,
                         format->any.c_type_name);
      break;
    }
}

static void
gskb_format_codegen__emit_structures   (GskbFormat *format,
                                        const GskbCodegenConfig *config,
                                        GskBuffer *output)
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
        uc_name = g_ascii_strup (format->any.c_func_prefix, -1);
        gsk_buffer_append_string (output, "typedef enum {\n");
        for (i = 0; i < format->v_enum.n_values; i++)
          {
            char *enum_uc_name = g_ascii_strup (format->v_enum.values[i].name, -1);
            if (i > 0)
              gsk_buffer_append_string (output, ",\n");
            if (format->v_enum.values[i].code == expected)
              gsk_buffer_printf (output,
                                 "  %s__%s", uc_name, enum_uc_name);
            else
              gsk_buffer_printf (output,
                                 "  %s__%s = %u", uc_name, enum_uc_name,
                                 format->v_enum.values[i].code);
            expected = format->v_enum.values[i].code + 1;
            g_free (enum_uc_name);
          }
        gsk_buffer_printf (output, "} %s;\n\n",
                           format->any.c_type_name);
        g_free (uc_name);
      }
      break;
    case GSKB_FORMAT_TYPE_FIXED_ARRAY:
      gsk_buffer_printf (output,
                         "struct _%s\n"
                         "{\n"
                         "  %s data[%u];\n"
                         "};\n\n",
                         format->any.c_type_name,
                         format->v_fixed_array.element_format->any.c_type_name,
                         format->v_fixed_array.length);
      break;
    case GSKB_FORMAT_TYPE_LENGTH_PREFIXED_ARRAY:
      gsk_buffer_printf (output,
                         "struct _%s\n"
                         "{\n"
                         "  guint32 length;\n"
                         "  %s *data;\n"
                         "};\n\n",
                         format->any.c_type_name,
                         format->v_length_prefixed_array.element_format->any.c_type_name);
      break;
    case GSKB_FORMAT_TYPE_STRUCT:
      gsk_buffer_printf (output,
                         "struct _%s\n"
                         "{\n",
                         format->any.c_type_name);
      if (format->v_struct.is_extensible)
        {
          gsk_buffer_printf (output,
                             "  GskbUnknownValueArray unknown_members;\n");
          for (i = 0; i < format->v_struct.n_members; i++)
            gsk_buffer_printf (output, "  guint8 has_%s : 1;\n",
                               format->v_struct.members[i].name);
        }
      for (i = 0; i < format->v_struct.n_members; i++)
        {
          GskbFormatStructMember *member = format->v_struct.members + i;
          gsk_buffer_printf (output,
                             "  %s %s;\n",
                             member->format->any.c_type_name,
                             member->name);
        }
      gsk_buffer_printf (output, "};\n\n");
      break;
    case GSKB_FORMAT_TYPE_UNION:
      gsk_buffer_printf (output,
                         "struct _%s\n"
                         "{\n"
                         "  %s_Type type;\n"
                         "  union {\n" ,
                         format->any.c_type_name,
                         format->any.c_type_name);
      for (i = 0; i < format->v_union.n_cases; i++)
        {
          GskbFormatUnionCase *c = format->v_union.cases + i;
          if (c->format != NULL)
            gsk_buffer_printf (output,
                               "    %s %s;\n",
                               c->format->any.c_type_name,
                               c->name);
        }
      if (format->v_union.is_extensible)
        gsk_buffer_printf (output, "    GskbUnknownValue unknown;\n");
      gsk_buffer_printf (output, "  } info;\n"
                                 "};\n\n");

      break;
    default:
      g_assert_not_reached ();
    }
}

static void
implement_format_any (GskbFormat *format,
                      const GskbCodegenConfig *config,
                      GskBuffer *output)
{

  gsk_buffer_printf (output,
                         "  {\n"
                         "    %s,         /* type */\n"
                         "    1,          /* ref_count */\n"
                         "    &%s_namespace,  /* the namespace itself */\n"
                         "    \"%s\", \"%s\", \"%s\",  /* various names */\n"
                         "    %s,         /* ctype */\n"
                         "    sizeof(%s), GSKB_ALIGNOF(%s),\n"
                         "    %u,       /* always by pointer */\n"
                         "    %u,       /* requires_destruct */\n"
                         "    TRUE,     /* is_global */\n"
                         "    %u        /* fixed_length */\n"
                         "  },\n",
                         gskb_format_type_enum_name (format->type),
                         format->any.ns->c_func_prefix,
                         format->any.name,
                         format->any.c_type_name,
                         format->any.c_func_prefix,
                         gskb_format_ctype_enum_name (format->any.ctype),
                         format->any.c_type_name,
                         format->any.c_type_name,
                         format->any.always_by_pointer,
                         format->any.requires_destruct,
                         format->any.fixed_length);
}

static void
gskb_format_codegen__emit_format_decls (GskbFormat *format,
                                        const GskbCodegenConfig *config,
                                        GskBuffer *output)
{
  gsk_buffer_printf (output,
                 "#define %s_format  ((GskbFormat *)(&%s_format_instance))\n",
                 format->any.c_func_prefix,
                 format->any.c_func_prefix);
}
static void
gskb_format_codegen__emit_format_private_decls (GskbFormat *format,
                                                const GskbCodegenConfig *config,
                                                GskBuffer *output)
{
  const char *type_name = NULL;
  switch (format->type)
    {
    case GSKB_FORMAT_TYPE_INT:
      type_name = "GskbFormatInt";
      break;
    case GSKB_FORMAT_TYPE_FLOAT:
      type_name = "GskbFormatFloat";
      break;
    case GSKB_FORMAT_TYPE_STRING:
      type_name = "GskbFormatString";
      break;
    case GSKB_FORMAT_TYPE_FIXED_ARRAY:
      type_name = "GskbFormatFixedArray";
      break;
    case GSKB_FORMAT_TYPE_LENGTH_PREFIXED_ARRAY:
      type_name = "GskbFormatLengthPrefixedArray";
      break;
    case GSKB_FORMAT_TYPE_STRUCT:
      type_name = "GskbFormatStruct";
      break;
    case GSKB_FORMAT_TYPE_BIT_FIELDS:
      type_name = "GskbFormatBitFields";
      break;
    case GSKB_FORMAT_TYPE_UNION:
      type_name = "GskbFormatUnion";
      break;
    case GSKB_FORMAT_TYPE_ENUM:
      type_name = "GskbFormatEnum";
      break;
    case GSKB_FORMAT_TYPE_ALIAS:
      type_name = "GskbFormatAlias";
      break;
    default:
      g_assert_not_reached ();
  };
  gsk_buffer_printf (output,
                     "extern %s %s_format_instance;\n",
                     type_name,
                     format->any.c_func_prefix);
}

static void
render_int (gconstpointer entry_data,
            GskBuffer    *dest)
{
  gsk_buffer_printf (dest, "%d", * (const gint *) entry_data);
}
static void
gskb_format_codegen__emit_format_impls (GskbFormat *format,
                                        const GskbCodegenConfig *config,
                                        GskBuffer *output)
{
  switch (format->type)
    {
    case GSKB_FORMAT_TYPE_INT:
    case GSKB_FORMAT_TYPE_FLOAT:
    case GSKB_FORMAT_TYPE_STRING:
      {
        g_assert_not_reached ();
        return;
      }
    case GSKB_FORMAT_TYPE_FIXED_ARRAY:
      {
      gsk_buffer_printf (output,
                         "%sGskbFormatFixedArray %s_format_instance =\n"
                         "{\n",
                         config->all_static ? "static " : "",
                         format->any.c_func_prefix);
      implement_format_any (format, config, output);
      gsk_buffer_printf (output,
                         "  %u,\n"
                         "  %s_format\n"
                         "};\n",
                         format->v_fixed_array.length,
                         format->v_fixed_array.element_format->any.c_func_prefix);
      return;
      }

    case GSKB_FORMAT_TYPE_LENGTH_PREFIXED_ARRAY:
      {
        gsk_buffer_printf (output,
                           "%sGskbFormatLengthPrefixedArray %s_format_instance =\n"
                           "{\n",
                           config->all_static ? "static " : "",
                           format->any.c_func_prefix);
        implement_format_any (format, config, output);
        gsk_buffer_printf (output,
                           "  %s_format,\n"
                           "  G_STRUCT_OFFSET (%s, length),\n"
                           "  G_STRUCT_OFFSET (%s, data)\n"
                           "};\n",
                           format->v_length_prefixed_array.element_format->any.c_func_prefix,
                           format->any.c_type_name,
                           format->any.c_type_name);
        return;
      }

    case GSKB_FORMAT_TYPE_STRUCT:
      {
        guint i;
        gsk_buffer_printf (output,
                           "static guint %s__member_offsets[%u] = {\n",
                           format->any.c_func_prefix,
                           format->v_struct.n_members);
        for (i = 0; i < format->v_struct.n_members; i++)
          {
            const char *mname = format->v_struct.members[i].name;
            gsk_buffer_printf (output,
                               "  G_STRUCT_OFFSET (%s, %s)%s\n",
                               format->any.c_type_name, 
                               mname,
                               (i + 1 == format->v_struct.n_members) ? "" : ",");
          }
        gsk_buffer_printf (output,
                           "};\n");

        gsk_buffer_printf (output,
                           "static GskbFormatStructMember %s__members[%u] = {\n",
                           format->any.c_func_prefix,
                           format->v_struct.n_members);

        for (i = 0; i < format->v_struct.n_members; i++)
          {
            GskbFormat *mformat = format->v_struct.members[i].format;
            const char *mname = format->v_struct.members[i].name;
            guint mcode = format->v_struct.members[i].code;
            gsk_buffer_printf (output,
                               "  { %u, \"%s\", %s_format }%s\n",
                               mcode,
                               mname,
                               mformat->any.c_func_prefix,
                               (i + 1 == format->v_struct.n_members) ? "" : ",");
          }
        gsk_buffer_printf (output,
                           "};\n");
        char *table_name = g_strdup_printf ("%s__name_to_member_index",
                                            format->any.c_func_prefix);
        gskb_str_table_print_compilable_deps (format->v_struct.name_to_index,
                                              table_name,
                                              "guint32",
                                              render_int,
                                              output);
        gsk_buffer_printf (output, "static GskbStrTable %s = ", table_name);
        gskb_str_table_print_compilable_object (format->v_struct.name_to_index,
                                              table_name,
                                              "sizeof (gint32)",
                                              "GSKB_ALIGNOF_UINT32",
                                              output);
        gsk_buffer_printf (output, ";\n");
        g_free (table_name);

        char *contents_format;
        if (format->v_struct.is_extensible)
          contents_format = g_strdup_printf ("%s_contents_format",
                                             format->any.c_func_prefix);
        else
          contents_format = g_strdup ("NULL");
        gsk_buffer_printf (output,
                           "%sGskbFormatStruct %s_format_instance =\n"
                           "{\n",
                           config->all_static ? "static " : "",
                           format->any.c_func_prefix);
        implement_format_any (format, config, output);
        gsk_buffer_printf (output,
                           "  %u,    /* is_extensible */\n"
                           "  %u, %s__members,\n"
                           "  %s,     /* contents_format */\n"
                           "  %s__member_offsets,\n"
                           "  &%s__name_to_member_index,\n",
                           format->v_struct.is_extensible,
                           format->v_struct.n_members,
                           format->any.c_func_prefix,
                           contents_format,
                           format->any.c_func_prefix,
                           format->any.c_func_prefix);
        g_free (contents_format);
        if (format->v_struct.is_extensible)
          gsk_buffer_printf (output, "  &%s__code_to_member_index\n",
                             format->any.c_func_prefix);
        else
          gsk_buffer_printf (output, "  NULL\n");
        gsk_buffer_printf (output, "};\n");
        return;
      }
    case GSKB_FORMAT_TYPE_UNION:
      {
        guint i;

        gsk_buffer_printf (output,
                           "static GskbFormatUnionCase %s__cases[] =\n"
                           "{\n",
                           format->any.c_func_prefix);
        for (i = 0; i < format->v_union.n_cases; i++)
          {
            GskbFormatUnionCase *c = format->v_union.cases + i;
            gsk_buffer_printf (output,
                               "  { \"%s\", %u, ",
                               c->name, c->code);
            if (c->format != NULL)
              gsk_buffer_printf (output, "%s_format",
                                 c->format->any.c_func_prefix);
            else
              gsk_buffer_printf (output, "NULL");
            gsk_buffer_printf (output, " }%s\n",
                               (i+1==format->v_union.n_cases) ? "" : ",");
          }
        gsk_buffer_printf (output,
                           "};\n");

        char *by_name_table_name, *by_code_table_name;
        by_name_table_name = g_strdup_printf ("%s__name_to_index",
                                              format->any.c_func_prefix);

        gskb_str_table_print (format->v_union.name_to_index,
                              FALSE,
                              by_name_table_name,
                              "guint32",
                              render_int,
                              "sizeof (guint32)",
                              "GSKB_ALIGNOF_UINT32",
                              output);
        by_code_table_name = g_strdup_printf ("%s__code_to_index",
                                              format->any.c_func_prefix);
        gskb_uint_table_print_static (format->v_union.code_to_index,
                                     by_code_table_name,
                                     "guint32",
                                     render_int,
                                     "sizeof (guint32)",
                                     "GSKB_ALIGNOF_UINT32",
                                     output);

        gsk_buffer_printf (output,
                           "%sGskbFormatUnion %s_format_instance =\n"
                           "{\n",
                           config->all_static ? "static " : "",
                           format->any.c_func_prefix);
        implement_format_any (format, config, output);
        gsk_buffer_printf (output,
                           "  %u,           /* is_extensible */\n"
                           "  %u,           /* n_cases */\n"
                           "  %s__cases,\n"
                           "  %s,\n"
                           "  G_STRUCT_OFFSET (%s, type),\n"
                           "  G_STRUCT_OFFSET (%s, info),\n"
                           "  &%s,\n"
                           "  &%s,\n"
                           "  %s_type_format\n"
                           "};\n",
                           format->v_union.is_extensible,
                           format->v_union.n_cases,
                           format->any.c_func_prefix,
                           gskb_format_int_type_enum_name (format->v_union.int_type),
                           format->any.c_type_name, 
                           format->any.c_type_name, 
                           by_name_table_name,
                           by_code_table_name,
                           format->any.c_func_prefix);
        g_free (by_name_table_name);
        g_free (by_code_table_name);
        break;
      }

    case GSKB_FORMAT_TYPE_ENUM:
      {
        guint i;
        gsk_buffer_printf (output,
                           "static GskbFormatEnumValue %s__enum_values[] =\n"
                           "{\n",
                           format->any.c_func_prefix);
        for (i = 0; i < format->v_enum.n_values; i++)
          {
            gsk_buffer_printf (output, "  { \"%s\", %u },\n",
                               format->v_enum.values[i].name,
                               format->v_enum.values[i].code);
          }
        gsk_buffer_printf (output,
                           "};\n");

        {
          char *table_name = g_strdup_printf ("%s__name_to_index",
                                              format->any.c_func_prefix);
          gskb_str_table_print (format->v_enum.name_to_index,
                                FALSE,
                                table_name,
                                "guint32",
                                render_int,
                                "sizeof (gint32)",
                                "GSKB_ALIGNOF_UINT32",
                                output);
          g_free (table_name);
        }
        {
          char *table_name = g_strdup_printf ("%s__code_to_index",
                                              format->any.c_func_prefix);
          gskb_uint_table_print_static (format->v_enum.code_to_index,
                                        table_name,
                                        "guint32",
                                        render_int,
                                        "sizeof (gint32)",
                                        "GSKB_ALIGNOF_UINT32",
                                        output);
          g_free (table_name);
        }

        gsk_buffer_printf (output,
                           "%sGskbFormatEnum %s_format_instance =\n"
                           "{\n",
                           config->all_static ? "static " : "",
                           format->any.c_func_prefix);
        implement_format_any (format, config, output);
        gsk_buffer_printf (output,
                           "  %s,\n"
                           "  %u,\n"
                           "  %s__enum_values,\n"
                           "  %u,       /* is_extensible */\n"
                           "  &%s__name_to_index,\n"
                           "  &%s__code_to_index,\n"
                           "};\n",
                           gskb_format_int_type_enum_name (format->v_enum.int_type),
                           format->v_enum.n_values,
                           format->any.c_func_prefix,
                           format->v_enum.is_extensible,
                           format->any.c_func_prefix,
                           format->any.c_func_prefix);
      }
      break;

    case GSKB_FORMAT_TYPE_ALIAS:
      gsk_buffer_printf (output,
                         "%sGskbFormatAlias %s_format_instance =\n"
                         "{\n",
                         config->all_static ? "static " : "",
                         format->any.c_func_prefix);
      implement_format_any (format, config, output);
      gsk_buffer_printf (output,
                         "  %s_format\n"
                         "};\n",
                         format->v_alias.format->any.c_func_prefix);
      break;

    default:
      g_assert_not_reached ();
    }
}



typedef void (*OutputFunctionImplementor)   (GskbFormat *format,
                                             const GskbCodegenConfig *config,
                                             GskBuffer *output);


/* helper functions, used by implementor functions */
static void start_function (const char *qualifiers,
                            gboolean    include_semicolon,
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
static void
implement__pack(GskbFormat *format,
                const GskbCodegenConfig *config,
                GskBuffer *output)
{
  guint i;
  switch (format->type)
    {
    case GSKB_FORMAT_TYPE_STRING:
    case GSKB_FORMAT_TYPE_FLOAT:
    case GSKB_FORMAT_TYPE_INT:
    case GSKB_FORMAT_TYPE_ALIAS:
      g_return_if_reached ();

    case GSKB_FORMAT_TYPE_FIXED_ARRAY:
      {
        GskbFormat *sub = format->v_fixed_array.element_format;
        if (format->v_fixed_array.length < 5)
          {
            for (i = 0; i < format->v_fixed_array.length; i++)
              gsk_buffer_printf (output,
             "  %s_pack (%svalue->data[%u], append_func, append_func_data);\n",
                                 sub->any.c_func_prefix,
                                 sub->any.always_by_pointer ? "&" : "", i);
          }
        else
          {
            gsk_buffer_printf (output,
                               "  guint i;\n"
                               "  for (i = 0; i < %u; i++)\n"
                               "    {\n"
                               "      %s_pack (%svalue->data[i], append_func, append_func_data);\n"
                               "    }\n",
                               format->v_fixed_array.length,
                               sub->any.c_func_prefix,
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
                           "    %s_pack (%svalue->data[i], append_func, append_func_data);\n",
                               sub->any.c_func_prefix,
                               sub->any.always_by_pointer ? "&" : "");

        break;
      }
    case GSKB_FORMAT_TYPE_STRUCT:
      {
        guint last_code = 0;
        gboolean ext = format->v_struct.is_extensible;
        if (ext)
          {
            gsk_buffer_printf (output,
                               "  guint unknown_index = 0;\n");
          }
        for (i = 0; i < format->v_struct.n_members; i++)
          {
            GskbFormatStructMember *member = format->v_struct.members + i;
            if (ext)
              {
                if (last_code + 1 != member->code)
                  {
                    gsk_buffer_printf (output,
                                       "  while (unknown_index < value->unknown_members.length\n"
                                       "     &&  value->unknown_members.values[unknown_members].code < %u)\n"
                                       "    {\n"
                                       "      gskb_unknown_value_pack (value->unknown_members.values + unknown_index,\n"
                                       "                               append_func, append_func_data);\n"
                                       "      unknown_index++;\n"
                                       "    }\n",
                                       member->code);
                  }
                gsk_buffer_printf (output,
                                   "  if (value->has.%s)\n"
                                   "    {\n"
                                   "      gskb_uint_pack (%u, append_func, append_func_data);\n"
                                   "      gskb_uint_pack (%s_get_packed_size (%svalue->%s), append_func, append_func_data);\n"
                                   "      %s_pack (%svalue->%s, append_func, append_func_data);\n"
                                   "    }\n",
                               member->name,
                               member->code,
                               member->format->any.c_func_prefix, member->format->any.always_by_pointer?"&":"", member->name,
                               member->format->any.c_func_prefix, member->format->any.always_by_pointer?"&":"", member->name);
                last_code = member->code;
              }
            else
              {
                gsk_buffer_printf (output,
                                   "%s  %s_pack (%svalue->%s, append_func, append_func_data);\n",
                                   ext ? "    " : "",
                                   member->format->any.c_func_prefix,
                                   member->format->any.always_by_pointer?"&":"",
                                   member->name);
              }
          }
      }
      break;
    case GSKB_FORMAT_TYPE_UNION:
      {
        char *ucname = g_ascii_strup (format->any.c_func_prefix, -1);
        gsk_buffer_printf (output,
                           "  guint32 code = value->type;\n");
        if (format->v_union.is_extensible)
          gsk_buffer_printf (output,
                             "  if (value->type != %s__UNKNOWN_VALUE)\n"
                             "    gskb_uint_pack (value->type, append_func, append_func_data);\n",
                             ucname);
        else
          gsk_buffer_printf (output,
                             "  gskb_uint_pack (value->type, append_func, append_func_data);\n");

        gsk_buffer_printf (output,
                           "  switch (value->type)\n"
                           "    {\n");
        for (i = 0; i < format->v_union.n_cases; i++)
          {
            GskbFormatUnionCase *c = format->v_union.cases + i;
            char *uccasename = g_ascii_strup (c->name, -1);
            gsk_buffer_printf (output,
                               "    case %s__%s:\n",
                               ucname, uccasename);
            if (c->format != NULL)
              {
                if (format->v_union.is_extensible)
                  gsk_buffer_printf (output,
                                     "      gskb_uint_pack (%s_get_packed_size (%svalue->info.%s), append_func, append_func_data);\n",
                                     c->format->any.c_func_prefix,
                                     c->format->any.always_by_pointer?"&":"", c->name);
                gsk_buffer_printf (output,
                                   "      %s_pack (%svalue->info.%s, append_func, append_func_data);\n"
                                   "      break;\n",
                                   c->format->any.c_func_prefix,
                                   c->format->any.always_by_pointer?"&":"", c->name);
              }
            else
              {
                if (format->v_union.is_extensible)
                  gsk_buffer_printf (output, "  guint8 zero = 0;\n"
                             "append_func (1, &zero, append_func_data);\n");
              }
            g_free (uccasename);
          }
        if (format->v_union.is_extensible)
          {
            gsk_buffer_printf (output,
                               "    case %s__UNKNOWN_VALUE:\n"
                               "      gskb_unknown_value_pack (&value->info.unknown_value,\n"
                               "                               append_func, append_func_data);\n"
                               "      break;\n",
                               ucname);
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

    case GSKB_FORMAT_TYPE_BIT_FIELDS:
      {
        gsk_buffer_printf (output,
                           "  const guint8 *unpacked = (const guint8 *) value;\n"
                           "  guint8 packed[%u];\n"
                           "  %s_pack_slab (value, packed);\n"
                           "  append_func (sizeof (packed), packed, append_func_data);\n",
                           format->any.fixed_length,
                           format->any.c_func_prefix);
        break;
      }
    }
}

/* get_packed_size */
#define return_value__get_packed_size       "guint"
static const char *type_name_pairs__get_packed_size[] = {
  "$maybe_const $type_name $maybe_ptr", "value",
  NULL
};
static void
implement__get_packed_size(GskbFormat *format,
                           const GskbCodegenConfig *config,
                           GskBuffer *output)
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
                                 "  rv += %s_get_packed_size (%svalue->data[%u]);\n",
                                 sub->any.c_func_prefix,
                                 sub->any.always_by_pointer ? "&" : "", i);
          }
        else
          {
            gsk_buffer_printf (output,
                               "  guint i, rv = 0;\n"
                               "  for (i = 0; i < %u; i++)\n"
                               "    rv += %s_get_packed_size (%svalue->data[i]);\n",
                                 format->v_fixed_array.length,
                                 sub->any.c_func_prefix,
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
        if (sub->any.fixed_length != 0)
          gsk_buffer_printf (output, "  rv += value->length * %u;\n", sub->any.fixed_length);
        else
          {
            gsk_buffer_printf (output,
                               "  guint i;\n"
                               "  for (i = 0; i < %u; i++)\n"
                               "    rv += %s_get_packed_size (%svalue->data[i]);\n",
                                 format->v_fixed_array.length,
                                 sub->any.c_func_prefix,
                                 sub->any.always_by_pointer ? "&" : "");
          }
        gsk_buffer_printf (output, "  return rv;\n");
        break;
      }
    case GSKB_FORMAT_TYPE_STRUCT:
      {
        gsk_buffer_printf (output, "  guint rv = 0;\n");
        if (format->v_struct.is_extensible)
          {
            gsk_buffer_printf (output,
                               "  {\n"
                               "    guint i = 0;\n"
                               "    for (i = 0; i < value->unknown_values.length; i++)\n"
                               "      rv += gskb_unknown_value_get_packed_size (value->unknown_values.values + i);\n"
                               "  }\n");
          }
        for (i = 0; i < format->v_struct.n_members; i++)
          {
            GskbFormatStructMember *member = format->v_struct.members + i;
            gsk_buffer_printf (output,
                               "  rv += %s_get_packed_size (%svalue->%s);\n",
                               member->format->any.c_func_prefix,
                               member->format->any.always_by_pointer?"&":"", member->name);
          }
        if (format->v_struct.is_extensible)
          gsk_buffer_printf (output, "  rv += 1;                /* 0 terminated */\n");
        gsk_buffer_printf (output, "  return rv;\n");
        break;
      }
    case GSKB_FORMAT_TYPE_UNION:
      {
        char *ucname = g_ascii_strup (format->any.c_func_prefix, -1);
        gsk_buffer_printf (output, "  switch (value->type)\n    {\n");
        for (i = 0; i < format->v_union.n_cases; i++)
          {
            GskbFormatUnionCase *c = format->v_union.cases + i;
            char *uccasename = g_ascii_strup (c->name, -1);
            gsk_buffer_printf (output, "    case %s__%s:\n",
                               ucname, uccasename);
            if (format->v_union.is_extensible)
              {
                if (c->format == NULL)
                  gsk_buffer_printf (output, "      return gskb_uint_get_packed_size (%s__%s);\n",
                                     ucname, uccasename);
                else
                  gsk_buffer_printf (output, "      return gskb_uint_get_packed_size (%s__%s) + %s_get_packed_size (%svalue->info.%s);\n",
                                     ucname, uccasename,
                                     c->format->any.c_func_prefix, c->format->any.always_by_pointer?"&":"", c->name);
              }
            else
              {
                if (c->format == NULL)
                  gsk_buffer_printf (output, "      return gskb_uint_get_packed_size (%s__%s) + 1;\n",
                                     ucname, uccasename);
                else
                  gsk_buffer_printf (output,
                                     "      {\n"
                                     "        guint subsize = %s_get_packed_size (%svalue->info.%s);\n"
                                     "        return gskb_uint_get_packed_size (%s__%s)\n"
                                     "             + gskb_uint_get_packed_size (subsize)\n"
                                     "             + subsize;\n"
                                     "      }\n",
                                     c->format->any.c_func_prefix, c->format->any.always_by_pointer?"&":"", c->name,
                                     ucname, uccasename);
              }
            g_free (uccasename);
          }
        if (format->v_union.is_extensible)
          {
            gsk_buffer_printf (output,
                               "  case %s__UNKNOWN_VALUE:\n"
                               "    return gskb_unknown_value_get_packed_size (&value->info.unknown_value);\n",
                               ucname);
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

    default:
      g_return_if_reached ();
    }
}

/* pack_slab */
#define return_value__pack_slab       "guint"
static const char *type_name_pairs__pack_slab[] = {
  "$maybe_const $type_name $maybe_ptr", "value",
  "guint8 *", "out",
  NULL
};
static void
implement__pack_slab      (GskbFormat *format,
                           const GskbCodegenConfig *config,
                           GskBuffer *output)
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
                                 "  rv += %s_pack_slab (%svalue->data[%u], out + rv);\n",
                                 sub->any.c_func_prefix,
                                 sub->any.always_by_pointer ? "&" : "", i);
          }
        else
          {
            gsk_buffer_printf (output,
                               "  guint i, rv = 0;\n"
                               "  for (i = 0; i < %u; i++)\n"
                               "    rv += %s_pack_slab (%svalue->data[i], out + rv);\n",
                                 format->v_fixed_array.length,
                                 sub->any.c_func_prefix,
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
                           "    rv += %s_pack_slab (%svalue->data[i], out + rv);\n",
                             format->v_fixed_array.length,
                             sub->any.c_func_prefix,
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
                               "  rv += %s_pack_slab (%svalue->%s, out + rv);\n",
                               member->format->any.c_func_prefix,
                               member->format->any.always_by_pointer?"&":"", member->name);
          }
        gsk_buffer_printf (output, "  return rv;\n");
        break;
      }
    case GSKB_FORMAT_TYPE_UNION:
      {
        char *ucname = g_ascii_strup (format->any.c_func_prefix, -1);
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
                                         "         return size1 + %s_pack_slab (%svalue->info.%s, out + size1);\n"
                                         "      }\n",
                                         ucname, uccasename,
                                         c->format->any.c_func_prefix, c->format->any.always_by_pointer?"&":"", c->name);
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
                           "  return gskb_%s_pack_slab (value, out);\n",
                           gskb_format_int_type_name (format->v_enum.int_type));
        break;
      }

    case GSKB_FORMAT_TYPE_BIT_FIELDS:
      {
        guint n_packed_bits = 0;
        for (i = 0; i < format->any.c_size_of; i++)
          {
            guint8 bits = format->v_bit_fields.bits_per_unpacked_byte[i];
            if (n_packed_bits % 8 + bits <= 8)
              {
                /* packs to one byte */
                if (n_packed_bits % 8 == 0)
                  {
                    /* need to initialize the byte */
                    if (bits == 8)
                      gsk_buffer_printf (output,
                                         "  packed[%u] = unpacked[%u];",
                                         n_packed_bits / 8, i);
                    else
                      gsk_buffer_printf (output,
                                         "  packed[%u] = unpacked[%u] & 0x%02x;\n",
                                         n_packed_bits / 8, i, (guint8)((1<<bits) - 1));
                  }
                else
                  {
                    gsk_buffer_printf (output,
                                       "  packed[%u] |= (unpacked[%u] & 0x%02x) << %u;\n",
                                       n_packed_bits / 8, i, (guint8)((1<<bits) - 1),
                                       n_packed_bits % 8);
                  }
              }
            else
              {
                /* straddles two bytes */
                guint bits1 = 8 - n_packed_bits % 8;
                guint bits2 = bits - bits1;
                gsk_buffer_printf (output,
                                   "  packed[%u] |= (unpacked[%u] & 0x%02x) << %u;\n"
                                   "  packed[%u] = (unpacked[%u] & 0x%02x) >> %u;\n",
                                   n_packed_bits / 8, i, ((1<<bits1)-1), n_packed_bits % 8,
                                   n_packed_bits / 8 + 1, i, ((1<<bits2)-1)<<bits1, bits1);
              }
            n_packed_bits += bits;
          }
        gsk_buffer_printf (output, "  return %u;\n", format->any.fixed_length);
        break;
      }

    default:
      g_message ("unhandled format type %u", format->type);
      g_return_if_reached ();
    }
}

/* validate_partial */
#define return_value__validate_partial     "guint"
static const char *type_name_pairs__validate_partial[] = {
  "guint", "length",
  "const guint8 *", "data",
  "GError **", "error",
  NULL
};

static void
implement__validate_partial  (GskbFormat *format,
                              const GskbCodegenConfig *config,
                              GskBuffer *output)
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
                                 "  if ((sub_used = %s_validate_partial (len - rv, data + rv, error)) == 0)\n"
                                 "    {\n"
                                 "      gsk_g_error_add_prefix (error, \"validating element #%%u of %%u\", %u, %u);\n"
                                 "      return 0;\n"
                                 "    }\n"
                                 "  rv += sub_used;\n",
                                 sub->any.c_func_prefix,
                                 i, format->v_fixed_array.length);
          }
        else
          {
            gsk_buffer_printf (output,
                               "  guint i, rv = 0;\n"
                               "  for (i = 0; i < %u; i++)\n"
                               "    if ((sub_used = %s_validate_partial (len - rv, out + rv, error)) == 0)\n"
                               "      {\n"
                                 "      gsk_g_error_add_prefix (error, \"validating element #%%u of %%u\", i, %u);\n"
                                 "      return 0;\n"
                                 "    }\n"
                                 "  rv += sub_used;\n",
                                 format->v_fixed_array.length,
                                 sub->any.c_func_prefix,
                                 format->v_fixed_array.length);
          }
        gsk_buffer_printf (output, "  return rv;\n");
        break;
      }
    case GSKB_FORMAT_TYPE_LENGTH_PREFIXED_ARRAY:
      {
        GskbFormat *sub = format->v_length_prefixed_array.element_format;
        gsk_buffer_printf (output,
                           "  guint rv, sub_used, i;\n"
                           "  guint32 n;\n"
                           "  if ((sub_used=gskb_uint_validate_unpack (len, data, &n, error)) == 0)\n"
                           "    {\n"
                           "      gsk_g_error_add_prefix (error, \"parsing length-prefix\");\n"
                           "      return 0;\n"
                           "    }\n"
                           "  rv = sub_used;\n");
        gsk_buffer_printf (output,
                           "  for (i = 0; i < n; i++)\n"
                           "    {\n"
                           "      if (!%s_validate_partial (len - rv, out + rv, &sub_used, error))\n"
                           "        {\n"
                           "          gsk_g_error_add_prefix (error, \"validating element #%%u of %%u\", i, n);\n"
                           "          return 0;\n"
                           "        }\n"
                           "      rv += sub_used;\n"
                           "    }\n",
                             sub->any.c_func_prefix);
        gsk_buffer_printf (output, "  return rv;\n");
        break;
      }
    case GSKB_FORMAT_TYPE_STRUCT:
      {
        gsk_buffer_printf (output, "  guint rv = 0;\n");
        if (format->v_struct.is_extensible)
          {
            gsk_buffer_printf (output,
                               "  guint sub_used;\n"
                               "  guint32 code, last_code = 0, sub_len;\n"
                               "  for (;;)\n"
                               "    {\n"
                               "      if ((sub_used=gskb_uint_validate_unpack (len - rv, out + rv, &code, error)) == 0)\n"
                               "        {\n"
                               "          gsk_g_error_add_prefix (error, \"error parsing member code in %%s\", \"%s\");\n"
                               "          return 0;\n"
                               "        }\n"
                               "      rv += sub_used;\n"
                               "      if (code == 0)\n"
                               "        return rv;\n"
                               "      if (code <= last_code)\n"
                               "        {\n"
                               "          g_set_error (error, \"expected extensible struct code to be ascending, got %%u then %%u\",\n"
                               "                              last_code, code);\n"
                               "          return 0;\n"
                               "        }\n"
                               "      if ((sub_used=gskb_uint_validate_unpack (len - rv, out + rv, &sub_len, error)) == 0)\n"
                               "        {\n"
                               "          gsk_g_error_add_prefix (error, \"error parsing length of member in %%s\", \"%s\");\n"
                               "          return 0;\n"
                               "        }\n"
                               "      rv += sub_used;\n"
                               "      switch (code)\n"
                               "        {\n",
                               format->any.name,
                               format->any.name);
            for (i = 0; i < format->v_struct.n_members; i++)
              {
                GskbFormatStructMember *member = format->v_struct.members + i;
                GskbFormat *sub = member->format;
                gsk_buffer_printf (output,
                                   "        case %u:\n"
                                   "          sub_used = %s_validate_partial (len - rv, out + rv, error);\n"
                                   "          if (G_UNLIKELY (sub_used == 0))\n"
                                   "            {\n"
                                   "              gsk_g_error_add_prefix (error, \"error parsing member %%s of %%s\", \"%s\", \"%s\");\n"
                                   "              return 0;\n"
                                   "            }\n"
                                   "          if (sub_len != sub_used)\n"
                                   "            {\n"
                                   "              gsk_g_error_add_prefix (error,\n"
                                   "                                      \"validated member %%s of %%s had length %%u, specified as %%u\",\n"
                                   "                                      \"%s\", \"%s\", sub_used, sub_len);\n"
                                   "              return 0;\n"
                                   "            }\n"
                                   "          rv += sub_used;\n"
                                   "          break;\n",
                                   member->code,
                                   sub->any.c_func_prefix,
                                   member->name, format->any.name,
                                   member->name, format->any.name);
              }
            gsk_buffer_printf (output,
                               "        default:\n"
                               "          rv += sub_len;\n"
                               "          break;\n"
                               "        }\n"
                               "    }\n"
                               "  g_return_val_if_reached (0);\n");
          }
        else
          {
            for (i = 0; i < format->v_struct.n_members; i++)
              {
                GskbFormatStructMember *member = format->v_struct.members + i;
                gsk_buffer_printf (output,
                                   "  if ((sub_used = %s_validate_partial (len - rv, out + rv, error)) == 0)\n"
                                   "    {\n"
                                   "      gsk_g_error_add_prefix (error, \"validating member '%%s' of %%s\", \"%s\", \"%s\");\n"
                                   "      return 0;\n"
                                   "    }\n"
                                   "  rv += sub_used;\n",
                                   member->format->any.c_func_prefix,
                                   member->name, format->any.name);
              }
            gsk_buffer_printf (output, "  return rv;\n");
          }
        break;
      }
    case GSKB_FORMAT_TYPE_UNION:
      {
        char *ucname = g_ascii_strup (format->any.c_func_prefix, -1);
        
        if (format->v_union.is_extensible)
          {
            gsk_buffer_printf (output,
                               "  guint sub_used2;\n"
                               "  guint32 len;\n"
                               "  guint32 last_code = 0;\n");
          }
        gsk_buffer_printf (output,
                           "  guint rv;\n"
                           "  guint32 type;\n"
                           "  guint sub_used;\n"
                           "  if ((sub_used=gskb_uint_validate_unpack (len, data, &type, error)) == 0)\n"
                           "    {\n"
                           "      gsk_g_error_add_prefix (error, \"error parsing union code\");\n"
                           "      return FALSE;\n"
                           "    }\n");
        if (format->v_union.is_extensible)
          {
            gsk_buffer_printf (output,
                               "  if ((sub_used2=gskb_uint_validate_unpack (len - sub_used, data + sub_used, &len, error)) == 0)\n"
                               "    {\n"
                               "      gsk_g_error_add_prefix (error, \"error parsing union code\");\n"
                               "      return FALSE;\n"
                               "    }\n"
                               "  sub_used += sub_used2;\n");
          }
        gsk_buffer_printf (output, "  switch (type)\n    {\n");
        for (i = 0; i < format->v_union.n_cases; i++)
          {
            GskbFormatUnionCase *c = format->v_union.cases + i;
            char *uccasename = g_ascii_strup (c->name, -1);
            gsk_buffer_printf (output, "    case %s__%s:\n",
                               ucname, uccasename);
            if (c->format == NULL)
              {
                if (format->v_union.is_extensible)
                  gsk_buffer_printf (output,
                                     "      if (len != 0)\n"
                                     "        {\n"
                                     "           g_set_error (error, GSK_G_ERROR_DOMAIN, GSK_ERROR_PARSE,\n"
                                     "                        \"expected length for empty union case to be 0, got %%u\",\n"
                                     "                        len);\n"
                                     "           return FALSE;\n"
                                     "        }\n");
                gsk_buffer_printf (output, "      *n_used_out = sub_used;\n"
                                           "      return TRUE;\n");
              }
            else
              {
                gsk_buffer_printf (output, "      rv = sub_used;\n"
                                           "      if ((sub_used = %s_validate_partial (len - rv, data + rv, error)) == 0)\n"
                                           "        {\n"
                                           "           gsk_g_error_add_prefix (error, \"validating case '%%s' of '%%s'\", \"%s\", \"%s\");\n"
                                           "           return FALSE;\n"
                                           "        }\n",
                                           c->format->any.c_func_prefix, c->name, format->any.name);
                if (format->v_union.is_extensible)
                  gsk_buffer_printf (output,
                                     "      if (sub_used != len)\n"
                                     "        {\n"
                                     "          g_set_error (error, GSK_G_ERROR_DOMAIN, GSK_ERROR_PARSE,\n"
         "                  \"validated length for case %%s of union %%s had length mismatch (header said %%u, got %%u)\",\n"
                                     "                       \"%s\", \"%s\", len, sub_used);\n"
                                     "          return FALSE;\n"
                                     "        }\n",
                                     c->name, format->any.name);
                gsk_buffer_printf (output,
                                   "      *n_used_out = rv + sub_used;\n"
                                   "      return TRUE;\n");
              }

            g_free (uccasename);
          }
        g_free (ucname);
        if (format->v_union.is_extensible)
          {
            gsk_buffer_printf (output,
                               "    default:\n"
                               "      *n_used_out = sub_used + len;\n"
                               "      return TRUE;\n"
                               "    }\n");
          }
        else
          {
            gsk_buffer_printf (output, "    default:\n"
                                       "      g_set_error (error, GSK_G_ERROR_DOMAIN_QUARK,\n"
                                       "                   GSK_ERROR_BAD_FORMAT,\n"
                                       "                   \"invalid tag %%u for '%%s'\", type, \"%s\");\n"
                                       "      return FALSE;\n"
                                       "    }\n", format->any.name);
          }
        break;
      }
    case GSKB_FORMAT_TYPE_ENUM:
      {
        char *ucname = g_ascii_strup (format->any.c_func_prefix, -1);
        if (format->v_enum.is_extensible)
          {
            gsk_buffer_printf (output,
                               "  return gskb_uint_validate_partial (len, data, error);\n");
            break;
          }
        gsk_buffer_printf (output,
                           "  if ((*n_used_out = gskb_uint_validate_unpack (len, data, &val, error)) == 0)\n"
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

    case GSKB_FORMAT_TYPE_BIT_FIELDS:
      if (format->v_bit_fields.total_bits % 8 != 0)
        {
          guint unused_bits = 8 - format->v_bit_fields.total_bits % 8;
          gsk_buffer_printf (output,
                             "  if ((data[%u] & 0x%02x) != 0)\n"
                             "    {\n"
                             "      g_set_error (error, GSK_G_ERROR_DOMAIN_QUARK,\n"
                             "                   GSK_ERROR_BAD_FORMAT,\n"
                             "                   \"expected few %%u bits of packed bit fields %%s to be zero\",\n"
                             "                   %u, \"%s\");\n"
                             "      return 0;\n"
                             "    }\n",
                             format->any.fixed_length-1, ((1<<unused_bits)-1) << (8-unused_bits),
                             unused_bits, format->any.name);
        }
      gsk_buffer_printf (output,
                         "return %u;\n", format->any.fixed_length);
      break;

    default:
      g_return_if_reached ();
    }
}


/* unpack */
#define return_value__unpack       "guint"
static const char *type_name_pairs__unpack[] = {
  "const guint8 *", "in",
  "$type_name *", "value_out",
  NULL
};
static void
implement_unpack_functions         (GskbFormat *format,
                                    const GskbCodegenConfig *config,
                                    gboolean with_mempool,
                                    GskBuffer *output)
{
  guint i;
  const char *mempool_suffix, *mempool_last_arg;
  if (with_mempool)
    {
      mempool_suffix = "_mempool";
      mempool_last_arg = ", mem_pool";
    }
  else
    {
      mempool_suffix = "";
      mempool_last_arg = "";
    }
  switch (format->type)
    {
    case GSKB_FORMAT_TYPE_INT:
    case GSKB_FORMAT_TYPE_FLOAT:
    case GSKB_FORMAT_TYPE_STRING:
    case GSKB_FORMAT_TYPE_ALIAS:
      g_assert_not_reached ();

    case GSKB_FORMAT_TYPE_FIXED_ARRAY:
      {
        GskbFormat *sub = format->v_fixed_array.element_format;
        gsk_buffer_printf (output, "  guint rv = 0;\n");
        if (format->v_fixed_array.length < 5)
          {
            for (i = 0; i < format->v_fixed_array.length; i++)
              gsk_buffer_printf (output,
                                 "  rv += %s_unpack%s (in + rv, &value_out->data[%u]%s);\n",
                                 sub->any.c_func_prefix, mempool_suffix, i, mempool_last_arg);
          }
        else
          {
            gsk_buffer_printf (output,
                               "  guint i;\n"
                               "  for (i = 0; i < %u; i++)\n"
                               "    rv += %s_unpack%s (in + rv, &value_out->data[i]%s);\n",
                               format->v_fixed_array.length,
                               sub->any.c_func_prefix, mempool_suffix, mempool_last_arg);
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
                           "  value_out->data = g_new (%s, n);\n",
                           sub->any.c_type_name);
        gsk_buffer_printf (output,
                           "  for (i = 0; i < n; i++)\n"
                           "    rv += %s_unpack%s (in + rv, value_out->data[i]%s);\n",
                           sub->any.c_func_prefix, mempool_suffix, mempool_last_arg);
        gsk_buffer_printf (output, "  return rv;\n");
        break;
      }
    case GSKB_FORMAT_TYPE_STRUCT:
      {
        gsk_buffer_printf (output, "  guint rv = 0;\n");
        if (format->v_struct.is_extensible)
          {
            gsk_buffer_printf (output,
                               "  GArray *unknown_members = NULL;\n"
                               "  guint sub_used;\n"
                               "  guint32 code;\n"
                               "  memset (&value_out->has, 0, sizeof (%s_Contents));\n"
                               "  for (;;)\n"
                               "    {\n"
                               "      rv += gskb_uint_unpack (data + rv, &code);\n"
                               "      if (code == 0)\n"
                               "        {\n"
                               "          if (unknown_members == NULL)\n"
                               "            {\n"
                               "              value_out->unknown_values.length = 0;\n"
                               "              value_out->unknown_values.data = NULL;\n"
                               "            }\n"
                               "          else\n"
                               "            {\n"
                               "              gsize size = unknown_members->len * sizeof (GskbUnknownValue);\n"
                               "              value_out->unknown_values.length = unknown_members->len;\n",
                               format->any.c_type_name);
            if (with_mempool)
              {
                gsk_buffer_printf (output,
                               "              value_out->unknown_members.data = gsk_mem_pool_alloc (mem_pool, size);\n");
              }
            else
              {
                gsk_buffer_printf (output,
                               "              value_out->unknown_members.data = g_malloc (size);\n");
              }
            gsk_buffer_printf (output,
                               "              memcpy (value_out->unknown_members.data, unknown_members->data, size);\n"
                               "              g_array_free (unknown_members, TRUE);\n"
                               "            }\n"
                               "          return rv;\n"
                               "        }\n"
                               "      rv = sub_used;\n"
                               "      sub_used = gskb_uint_unpack (data + rv, &sub_len);\n"
                               "      rv += sub_used;\n"
                               "      switch (code)\n"
                               "        {\n");
            for (i = 0; i < format->v_struct.n_members; i++)
              {
                GskbFormatStructMember *member = format->v_struct.members + i;
                gsk_buffer_printf (output,
                               "        case %u:\n"
                               "          g_assert (!value_out->has.%s);\n"
                               "          value_out->has.%s = 1;\n"
                               "          rv += %s_unpack%s (in + rv, &value_out->%s%s);\n"
                               "          break;\n",
                                   member->code,
                                   member->name,
                                   member->name,
                                   member->format->any.c_func_prefix,
                                   mempool_suffix, member->name, mempool_last_arg);
              }
            gsk_buffer_printf (output,
                               "        default:\n"
                               "          {\n"
                               "            GskbUnknownValue uv;\n"
                               "            uv.code = code;\n"
                               "            uv.length = length;\n"
                               "            uv.data = %slength);\n"
                               "            memcpy (uv.data, in + rv, length);\n"
                               "            rv += length;\n"
                               "          }\n"
                               "          break;\n"
                               "        }\n"
                               "    }\n",
                               with_mempool ? "gsk_mem_pool_alloc_unaligned (mem_pool, " : "g_malloc (");
          }
        else
          {
            for (i = 0; i < format->v_struct.n_members; i++)
              {
                GskbFormatStructMember *member = format->v_struct.members + i;
                gsk_buffer_printf (output,
                                   "  rv += %s_unpack%s (in + rv, &value_out->%s%s);\n",
                                   member->format->any.c_func_prefix, mempool_suffix,
                                   member->name, mempool_last_arg);
              }
            gsk_buffer_printf (output, "  return rv;\n");
          }
        break;
      }
    case GSKB_FORMAT_TYPE_UNION:
      {
        char *ucname = g_ascii_strup (format->any.c_func_prefix, -1);
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
              gsk_buffer_printf (output, "      return rv + %s_unpack%s (in + rv, &value_out->info.%s%s);\n",
                                         c->format->any.c_func_prefix, mempool_suffix, c->name, mempool_last_arg);

            g_free (uccasename);
          }
        g_free (ucname);
        gsk_buffer_printf (output, "    default:\n"
                                   "      g_return_val_if_reached (0);\n"
                                   "    }\n");
        break;
      }
    case GSKB_FORMAT_TYPE_ENUM:
      gsk_buffer_printf (output,
                         "  gskb_%s v;\n"
                         "  guint rv;\n"
                         "  rv = gskb_%s_unpack (in, &v);\n"
                         "  *value_out = v;\n"
                         "  return rv;\n",
                         gskb_format_int_type_name (format->v_enum.int_type),
                         gskb_format_int_type_name (format->v_enum.int_type));
      break;

    case GSKB_FORMAT_TYPE_BIT_FIELDS:
      {
        guint packed_bits_used = 0;
        for (i = 0; i < format->any.fixed_length; i++)
          {
            guint bits = format->v_bit_fields.bits_per_unpacked_byte[i];
            if (packed_bits_used % 8 + bits > 8)
              {
                char maybe_mask[20];
                if (bits == 8)
                  maybe_mask[0] = 0;
                else
                  g_snprintf (maybe_mask, sizeof (maybe_mask), " & 0x%02x", (1<<bits)-1);
                gsk_buffer_printf (output,
                                   "  unpacked[%u] = ((data[%u] >> %u) | (data[%u] << %u))%s;\n",
                                   i,
                                   packed_bits_used / 8, packed_bits_used % 8,
                                   packed_bits_used / 8 + 1, 8 - packed_bits_used % 8,
                                   maybe_mask);
              }
            else
              {
                char maybe_shift[20], maybe_mask[20];
                if (packed_bits_used % 8 == 0)
                  maybe_shift[0] = 0;
                else
                  g_snprintf (maybe_shift, sizeof (maybe_shift), " >> %u", packed_bits_used % 8);
                if ((packed_bits_used + bits) % 8 == 0
                  || (packed_bits_used + bits == format->v_bit_fields.total_bits))
                  maybe_mask[0] = 0;
                else
                  g_snprintf (maybe_mask, sizeof (maybe_mask), " & 0x%02x", (1<<bits)-1);
                gsk_buffer_printf (output,
                                   "  unpacked[%u] = ((data[%u]%s)%s);\n",
                                   i, packed_bits_used/8, maybe_shift, maybe_mask);
              }
            packed_bits_used += bits;
          }
      }

    default:
      g_return_if_reached ();
    }
}
static void
implement__unpack (GskbFormat *format,
                   const GskbCodegenConfig *config,
                   GskBuffer *output)
{
  implement_unpack_functions (format, config, FALSE, output);
}

/* unpack_mempool */
#define return_value__unpack_mempool       "guint"
static const char *type_name_pairs__unpack_mempool[] = {
  "const guint8 *", "in",
  "$type_name *", "value_out",
  "GskMemPool *", "mem_pool",
  NULL
};
static void
implement__unpack_mempool (GskbFormat *format,
                           const GskbCodegenConfig *config,
                           GskBuffer *output)
{
  implement_unpack_functions (format, config, TRUE, output);
}

/* destruct */
#define return_value__destruct       "void"
static const char *type_name_pairs__destruct[] = {
  "$type_name *", "value",
  NULL
};
static void
implement__destruct (GskbFormat *format,
                     const GskbCodegenConfig *config,
                     GskBuffer *output)
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
                                 "  rv += %s_destruct (&value->data[%u]);\n",
                                 sub->any.c_func_prefix, i);
          }
        else
          {
            gsk_buffer_printf (output,
                               "  guint i;\n"
                               "  for (i = 0; i < %u; i++)\n"
                               "    rv += %s_destruct (&value->data[i]);\n",
                               format->v_fixed_array.length,
                               sub->any.c_func_prefix);
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
                             "    %s_destruct (value->data[i]);\n",
                             sub->any.c_func_prefix);
        gsk_buffer_printf (output, "  g_free (value->data);\n");
        break;
      }
    case GSKB_FORMAT_TYPE_STRUCT:
      {
        for (i = 0; i < format->v_struct.n_members; i++)
          {
            GskbFormatStructMember *member = format->v_struct.members + i;
            gsk_buffer_printf (output,
                               "  %s_destruct (&value->%s);\n",
                               member->format->any.c_func_prefix,
                               member->name);
          }
        break;
      }
    case GSKB_FORMAT_TYPE_UNION:
      {
        char *ucname = g_ascii_strup (format->any.c_func_prefix, -1);
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
              gsk_buffer_printf (output, "      %s_destruct (&value->info.%s);\n"
                                         "      break;\n",
                                         c->format->any.c_func_prefix, c->name);

            g_free (uccasename);
          }
        g_free (ucname);
        gsk_buffer_printf (output, "    default:\n"
                                   "      g_return_if_reached ();\n"
                                   "    }\n");
        break;
      }
    default:
      g_error ("called on to implement destruct for format type %s",
               gskb_format_type_name (format->type));
    }
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
  ENTRY (VALIDATE_PARTIAL, validate_partial),
  ENTRY (UNPACK, unpack),
  ENTRY (UNPACK_MEMPOOL, unpack_mempool),
  ENTRY (DESTRUCT, destruct)
};

static void
dissect_type_str (const char *type,
                  guint      *stripped_type_len_out,
                  guint      *n_stars_out)
{
  guint i, n_stars = 0;
  for (i = 0; type[i] == '_' || g_ascii_isalnum (type[i]) || type[i] == ' '; i++)
    ;
  while (i > 0 && type[i-1] == ' ')
    i--;
  *stripped_type_len_out = i;
  for (     ; type[i] != 0; i++)
    if (type[i] == '*')
      n_stars++;
  *n_stars_out = n_stars;
}

static void
generic_start_function (GskBuffer *buffer,
                        const char *qualifiers,
                        const char *ret_value,
                        const char *func_name,
                        gboolean    include_semicolon,
                        guint n_args,
                        char **args_type_name_pairs)
{
  guint func_name_len = strlen (func_name);
  guint max_stripped_type_len = 0, max_stars = 0;
  guint i;
  for (i = 0; i < n_args; i++)
    {
      guint stl, s;
      dissect_type_str (args_type_name_pairs[2*i], &stl, &s);
      max_stripped_type_len = MAX (max_stripped_type_len, stl);
      max_stars = MAX (max_stars, s);
    }

  if (qualifiers[0])
    gsk_buffer_printf (buffer, "%s %s\n", qualifiers, ret_value);
  else
    gsk_buffer_printf (buffer, "%s\n", ret_value);
  if (n_args == 0)
    {
      gsk_buffer_printf (buffer, "%s (void);\n", func_name);
    }
  else
    {
      gsk_buffer_printf (buffer, "%s (", func_name);
      for (i = 0; i < n_args; i++)
        {
          guint stl, s;
          dissect_type_str (args_type_name_pairs[2*i], &stl, &s);
          gsk_buffer_append (buffer, args_type_name_pairs[2*i], stl);
          gsk_buffer_append_repeated_char (buffer, ' ', max_stripped_type_len - stl + 1 + max_stars - s);
          gsk_buffer_append_repeated_char (buffer, '*', s);
          gsk_buffer_append_string (buffer, args_type_name_pairs[2*i+1]);
          if (i + 1 == n_args)
            gsk_buffer_printf (buffer, ")%s\n",
                               include_semicolon ? ";" : "");
          else
            {
              gsk_buffer_append_string (buffer, ",\n");
              gsk_buffer_append_repeated_char (buffer, ' ', func_name_len + 2);
            }
        }
    }
}
static void
start_function (const char *qualifiers,
                gboolean    include_semicolon,
                GskbFormat *format,
                const GskbCodegenConfig *config,
                GskbCodegenOutputFunction function,
                GskBuffer *buffer)
{
  char *func_name = g_strdup_printf ("%s_%s",
                                     format->any.c_func_prefix,
                                     output_function_info[function].name);
  const char *const*raw_args = output_function_info[function].type_name_pairs_templates;
  char **substituted_names;
  guint i;
  guint n_args = g_strv_length ((char**)raw_args) / 2;
  substituted_names = g_new (char *, n_args * 2 + 1);
  for (i = 0; i < n_args * 2; i++)
    {
      GString *str = g_string_new ("");
      const char *at = raw_args[i];
      while (*at)
        {
          if (*at == '$')
            {
              guint len = 0;
              at++;
              while (at[len] && (at[len] == '_' || g_ascii_isalnum(at[len])))
                len++;
              if (len == strlen ("maybe_const") && strncmp ("maybe_const", at, len) == 0)
                {
                  if (format->any.always_by_pointer)
                    g_string_append (str, "const");
                }
              else if (len == strlen ("maybe_ptr") && strncmp ("maybe_ptr", at, len) == 0)
                {
                  if (format->any.always_by_pointer)
                    g_string_append (str, "*");
                }
              else if (len == strlen ("type_name") && strncmp ("type_name", at, len) == 0)
                {
                  g_string_append (str, format->any.c_type_name);
                }
              else
                {
                  g_error ("unknown template value $%.*s", len, at);
                }
              at += len;
            }
          else if (*at == ' ')
            {
              if (str->len != 0 && str->str[str->len-1] != ' ')
                g_string_append_c (str, ' ');
              at++;
            }
          else
            {
              g_string_append_c (str, *at++);
            }
        }
      if (str->len > 0 && str->str[str->len-1] == ' ')
        g_string_set_size (str, str->len - 1);
      substituted_names[i] = g_string_free (str, FALSE);
    }
  substituted_names[2*n_args] = NULL;
  generic_start_function (buffer, qualifiers,
                          output_function_info[function].ret_value,
                          func_name,
                          include_semicolon,
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

static void
gskb_format_codegen_emit_function      (GskbFormat *format,
                                        GskbCodegenOutputFunction function,
                                        const char *qualifiers,
                                        gboolean emit_implementation,
                                        const GskbCodegenConfig *config,
                                        GskBuffer *output)
{
  const char *name = output_function_info[function].name;
  g_assert (output_function_info[function].output_function == function);
  if (is_fundamental (format))
    {
      g_assert_not_reached ();
      //if (emit_implementation)
      //  return;
      //gsk_buffer_printf (output,
      //                   "#define %s%s_%s gskb_%s_%s\n",
      //                   config->func_prefix, format->any.lc_name, name,
      //                   format->any.lc_name, name);
      return;
    }
  if (format->type == GSKB_FORMAT_TYPE_ALIAS)
    {
      gsk_buffer_printf (output,
                         "#define %s_%s %s_%s\n",
                         format->any.c_func_prefix, name,
                         format->v_alias.format->any.c_func_prefix, name);
      return;
    }

  /* misc hacks. */
  switch (function)
    {
    case GSKB_CODEGEN_OUTPUT_GET_PACKED_SIZE:
      if (format->any.fixed_length > 0)
        {
          gsk_buffer_printf (output,
                             "#define %s_get_packed_size(value) %u\n",
                             format->any.c_func_prefix,
                             format->any.fixed_length);
          return;
        }
      if (format->type == GSKB_FORMAT_TYPE_ENUM)
        {
          gsk_buffer_printf (output,
                             "#define %s_get_packed_size(value) gskb_uint_get_packed_size(value)\n",
                             format->any.c_func_prefix);
          return;
        }
      break;
    case GSKB_CODEGEN_OUTPUT_DESTRUCT:
      if (!format->any.requires_destruct)
        {
          gsk_buffer_printf (output,
                             "#define %s_destruct(value)   /* do nothing */\n",
                             format->any.c_func_prefix);
          return;
        }
      break;

    default:
      break;
    }

  start_function (qualifiers, !emit_implementation,
                  format, config, function, output);
  if (emit_implementation)
    {
      gsk_buffer_append_string (output, "\n{\n");
      output_function_info[function].implementor (format, config, output);
      gsk_buffer_append_string (output, "}\n\n");
    }
  return;
}


static void
gskb_format_codegen_emit_functions(GskbFormat *format,
                                    const GskbCodegenConfig *config,
                                    gboolean emit_implementation,
                                    GskBuffer *output)
{
  GskbCodegenOutputFunction i;
  const char *qualifiers;
  if (config->all_static)
    qualifiers = "static ";
  else
    qualifiers = "";
  for (i = 0; i < GSKB_N_CODEGEN_OUTPUT_FUNCTIONS; i++)
    gskb_format_codegen_emit_function (format, i, qualifiers,
                                       emit_implementation, config, output);
}
static void
gskb_format_codegen__emit_function_decls (GskbFormat *format,
                                          const GskbCodegenConfig *config,
                                          GskBuffer *output)
{
  gskb_format_codegen_emit_functions (format, config, FALSE, output);
}
static void
gskb_format_codegen__emit_function_impls (GskbFormat *format,
                                          const GskbCodegenConfig *config,
                                          GskBuffer *output)
{
  gskb_format_codegen_emit_functions (format, config, TRUE, output);
}

typedef void (*Emitter) (GskbFormat *format,
                         const GskbCodegenConfig *config,
                         GskBuffer *output);
/* must match order of GskbCodegenSection */
static Emitter emitters[] = {
  gskb_format_codegen__emit_typedefs,
  gskb_format_codegen__emit_structures,
  gskb_format_codegen__emit_format_decls,
  gskb_format_codegen__emit_format_private_decls,
  gskb_format_codegen__emit_format_impls,
  gskb_format_codegen__emit_function_decls,
  gskb_format_codegen__emit_function_impls
};
void        gskb_format_codegen        (GskbFormat *format,
                                        GskbCodegenSection phase,
                                        const GskbCodegenConfig *config,
                                        GskBuffer *output)
{
  emitters[phase] (format, config, output);
}

static void
render_gskb_format (gconstpointer entry_data,
                    GskBuffer *output)
{
  const GskbFormat *format = entry_data;
  gsk_buffer_printf (output, "%s_format", format->any.c_func_prefix);
}

void        gskb_namespace_codegen     (GskbNamespace *ns,
                                        GskbCodegenSection section,
                                        const GskbCodegenConfig *config,
                                        GskBuffer *output)
{
  const char *section_comment = NULL;
  guint i;

  switch (section)
    {
    case GSKB_CODEGEN_SECTION_TYPEDEFS:
      section_comment = "typedefs";
      break;
    case GSKB_CODEGEN_SECTION_STRUCTURES:
      section_comment = "structures";
      break;
    case GSKB_CODEGEN_SECTION_FORMAT_DECLS:
      section_comment = "format declarations";
      break;
    case GSKB_CODEGEN_SECTION_FORMAT_PRIVATE_DECLS:
      section_comment = "format private declarations";
      break;
    case GSKB_CODEGEN_SECTION_FORMAT_IMPLS:
      section_comment = "format implementations";
      break;
    case GSKB_CODEGEN_SECTION_FUNCTION_PROTOTYPES:
      section_comment = "function prototypes";
      break;
    case GSKB_CODEGEN_SECTION_FUNCTION_IMPLS:
      section_comment = "function implementations";
      break;
    case GSKB_CODEGEN_SECTION_NAMESPACE_DECL:
      section_comment = "namespace declaration";
      break;
    case GSKB_CODEGEN_SECTION_NAMESPACE_IMPL:
      section_comment = "namespace implementation";
      break;
    }
  g_assert (section_comment != NULL);
  gsk_buffer_printf (output, "\n/* %s */\n", section_comment);

  if (section == GSKB_CODEGEN_SECTION_NAMESPACE_DECL)
    {
      gsk_buffer_printf (output, "extern GskbNamespace %s_namespace;\n",
                         ns->c_func_prefix);
    }
  else if (section == GSKB_CODEGEN_SECTION_NAMESPACE_IMPL)
    {
      char *tmp;
      g_assert (!ns->is_writable);

      /* write string table */
      tmp = g_strdup_printf ("%s_name_to_format", ns->c_func_prefix);
      gskb_str_table_print (ns->name_to_format,
                            FALSE,
                            tmp,
                            "GskbFormat *",
                            render_gskb_format,
                            "sizeof (GskbFormat *)",
                            "GSKB_ALIGNOF_POINTER",
                            output);
      g_free (tmp);

      /* write formats array */
      gsk_buffer_printf (output,
                         "static GskbFormat *%s_formats[] =\n"
                         "{\n",
                         ns->c_func_prefix);
      for (i = 0; i < ns->n_formats; i++)
        {
          gsk_buffer_printf (output,
                             "  %s_format,\n",
                             ns->formats[i]->any.c_func_prefix);
        }
      gsk_buffer_printf (output,
                         "};\n");

      gsk_buffer_printf (output,
                         "GskbNamespace %s_namespace =\n"
                         "{\n"
                         "  1,    /* ref_count */\n"
                         "  &%s_name_to_format,\n"
                         "  \"%s\", \"%s\", \"%s\",\n"
                         "  G_N_ELEMENTS (%s_formats),\n"
                         "  %s_formats,\n"
                         "  G_N_ELEMENTS (%s_formats),\n"
                         "  1,    /* is_global */\n"
                         "  0     /* !is_writable */\n"
                         "};\n",
                         ns->c_func_prefix,
                         ns->c_func_prefix,
                         ns->name, ns->c_func_prefix, ns->c_type_prefix,
                         ns->c_func_prefix,
                         ns->c_func_prefix,
                         ns->c_func_prefix);

    }
  else
    {
      for (i = 0; i < ns->n_formats; i++)
        gskb_format_codegen (ns->formats[i], section, config, output);
    }
}
