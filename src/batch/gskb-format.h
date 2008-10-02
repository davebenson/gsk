/*
    GSKB - a batch processing framework

    gskb-format: specify interpretation of packed bytes.

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

#ifndef __GSKB_FORMAT_H_
#define __GSKB_FORMAT_H_

#include <glib.h>
#include "../gskmempool.h"

/* description of data format */

typedef union _GskbFormat GskbFormat;

typedef struct _GskbFormatAny GskbFormatAny;
typedef struct _GskbFormatInt GskbFormatInt;
typedef struct _GskbFormatFloat GskbFormatFloat;
typedef struct _GskbFormatString GskbFormatString;
typedef struct _GskbFormatFixedArray GskbFormatFixedArray;
typedef struct _GskbFormatLengthPrefixedArray GskbFormatLengthPrefixedArray;
typedef struct _GskbFormatStructMember GskbFormatStructMember;
typedef struct _GskbFormatStruct GskbFormatStruct;
typedef struct _GskbFormatUnionCase GskbFormatUnionCase;
typedef struct _GskbFormatUnion GskbFormatUnion;
typedef struct _GskbFormatEnumValue GskbFormatEnumValue;
typedef struct _GskbFormatEnum GskbFormatEnum;
typedef struct _GskbFormatAlias GskbFormatAlias;
typedef struct _GskbFormatExtensibleMember GskbFormatExtensibleMember;
typedef struct _GskbFormatExtensible GskbFormatExtensible;
typedef struct _GskbExtensibleUnknownValue GskbExtensibleUnknownValue;
typedef struct _GskbExtensible GskbExtensible;
typedef struct _GskbFormatCMember GskbFormatCMember;

typedef enum
{
  GSKB_FORMAT_TYPE_INT,
  GSKB_FORMAT_TYPE_FLOAT,
  GSKB_FORMAT_TYPE_STRING,
  GSKB_FORMAT_TYPE_FIXED_ARRAY,
  GSKB_FORMAT_TYPE_LENGTH_PREFIXED_ARRAY,
  GSKB_FORMAT_TYPE_STRUCT,
  GSKB_FORMAT_TYPE_UNION,
  GSKB_FORMAT_TYPE_ENUM,
  GSKB_FORMAT_TYPE_ALIAS,
  GSKB_FORMAT_TYPE_EXTENSIBLE
} GskbFormatType;
#define GSKB_N_FORMAT_TYPES 10

typedef enum
{
  GSKB_FORMAT_CTYPE_INT8,
  GSKB_FORMAT_CTYPE_INT16,
  GSKB_FORMAT_CTYPE_INT32,
  GSKB_FORMAT_CTYPE_INT64,
  GSKB_FORMAT_CTYPE_UINT8,
  GSKB_FORMAT_CTYPE_UINT16,
  GSKB_FORMAT_CTYPE_UINT32,
  GSKB_FORMAT_CTYPE_UINT64,
  GSKB_FORMAT_CTYPE_FLOAT32,
  GSKB_FORMAT_CTYPE_FLOAT64,
  GSKB_FORMAT_CTYPE_STRING,
  GSKB_FORMAT_CTYPE_COMPOSITE,
  GSKB_FORMAT_CTYPE_UNION_DATA,
  GSKB_FORMAT_CTYPE_ARRAY_POINTER
} GskbFormatCType;

struct _GskbFormatCMember
{
  char *name;
  GskbFormatCType ctype;
  GskbFormat *format;           /* NULL for UNION_DATA */
  guint c_offset_of;
};

struct _GskbFormatAny
{
  GskbFormatType type;		/* must be first */
  guint ref_count;

  char *name, *TypeName, *lc_name;

  /* how this maps to C structures */
  GskbFormatCType ctype;
  guint c_size_of, c_align_of;
  guint8 always_by_pointer : 1;
  guint8 requires_destruct : 1;

  guint8 is_global : 1;         /* private */

  /* general info about this things packed values */
  guint fixed_length;           /* or 0 */

  /* for ctype==COMPOSITE */
  guint n_members;
  GskbFormatCMember *members;
};

typedef enum
{
  GSKB_FORMAT_INT_INT8,         /* signed byte                         */
  GSKB_FORMAT_INT_INT16,        /* int16, encoded little-endian        */
  GSKB_FORMAT_INT_INT32,        /* int32, encoded little-endian        */
  GSKB_FORMAT_INT_INT64,        /* int32, encoded little-endian        */
  GSKB_FORMAT_INT_UINT8,        /* unsigned byte                       */
  GSKB_FORMAT_INT_UINT16,       /* uint16, encoded little-endian       */
  GSKB_FORMAT_INT_UINT32,       /* uint32, encoded little-endian       */
  GSKB_FORMAT_INT_UINT64,       /* uint32, encoded little-endian       */
  GSKB_FORMAT_INT_INT,          /* var-len signed int, max 32 bits     */
  GSKB_FORMAT_INT_UINT,         /* var-len unsigned int, max 32 bits   */
  GSKB_FORMAT_INT_LONG,         /* var-len signed int, max 64 bits     */
  GSKB_FORMAT_INT_ULONG,        /* var-len unsigned int, max 64 bits   */
  GSKB_FORMAT_INT_BIT           /* byte that may only be set to 0 or 1 */
} GskbFormatIntType;

struct _GskbFormatInt
{
  GskbFormatAny     base;
  GskbFormatIntType int_type;
};

typedef enum
{
  GSKB_FORMAT_FLOAT_FLOAT32,
  GSKB_FORMAT_FLOAT_FLOAT64,
} GskbFormatFloatType;

