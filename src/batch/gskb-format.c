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

/* align an offset to 'alignment';
   note that 'alignment' is evaluated twice!
   only works if alignment is a power-of-two. */
#define ALIGN(unaligned_offset, alignment)          \
     ( ((unaligned_offset)+((alignment)-1))         \
                & ~((alignment)-1)          )

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
                        guint n_members,
                        GskbFormatStructMember *members)
{
  GskbFormatStruct *rv;
  guint i;
  guint size;
  guint align_of;
  gboolean requires_destruct;
  gboolean is_fixed;
  guint fixed_length;
  g_return_val_if_reach (n_members > 0, NULL);

  rv = g_new0 (GskbFormatStruct, 1);
  rv->base.type = GSKB_FORMAT_TYPE_STRUCT;
  rv->base.ref_count = 1;
  rv->base.name = g_strdup (name);
  rv->base.TypeName = g_strdup (name);
  rv->base.lc_name = name ? mixed_to_lc (name) : NULL;
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
    rv->name_to_member_index = gskb_str_table_new (sizeof (gint32),
                                                   n_members, entries);
    g_free (indices);
    g_free (entries);
  }
  return (GskbFormat *) rv;
}

GskbFormat *
gskb_format_union_new (const char *name,
                       GskbFormatIntType int_type,
                       guint n_cases,
                       GskbFormatUnionCase *cases)
{
  GskbFormatUnion *rv;
  guint info_align, info_size;
  guint i;
  GskbFormat *type_format;

  /* XXX: this goes through very similar attempts at perfect hashing.
     combine the work.  perhaps using a private api. */
  {
    type_format = gskb_format_enum_new (...);
  }

  rv = g_new0 (GskbFormatUnion, 1);
  rv->base.type = GSKB_FORMAT_TYPE_UNION;
  rv->base.ref_count = 1;
  rv->base.name = g_strdup (name);
  rv->base.TypeName = g_strdup (name);
  rv->base.lc_name = name ? mixed_to_lc (name) : NULL;
  rv->base.ctype = GSKB_FORMAT_CTYPE_COMPOSITE;

  rv->base.members[0].name = g_strdup ("type");
  rv->base.members[0].ctype = GSKB_FORMAT_CTYPE_UINT32;
  rv->base.members[0].c_offset_of = 0;
  rv->base.members[0].format = type_format;

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
  return rv;
}

GskbFormat *
gskb_format_enum_new  (const char *name,
                       GskbFormatIntType int_type,
                       guint n_values,
                       GskbFormatEnumValue *values)
{
  ...
}

GskbFormat *
gskb_format_extensible_new(const char *name,
                           guint n_known_members,
                           GskbFormatExtensibleMember *known_members)
{
  ...
}

/* methods on certain formats */
GskbFormatStructMember *gskb_format_struct_find_member (GskbFormat *format,
                                                        const char *name)
{
  const gint32 *index_ptr;
  g_return_val_if_fail (format->type == GSKB_FORMAT_TYPE_STRUCT, NULL);
  index_ptr = gskb_str_table_lookup (format->v_struct.name_to_member_index, name);
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
  index_ptr = gskb_str_table_lookup (format->v_union.name_to_case_index, name);
  if (index_ptr == NULL)
    return NULL;
  else
    return format->v_union.cases + (*index_ptr);
}
GskbFormatUnionCase    *gskb_format_union_find_case_value(GskbFormat *format,
                                                        guint       case_value)
{
  ...
}
GskbFormatExtensibleMember *gskb_format_extensible_find_member
                                                       (GskbFormat *format,
                                                        const char *name)
{
  ...
}
GskbFormatExtensibleMember *gskb_format_extensible_find_member_code
                                                       (GskbFormat *format,
                                                        guint       code)
{
  ...
}


/* used internally by union_new and struct_new */
gboolean    gskb_format_is_anonymous   (GskbFormat *format)
{
  return format->any.lc_name == NULL;
}

GskbFormat *gskb_format_name_anonymous (GskbFormat *anon_format,
                                        const char *name);

void        gskb_format_pack           (GskbFormat    *format,
                                        gconstpointer  value,
                                        GskbAppendFunc append_func,
                                        gpointer       append_func_data)
{
  ...
}
guint       gskb_format_get_packed_size(GskbFormat    *format,
                                        gconstpointer  value)
{
  ...
}
guint       gskb_format_pack_slab      (GskbFormat    *format,
                                        gconstpointer  value,
                                        guint8        *slab)
{
  ...
}

gboolean    gskb_format_unpack_value   (GskbFormat    *format,
                                        const guint8  *data,
                                        guint         *n_used_out,
                                        gpointer       value,
                                        GError       **error)
{
  ...
}
void        gskb_format_clear_value    (GskbFormat    *format,
                                        gpointer       value)
{
  ...
}
gboolean    gskb_format_unpack_value_mempool
                                       (GskbFormat    *format,
                                        const guint8  *data,
                                        guint         *n_used_out,
                                        gpointer       value,
                                        GskMemPool    *mem_pool,
                                        GError       **error)
{
  ...
}
