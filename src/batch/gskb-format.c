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

/* TODO: rename "c_" stuff to "sys_" stuff to emphasize that
   it is system dependent.  the generated code should not
   assume system-dependent stuff will be the same on the host machine.
 */


#include <string.h>
#include "gskb-alignment.h"
#include "gskb-format.h"
#include "gskb-config.h"
#include "gskb-fundamental-formats.h"
#include "gskb-str-table.h"
#include "gskb-uint-table.h"
#include "../gskerror.h"
#include "../gskghelpers.h"

/* align an offset to 'alignment';
   note that 'alignment' is evaluated twice!
   only works if alignment is a power-of-two. */
#define ALIGN(unaligned_offset, alignment)    \
      GSKB_ALIGN(unaligned_offset, alignment)
#define IS_ALLOWED_ENUM_INT_TYPE(int_type)   \
   (  ((int_type) == GSKB_FORMAT_INT_UINT8)  \
   || ((int_type) == GSKB_FORMAT_INT_UINT16) \
   || ((int_type) == GSKB_FORMAT_INT_UINT32) \
   || ((int_type) == GSKB_FORMAT_INT_UINT)   )

typedef struct {
  guint32 length;
  char *data;
} GenericLengthPrefixedArray;

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

/* NOTE: must match order in GskbFormatType in gskb-format.h */
const char *
gskb_format_type_enum_name (GskbFormatType type)
{
  static const char *enum_names[] = {
    "GSKB_FORMAT_TYPE_INT",
    "GSKB_FORMAT_TYPE_FLOAT",
    "GSKB_FORMAT_TYPE_STRING",
    "GSKB_FORMAT_TYPE_FIXED_ARRAY",
    "GSKB_FORMAT_TYPE_LENGTH_PREFIXED_ARRAY",
    "GSKB_FORMAT_TYPE_STRUCT",
    "GSKB_FORMAT_TYPE_UNION",
    "GSKB_FORMAT_TYPE_BIT_FIELDS",
    "GSKB_FORMAT_TYPE_ENUM",
    "GSKB_FORMAT_TYPE_ALIAS"
  };
  return enum_names[type];
}
/* NOTE: must match order in GskbFormatType in gskb-format.h */
const char *
gskb_format_type_name (GskbFormatType type)
{
  static const char *names[] = {
    "int",
    "float",
    "string",
    "fixed_array",
    "length_prefixed_array",
    "struct",
    "union",
    "bit_fields",
    "enum",
    "alias"
  };
  return names[type];
}

/* NOTE: must match order in GskbFormatCType in gskb-format.h */
const char *
gskb_format_ctype_enum_name (GskbFormatCType type)
{
  static const char *enum_names[] = {
    "GSKB_FORMAT_CTYPE_INT8",
    "GSKB_FORMAT_CTYPE_INT16",
    "GSKB_FORMAT_CTYPE_INT32",
    "GSKB_FORMAT_CTYPE_INT64",
    "GSKB_FORMAT_CTYPE_UINT8",
    "GSKB_FORMAT_CTYPE_UINT16",
    "GSKB_FORMAT_CTYPE_UINT32",
    "GSKB_FORMAT_CTYPE_UINT64",
    "GSKB_FORMAT_CTYPE_FLOAT32",
    "GSKB_FORMAT_CTYPE_FLOAT64",
    "GSKB_FORMAT_CTYPE_STRING",
    "GSKB_FORMAT_CTYPE_COMPOSITE"
  };
  return enum_names[type];
}

/* NOTE: must match order in GskbFormatIntType in gskb-format.h */
const char *gskb_format_int_type_enum_name (GskbFormatIntType type)
{
  static const char *enum_names[] = {
    "GSKB_FORMAT_INT_INT8",
    "GSKB_FORMAT_INT_INT16",
    "GSKB_FORMAT_INT_INT32",
    "GSKB_FORMAT_INT_INT64",
    "GSKB_FORMAT_INT_UINT8",
    "GSKB_FORMAT_INT_UINT16",
    "GSKB_FORMAT_INT_UINT32",
    "GSKB_FORMAT_INT_UINT64",
    "GSKB_FORMAT_INT_INT",
    "GSKB_FORMAT_INT_UINT",
    "GSKB_FORMAT_INT_LONG",
    "GSKB_FORMAT_INT_ULONG",
    "GSKB_FORMAT_INT_BIT"
  };
  return enum_names[type];
}
/* NOTE: must match order in GskbFormatFloatType in gskb-format.h */
const char *gskb_format_float_type_enum_name (GskbFormatFloatType type)
{
  static const char *enum_names[] = {
    "GSKB_FORMAT_FLOAT_FLOAT32",
    "GSKB_FORMAT_FLOAT_FLOAT64"
  };
  return enum_names[type];
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
  //rv->base.n_members = length;
  //rv->base.members = g_new (GskbFormatCMember, length);
  //for (i = 0; i < length; i++)
  //  {
  //    rv->base.members[i].name = g_strdup_printf ("%u", i);
  //    rv->base.members[i].ctype = element_format->any.ctype;
  //    rv->base.members[i].format = gskb_format_ref (element_format);
  //    rv->base.members[i].c_offset_of = element_format->any.c_size_of * i;
  //  }
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
  rv->base.c_size_of = sizeof (GenericLengthPrefixedArray);
  rv->base.c_align_of = MAX (GSKB_ALIGNOF_UINT32, GSKB_ALIGNOF_POINTER);
  rv->base.always_by_pointer = 1;
  rv->base.requires_destruct = TRUE;
  rv->base.is_global = 0;
  rv->base.fixed_length = 0;
  rv->sys_length_offset = G_STRUCT_OFFSET (GenericLengthPrefixedArray, length);
  rv->sys_data_offset = G_STRUCT_OFFSET (GenericLengthPrefixedArray, data);
  rv->element_format = gskb_format_ref (element_format);
  return (GskbFormat *) rv;
}

/**
 * gskb_format_struct_new:
 * @name: the name of the structure. (may be NULL)
 * @is_extensible: whether it is possible to modify the structure
 * and retain binary-compatibility.  Extensible structures and slower
 * and use more space (both in RAM and serialized), but often extensibility
 * is more important.
 * @n_members: 
 * @members:
 * @error: where to put an error is something is wrong in arguments.
 *
 * Create a packed or extensible structure Format.
 *
 * For a packed struct, the number, order, type and names of fields
 * is fixed forever.  That is, no binary-compatible upgrading is possible.
 * The serialized representation of the packed struct is the
 * concatenation of the serialized representation of each of its members.
 * Likewise, in C, the members are listed in the same order in the C structure.
 * System-specific alignment constraints tell whether padding is
 * inserted for alignment purposes.
 * So, if you say:
 *    packed struct MyStruct {
 *      uint a;
 *      string b;
 *    };
 * The corresponding C structure will be simply:
 *    struct MyStruct {
 *      gskb_uint a;
 *      gskb_string b;
 *    };
 *
 * A extensible struct has a rather different format!
 * First, in an extensible struct, every member is optional.
 * Their availability is in a bitfields member named "has".
 * So, if you say:
 *    extensible struct MyStruct {
 *      uint a;
 *      string b;
 *    };
 * The corresponding C structures will be:
 *    struct MyStruct_Contents {
 *      guint8 a : 1;
 *      guint8 b : 1;
 *    };
 *    struct MyStruct {
 *      GskbUnknownValueArray unknown_values;
 *      MyStruct_Contents has;
 *      gskb_uint a;
 *      gskb_string b;
 *    };
 *
 * The unknown_values member is reserved to store members that
 * we do not know about that should be quietly passed through the system.
 *
 * The bitfields 'has' tells whether each of the members there
 * has been initialized.   If the bit is not set, pack()
 * and destruct() (etc) ignore the member.
 *
 * returns: a GskbFormat representing the structure.
 */
GskbFormat *
gskb_format_struct_new (const char             *name,
                        gboolean                is_extensible,
                        guint                   n_members,
                        GskbFormatStructMember *members,
                        GError                **error)
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
  rv->sys_member_offsets = g_new (guint, n_members);
  for (i = 0; i < n_members; i++)
    {
      guint member_align_of = members[i].format->any.c_align_of;
      size = ALIGN (size, member_align_of);
      rv->sys_member_offsets[i] = size;
      align_of = MAX (align_of, member_align_of);
      if (members[i].format->any.requires_destruct)
        requires_destruct = TRUE;
      if (members[i].format->any.fixed_length)
        fixed_length += members[i].format->any.fixed_length;
      else
        is_fixed = FALSE;
    }
  align_of = MAX (align_of, GSKB_ALIGNOF_STRUCT);
  size += align_of - 1;
  size &= ~(align_of - 1);
  rv->base.c_size_of = size;
  rv->base.c_align_of = align_of;
  rv->base.always_by_pointer = 1;
  rv->base.requires_destruct = requires_destruct;
  rv->base.is_global = 0;
  rv->base.fixed_length = is_fixed ? fixed_length : 0;
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
    rv->name_to_index = gskb_str_table_new (sizeof (gint32), GSKB_ALIGNOF_UINT32, n_members, entries);
    g_free (indices);
    g_free (entries);
  }

  if (name != NULL)
    gskb_format_name_anonymous ((GskbFormat*)rv, name);
  return (GskbFormat *) rv;
}

