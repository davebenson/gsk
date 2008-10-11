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
typedef struct _GskbFormatBitField GskbFormatBitField;
typedef struct _GskbFormatBitFields GskbFormatBitFields;
typedef struct _GskbFormatAlias GskbFormatAlias;
typedef struct _GskbUnknownValue GskbUnknownValue;
typedef struct _GskbUnknownValueArray GskbUnknownValueArray;
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
  GSKB_FORMAT_TYPE_BIT_FIELDS,
  GSKB_FORMAT_TYPE_ENUM,
  GSKB_FORMAT_TYPE_ALIAS
} GskbFormatType;
#define GSKB_N_FORMAT_TYPES 10

const char *gskb_format_type_name (GskbFormatType type);

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
extern GskbFormat *gskb_format_ints_array[];
#define gskb_format_int_type_name(int_type) \
  ((const char *)(gskb_format_ints_array[(int_type)]->any.name))

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
  guint code;           /* 0 if not extensible */
  const char *name;
  GskbFormat *format;
};
struct _GskbFormatStruct
{
  GskbFormatAny base;
  gboolean is_extensible;
  guint n_members;
  GskbFormatStructMember *members;
  GskbFormat *contents_format;
  gpointer name_to_index;
  gpointer code_to_index;
};

struct _GskbFormatUnionCase
{
  const char *name;
  guint code;
  GskbFormat *format;
};
struct _GskbFormatUnion
{
  GskbFormatAny base;
  gboolean is_extensible;
  guint n_cases;
  GskbFormatUnionCase *cases;
  gpointer name_to_index;
  gpointer code_to_index;
  GskbFormat *type_format;
};

struct _GskbFormatBitField
{
  const char *name;
  guint length;                 /* 1..8 */
};
struct _GskbFormatBitFields
{
  GskbFormatAny base;
  // XXX: do we want an is_extensible member?
  guint n_fields;
  GskbFormatBitField *fields;

  /* optimization: whether padding is needed to keep things aligned. */
  gboolean has_holes;
  guint8 *bits_per_unpacked_byte;

  guint total_bits;


  gpointer name_to_index;
};

struct _GskbFormatEnumValue
{
  const char *name;
  guint code;
};

struct _GskbFormatEnum
{
  GskbFormatAny base;
  GskbFormatIntType int_type;
  guint n_values;
  GskbFormatEnumValue *values;
  gboolean is_extensible;

  gpointer name_to_index;
  gpointer value_to_index;
};
#define GSKB_FORMAT_ENUM_UNKNOWN_VALUE_CODE 0x7fffffff

struct _GskbFormatAlias
{
  GskbFormatAny base;
  GskbFormat *format;
};

struct _GskbUnknownValue
{
  guint32 code;
  gsize length;
  guint8 *data;
};
struct _GskbUnknownValueArray
{
  guint length;
  GskbUnknownValue *values;
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
  GskbFormatBitFields  v_bit_fields;
  GskbFormatEnum       v_enum;
  GskbFormatAlias      v_alias;
};

GskbFormat *gskb_format_fixed_array_new (guint length,
                                         GskbFormat *element_format);
GskbFormat *gskb_format_length_prefixed_array_new (GskbFormat *element_format);
GskbFormat *gskb_format_struct_new (const char *name,
                                    gboolean is_extensible,
                                    guint n_members,
                                    GskbFormatStructMember *members,
                                    GError       **error);
GskbFormat *gskb_format_union_new (const char *name,
                                   gboolean is_extensible,
                                   GskbFormatIntType int_type,
                                   guint n_cases,
                                   GskbFormatUnionCase *cases,
                                   GError       **error);
GskbFormat *gskb_format_enum_new  (const char *name,
                                   gboolean    is_extensible,
                                   GskbFormatIntType int_type,
                                   guint n_values,
                                   GskbFormatEnumValue *values,
                                   GError       **error);

/* ref-count handling */
GskbFormat *gskb_format_ref (GskbFormat *format);
void        gskb_format_unref (GskbFormat *format);

/* methods on certain formats */
GskbFormatStructMember *gskb_format_struct_find_member (GskbFormat *format,
                                                        const char *name);
GskbFormatStructMember *gskb_format_struct_find_member_code
                                                       (GskbFormat *format,
                                                        guint       code);
GskbFormatUnionCase    *gskb_format_union_find_case    (GskbFormat *format,
                                                        const char *name);
GskbFormatUnionCase    *gskb_format_union_find_case_code(GskbFormat *format,
                                                        guint       case_value);
GskbFormatEnumValue    *gskb_format_enum_find_value    (GskbFormat *format,
                                                        const char *name);
GskbFormatEnumValue    *gskb_format_enum_find_value_code(GskbFormat *format,
                                                        guint32      code);
GskbFormatBitField     *gskb_format_bit_fields_find_field(GskbFormat *format,
                                                        const char *name);


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
guint       gskb_format_validate_partial(GskbFormat    *format,
                                        guint          len,
                                        const guint8  *data,
                                        GError       **error);
gboolean    gskb_format_validate_packed(GskbFormat    *format,
                                        guint          len,
                                        const guint8  *data,
                                        GError       **error);
guint       gskb_format_unpack         (GskbFormat    *format,
                                        const guint8  *data,
                                        gpointer       value);
void        gskb_format_destruct_value (GskbFormat    *format,
                                        gpointer       value);
gboolean    gskb_format_unpack_value_mempool
                                       (GskbFormat    *format,
                                        const guint8  *data,
                                        guint         *n_used_out,
                                        gpointer       value,
                                        GskMemPool    *mem_pool);


/* useful for code generation */
const char *gskb_format_type_enum_name (GskbFormatType type);
const char *gskb_format_ctype_enum_name (GskbFormatCType type);
const char *gskb_format_int_type_enum_name (GskbFormatIntType type);
const char *gskb_format_float_type_enum_name (GskbFormatFloatType type);

#endif
