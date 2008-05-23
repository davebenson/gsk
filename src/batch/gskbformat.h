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
  GSKB_FORMAT_TYPE_ALIAS
} GskbFormatType;

struct _GskbFormatAny
{
  GskbFormatType type;		/* must be first */
  guint ref_count;
};

typedef enum
{
  GSKB_FORMAT_INT_INT8,
  GSKB_FORMAT_INT_INT16,
  GSKB_FORMAT_INT_INT32,
  GSKB_FORMAT_INT_INT64,
  GSKB_FORMAT_INT_UINT8,
  GSKB_FORMAT_INT_UINT16,
  GSKB_FORMAT_INT_UINT32,
  GSKB_FORMAT_INT_UINT64,
  GSKB_FORMAT_INT_VARLEN_INT32,
  GSKB_FORMAT_INT_VARLEN_INT64,
  GSKB_FORMAT_INT_VARLEN_UINT32,
  GSKB_FORMAT_INT_VARLEN_UINT64,
} GskbFormatIntType;

struct _GskbFormatInt
{
  GskbFormatAny     base;
  GskbFormatIntType int_type;
  const char       *name;
};

typedef enum
{
  GSKB_FORMAT_FLOAT_SINGLE,
  GSKB_FORMAT_FLOAT_DOUBLE,
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
  guint len;
  GskbFormat *element_format;
};

struct _GskbFormatLengthPrefixedArray
{
  GskbFormatAny base;
  guint len;
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
  const char *name;
  guint n_members;
  GskbFormatStructMember *members;
  GHashTable *name_to_member;
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
  const char *name;
  guint n_cases;
  GskbFormatUnionCase *cases;
  GHashTable *name_to_case;
  GHashTable *value_to_case;
  GskbFormat *type_enum;
};

struct _GskbFormatEnumValue
{
  const char *name;
  guint value;
};
struct _GskbFormatEnum
{
  GskbFormatAny base;
  const char *name;
  GskbFormatIntType int_type;
  guint n_values;
  GskbFormatEnumValue *values;

  /* char* => GskbFormatEnumValue */
  GHashTable *name_to_value;
  /* map from int => GskbFormatEnumValue,
     or NULL if the enum values are 0-indexed corresponding to 'values' */
  GHashTable *value_to_value;
};
struct _GskbFormatAlias
{
  GskbFormatAny base;
  const char *name;
  GskbFormat *format;
};

union _GskbFormat
{
  GskbFormatType   type;
  GskbFormatAny    any;

  GskbFormatInt    v_int;
  GskbFormatFloat  v_float;
  GskbFormatString v_string;
  GskbFormatFixedArray  v_fixed_array;
  GskbFormatLengthPrefixedArray  v_length_prefixed_array;
  GskbFormatStruct v_struct;
  GskbFormatUnion  v_union;
  GskbFormatEnum   v_enum;
  GskbFormatAlias  v_alias;
};
GskbFormat *gskb_format_peek_int8 (void);
GskbFormat *gskb_format_peek_int16 (void);
GskbFormat *gskb_format_peek_int32 (void);
GskbFormat *gskb_format_peek_int64 (void);
GskbFormat *gskb_format_peek_uint8 (void);
GskbFormat *gskb_format_peek_uint16 (void);
GskbFormat *gskb_format_peek_uint32 (void);
GskbFormat *gskb_format_peek_uint64 (void);
GskbFormat *gskb_format_peek_varlen_int32 (void);
GskbFormat *gskb_format_peek_varlen_int64 (void);
GskbFormat *gskb_format_peek_varlen_uint32 (void);
GskbFormat *gskb_format_peek_varlen_uint64 (void);
GskbFormat *gskb_format_peek_float32 (void);
GskbFormat *gskb_format_peek_float64 (void);
GskbFormat *gskb_format_peek_string (void);
GskbFormat *gskb_format_fixed_array_new (guint length,
                                         GskbFormat *element_format);
GskbFormat *gskb_format_length_prefixed_array_new (GskbFormatIntType len_enc,
                                                   guint max_len,
                                                   GskbFormat *element_format);
GskbFormat *gskb_format_struct_new (const char *name,
                                    guint n_members,
                                    GskbFormatStructMember *members);
GskbFormat *gskb_format_union_new (const char *name,
                                   GskbFormatIntType int_type,
                                   guint n_cases,
                                   GskbFormatUnionCase *cases);
GskbFormat *gskb_format_enum_new  (const char *name,
                                   GskbFormatIntType int_type,
                                   guint n_values,
                                   GskbFormatEnumValue *values);

/* methods on certain formats */
GskbFormatStructMember *gskb_format_struct_find_member (GskbFormat *format,
                                                        const char *name);
GskbFormatUnionCase    *gskb_format_union_find_case    (GskbFormat *format,
                                                        const char *name);
GskbFormatUnionCase    *gskb_format_union_find_case_value(GskbFormat *format,
                                                        guint       case_value);

/* used internally by union_new and struct_new */
gboolean    gskb_format_is_anonymous   (GskbFormat *format);
GskbFormat *gskb_format_name_anonymous (GskbFormat *anon_format,
                                        const char *name);

