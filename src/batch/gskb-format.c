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

#include "gskb-format.h"
#include "gskb-config.h"
#include "gskb-fundamental-formats.h"
#include "gskb-utils.h"
#include "../gskerror.h"

/* align an offset to 'alignment';
   note that 'alignment' is evaluated twice!
   only works if alignment is a power-of-two. */
#define ALIGN(unaligned_offset, alignment)          \
     ( ((unaligned_offset)+((alignment)-1))         \
                & ~((alignment)-1)          )

#define FOREACH_INT_TYPE(macro) \
  macro(INT8, int8) \
  macro(INT16, int16) \
  macro(INT32, int32) \
  macro(INT64, int64) \
  macro(UINT8, uint8) \
  macro(UINT16, uint16) \
  macro(UINT32, uint32) \
  macro(UINT64, uint64) \
  macro(INT, int) \
  macro(UINT, uint) \
  macro(LONG, long) \
  macro(ULONG, ulong) \
  macro(BIT, bit) 
#define FOREACH_FLOAT_TYPE(macro) \
  macro(FLOAT32, float32) \
  macro(FLOAT64, float64)
static char *
mixed_to_lc (const char *name)
{
  GString *rv = g_string_new ("");
  gboolean last_was_upper = TRUE;
  while (*name)
    {
      if (g_ascii_isupper (*name))
        {
          if (!last_was_upper)
            g_string_append_c (rv, '_');
          last_was_upper = TRUE;
          g_string_append_c (rv, g_ascii_tolower (*name));
        }
      else
        {
          g_string_append_c (rv, *name);
          last_was_upper = FALSE;
        }
      name++;
    }
  return g_string_free (rv, FALSE);
}
static char *
lc_to_mixed (const char *str)
{
  GString *rv = g_string_new ("");
  gboolean uc_next = TRUE;
  while (*str)
    {
      if (*str == '_')
        uc_next = TRUE;
      else
        {
          if (uc_next)
            {
              g_string_append_c (rv, g_ascii_toupper (*str));
              uc_next = FALSE;
            }
          else
            g_string_append_c (rv, *str);
        }
      str++;
    }
  return g_string_free (rv, FALSE);
}

/**
 * gskb_format_fixed_array_new:
 * @length: number of elements if the format.
 * @element_format: the format of each element in the array.
 *
 * Create a fixed-length array format.
 *
 * returns: a GskbFormat representing an array with a constant
 * number of elements.
 */
GskbFormat *
gskb_format_fixed_array_new (guint length,
                             GskbFormat *element_format)
{
  GskbFormatFixedArray *rv = g_new0 (GskbFormatFixedArray, 1);
  guint i;
  g_return_val_if_fail (length > 0, NULL);
  rv->base.type = GSKB_FORMAT_TYPE_FIXED_ARRAY;
  rv->base.ref_count = 1;
  if (element_format->any.name == NULL)
    {
      rv->base.name = NULL;
      rv->base.TypeName = NULL;
      rv->base.lc_name = NULL;
    }
  else
    {
      rv->base.name = g_strdup_printf ("%s_Array%u", element_format->any.TypeName, length);
      rv->base.TypeName = g_strdup (rv->base.name);
      rv->base.lc_name = g_strdup_printf ("%s_array%u", element_format->any.lc_name, length);
    }
  rv->base.ctype = GSKB_FORMAT_CTYPE_COMPOSITE;
  rv->base.c_size_of = length * element_format->any.c_size_of;
  rv->base.c_align_of = element_format->any.c_align_of;
  rv->base.always_by_pointer = 1;
  rv->base.requires_destruct = element_format->any.requires_destruct;
  rv->base.is_global = 0;
  rv->base.fixed_length = element_format->any.fixed_length * length;
  rv->base.n_members = length;
  rv->base.members = g_new (GskbFormatCMember, length);
  for (i = 0; i < length; i++)
    {
      rv->base.members[i].name = g_strdup_printf ("%u", i);
      rv->base.members[i].ctype = element_format->any.ctype;
      rv->base.members[i].format = gskb_format_ref (element_format);
      rv->base.members[i].c_offset_of = element_format->any.c_size_of * i;
    }
  rv->length = length;
  rv->element_format = gskb_format_ref (element_format);
  return (GskbFormat *) rv;
}