/**
 * gskb_format_union_new:
 * @name: the name of the union. (may be NULL)
 * @is_extensible: ...
 * @int_type: ...
 * @n_cases: ...
 * @cases: ...
 * @error: ...
 *
 * Create a new union structure.  A union is an object with
 * several typed branches, with a single branch active at a time.  
 *
 * An extensible union permits new types to be added later.
 * Existing code will pass the value through without requiring
 * rebuilding/restarting.  This is implemented by adding
 * a length prefix before the serialized value (if there is no value,
 * we encode length 0).
 *
 * returns: a GskbFormat representing the union.
 */
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
  if (!IS_ALLOWED_ENUM_INT_TYPE (int_type))
    {
      g_set_error (error, GSK_G_ERROR_DOMAIN,
                   GSK_ERROR_INVALID_ARGUMENT,
                   "invalid int-type %s for union (%s)",
                   gskb_format_int_type_name (int_type),
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
        if (cases[i].code == GSKB_FORMAT_UNION_UNKNOWN_VALUE_CODE)
          {
            g_set_error (error, GSK_G_ERROR_DOMAIN,
                         GSK_ERROR_INVALID_ARGUMENT,
                         "case '%s' had code 0x%x: reserved for unknown-value",
                         cases[i].name, GSKB_FORMAT_UNION_UNKNOWN_VALUE_CODE);
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

  /* XXX: if !is_extensible, this goes through very similar attempts at perfect hashing.
     combine the work.  perhaps using a private api. */
  {
    guint n_enum_codes = n_cases + (is_extensible ? 1 : 0);
    GskbFormatEnumValue *ev = g_new (GskbFormatEnumValue, n_enum_codes);
    for (i = 0 ; i < n_cases; i++)
      {
        ev[i].name = cases[i].name;
        ev[i].code = cases[i].code;
      }
    if (is_extensible)
      {
        ev[i].name = "unknown_value";
        ev[i].code = GSKB_FORMAT_UNION_UNKNOWN_VALUE_CODE;
      }
    type_format = gskb_format_enum_new (NULL, is_extensible, int_type, n_cases, ev, error);
    g_assert (type_format != NULL);
    g_free (ev);
  }

  rv = g_new0 (GskbFormatUnion, 1);
  rv->base.type = GSKB_FORMAT_TYPE_UNION;
  rv->base.ref_count = 1;
  rv->base.ctype = GSKB_FORMAT_CTYPE_COMPOSITE;

  rv->sys_type_offset = 0;

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
  rv->sys_info_offset = ALIGN (sizeof (guint32), info_align);
  rv->base.c_align_of = MAX (GSKB_ALIGNOF_UINT32, info_align);
  rv->base.c_size_of = ALIGN (rv->sys_info_offset + info_size, info_align);
  rv->type_format = type_format;                /* takes ownership */
  rv->int_type = int_type;
  rv->is_extensible = is_extensible;

  if (name != NULL)
    gskb_format_name_anonymous ((GskbFormat*)rv, name);

  return (GskbFormat *) rv;
}

/**
 * gskb_format_enum_new:
 * @name: the name of the enumeration. (may be NULL)
 * @is_extensible: whether to allow unknown values.
 * @int_type: type of integer encoding to use.
 * Must be a less-than-long unsigned integer (uint8, uint16, uint32, or uint).
 * @n_values: the number of (known) values in the enumeration.
 * @values: array of (known) values.  All values must have unique
 * names and codes.
 * @error: where to put an error is something is wrong in arguments.
 *
 * This creates a format that is binary-identical to an int of
 * the same int-type.  However, we associate a name to each value
 * instead of a number.  This makes it easier for humans to interpret
 * than a random integer.
 *
 * returns: a GskbFormat representing the enum.
 */
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
  if (!IS_ALLOWED_ENUM_INT_TYPE (int_type))
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
  rv->is_extensible = is_extensible;
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
  rv->name_to_index = gskb_str_table_new (sizeof (gint32), GSKB_ALIGNOF_UINT32, n_values, str_table_entries);
  rv->value_to_index = gskb_uint_table_new (sizeof (gint32), GSKB_ALIGNOF_UINT32, n_values, uint_table_entries);
  g_free (indices);
  g_free (str_table_entries);
  g_free (uint_table_entries);
  if (name != NULL)
    gskb_format_name_anonymous ((GskbFormat*)rv, name);
  return (GskbFormat *) rv;
}

/**
 * gskb_format_bit_fields_new:
 * @name:
 * @n_fields:
 * @fields:
 * @error:
 *
 * Create a new bit-fields data-type.  A bit-fields is an object with
 * with various small unsigned integers packed together.
 * It is a generalization of the idea of 'flags' (a set of bits),
 * where each member can itself have up to 8 bits. (So to implement
 * 'flags', as we do for extensible structs, we set each length to 1, a bit).
 *
 * The data is packed, little-endian-style, both in the memory format
 * and serialized.  However, we pad the structures to
 * force each member into a single byte.  (Most systems would
 * do that anyway, and it minimizes the amount of non-portable code to deal
 * with.)
 *
 * returns: a GskbFormat representing the bit-fields.
 */
GskbFormat *
gskb_format_bit_fields_new (const char            *name,
                            guint                  n_fields,
                            GskbFormatBitField    *fields,
                            GError               **error)
{
  GskbFormatBitFields *rv;
  guint i;
  guint *indices = g_newa (guint, n_fields);
  GskbStrTableEntry *table_entry = g_newa (GskbStrTableEntry, n_fields);
  GskbStrTable *table;
  guint n_unpacked_bytes;
  gboolean has_holes;
  guint bit_align;
  guint total_bits;
  for (i = 0; i < n_fields; i++)
    {
      if (fields[i].length == 0)
        {
          g_set_error (error, GSK_G_ERROR_DOMAIN, GSK_ERROR_INVALID_ARGUMENT,
                       "zero length field not allowed (field name %s, in %s)",
                       fields[i].name, name ? name : "*unnamed bitfields*");
          return NULL;
        }
      if (fields[i].length > 8)
        {
          g_set_error (error, GSK_G_ERROR_DOMAIN, GSK_ERROR_INVALID_ARGUMENT,
                       "field too many bits (%u) (field name %s, in %s)",
                       fields[i].length,
                       fields[i].name, name ? name : "*unnamed bitfields*");
          return NULL;
        }
      indices[i] = i;
      table_entry[i].str = fields[i].name;
      table_entry[i].entry_data = indices + i;
    }
  table = gskb_str_table_new (sizeof (guint32), GSKB_ALIGNOF_UINT32, n_fields, table_entry);
  if (table == NULL)
    {
      /* duplicate name error */
      g_set_error (error, GSK_G_ERROR_DOMAIN, GSK_ERROR_INVALID_ARGUMENT,
                   "two fields in %s have the same name",
                   name ? name : "*unnamed bitfields*");
      return NULL;
    }

  rv = g_new0 (GskbFormatBitFields, 1);
  rv->base.type = GSKB_FORMAT_TYPE_BIT_FIELDS;
  rv->base.ref_count = 1;
  rv->base.c_align_of = 1;              /* XXX: is that true? */
  rv->base.always_by_pointer = 1;
  rv->name_to_index = table;

  total_bits = 0;
  for (i = 0; i < n_fields; i++)
    total_bits += fields[i].length;
  rv->base.fixed_length = (total_bits + 7) / 8;
  rv->total_bits = total_bits;

  /* Initialize n_fields, fields, has_holes, c_size_of */
  rv->n_fields = n_fields;
  rv->fields = g_new (GskbFormatBitField, n_fields);
  has_holes = FALSE;
  bit_align = 0;
  n_unpacked_bytes = 0;
  for (i = 0; i < n_fields; i++)
    {
      rv->fields[i].name = g_strdup (fields[i].name);
      rv->fields[i].length = fields[i].length;
      if (bit_align + fields[i].length > 8)
        {
          has_holes = TRUE;
          bit_align = 0;
          n_unpacked_bytes++;
        }
      bit_align += fields[i].length;
      if (bit_align == 8)
        {
          bit_align = 0;
          n_unpacked_bytes++;
        }
    }
  if (bit_align > 0)
    n_unpacked_bytes++;
  rv->base.c_size_of = n_unpacked_bytes;
  rv->has_holes = has_holes;

  /* compute bits_per_unpacked_byte */
  bit_align = 0;
  n_unpacked_bytes = 0;
  rv->bits_per_unpacked_byte = g_malloc (rv->base.c_size_of);
  for (i = 0; i < n_fields; i++)
    {
      if (bit_align + fields[i].length > 8)
        {
          has_holes = TRUE;
          rv->bits_per_unpacked_byte[n_unpacked_bytes++] = bit_align;
          bit_align = 0;
        }
      bit_align += fields[i].length;
      if (bit_align == 8)
        {
          bit_align = 0;
          rv->bits_per_unpacked_byte[n_unpacked_bytes++] = 8;
        }
    }

  return (GskbFormat *) rv;
}

GskbFormat *
gskb_format_ref (GskbFormat *format)
{
  g_return_val_if_fail (format->any.ref_count > 0, format);
  ++(format->any.ref_count);
  return format;
}

void
gskb_format_unref (GskbFormat *format)
{
  g_return_if_fail (format->any.ref_count > 0);
  if (--(format->any.ref_count) == 0)
    {
      guint i;
      switch (format->type)
        {
        case GSKB_FORMAT_TYPE_INT:
          g_assert_not_reached ();
        case GSKB_FORMAT_TYPE_FLOAT:
          g_assert_not_reached ();
        case GSKB_FORMAT_TYPE_STRING:
          g_assert_not_reached ();
        case GSKB_FORMAT_TYPE_FIXED_ARRAY:
          gskb_format_unref (format->v_fixed_array.element_format);
          break;
        case GSKB_FORMAT_TYPE_LENGTH_PREFIXED_ARRAY:
          gskb_format_unref (format->v_length_prefixed_array.element_format);
          break;
        case GSKB_FORMAT_TYPE_STRUCT:
          for (i = 0; i < format->v_struct.n_members; i++)
            {
              g_free ((char*) format->v_struct.members[i].name);
              gskb_format_unref (format->v_struct.members[i].format);
            }
          g_free (format->v_struct.members);
          g_free (format->v_struct.sys_member_offsets);
          gskb_str_table_free (format->v_struct.name_to_index);
          if (format->v_struct.is_extensible)
            gskb_uint_table_free (format->v_struct.code_to_index);
          break;
        case GSKB_FORMAT_TYPE_UNION:
          for (i = 0; i < format->v_union.n_cases; i++)
            {
              g_free ((char*) format->v_union.cases[i].name);
              if (format->v_union.cases[i].format != NULL)
                gskb_format_unref (format->v_union.cases[i].format);
            }
          g_free (format->v_union.cases);
          gskb_str_table_free (format->v_union.name_to_index);
          gskb_uint_table_free (format->v_union.code_to_index);
          break;
        case GSKB_FORMAT_TYPE_BIT_FIELDS:
          for (i = 0; i < format->v_bit_fields.n_fields; i++)
            g_free ((char *) format->v_bit_fields.fields[i].name);
          g_free (format->v_bit_fields.fields);
          gskb_str_table_free (format->v_bit_fields.name_to_index);
          break;
        case GSKB_FORMAT_TYPE_ENUM:
          for (i = 0; i < format->v_enum.n_values; i++)
            g_free ((char *) format->v_enum.values[i].name);
          g_free (format->v_enum.values);
          gskb_str_table_free (format->v_enum.name_to_index);
          gskb_uint_table_free (format->v_enum.value_to_index);
          break;
        case GSKB_FORMAT_TYPE_ALIAS:
          gskb_format_unref (format->v_alias.format);
          break;
        }
      g_free (format->any.name);
      g_free (format->any.TypeName);
      g_free (format->any.lc_name);
      g_free (format);
    }
}

/* methods on certain formats */
/**
 * gskb_format_struct_find_member:
 *
 * @format: the structure to query
 * @name: name of the member
 *
 * Find a given member of the structure.
 */
GskbFormatStructMember *
gskb_format_struct_find_member (GskbFormat *format,
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
GskbFormatUnionCase    *gskb_format_union_find_case_code(GskbFormat *format,
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
pack_code_and_size (guint32        code,
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
  pack_code_and_size (uv->code, uv->length, append_func, append_func_data);
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
        const GenericLengthPrefixedArray *s = value;
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
          guint8 mask = 1;
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
                  gconstpointer field_value = G_STRUCT_MEMBER_P (value, format->v_struct.sys_member_offsets[i]);
                  guint packed_size = gskb_format_get_packed_size (member->format, field_value);
                  pack_code_and_size (member->code, packed_size, append_func, append_func_data);
                  gskb_format_pack (member->format, field_value, append_func, append_func_data);
                }
              mask <<= 1;
              if (mask == 0)
                {
                  mask = 1;
                  bits_at++;
                }
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
                              (char*)value + format->v_struct.sys_member_offsets[i],
                              append_func, append_func_data);
        }
      break;
    case GSKB_FORMAT_TYPE_UNION:
      {
        guint32 case_value = * (const guint32 *) value;
        gconstpointer uvalue = (char*)value + format->v_union.sys_info_offset;
        GskbFormatUnionCase *c = gskb_format_union_find_case_code (format, case_value);
        if (format->v_union.is_extensible)
          {
            if (c == NULL)
              {
                /* TODO: assert case_value == UNKNOWN */
                const GskbUnknownValue *uv = uvalue;
                gskb_format_pack (format->v_union.type_format, &uv->code, append_func, append_func_data);
                gskb_uint_pack (uv->length, append_func, append_func_data);
                append_func (uv->length, uv->data, append_func_data);
              }
            else if (c->format == NULL)
              {
                guint8 zero = 0;
                gskb_format_pack (format->v_union.type_format, value, append_func, append_func_data);
                append_func (1, &zero, append_func_data);
              }
            else
              {
                guint size = gskb_format_get_packed_size (c->format, uvalue);
                gskb_format_pack (format->v_union.type_format, value, append_func, append_func_data);
                gskb_uint_pack (size, append_func, append_func_data);
                gskb_format_pack (c->format, uvalue, append_func, append_func_data);
              }
          }
        else
          {
            gskb_format_pack (format->v_union.type_format, value, append_func, append_func_data);
            if (c->format != NULL)
              gskb_format_pack (c->format,
                                (char*) value + format->v_union.sys_info_offset,
                                append_func, append_func_data);
          }
      }
      break;
    case GSKB_FORMAT_TYPE_BIT_FIELDS:
      {
        if (format->v_bit_fields.has_holes)
          {
            guint8 *packed;
            guint8 *to_free;
            guint8 bits = 0;
            guint8 *bits_at;
            guint8 bits_shift = 0;
            const guint8 *at = value;
            guint at_shift = 0;
            guint i;
            if (G_LIKELY (format->any.fixed_length < 64))
              {
                packed = g_alloca (format->any.fixed_length);
                to_free = NULL;
              }
            else
              {
                to_free = packed = g_malloc (format->any.fixed_length);
              }
            bits_at = packed;
            for (i = 0; i < format->v_bit_fields.n_fields; i++)
              {
                const GskbFormatBitField *field = format->v_bit_fields.fields + i;
                guint8 v;
                if (at_shift + field->length > 8)
                  {
                    at_shift = 0;
                    at++;
                  }
                v = *at >> at_shift;
                if (bits_shift + field->length > 8)
                  {
                    guint bits_avail = field->length;
                    if (bits_shift < 8)
                      {
                        bits |= v << bits_shift;
                        v >>= (8 - bits_shift);
                        bits_avail -= (8 - bits_shift);
                      }
                    *bits_at++ = bits;
                    bits = v & ((1<<bits_avail)-1);
                    bits_shift = bits_avail;
                  }
                else
                  {
                    bits |= ((v & ((1<<field->length)-1)) << bits_shift);
                    bits_shift += field->length;
                  }
              }
            g_assert (bits_shift != 0);
            *bits_at++ = bits;
            g_assert (bits_at == packed + format->any.fixed_length);
            append_func (format->any.fixed_length, packed, append_func_data);
            if (to_free)
              g_free (to_free);
          }
        else
          {
            if (format->v_bit_fields.total_bits % 8 == 0)
              append_func (format->any.fixed_length, value, append_func_data);
            else
              {
                guint8 buf[16];
                guint fixed_length = format->any.fixed_length;
                guint8 end_mask = (1<<(format->v_bit_fields.total_bits%8)) - 1;
                if (fixed_length <= sizeof(buf))
                  {
                    memcpy (buf, value, fixed_length);
                    buf[fixed_length-1] &= end_mask;
                    append_func (fixed_length, buf, append_func_data);
                  }
                else
                  {
                    guint8 end = end_mask & ((const guint8*)value)[fixed_length-1];
                    append_func (fixed_length - 1, value, append_func_data);
                    append_func (1, &end, append_func_data);
                  }
              }
          }
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
  switch (format->type)
    {
    case GSKB_FORMAT_TYPE_INT:
      if (format->v_int.int_type == GSKB_FORMAT_INT_INT)
        return gskb_int_get_packed_size (*(const gint32*)value);
      else if (format->v_int.int_type == GSKB_FORMAT_INT_UINT)
        return gskb_uint_get_packed_size (*(const guint32*)value);
      if (format->v_int.int_type == GSKB_FORMAT_INT_LONG)
        return gskb_long_get_packed_size (*(const gint64*)value);
      else if (format->v_int.int_type == GSKB_FORMAT_INT_ULONG)
        return gskb_ulong_get_packed_size (*(const guint64*)value);
      else
        g_return_val_if_reached (0);
      break;
    case GSKB_FORMAT_TYPE_STRING:
      return gskb_string_get_packed_size (*(const char*const *)value);
    case GSKB_FORMAT_TYPE_FIXED_ARRAY:
      {
        guint i;
        GskbFormat *sub = format->v_fixed_array.element_format;
        guint rv = 0;
        for (i = 0; i < format->v_fixed_array.length; i++)
          {
            rv += gskb_format_get_packed_size (sub, value);
            value = (char*) value + sub->any.c_size_of;
          }
        return rv;
      }
    case GSKB_FORMAT_TYPE_LENGTH_PREFIXED_ARRAY:
      {
        const GenericLengthPrefixedArray *s = value;
        const char *data = (const char*) s->data;;
        GskbFormat *sub = format->v_length_prefixed_array.element_format;
        guint i;
        guint rv = gskb_uint_get_packed_size (s->length);
        for (i = 0; i < s->length; i++)
          {
            rv += gskb_format_get_packed_size (sub, data);
            data += sub->any.c_size_of;
          }
        return rv;
      }
      break;
    case GSKB_FORMAT_TYPE_STRUCT:
      if (format->v_struct.is_extensible)
        {
          guint i;
          guint rv = 0;
          const GskbUnknownValueArray *uv = value;
          const guint8 *bit_fields = (const guint8 *) (uv + 1);
          guint8 mask = 1;
          for (i = 0; i < uv->length; i++)
            {
              rv += gskb_uint_get_packed_size (uv->values[i].code);
              rv += gskb_uint_get_packed_size (uv->values[i].length);
              rv += uv->values[i].length;
            }
          for (i = 0; i < format->v_struct.n_members; i++)
            {
              if ((*bit_fields & mask) != 0)
                {
                  GskbFormatStructMember *member = format->v_struct.members + i;
                  GskbFormat *sub = member->format;
                  guint sub_length = gskb_format_get_packed_size (sub, (const guint8 *)value + format->v_struct.sys_member_offsets[i]);
                  rv += gskb_uint_get_packed_size (member->code);
                  rv += gskb_uint_get_packed_size (sub_length);
                  rv += sub_length;
                }
            }
          rv += 1;              /* final NUL */
          return rv;
        }
      else
        {
          guint i;
          guint rv = 0;
          for (i = 0; i < format->v_struct.n_members; i++)
            rv = gskb_format_get_packed_size (format->v_struct.members[i].format,
                                              (const char*)value + format->v_struct.sys_member_offsets[i]);
          return rv;
        }
      break;
    case GSKB_FORMAT_TYPE_UNION:
      {
        guint32 code = * (const guint32*)value;
        GskbFormatUnionCase *ca = gskb_format_union_find_case_code (format, code);
        gconstpointer info = (const guint8 *) value + format->v_union.sys_info_offset;
        guint real_code, value_len;
        guint code_len;
        if (ca == NULL)
          {
            const GskbUnknownValue *uv = info;
            g_assert (code == GSKB_FORMAT_UNION_UNKNOWN_VALUE_CODE);
            real_code = uv->code;
            value_len = uv->length;
          }
        else
          {
            real_code = code;
            if (ca->format == NULL)
              {
                value_len = 0;
              }
            else
              {
                value_len = gskb_format_get_packed_size (ca->format, info);
              }
          }
        switch (format->v_union.type_format->v_int.int_type)
          {
#define WRITE_CASE(UC, lc) \
          case GSKB_FORMAT_INT_##UC: \
            code_len = gskb_##lc##_get_packed_size (real_code); \
            break;
          FOREACH_INT_TYPE(WRITE_CASE)
#undef WRITE_CASE
          default:
            g_assert_not_reached ();
          }
        if (format->v_union.is_extensible)
          return code_len + gskb_uint_get_packed_size (value_len) + value_len;
        else
          return code_len + value_len;
      }
    case GSKB_FORMAT_TYPE_ENUM:
      {
        gskb_enum v = * (gskb_enum *) value;
        switch (format->v_enum.int_type)
          {
#define WRITE_CASE(UC, lc) \
          case GSKB_FORMAT_INT_##UC: \
            return gskb_##lc##_get_packed_size (v);
          FOREACH_INT_TYPE(WRITE_CASE)
#undef WRITE_CASE
          default:
            g_assert_not_reached ();
          }
      }
      break;
    case GSKB_FORMAT_TYPE_ALIAS:
      return gskb_format_get_packed_size (format->v_alias.format, value);
    default:
      g_assert_not_reached ();
    }
  g_return_val_if_reached (0);
}

static inline guint
pack_slab_code_and_size (guint32        code,
                         guint32        size,
                         guint8        *slab)
{
  guint sub_rv = gskb_uint_pack_slab (code, slab);
  return sub_rv + gskb_uint_pack_slab (size, slab + sub_rv);
}

static inline guint
pack_slab_unknown_value (const GskbUnknownValue *uv,
                         guint8                 *slab)
{
  guint a = pack_slab_code_and_size (uv->code, uv->length, slab);
  memcpy (slab + a, uv->data, uv->length);
  return a + uv->length;
}

guint
gskb_format_pack_slab      (GskbFormat    *format,
                            gconstpointer  value,
                            guint8        *slab)
{
  switch (format->type)
    {
    case GSKB_FORMAT_TYPE_INT:
      switch (format->v_int.int_type)
        {
#define WRITE_CASE(UC, lc) \
        case GSKB_FORMAT_INT_##UC: \
          return gskb_##lc##_pack_slab (*(gskb_##lc*)value, slab); \
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
          return gskb_##lc##_pack_slab (*(gskb_##lc*)value, slab);
        FOREACH_FLOAT_TYPE(WRITE_CASE)
#undef WRITE_CASE
        default:
          g_assert_not_reached ();
        }
    case GSKB_FORMAT_TYPE_STRING:
      return gskb_string_pack_slab (*(char**)value, slab);
    case GSKB_FORMAT_TYPE_FIXED_ARRAY:
      {
        guint i;
        GskbFormat *sub = format->v_fixed_array.element_format;
        guint rv = 0;
        for (i = 0; i < format->v_fixed_array.length; i++)
          {
            rv += gskb_format_pack_slab (sub, value, slab + rv);
            value = (char*) value + sub->any.c_size_of;
          }
        return rv;
      }
    case GSKB_FORMAT_TYPE_LENGTH_PREFIXED_ARRAY:
      {
        const GenericLengthPrefixedArray *s = value;
        char *data = (char*) s->data;;
        GskbFormat *sub = format->v_length_prefixed_array.element_format;
        guint i;
        guint rv = gskb_uint_pack_slab (s->length, slab);
        for (i = 0; i < s->length; i++)
          {
            rv += gskb_format_pack_slab (sub, data, slab + rv);
            data += sub->any.c_size_of;
          }
      }
      break;
    case GSKB_FORMAT_TYPE_STRUCT:
      if (format->v_struct.is_extensible)
        {
          GskbUnknownValueArray *uva = (GskbUnknownValueArray *) value;
          guint umi = 0;
          guint8 mask = 1;
          const guint8 *bits_at = (const guint8 *) (uva + 1);
          guint i;
          guint rv = 0;
          for (i = 0; i < format->v_struct.n_members; i++)
            {
              GskbFormatStructMember *member = format->v_struct.members + i;
              while (umi < uva->length
                  && uva->values[umi].code < member->code)
                {
                  rv += pack_slab_unknown_value (uva->values + umi, slab + rv);
                  umi++;
                }
              if (*bits_at & mask)
                {
                  gconstpointer field_value = G_STRUCT_MEMBER_P (value, format->v_struct.sys_member_offsets[i]);
                  guint packed_size = gskb_format_get_packed_size (member->format, field_value);
                  rv += pack_slab_code_and_size (member->code, packed_size, slab + rv);
                  rv += gskb_format_pack_slab (member->format, field_value, slab + rv);
                }
              mask <<= 1;
              if (mask == 0)
                {
                  mask = 1;
                  bits_at++;
                }
            }
          while (umi < uva->length)
            {
              rv += pack_slab_unknown_value (uva->values + umi, slab + rv);
              umi++;
            }
          slab[rv++] = 0;
          return rv;
        }
      else
        {
          guint i;
          guint rv = 0;
          for (i = 0; i < format->v_struct.n_members; i++)
            rv += gskb_format_pack_slab (format->v_struct.members[i].format,
                                         (char*)value + format->v_struct.sys_member_offsets[i],
                                         slab + rv);
          return rv;
        }
    case GSKB_FORMAT_TYPE_UNION:
      {
        guint32 case_value = * (const guint32 *) value;
        gconstpointer uvalue = (char*)value + format->v_union.sys_info_offset;
        GskbFormatUnionCase *c = gskb_format_union_find_case_code (format, case_value);
        if (format->v_union.is_extensible)
          {
            if (c == NULL)
              {
                /* TODO: assert case_value == UNKNOWN */
                const GskbUnknownValue *uv = uvalue;
                guint rv = gskb_format_pack_slab (format->v_union.type_format, &uv->code, slab);
                rv += gskb_uint_pack_slab (uv->length, slab + rv);
                memcpy (slab + rv, uv->data, uv->length);
                return rv + uv->length;
              }
            else if (c->format == NULL)
              {
                guint rv = gskb_format_pack_slab (format->v_union.type_format, value, slab);
                slab[rv++] = 0;
                return rv;
              }
            else
              {
                guint size = gskb_format_get_packed_size (c->format, uvalue);
                guint rv = gskb_format_pack_slab (format->v_union.type_format, value, slab);
                rv += gskb_uint_pack_slab (size, slab + rv);
                rv += gskb_format_pack_slab (c->format, uvalue, slab + rv);
                return rv;
              }
          }
        else
          {
            guint rv = gskb_format_pack_slab (format->v_union.type_format, value, slab);
            if (c->format != NULL)
              rv += gskb_format_pack_slab (c->format,
                                           (char*) value + format->v_union.sys_info_offset,
                                           slab + rv);
            return rv;
          }
      }
      break;
    case GSKB_FORMAT_TYPE_BIT_FIELDS:
      {
        if (format->v_bit_fields.has_holes)
          {
            guint8 bits = 0;
            guint8 *bits_at = slab;
            guint8 bits_shift = 0;
            const guint8 *at = value;
            guint at_shift = 0;
            guint i;
            for (i = 0; i < format->v_bit_fields.n_fields; i++)
              {
                const GskbFormatBitField *field = format->v_bit_fields.fields + i;
                guint8 v;
                if (at_shift + field->length > 8)
                  {
                    at_shift = 0;
                    at++;
                  }
                v = *at >> at_shift;
                if (bits_shift + field->length > 8)
                  {
                    guint bits_avail = field->length;
                    if (bits_shift < 8)
                      {
                        bits |= v << bits_shift;
                        v >>= (8 - bits_shift);
                        bits_avail -= (8 - bits_shift);
                      }
                    *bits_at++ = bits;
                    bits = v & ((1<<bits_avail)-1);
                    bits_shift = bits_avail;
                  }
                else
                  {
                    bits |= ((v & ((1<<field->length)-1)) << bits_shift);
                    bits_shift += field->length;
                  }
              }
            g_assert (bits_shift != 0);
            *bits_at++ = bits;
            g_assert (bits_at == slab + format->any.fixed_length);
          }
        else
          {
            guint total_bits = format->v_bit_fields.total_bits;
            memcpy (slab, value, format->any.fixed_length);
            if (total_bits % 8 != 0)
              slab[format->any.fixed_length-1] &= ~((1<<(total_bits%8))-1);
          }
          return format->any.fixed_length;
      }
      break;
    case GSKB_FORMAT_TYPE_ENUM:
      {
        gskb_enum v = * (gskb_enum *) value;
        switch (format->v_enum.int_type)
          {
#define WRITE_CASE(UC, lc) \
          case GSKB_FORMAT_INT_##UC: \
            return gskb_##lc##_pack_slab (v, slab);
          FOREACH_INT_TYPE(WRITE_CASE)
#undef WRITE_CASE
          }
      }
      break;
    case GSKB_FORMAT_TYPE_ALIAS:
      return gskb_format_pack_slab (format->v_alias.format, value, slab);
    }
  g_return_val_if_reached (0);
}

/* TODO: use G_UNLIKELY() for error cases? */
guint
gskb_format_validate_partial(GskbFormat    *format,
                            guint          len,
                            const guint8  *data,
                            GError       **error)
{
  if (format->any.fixed_length != 0
   && len < format->any.fixed_length)
    {
      g_set_error (error, GSK_G_ERROR_DOMAIN, GSK_ERROR_TOO_SHORT,
                   "validating fixed-length %s (%s): too short (expected %u, got %u)",
                   gskb_format_type_name (format->type),
                   format->any.name ?  format->any.name : "*unnamed*",
                   format->any.fixed_length, len);
      return 0;
    }

  switch (format->type)
    {
    case GSKB_FORMAT_TYPE_INT:
      switch (format->v_int.int_type)
        {
        case GSKB_FORMAT_INT_INT:
          return gskb_int_validate_partial (len, data, error);
        case GSKB_FORMAT_INT_UINT:
          return gskb_uint_validate_partial (len, data, error);
        case GSKB_FORMAT_INT_LONG:
          return gskb_long_validate_partial (len, data, error);
        case GSKB_FORMAT_INT_ULONG:
          return gskb_ulong_validate_partial (len, data, error);
        default:
          return format->any.fixed_length;
        }
    case GSKB_FORMAT_TYPE_FLOAT:
      switch (format->v_float.float_type)
        {
#define WRITE_CASE(UC, lc) \
        case GSKB_FORMAT_FLOAT_##UC: \
          return gskb_##lc##_validate_partial (len, data, error);
        FOREACH_FLOAT_TYPE(WRITE_CASE)
        }
#undef WRITE_CASE
      g_return_val_if_reached (0);

    case GSKB_FORMAT_TYPE_STRING:
      return gskb_string_validate_partial (len, data, error);

    case GSKB_FORMAT_TYPE_FIXED_ARRAY:
      {
        guint i, N = format->v_fixed_array.length;
        guint rv = 0;
        GskbFormat *sub = format->v_fixed_array.element_format;
        for (i = 0; i < N; i++)
          {
            guint sub_rv = gskb_format_validate_partial (sub, len - rv, data + rv, error);
            if (sub_rv == 0)
              {
                gsk_g_error_add_prefix (error, "validating %s element %u failed",
                                        format->any.name ? format->any.name : "unnamed FixedLengthArray",
                                        i);
                return 0;
              }
            rv += sub_rv;
          }
        return rv;
      }

    case GSKB_FORMAT_TYPE_LENGTH_PREFIXED_ARRAY:
      {
        guint32 N;
        guint rv = gskb_uint_validate_unpack (len, data, &N, error);
        GskbFormat *sub = format->v_length_prefixed_array.element_format;
        guint i;
        if (rv == 0)
          {
            gsk_g_error_add_prefix (error, "validating length-prefix of %s failed",
                                    format->any.name ? format->any.name : "unnamed FixedLengthArray");
            return 0;
          }
        for (i = 0; i < N; i++)
          {
            guint sub_rv = gskb_format_validate_partial (sub, len - rv, data + rv, error);
            if (sub_rv == 0)
              {
                gsk_g_error_add_prefix (error, "validating %s element %u failed",
                                        format->any.name ? format->any.name : "unnamed FixedLengthArray", i);
                return 0;
              }
            rv += sub_rv;
          }
        return rv;
      }

    case GSKB_FORMAT_TYPE_STRUCT:
      if (format->v_struct.is_extensible)
        {
          guint rv = 0;
          guint32 last_code = 0;
          for (;;)
            {
              guint32 code, sub_len;
              guint sub_rv;
              GskbFormatStructMember *member;
              sub_rv = gskb_uint_validate_unpack (len - rv, data + rv, &code, error);
              if (sub_rv == 0)
                {
                  gsk_g_error_add_prefix (error, "validating member's code in %s",
                                          format->any.name ? format->any.name : "unnamed extensible struct");
                  return 0;
                }
              rv += sub_rv;
              if (code == 0)
                return rv;
              if (code <= last_code)
                {
                  gsk_g_error_add_prefix (error, "member's codes must be ascending in %s",
                                          format->any.name ? format->any.name : "unnamed extensible struct");
                  return 0;
                }
              sub_rv = gskb_uint_validate_unpack (len - rv, data + rv, &sub_len, error);
              if (sub_rv == 0)
                {
                  gsk_g_error_add_prefix (error, "validating member's length in %s",
                                          format->any.name ? format->any.name : "unnamed extensible struct");
                  return 0;
                }

              last_code = code;
              member = gskb_format_struct_find_member_code (format, code);
              if (rv + sub_len > len)
                {
                  g_set_error (error, GSK_G_ERROR_DOMAIN, GSK_ERROR_PREMATURE_EOF,
                               "data too short in %s field of %s with code %u (need at least %u more)",
                               member ? member->name : "*unknown*",
                               format->any.name ? format->any.name : "unnamed extensible struct",
                               code, rv + sub_len - len);
                  return 0;
                }
              if (member != NULL)
                {
                  if (!gskb_format_validate_packed (member->format, sub_len, data + rv, error))
                    {
                      gsk_g_error_add_prefix (error, "validating member %s of %s",
                                              member->name, format->any.name ? format->any.name : "unnamed extensible struct");
                      return 0;
                    }
                }
              rv += sub_len;
            }
        }
      else
        {
          guint rv = 0;
          guint i;
          for (i = 0; i < format->v_struct.n_members; i++)
            {
              GskbFormatStructMember *member = format->v_struct.members + i;
              guint sub_rv = gskb_format_validate_partial (member->format, len - rv, data + rv, error);
              if (sub_rv == 0)
                {
                  gsk_g_error_add_prefix (error, "validating %s member %s failed",
                                          format->any.name ? format->any.name : "unnamed extensible struct",
                                          member->name);
                  return 0;
                }
              rv += sub_rv;
            }
          return rv;
        }

    case GSKB_FORMAT_TYPE_UNION:
      {
        gskb_enum code;
        GskbFormatUnionCase *c;
        guint rv;
        switch (format->v_union.int_type)
          {
          case GSKB_FORMAT_INT_UINT8:
            {
              guint8 v=0;
              rv = gskb_uint8_validate_unpack (len, data, &v, error);
              code = v;
              break;
            }
          case GSKB_FORMAT_INT_UINT16:
            {
              guint16 v=0;
              rv = gskb_uint16_validate_unpack (len, data, &v, error);
              code = v;
              break;
            }
          case GSKB_FORMAT_INT_UINT32:
            {
              guint32 v=0;
              rv = gskb_uint32_validate_unpack (len, data, &v, error);
              code = v;
              break;
            }
          case GSKB_FORMAT_INT_UINT:
            {
              guint32 v=0;
              rv = gskb_uint_validate_unpack (len, data, &v, error);
              code = v;
              break;
            }
          default:
            g_assert_not_reached ();
          }
        if (rv == 0)
          {
            gsk_g_error_add_prefix (error, "validating %s code failed",
                                    format->any.name ? format->any.name : "unnamed union");
            return 0;
          }
        c = gskb_format_union_find_case_code (format, code);
        if (format->v_union.is_extensible)
          {
            guint32 sub_len;
            guint sub_rv = gskb_uint_validate_unpack (len - rv, data + rv, &sub_len, error);
            if (sub_rv == 0)
              {
                gsk_g_error_add_prefix (error, "validating length of union data");
                return 0;
              }
            rv += sub_rv;
            if (rv + sub_len > len)
              {
                g_set_error (error, GSK_G_ERROR_DOMAIN, GSK_ERROR_PREMATURE_EOF,
                             "data too short in %s field of %s with code %u (need at least %u more)",
                             c ? c->name : "*unknown*",
                             format->any.name ? format->any.name : "unnamed extensible union",
                             code, rv + sub_len - len);
                return 0;
              }
            if (c != NULL)
              {
                if (c->format == NULL)
                  {
                    if (sub_len != 0)
                      {
                        g_set_error (error, GSK_G_ERROR_DOMAIN, GSK_ERROR_PARSE,
                                     "got data where none expected in case %s of %s",
                                     c->name, format->any.name ? format->any.name : "unnamed extensible union");
                        return 0;
                      }
                  }
                else
                  {
                    if (!gskb_format_validate_packed (c->format, sub_len, data + rv, error))
                      {
                        gsk_g_error_add_prefix (error, "validating case %s of %s",
                                                c->name, format->any.name ? format->any.name : "unnamed extensible union");
                        return 0;
                      }
                  }
              }
            return rv + sub_len;
          }
        else
          {
            if (c == NULL)
              {
                gsk_g_error_add_prefix (error, "validating %s got unknown member code %u",
                                        format->any.name ? format->any.name : "unnamed union",
                                        code);
                return 0;
              }
            if (c->format != NULL)
              {
                guint sub_rv = gskb_format_validate_packed (c->format, len - rv, data + rv, error);
                if (sub_rv == 0)
                  {
                    gsk_g_error_add_prefix (error,
                                            "validating %s case %s",
                                            format->any.name ? format->any.name : "unnamed union",
                                            c->name);
                    return 0;
                  }
                rv += sub_rv;
              }
            return rv;
          }
      }

    case GSKB_FORMAT_TYPE_BIT_FIELDS:
      if (format->v_bit_fields.n_fields % 8 != 0)
        {
          guint n_unused_bits = 8 - format->v_bit_fields.n_fields % 8;
          guint8 unused_mask = ((1<<n_unused_bits) - 1) << (8 - n_unused_bits);
          guint8 last = data[format->any.fixed_length - 1];
          if ((last & unused_mask) != 0)
            {
              g_set_error (error, GSK_G_ERROR_DOMAIN, GSK_ERROR_PARSE,
                           "%u unused bits at the end of bit fields (%s) were not 0",
                           n_unused_bits,
                           format->any.name ? format->any.name : "unnamed bitfield");
              return 0;
            }
        }
      return format->any.fixed_length;

    case GSKB_FORMAT_TYPE_ENUM:
      {
        gskb_enum value;
        guint rv = format->any.fixed_length;
        switch (format->v_enum.int_type)
          {
          case GSKB_FORMAT_INT_UINT8:
            {
              value = data[0];
              break;
            }
          case GSKB_FORMAT_INT_UINT16:
            {
              guint16 v;
              gskb_uint16_unpack (data, &v);
              value = v;
              break;
            }
          case GSKB_FORMAT_INT_UINT32:
            {
              guint32 v;
              gskb_uint32_unpack (data, &v);
              value = v;
              break;
            }
          case GSKB_FORMAT_INT_UINT:
            {
              gskb_uint v;
              rv = gskb_uint_validate_unpack (len, data, &v, error);
              if (rv == 0)
                {
                  gsk_g_error_add_prefix (error, "validating enum (%s)",
                                        format->any.name ? format->any.name : "unnamed");
                  return 0;
                }
              value = v;
              break;
            }
          default:
            g_assert_not_reached ();
          }
        if (format->v_enum.is_extensible)
          return rv;
        if (gskb_format_enum_find_value_code (format, value) == NULL)
          {
            g_set_error (error, GSK_G_ERROR_DOMAIN, GSK_ERROR_PARSE,
                         "no enum with value %u (%s)",
                         value, format->any.name ? format->any.name : "unnamed");
            return 0;
          }
        return rv;
      }

    case GSKB_FORMAT_TYPE_ALIAS:
      return gskb_format_validate_partial (format->v_alias.format, len, data, error);

    default:
        g_return_val_if_reached (0);
    }
}

gboolean
gskb_format_validate_packed(GskbFormat    *format,
                            guint          len,
                            const guint8  *data,
                            GError       **error)
{
  guint vlen = gskb_format_validate_partial (format, len, data, error);
  if (vlen == 0)
    return FALSE;
  if (vlen != len)
    {
      g_set_error (error, GSK_G_ERROR_DOMAIN,
                   GSK_ERROR_BAD_FORMAT,
                   "validate_packed: partial data: used %u bytes out of %u",
                   vlen, len);
      return FALSE;
    }
  return TRUE;
}

guint
gskb_format_unpack_value   (GskbFormat    *format,
                            const guint8  *data,
                            gpointer       value)
{
  switch (format->type)
    {
    case GSKB_FORMAT_TYPE_INT:
      switch (format->v_int.int_type)
        {
#define WRITE_CASE(UC, lc) \
        case GSKB_FORMAT_INT_##UC: \
          return gskb_##lc##_unpack (data, value);
        FOREACH_INT_TYPE(WRITE_CASE)
#undef WRITE_CASE
        }
      g_return_val_if_reached (0);

    case GSKB_FORMAT_TYPE_FLOAT:
      switch (format->v_float.float_type)
        {
#define WRITE_CASE(UC, lc) \
        case GSKB_FORMAT_FLOAT_##UC: \
          return gskb_##lc##_unpack (data, value);
        FOREACH_FLOAT_TYPE(WRITE_CASE)
#undef WRITE_CASE
        }
      g_return_val_if_reached (0);

    case GSKB_FORMAT_TYPE_STRING:
      return gskb_string_unpack (data, value);

    case GSKB_FORMAT_TYPE_FIXED_ARRAY:
      {
        guint rv = 0;
        GskbFormat *sub = format->v_fixed_array.element_format;
        guint i;
        for (i = 0; i < format->v_fixed_array.length; i++)
          {
            rv += gskb_format_unpack_value (sub, data + rv, value);
            value = (char*) value + format->any.c_size_of;
          }
        return rv;
      }
    case GSKB_FORMAT_TYPE_LENGTH_PREFIXED_ARRAY:
      {
        guint32 n_elements;
        guint rv = gskb_uint_unpack (data, &n_elements);
        GenericLengthPrefixedArray *array = value;
        GskbFormat *sub = format->v_length_prefixed_array.element_format;
        char *elements_at;
        guint i;
        array->length = n_elements;
        array->data = g_malloc (sub->any.c_size_of * n_elements);
        elements_at = array->data;
        for (i = 0; i < n_elements; i++)
          {
            rv += gskb_format_unpack_value (sub, data + rv, elements_at);
            elements_at += sub->any.c_size_of;
          }
        return rv;
      }
    case GSKB_FORMAT_TYPE_STRUCT:
      if (format->v_struct.is_extensible)
        {
          GArray *unknown_values = NULL;
          guint rv = 0;
          guint8 *has_bits = (guint8*) ((GskbUnknownValueArray*)value + 1);

          /* clear 'has' bits */
          memset (has_bits, 0, (format->v_struct.n_members + 7) / 8);

          for (;;)
            {
              guint32 code, member_len;
              GskbFormatStructMember *member;
              rv += gskb_uint_unpack (data + rv, &code);
              if (code == 0)
                break;
              rv += gskb_uint_unpack (data + rv, &member_len);
              member = gskb_format_struct_find_member_code (format, code);
              if (member == NULL)
                {
                  /* add unknown value */
                  GskbUnknownValue uv;
                  if (unknown_values == NULL)
                    unknown_values = g_array_new (FALSE, FALSE, sizeof (GskbUnknownValueArray));
                  uv.code = code;
                  uv.length = member_len;
                  uv.data = g_malloc (member_len);
                  memcpy (uv.data, data + rv, member_len);
                }
              else
                {
                  guint member_index = format->v_struct.members - member;  /* the member's index */
                  GskbFormat *member_format = member->format;
                  guint member_c_offset = format->v_struct.sys_member_offsets[member_index];
                  gpointer member_value = G_STRUCT_MEMBER_P (value, member_c_offset);

                  /* mark 'has' bit */
                  has_bits[member_index / 8] |= (1 << (member_index % 8));

                  /* unpack value */
                  rv += gskb_format_unpack_value (member_format, data + rv, member_value);
                }
              rv += member_len;
            }
          return rv;
        }
      else
        {
          guint rv = 0;
          guint i;
          for (i = 0; i < format->v_struct.n_members; i++)
            rv += gskb_format_unpack_value (format->v_struct.members[i].format,
                                            data + rv,
                                            G_STRUCT_MEMBER_P (value, format->v_struct.sys_member_offsets[i]));
          return rv;
        }
    case GSKB_FORMAT_TYPE_UNION:
      {
        gskb_enum evalue;
        GskbFormatUnionCase *c;
        guint rv;
        gpointer info;
        switch (format->v_union.type_format->v_enum.int_type)
          {
#define WRITE_CASE(UC, lc) \
          case GSKB_FORMAT_INT_##UC: \
            { \
              gskb_##lc t; \
              rv = gskb_##lc##_unpack (data, &t); \
              evalue = t; \
              break; \
            }
          FOREACH_INT_TYPE(WRITE_CASE)
#undef WRITE_CASE
          default:
            g_return_val_if_reached (0);
          }
        * (guint32 *) value = evalue;
        info = (char*)value + format->v_union.sys_info_offset;
        c = gskb_format_union_find_case_code (format, evalue);
        if (format->v_union.is_extensible)
          {
            guint32 piece_len;
            rv += gskb_uint_unpack (data + rv, &piece_len);
            if (c == NULL)
              {
                /* set to unknown */
                GskbUnknownValue *uv = info;
                g_assert (format->v_union.is_extensible);
                uv->code = evalue;
                uv->length = piece_len;
                uv->data = g_memdup (data + rv, piece_len);
                rv += piece_len;
              }
            else
              {
                * (guint32 *) value = evalue;
                if (c->format)
                  rv += gskb_format_unpack_value (c->format, data + rv, info);
              }
          }
        else
          {
            g_assert (c != NULL);
            * (guint32 *) value = evalue;
            if (c->format)
              rv += gskb_format_unpack_value (c->format, data + rv, info);
          }
        return rv;
      }
    case GSKB_FORMAT_TYPE_BIT_FIELDS:
      {
        guint len = format->any.c_size_of;
        guint8 *v_out = value;
        if (format->v_bit_fields.has_holes)
          {
            const guint8 *in = data;
            guint8 in_n_bits = 8;
            const guint8 *bpub = format->v_bit_fields.bits_per_unpacked_byte;
            guint i;
            for (i = 0; i < len; i++)
              {
                guint bits = *bpub++;
                if (bits < in_n_bits)
                  {
                    v_out[i] = (*in >> (8-in_n_bits)) & ((1<<bits)-1);
                    in_n_bits -= bits;
                  }
                else if (bits > in_n_bits)
                  {
                    v_out[i] = (in[0] >> (8-in_n_bits))
                             | (in[1] << in_n_bits);
                    in_n_bits = in_n_bits + 8 - bits;
                    in++;
                  }
                else
                  {
                    /* bits == in_n_bits */
                    v_out[i] = (in[0] >> (8-in_n_bits));
                    in++;
                    in_n_bits = 8;
                  }
              }
          }
        return format->any.fixed_length;
      }
    case GSKB_FORMAT_TYPE_ENUM:
      {
        switch (format->v_enum.int_type)
          {
          case GSKB_FORMAT_INT_UINT8:
            {
              guint8 v;
              gskb_uint8_unpack (data, &v);
              *(gskb_enum*)value = v;
              return 1;
            }
          case GSKB_FORMAT_INT_UINT16:
            {
              guint16 v;
              gskb_uint16_unpack (data, &v);
              *(gskb_enum*)value = v;
              return 1;
            }
          case GSKB_FORMAT_INT_UINT32:
            {
              guint32 v;
              gskb_uint32_unpack (data, &v);
              *(gskb_enum*)value = v;
              return 1;
            }
          case GSKB_FORMAT_INT_UINT:
            {
              guint32 v;
              guint rv = gskb_uint_unpack (data, &v);
              *(gskb_enum*)value = v;
              return rv;
            }
          default:
            g_return_val_if_reached (0);
          }
      }
    case GSKB_FORMAT_TYPE_ALIAS:
      return gskb_format_unpack_value (format->v_alias.format, data, value);
    default:
      g_return_val_if_reached (0);
    }
}

void
gskb_format_destruct_value (GskbFormat    *format,
                            gpointer       value)
{
  if (!format->any.requires_destruct)
    return;
  switch (format->type)
    {
    case GSKB_FORMAT_TYPE_STRING:
      g_free (* (char **) value);
      break;
    case GSKB_FORMAT_TYPE_FIXED_ARRAY:
      {
        guint i;
        GskbFormat *sub = format->v_fixed_array.element_format;
        for (i = 0; i < format->v_fixed_array.length; i++)
          {
            gskb_format_destruct_value (sub, value);
            value = (char*) value + sub->any.c_size_of;
          }
      }
      break;
    case GSKB_FORMAT_TYPE_LENGTH_PREFIXED_ARRAY:
      {
        GenericLengthPrefixedArray *s = value;
        GskbFormat *sub = format->v_length_prefixed_array.element_format;
        guint i;
        if (sub->any.requires_destruct)
          {
            char *data = s->data;
            for (i = 0; i < s->length; i++)
              {
                gskb_format_destruct_value (sub, data);
                data += sub->any.c_size_of;
              }
          }
        g_free (s->data);
      }
      break;
    case GSKB_FORMAT_TYPE_STRUCT:
      if (format->v_struct.is_extensible)
        {
          GskbUnknownValueArray *base = value;
          guint8 *bits = (guint8 *) (base + 1);
          guint i;
          for (i = 0; i < base->length; i++)
            g_free (base->values[i].data);
          for (i = 0; i < format->v_struct.n_members; i++)
            {
              GskbFormat *sub = format->v_struct.members[i].format;
              if (sub->any.requires_destruct
                && ((bits[i/8] & (1<<(i%8))) != 0))
                {
                  gpointer member_ptr = (char*)base
                                      + format->v_struct.sys_member_offsets[i];
                  gskb_format_destruct_value (sub, member_ptr);
                }
            }
        }
      else
        {
          guint i;
          for (i = 0; i < format->v_struct.n_members; i++)
            {
              GskbFormat *sub = format->v_struct.members[i].format;
              gpointer member_ptr = (char*)value
                                  + format->v_struct.sys_member_offsets[i];
              gskb_format_destruct_value (sub, member_ptr);
            }
        }
      break;
    case GSKB_FORMAT_TYPE_UNION:
      {
        guint32 code = G_STRUCT_MEMBER (guint32, value, 0);
        gpointer info = G_STRUCT_MEMBER_P (value, format->v_union.sys_info_offset);
        if (format->v_union.is_extensible
         && code == GSKB_FORMAT_UNION_UNKNOWN_VALUE_CODE)
          {
            GskbUnknownValue *uv = info;
            g_free (uv->data);
          }
        else
          {
            GskbFormatUnionCase *c = gskb_format_union_find_case_code (format, code);
            g_assert (c != NULL);
            if (c->format && c->format->any.requires_destruct)
              gskb_format_destruct_value (c->format, info);
          }
      }
      break;
    case GSKB_FORMAT_TYPE_ALIAS:
      gskb_format_destruct_value (format->v_alias.format, value);
      break;
    default:
      g_assert_not_reached ();
    }
}

#if 0           /* save til gskb_format_unpack_value() is tested */
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
#endif



GskbFormatInt gskb_format_ints_array[GSKB_N_FORMAT_INT_TYPES] =
{
  {                             /* int8 */
    {
      GSKB_FORMAT_TYPE_INT,
      1,                /* ref_count */
      "int8", "Int8", "int8",
      GSKB_FORMAT_CTYPE_INT8,
      GSKB_ALIGNOF_UINT8, sizeof(gint8),
      FALSE,            /* always_by_pointer */
      FALSE,            /* requires_destruct */
      TRUE,             /* is global */
      1                 /* fixed_length */
    },
    GSKB_FORMAT_INT_INT8
  },
  {                             /* int16 */
    {
      GSKB_FORMAT_TYPE_INT,
      1,                /* ref_count */
      "int16", "Int16", "int16",
      GSKB_FORMAT_CTYPE_INT16,
      GSKB_ALIGNOF_UINT16, sizeof(gint16),
      FALSE,            /* always_by_pointer */
      FALSE,            /* requires_destruct */
      TRUE,             /* is global */
      2                 /* fixed_length */
    },
    GSKB_FORMAT_INT_INT16
  },
  {                             /* int32 */
    {
      GSKB_FORMAT_TYPE_INT,
      1,                /* ref_count */
      "int32", "Int32", "int32",
      GSKB_FORMAT_CTYPE_INT32,
      GSKB_ALIGNOF_UINT32, sizeof(gint32),
      FALSE,            /* always_by_pointer */
      FALSE,            /* requires_destruct */
      TRUE,             /* is global */
      4                 /* fixed_length */
    },
    GSKB_FORMAT_INT_INT32
  },
  {                             /* int64 */
    {
      GSKB_FORMAT_TYPE_INT,
      1,                /* ref_count */
      "int64", "Int64", "int64",
      GSKB_FORMAT_CTYPE_INT64,
      GSKB_ALIGNOF_UINT64, sizeof(gint64),
      FALSE,            /* always_by_pointer */
      FALSE,            /* requires_destruct */
      TRUE,             /* is global */
      8                 /* fixed_length */
    },
    GSKB_FORMAT_INT_INT64
  },
  {                             /* uint8 */
    {
      GSKB_FORMAT_TYPE_INT,
      1,                /* ref_count */
      "uint8", "Int8", "uint8",
      GSKB_FORMAT_CTYPE_UINT8,
      GSKB_ALIGNOF_UINT8, sizeof(guint8),
      FALSE,            /* always_by_pointer */
      FALSE,            /* requires_destruct */
      TRUE,             /* is global */
      1                 /* fixed_length */
    },
    GSKB_FORMAT_INT_UINT8
  },
  {                             /* uint16 */
    {
      GSKB_FORMAT_TYPE_INT,
      1,                /* ref_count */
      "uint16", "UInt16", "uint16",
      GSKB_FORMAT_CTYPE_UINT16,
      GSKB_ALIGNOF_UINT16, sizeof(guint16),
      FALSE,            /* always_by_pointer */
      FALSE,            /* requires_destruct */
      TRUE,             /* is global */
      2                 /* fixed_length */
    },
    GSKB_FORMAT_INT_UINT16
  },
  {                             /* uint32 */
    {
      GSKB_FORMAT_TYPE_INT,
      1,                /* ref_count */
      "uint32", "UInt32", "uint32",
      GSKB_FORMAT_CTYPE_UINT32,
      GSKB_ALIGNOF_UINT32, sizeof(guint32),
      FALSE,            /* always_by_pointer */
      FALSE,            /* requires_destruct */
      TRUE,             /* is global */
      4                 /* fixed_length */
    },
    GSKB_FORMAT_INT_UINT32
  },
  {                             /* uint64 */
    {
      GSKB_FORMAT_TYPE_INT,
      1,                /* ref_count */
      "uint64", "UInt64", "uint64",
      GSKB_FORMAT_CTYPE_UINT64,
      GSKB_ALIGNOF_UINT64, sizeof(guint64),
      FALSE,            /* always_by_pointer */
      FALSE,            /* requires_destruct */
      TRUE,             /* is global */
      8                 /* fixed_length */
    },
    GSKB_FORMAT_INT_UINT64
  },
  {                             /* int */
    {
      GSKB_FORMAT_TYPE_INT,
      1,                /* ref_count */
      "int", "Int", "int",
      GSKB_FORMAT_CTYPE_INT32,
      GSKB_ALIGNOF_UINT32, sizeof(guint32),
      FALSE,            /* always_by_pointer */
      FALSE,            /* requires_destruct */
      TRUE,             /* is global */
      0                 /* fixed_length */
    },
    GSKB_FORMAT_INT_INT
  },
  {                             /* uint */
    {
      GSKB_FORMAT_TYPE_INT,
      1,                /* ref_count */
      "uint", "UInt", "uint",
      GSKB_FORMAT_CTYPE_UINT32,
      GSKB_ALIGNOF_UINT32, sizeof(guint32),
      FALSE,            /* always_by_pointer */
      FALSE,            /* requires_destruct */
      TRUE,             /* is global */
      0                 /* fixed_length */
    },
    GSKB_FORMAT_INT_UINT
  },
  {                             /* long */
    {
      GSKB_FORMAT_TYPE_INT,
      1,                /* ref_count */
      "long", "Long", "long",
      GSKB_FORMAT_CTYPE_INT64,
      GSKB_ALIGNOF_UINT64, sizeof(guint64),
      FALSE,            /* always_by_pointer */
      FALSE,            /* requires_destruct */
      TRUE,             /* is global */
      0                 /* fixed_length */
    },
    GSKB_FORMAT_INT_LONG
  },
  {                             /* ulong */
    {
      GSKB_FORMAT_TYPE_INT,
      1,                /* ref_count */
      "ulong", "ULong", "ulong",
      GSKB_FORMAT_CTYPE_UINT64,
      GSKB_ALIGNOF_UINT64, sizeof(guint64),
      FALSE,            /* always_by_pointer */
      FALSE,            /* requires_destruct */
      TRUE,             /* is global */
      0                 /* fixed_length */
    },
    GSKB_FORMAT_INT_ULONG
  },
  {                             /* bit */
    {
      GSKB_FORMAT_TYPE_INT,
      1,                /* ref_count */
      "bit", "Bit", "bit",
      GSKB_FORMAT_CTYPE_UINT8,
      GSKB_ALIGNOF_UINT8, sizeof(guint8),
      FALSE,            /* always_by_pointer */
      FALSE,            /* requires_destruct */
      TRUE,             /* is global */
      1                 /* fixed_length */
    },
    GSKB_FORMAT_INT_BIT
  },
};