struct _GskbFormatFloat
{
  GskbFormatAny base;
  GskbFormatFloatType float_type;
};

struct _GskbFormatString
{
  GskbFormatAny base;
};

struct _GskbFormatFixedArray
{
  GskbFormatAny base;
  guint length;
  GskbFormat *element_format;
};

struct _GskbFormatLengthPrefixedArray
{
  GskbFormatAny base;
  GskbFormat *element_format;
};

struct _GskbFormatStructMember
{
  const char *name;
  GskbFormat *format;
};
struct _GskbFormatStruct
{
  GskbFormatAny base;
  guint n_members;
  GskbFormatStructMember *members;
  gpointer name_to_member_index;
};

struct _GskbFormatUnionCase
{
  const char *name;
  guint value;
  GskbFormat *format;
};
struct _GskbFormatUnion
{
  GskbFormatAny base;
  guint n_cases;
  GskbFormatUnionCase *cases;
  gpointer name_to_case_index;
  gpointer value_to_case_index;
  GskbFormat *type_format;
};

struct _GskbFormatEnumValue
{
  const char *name;
  guint value;
};

struct _GskbFormatEnum
{
  GskbFormatAny base;
  GskbFormatIntType int_type;
  guint n_values;
  GskbFormatEnumValue *values;

  gpointer name_to_index;
  gpointer value_to_index;
};

struct _GskbFormatAlias
{
  GskbFormatAny base;
  GskbFormat *format;
};

struct _GskbFormatExtensibleMember
{
  guint code;
  char *name;
  GskbFormat *format;
};

struct _GskbFormatExtensible
{
  GskbFormatAny base;
  guint n_members;
  GskbFormatExtensibleMember *members;

  gpointer name_to_index;
  gpointer code_to_index;
};

struct _GskbExtensibleUnknownValue
{
  guint32 code;
  gsize len;
  guint8 *data;
};
struct _GskbExtensible
{
  guint n_unknown_members;
  GskbExtensibleUnknownValue *unknown_members;
};

union _GskbFormat
{
  GskbFormatType   type;
  GskbFormatAny    any;

  GskbFormatInt        v_int;
  GskbFormatFloat      v_float;
  GskbFormatString     v_string;
  GskbFormatFixedArray v_fixed_array;
  GskbFormatLengthPrefixedArray  v_length_prefixed_array;
  GskbFormatStruct     v_struct;
  GskbFormatUnion      v_union;
  GskbFormatEnum       v_enum;
  GskbFormatAlias      v_alias;
  GskbFormatExtensible v_extensible;
};

GskbFormat *gskb_format_fixed_array_new (guint length,
                                         GskbFormat *element_format);
GskbFormat *gskb_format_length_prefixed_array_new (GskbFormat *element_format);
GskbFormat *gskb_format_struct_new (const char *name,
                                    guint n_members,
                                    GskbFormatStructMember *members,
                                    GError       **error);
GskbFormat *gskb_format_union_new (const char *name,
                                   GskbFormatIntType int_type,
                                   guint n_cases,
                                   GskbFormatUnionCase *cases,
                                   GError       **error);
GskbFormat *gskb_format_enum_new  (const char *name,
                                   GskbFormatIntType int_type,
                                   guint n_values,
                                   GskbFormatEnumValue *values,
                                   GError       **error);
GskbFormat *gskb_format_extensible_new(const char *name,
                                   guint n_known_members,
                                   GskbFormatExtensibleMember *known_members,
                                   GError       **error);

/* ref-count handling */
GskbFormat *gskb_format_ref (GskbFormat *format);
void        gskb_format_unref (GskbFormat *format);

/* methods on certain formats */
GskbFormatStructMember *gskb_format_struct_find_member (GskbFormat *format,
                                                        const char *name);
GskbFormatUnionCase    *gskb_format_union_find_case    (GskbFormat *format,
                                                        const char *name);
GskbFormatUnionCase    *gskb_format_union_find_case_value(GskbFormat *format,
                                                        guint       case_value);
GskbFormatExtensibleMember *gskb_format_extensible_find_member
                                                       (GskbFormat *format,
                                                        const char *name);
GskbFormatExtensibleMember *gskb_format_extensible_find_member_code
                                                       (GskbFormat *format,
                                                        guint       code);


/* used internally by union_new and struct_new */
gboolean    gskb_format_is_anonymous   (GskbFormat *format);
void        gskb_format_name_anonymous (GskbFormat *anon_format,
                                        const char *name);

typedef void (*GskbAppendFunc) (guint len,
                                const guint8 *data,
                                gpointer func_data);

void        gskb_format_pack           (GskbFormat    *format,
                                        gconstpointer  value,
                                        GskbAppendFunc append_func,
                                        gpointer       append_func_data);
guint       gskb_format_get_packed_size(GskbFormat    *format,
                                        gconstpointer  value);
guint       gskb_format_pack_slab      (GskbFormat    *format,
                                        gconstpointer  value,
                                        guint8        *slab);

gboolean    gskb_format_unpack_value   (GskbFormat    *format,
                                        const guint8  *data,
                                        guint         *n_used_out,
                                        gpointer       value,
                                        GError       **error);
void        gskb_format_clear_value    (GskbFormat    *format,
                                        gpointer       value);
gboolean    gskb_format_unpack_value_mempool
                                       (GskbFormat    *format,
                                        const guint8  *data,
                                        guint         *n_used_out,
                                        gpointer       value,
                                        GskMemPool    *mem_pool,
                                        GError       **error);


#endif