/**
 * gskb_format_length_prefixed_array_new:
 * @element_format: the format of each element in the array.
 *
 * Create a variable-length array Format.
 * The array is prefixed with a varuint32 that gives
 * the number of elements.
 *
 * returns: a GskbFormat representing an array with a variable number of elements.
 */
GskbFormat *
gskb_format_length_prefixed_array_new (GskbFormat *element_format)
{
  GskbFormatLengthPrefixedArray *rv = g_new0 (GskbFormatLengthPrefixedArray, 1);
  rv->base.type = GSKB_FORMAT_TYPE_LENGTH_PREFIXED_ARRAY;
  rv->base.ref_count = 1;
  if (element_format->any.name == NULL)
    {
      rv->base.name = NULL;
      rv->base.TypeName = NULL;
      rv->base.lc_name = NULL;
    }
  else
    {
      rv->base.name = g_strdup_printf ("%s_Array", element_format->any.TypeName);
      rv->base.TypeName = g_strdup (rv->base.name);
      rv->base.lc_name = g_strdup_printf ("%s_array", element_format->any.lc_name);
    }
  rv->base.ctype = GSKB_FORMAT_CTYPE_COMPOSITE;
  typedef struct { guint32 len; gpointer data; } StdArray;
  rv->base.c_size_of = sizeof (StdArray);
  rv->base.c_align_of = MAX (GSKB_ALIGNOF_UINT32, GSKB_ALIGNOF_POINTER);
  rv->base.always_by_pointer = 1;
  rv->base.requires_destruct = TRUE;
  rv->base.is_global = 0;
  rv->base.fixed_length = 0;
  rv->base.n_members = 2;
  rv->base.members = g_new (GskbFormatCMember, 2);
  rv->base.members[0].name = g_strdup ("length");
  rv->base.members[0].ctype = GSKB_FORMAT_CTYPE_UINT32;
  rv->base.members[0].format = gskb_uint_format;
  rv->base.members[0].c_offset_of = 0;
  rv->base.members[1].name = g_strdup ("data");
  rv->base.members[1].ctype = GSKB_FORMAT_CTYPE_ARRAY_POINTER;
  rv->base.members[1].format = NULL;
  rv->base.members[1].c_offset_of = G_STRUCT_OFFSET (StdArray, data);
  rv->element_format = gskb_format_ref (element_format);
  return (GskbFormat *) rv;
}

/**
 * gskb_format_struct_new:
 * @name: 
 * @n_members: 
 * @members:
 *
 * Create a packed structure Format.
 *
 * returns: a GskbFormat representing the structure.
 */
GskbFormat *
gskb_format_struct_new (const char *name,
                        gboolean is_extensible,
                        guint n_members,
                        GskbFormatStructMember *members,
                        GError       **error)
{
  GskbFormatStruct *rv;
  guint i;
  guint size;
  guint align_of;
  gboolean requires_destruct;
  gboolean is_fixed;
  guint fixed_length;
  if (n_members == 0)
    {
      g_set_error (error, GSK_G_ERROR_DOMAIN,
                   GSK_ERROR_INVALID_ARGUMENT,
                   "structure must have at least one member (%s)",
                   name ? name : "anonymous struct");
      return NULL;
    }
  {
    GHashTable *dup_check = g_hash_table_new (g_str_hash, g_str_equal);
    for (i = 0; i < n_members; i++)
      {
        if (members[i].name == NULL)
          {
            g_set_error (error, GSK_G_ERROR_DOMAIN,
                         GSK_ERROR_INVALID_ARGUMENT,
                         "member %u was unnamed -- not allowed (%s)",
                         i, name ? name : "anonymous struct");
            g_hash_table_destroy (dup_check);
            return NULL;
          }
        if (g_hash_table_lookup (dup_check, members[i].name) != NULL)
          {
            g_set_error (error, GSK_G_ERROR_DOMAIN,
                         GSK_ERROR_INVALID_ARGUMENT,
                         "non-unique member name %s -- not allowed (%s)",
                         members[i].name, name ? name : "anonymous struct");
            g_hash_table_destroy (dup_check);
            return NULL;
          }
        g_hash_table_insert (dup_check, (gpointer) members[i].name, members + i);
      }
    g_hash_table_destroy (dup_check);
  }

  rv = g_new0 (GskbFormatStruct, 1);
  rv->base.type = GSKB_FORMAT_TYPE_STRUCT;
  rv->base.ref_count = 1;
  rv->base.ctype = GSKB_FORMAT_CTYPE_COMPOSITE;

  /* PORTABILITY:  in order for this to work,
     the ABI of your compilers must be pretty sane wrt to
     structure packing. */
  size = 0;
  align_of = 1;
  requires_destruct = FALSE;
  is_fixed = TRUE;
  fixed_length = 0;
  rv->base.members = g_new (GskbFormatCMember, n_members);
  for (i = 0; i < n_members; i++)
    {
      guint member_align_of = members[i].format->any.c_align_of;
      size = ALIGN (size, member_align_of);
      rv->base.members[i].name = g_strdup (members[i].name);
      rv->base.members[i].ctype = members[i].format->any.ctype;
      rv->base.members[i].format = gskb_format_ref (members[i].format);
      rv->base.members[i].c_offset_of = size;
      align_of = MAX (align_of, member_align_of);
      if (members[i].format->any.requires_destruct)
        requires_destruct = TRUE;
      if (members[i].format->any.fixed_length)
        fixed_length += members[i].format->any.fixed_length;
      else
        is_fixed = FALSE;
    }
  size += align_of - 1;
  size &= ~(align_of - 1);
  rv->base.c_size_of = size;
  rv->base.c_align_of = size;
  rv->base.always_by_pointer = 1;
  rv->base.requires_destruct = requires_destruct;
  rv->base.is_global = 0;
  rv->base.fixed_length = is_fixed ? fixed_length : 0;
  rv->base.n_members = n_members;
  rv->n_members = n_members;
  rv->members = g_new (GskbFormatStructMember, n_members);
  rv->is_extensible = is_extensible;
  for (i = 0; i < n_members; i++)
    {
      rv->members[i].name = g_strdup (members[i].name);
      rv->members[i].format = gskb_format_ref (members[i].format);
    }

  /* generate name => index map */
  {
    gint32 *indices = g_new (gint32, n_members);
    GskbStrTableEntry *entries = g_new (GskbStrTableEntry, n_members);
    for (i = 0; i < n_members; i++)
      {
        entries[i].str = members[i].name;
        entries[i].entry_data = indices + i;
        indices[i] = i;
      }
    rv->name_to_index = gskb_str_table_new (sizeof (gint32), n_members, entries);
    g_free (indices);
    g_free (entries);
  }

  if (name != NULL)
    gskb_format_name_anonymous ((GskbFormat*)rv, name);
  return (GskbFormat *) rv;
}

GskbFormat *
gskb_format_union_new (const char *name,
                       gboolean is_extensible,
                       GskbFormatIntType int_type,
                       guint n_cases,
                       GskbFormatUnionCase *cases,
                       GError       **error)
{
  GskbFormatUnion *rv;
  guint info_align, info_size;
  guint i;
  GskbFormat *type_format;

  if (n_cases == 0)
    {
      g_set_error (error, GSK_G_ERROR_DOMAIN,
                   GSK_ERROR_INVALID_ARGUMENT,
                   "union must have at least one case (%s)",
                   name ? name : "anonymous union");
      return NULL;
    }
  {
    GHashTable *dup_name_check = g_hash_table_new (g_str_hash, g_str_equal);
    GHashTable *dup_value_check = g_hash_table_new (NULL, NULL);
    for (i = 0; i < n_cases;i ++)
      {
        if (cases[i].name == NULL)
          {
            g_set_error (error, GSK_G_ERROR_DOMAIN,
                         GSK_ERROR_INVALID_ARGUMENT,
                         "case %u was unnamed -- not allowed (%s)",
                         i, name ? name : "anonymous union");
            g_hash_table_destroy (dup_name_check);
            g_hash_table_destroy (dup_value_check);
            return NULL;
          }
        if (g_hash_table_lookup (dup_name_check, cases[i].name) != NULL)
          {
            g_set_error (error, GSK_G_ERROR_DOMAIN,
                         GSK_ERROR_INVALID_ARGUMENT,
                         "non-unique member name %s -- not allowed (%s)",
                         cases[i].name, name ? name : "anonymous union");
            g_hash_table_destroy (dup_name_check);
            g_hash_table_destroy (dup_value_check);
            return NULL;
          }
        if (g_hash_table_lookup (dup_value_check, GUINT_TO_POINTER (cases[i].code)) != NULL)
          {
            g_set_error (error, GSK_G_ERROR_DOMAIN,
                         GSK_ERROR_INVALID_ARGUMENT,
                         "non-unique member value %u -- not allowed (%s)",
                         cases[i].code, name ? name : "anonymous union");
            g_hash_table_destroy (dup_name_check);
            g_hash_table_destroy (dup_value_check);
            return NULL;
          }
        g_hash_table_insert (dup_name_check, (gpointer) cases[i].name, cases + i);
        g_hash_table_insert (dup_value_check, GUINT_TO_POINTER (cases[i].code), cases + i);
      }
    g_hash_table_destroy (dup_name_check);
    g_hash_table_destroy (dup_value_check);
  }

  /* XXX: this goes through very similar attempts at perfect hashing.
     combine the work.  perhaps using a private api. */
  {
    GskbFormatEnumValue *ev = g_new (GskbFormatEnumValue, n_cases);
    for (i = 0 ; i < n_cases; i++)
      {
        ev[i].name = cases[i].name;
        ev[i].code = cases[i].code;
      }
    type_format = gskb_format_enum_new (NULL, is_extensible, GSKB_FORMAT_INT_UINT, n_cases, ev, error);
    g_assert (type_format != NULL);
    g_free (ev);
  }

  rv = g_new0 (GskbFormatUnion, 1);
  rv->base.type = GSKB_FORMAT_TYPE_UNION;
  rv->base.ref_count = 1;
  rv->base.ctype = GSKB_FORMAT_CTYPE_COMPOSITE;

  rv->base.members[0].name = g_strdup ("type");
  rv->base.members[0].ctype = GSKB_FORMAT_CTYPE_UINT32;
  rv->base.members[0].c_offset_of = 0;
  rv->base.members[0].format = gskb_format_ref (type_format);

  /* requires not only that the structure packing system is fairly normal,
     also requires that c unions have alignment equal to the their
     maximum members alignment.  It also requires that anonymously typed unions
     have the same ABI and normal pre-declared unions. */
  info_align = GSKB_ALIGNOF_UINT32;
  info_size = 0;
  for (i = 0 ; i < n_cases; i++)
    if (cases[i].format != NULL)
      {
        info_align = MAX (info_align, cases[i].format->any.c_align_of);
        info_size = MAX (info_size, cases[i].format->any.c_size_of);
      }
  rv->base.members[1].name = g_strdup ("info");
  rv->base.members[1].ctype = GSKB_FORMAT_CTYPE_UNION_DATA;
  rv->base.members[1].c_offset_of = ALIGN (sizeof (guint32), info_align);
  rv->base.members[1].format = NULL;
  rv->base.c_align_of = MAX (GSKB_ALIGNOF_UINT32, info_align);
  rv->base.c_size_of = rv->base.members[1].c_offset_of + ALIGN (info_size, info_align);
  info_size += (info_align - 1);
  info_size &= ~(info_align - 1);
  rv->type_format = type_format;                /* takes ownership */
  rv->is_extensible = is_extensible;

  if (name != NULL)
    gskb_format_name_anonymous ((GskbFormat*)rv, name);

  return (GskbFormat *) rv;
}

GskbFormat *
gskb_format_enum_new  (const char *name,
                       gboolean    is_extensible,
                       GskbFormatIntType int_type,
                       guint n_values,
                       GskbFormatEnumValue *values,
                       GError **error)
{
  GskbFormatEnum *rv;
  gint32 *indices;
  GskbStrTableEntry *str_table_entries;
  GskbUIntTableEntry *uint_table_entries;
  guint i;
  if (int_type != GSKB_FORMAT_INT_UINT8
   && int_type != GSKB_FORMAT_INT_UINT16
   && int_type != GSKB_FORMAT_INT_UINT32
   && int_type != GSKB_FORMAT_INT_UINT)
    {
      g_set_error (error, GSK_G_ERROR_DOMAIN,
                   GSK_ERROR_INVALID_ARGUMENT,
                   "invalid int-type %s for enum (%s)",
                   gskb_format_int_type_name (int_type),
                   name ? name : "anonymous enum");
      return NULL;
    }
  {
    GHashTable *dup_name_check = g_hash_table_new (g_str_hash, g_str_equal);
    GHashTable *dup_value_check = g_hash_table_new (NULL, NULL);
    for (i = 0; i < n_values;i ++)
      {
        if (values[i].name == NULL)
          {
            g_set_error (error, GSK_G_ERROR_DOMAIN,
                         GSK_ERROR_INVALID_ARGUMENT,
                         "value %u was unnamed -- not allowed (%s)",
                         i, name ? name : "anonymous enum");
            g_hash_table_destroy (dup_name_check);
            g_hash_table_destroy (dup_value_check);
            return NULL;
          }
        if (g_hash_table_lookup (dup_name_check, values[i].name) != NULL)
          {
            g_set_error (error, GSK_G_ERROR_DOMAIN,
                         GSK_ERROR_INVALID_ARGUMENT,
                         "non-unique member name %s -- not allowed (%s)",
                         values[i].name, name ? name : "anonymous enum");
            g_hash_table_destroy (dup_name_check);
            g_hash_table_destroy (dup_value_check);
            return NULL;
          }
        if (g_hash_table_lookup (dup_value_check, GUINT_TO_POINTER (values[i].code)) != NULL)
          {
            g_set_error (error, GSK_G_ERROR_DOMAIN,
                         GSK_ERROR_INVALID_ARGUMENT,
                         "non-unique member value %u -- not allowed (%s)",
                         values[i].code, name ? name : "anonymous enum");
            g_hash_table_destroy (dup_name_check);
            g_hash_table_destroy (dup_value_check);
            return NULL;
          }
        g_hash_table_insert (dup_name_check, (gpointer) values[i].name, values + i);
        g_hash_table_insert (dup_value_check, GUINT_TO_POINTER (values[i].code), values + i);
      }
    g_hash_table_destroy (dup_name_check);
    g_hash_table_destroy (dup_value_check);
  }


  rv = g_new0 (GskbFormatEnum, 1);
  rv->base.type = GSKB_FORMAT_TYPE_ENUM;
  rv->base.ref_count = 1;
  switch (int_type)
    {
    case GSKB_FORMAT_INT_UINT8:
      rv->base.fixed_length = 1;
      break;
    case GSKB_FORMAT_INT_UINT16:
      rv->base.fixed_length = 2;
      break;
    case GSKB_FORMAT_INT_UINT32:
      rv->base.fixed_length = 4;
      break;
    case GSKB_FORMAT_INT_UINT:
      rv->base.fixed_length = 0;
      break;
    default:
      g_assert_not_reached ();
    }
  rv->base.ctype = GSKB_FORMAT_CTYPE_UINT32;
  rv->base.c_size_of = 4;
  rv->base.c_align_of = GSKB_ALIGNOF_UINT32;
  rv->int_type = int_type;
  rv->n_values = n_values;
  rv->values = g_new (GskbFormatEnumValue, n_values);
  indices = g_new (gint32, n_values);
  str_table_entries = g_new (GskbStrTableEntry, n_values);
  uint_table_entries = g_new (GskbUIntTableEntry, n_values);
  for (i = 0; i < n_values; i++)
    {
      rv->values[i].code = values[i].code;
      rv->values[i].name = g_strdup (values[i].name);
      indices[i] = i;
      str_table_entries[i].str = values[i].name;
      str_table_entries[i].entry_data = indices + i;
      uint_table_entries[i].value = values[i].code;
      uint_table_entries[i].entry_data = indices + i;
    }
  rv->name_to_index = gskb_str_table_new (sizeof (gint32), n_values, str_table_entries);
  rv->value_to_index = gskb_uint_table_new (sizeof (gint32), n_values, uint_table_entries);
  g_free (indices);
  g_free (str_table_entries);
  g_free (uint_table_entries);
  if (name != NULL)
    gskb_format_name_anonymous ((GskbFormat*)rv, name);
  return (GskbFormat *) rv;
}

/* methods on certain formats */
GskbFormatStructMember *gskb_format_struct_find_member (GskbFormat *format,
                                                        const char *name)
{
  const gint32 *index_ptr;
  g_return_val_if_fail (format->type == GSKB_FORMAT_TYPE_STRUCT, NULL);
  index_ptr = gskb_str_table_lookup (format->v_struct.name_to_index, name);
  if (index_ptr == NULL)
    return NULL;
  else
    return format->v_struct.members + (*index_ptr);
}
GskbFormatStructMember *gskb_format_struct_find_member_code
                                                       (GskbFormat *format,
                                                        guint       code)
{
  const gint32 *index_ptr;
  g_return_val_if_fail (format->type == GSKB_FORMAT_TYPE_STRUCT, NULL);
  g_return_val_if_fail (format->v_struct.is_extensible, NULL);
  index_ptr = gskb_uint_table_lookup (format->v_struct.code_to_index, code);
  if (index_ptr == NULL)
    return NULL;
  else
    return format->v_struct.members + (*index_ptr);
}

GskbFormatUnionCase *
gskb_format_union_find_case    (GskbFormat *format,
                                const char *name)
{
  const gint32 *index_ptr;
  g_return_val_if_fail (format->type == GSKB_FORMAT_TYPE_UNION, NULL);
  index_ptr = gskb_str_table_lookup (format->v_union.name_to_index, name);
  if (index_ptr == NULL)
    return NULL;
  else
    return format->v_union.cases + (*index_ptr);
}
GskbFormatUnionCase    *gskb_format_union_find_case_value(GskbFormat *format,
                                                        guint       case_value)
{
  const gint32 *index_ptr;
  g_return_val_if_fail (format->type == GSKB_FORMAT_TYPE_UNION, NULL);
  index_ptr = gskb_uint_table_lookup (format->v_union.code_to_index, case_value);
  if (index_ptr == NULL)
    return NULL;
  else
    return format->v_union.cases + (*index_ptr);
}

/* used internally by union_new and struct_new */
gboolean    gskb_format_is_anonymous   (GskbFormat *format)
{
  return format->any.lc_name == NULL;
}

static void
maybe_name_subformat (GskbFormat *format,
                      const char *parent_name,
                      const char *member_name)
{
  if (format->any.name == NULL)
    {
      char *mixed = lc_to_mixed (member_name);
      char *type_name = g_strdup_printf ("%s_%s", parent_name, mixed);
      gskb_format_name_anonymous (format, type_name);
      g_free (mixed);
      g_free (type_name);
    }
}

void
gskb_format_name_anonymous (GskbFormat *anon_format,
                            const char *name)
{
  guint i;
  g_return_if_fail (anon_format->any.lc_name == NULL);
  anon_format->any.lc_name = mixed_to_lc (name);
  anon_format->any.name = g_strdup (name);
  anon_format->any.TypeName = anon_format->any.name;

  switch (anon_format->type)
    {
    case GSKB_FORMAT_TYPE_STRUCT:
      for (i = 0; i < anon_format->v_struct.n_members; i++)
        {
          GskbFormatStructMember *m = anon_format->v_struct.members + i;
          maybe_name_subformat (m->format, name, m->name);
        }
      break;
    case GSKB_FORMAT_TYPE_UNION:
      for (i = 0; i < anon_format->v_union.n_cases; i++)
        {
          GskbFormatUnionCase *m = anon_format->v_union.cases + i;
          if (m->format != NULL)
            maybe_name_subformat (m->format, name, m->name);
        }
      maybe_name_subformat (anon_format->v_union.type_format, name, "type");
      break;
    default:
      /* no child types to name */
      break;
    }
}

static inline void
append_code_and_size (guint32        code,
                      guint32        size,
                      GskbAppendFunc append_func,
                      gpointer       append_func_data)
{
  guint8 packed_code_and_size[16];
  guint rv = gskb_uint_pack_slab (code, packed_code_and_size);
  rv += gskb_uint_pack_slab (size, packed_code_and_size + rv);
  append_func (rv, packed_code_and_size, append_func_data);
}

static inline void
pack_unknown_value (const GskbUnknownValue *uv,
                    GskbAppendFunc          append_func,
                    gpointer                append_func_data)
{
  append_code_and_size (uv->code, uv->length, append_func, append_func_data);
  append_func (uv->length, uv->data, append_func_data);
}


void
gskb_format_pack           (GskbFormat    *format,
                            gconstpointer  value,
                            GskbAppendFunc append_func,
                            gpointer       append_func_data)
{
  switch (format->type)
    {
    case GSKB_FORMAT_TYPE_INT:
      switch (format->v_int.int_type)
        {
#define WRITE_CASE(UC, lc) \
        case GSKB_FORMAT_INT_##UC: \
          gskb_##lc##_pack (*(gskb_##lc*)value, append_func, append_func_data); \
          break;
        FOREACH_INT_TYPE(WRITE_CASE)
#undef WRITE_CASE
        default:
          g_assert_not_reached ();
        }
      break;
    case GSKB_FORMAT_TYPE_FLOAT:
      switch (format->v_float.float_type)
        {
#define WRITE_CASE(UC, lc) \
        case GSKB_FORMAT_FLOAT_##UC: \
          gskb_##lc##_pack (*(gskb_##lc*)value, append_func, append_func_data); \
          break;
        FOREACH_FLOAT_TYPE(WRITE_CASE)
#undef WRITE_CASE
        default:
          g_assert_not_reached ();
        }
    case GSKB_FORMAT_TYPE_STRING:
      gskb_string_pack (*(char**)value, append_func, append_func_data);
      break;
    case GSKB_FORMAT_TYPE_FIXED_ARRAY:
      {
        guint i;
        GskbFormat *sub = format->v_fixed_array.element_format;
        for (i = 0; i < format->v_fixed_array.length; i++)
          {
            gskb_format_pack (sub, value, append_func, append_func_data);
            value = (char*) value + sub->any.c_size_of;
          }
      }
      break;
    case GSKB_FORMAT_TYPE_LENGTH_PREFIXED_ARRAY:
      {
        const struct { guint32 length; char *data; } *s = value;
        char *data = (char*) s->data;;
        GskbFormat *sub = format->v_length_prefixed_array.element_format;
        guint i;
        gskb_uint_pack (s->length, append_func, append_func_data);
        for (i = 0; i < s->length; i++)
          {
            gskb_format_pack (sub, data, append_func, append_func_data);
            data += sub->any.c_size_of;
          }
      }
      break;
    case GSKB_FORMAT_TYPE_STRUCT:
      if (format->v_struct.is_extensible)
        {
          GskbUnknownValueArray *uva = (GskbUnknownValueArray *) value;
          guint umi = 0;
          guint8 mask = GSKB_BITFIELD_START_MASK;
          const guint8 *bits_at = (const guint8 *) (uva + 1);
          guint i;
          guint8 zero = 0;
          for (i = 0; i < format->v_struct.n_members; i++)
            {
              GskbFormatStructMember *member = format->v_struct.members + i;
              while (umi < uva->length
                  && uva->values[umi].code < member->code)
                {
                  pack_unknown_value (uva->values + umi, append_func, append_func_data);
                  umi++;
                }
              if (*bits_at & mask)
                {
                  gconstpointer field_value = G_STRUCT_MEMBER_P (value, format->any.members[i].c_offset_of);
                  guint packed_size = gskb_format_get_packed_size (member->format, field_value);
                  append_code_and_size (member->code, packed_size, append_func, append_func_data);
                  gskb_format_pack (member->format, field_value, append_func, append_func_data);
                }
              mask = GSKB_BITFIELD_NEXT_MASK (mask);
            }
          while (umi < uva->length)
            {
              pack_unknown_value (uva->values + umi, append_func, append_func_data);
              umi++;
            }
          append_func (1, &zero, append_func_data);
        }
      else
        {
          guint i;
          for (i = 0; i < format->v_struct.n_members; i++)
            gskb_format_pack (format->v_struct.members[i].format,
                              (char*)value + format->any.members[i].c_offset_of,
                              append_func, append_func_data);
        }
      break;
    case GSKB_FORMAT_TYPE_UNION:
      {
        guint32 case_value = * (guint32 *) value;
        GskbFormatUnionCase *c = gskb_format_union_find_case_value (format, case_value);
        if (c == NULL)
          {
            g_assert (format->v_union.is_extensible);
            ...
          }
        gskb_uint32_pack (case_value, append_func, append_func_data);
        if (c->format != NULL)
          gskb_format_pack (c->format,
                            (char*) value + format->any.members[1].c_offset_of,
                            append_func, append_func_data);
      }
      break;
    case GSKB_FORMAT_TYPE_ENUM:
      {
        gskb_enum v = * (gskb_enum *) value;
        switch (format->v_enum.int_type)
          {
#define WRITE_CASE(UC, lc) \
          case GSKB_FORMAT_INT_##UC: \
            gskb_##lc##_pack (v, append_func, append_func_data); \
            break;
          FOREACH_INT_TYPE(WRITE_CASE)
#undef WRITE_CASE
          default:
            g_assert_not_reached ();
          }
      }
      break;
    case GSKB_FORMAT_TYPE_ALIAS:
      gskb_format_pack (format->v_alias.format, value, 
                        append_func, append_func_data);
      break;
    default:
      g_assert_not_reached ();
    }
}
guint
gskb_format_get_packed_size(GskbFormat    *format,
                            gconstpointer  value)
{
  if (format->any.fixed_length != 0)
    return format->any.fixed_length;
  ...
}
guint
gskb_format_pack_slab      (GskbFormat    *format,
                            gconstpointer  value,
                            guint8        *slab)
{
  ...
}

gboolean
gskb_format_unpack_value   (GskbFormat    *format,
                            const guint8  *data,
                            guint         *n_used_out,
                            gpointer       value,
                            GError       **error)
{
  ...
}
void
gskb_format_clear_value    (GskbFormat    *format,
                            gpointer       value)
{
  ...
}
gboolean
gskb_format_unpack_value_mempool (GskbFormat    *format,
                                  const guint8  *data,
                                  guint         *n_used_out,
                                  gpointer       value,
                                  GskMemPool    *mem_pool,
                                  GError       **error)
{
  ...
}
