
/* tedious, consistent with other output functions,
 * implementations of all fundamental types. */

/* the following types: (see is_fundamental() in gskb-format-codegen.c)
 *    integers:int8,int16,int32,int64,uint8,uint16,uint32,uint64,int,
 *             uint,long,ulong,bit
 *    floats:  float32,float64
 *    string
 * meet the following functions: (see GskbCodegenOutputFunction enum)
 *     pack, get_packed_size, pack_slab, validate, unpack, unpack_full,
 *     destruct
 * and the macro 'format'.
 */ 

/* types */
typedef gint8 gskb_int8;
typedef gint16 gskb_int16;
typedef gint32 gskb_int32;
typedef gint64 gskb_int64;
typedef guint8 gskb_uint8;
typedef guint16 gskb_uint16;
typedef guint32 gskb_uint32;
typedef guint64 gskb_uint64;
typedef gint32 gskb_int;
typedef guint32 gskb_uint;
typedef gint64 gskb_long;
typedef guint64 gskb_ulong;
typedef guint8 gskb_bit;
typedef gfloat gskb_float32;
typedef gdouble gskb_float64;
typedef char * gskb_string;
typedef guint32 gskb_enum;              /* all enums map to this type in the c binding */

#define GSKB_FOREACH_FUNDAMENTAL_TYPE(macro) \
 macro(int8, ) \
 macro(int16, ) \
 macro(int32, ) \
 macro(int64, ) \
 macro(uint8, ) \
 macro(uint16, ) \
 macro(uint32, ) \
 macro(uint64, ) \
 macro(int, ) \
 macro(uint, ) \
 macro(long, ) \
 macro(ulong, ) \
 macro(bit, ) \
 macro(float32, ) \
 macro(float64, ) \
 macro(string, const)


/* declare pack() */
#define GSKB_DECLARE_PACK(lctypename, maybe_const) \
G_INLINE_FUNC void  gskb_##lctypename##_pack (maybe_const gskb_##lctypename value, \
                                              GskbAppendFunc append, \
                                              gpointer append_data);

GSKB_FOREACH_FUNDAMENTAL_TYPE(GSKB_DECLARE_PACK)
#undef GSKB_DECLARE_PACK

#define GSKB_IS_LITTLE_ENDIAN       (G_BYTE_ORDER==G_LITTLE_ENDIAN)

#if 1  /* set to 0 to test unoptimized path on little-endian machines */
# define GSKB_OPTIMIZE_LITTLE_ENDIAN GSKB_IS_LITTLE_ENDIAN
#else
# define GSKB_OPTIMIZE_LITTLE_ENDIAN 0
#endif

/* declare get_packed_size() */
#define gskb_int8_get_packed_size(value)    1
#define gskb_int16_get_packed_size(value)   2
#define gskb_int32_get_packed_size(value)   4
#define gskb_int64_get_packed_size(value)   8
#define gskb_uint8_get_packed_size(value)   1
#define gskb_uint16_get_packed_size(value)  2
#define gskb_uint32_get_packed_size(value)  4
#define gskb_uint64_get_packed_size(value)  8
#define gskb_bit_get_packed_size(value)     1
G_INLINE_FUNC guint gskb_int_get_packed_size(gint32 value);
G_INLINE_FUNC guint gskb_long_get_packed_size(gint64 value);
G_INLINE_FUNC guint gskb_uint_get_packed_size(guint32 value);
G_INLINE_FUNC guint gskb_ulong_get_packed_size(guint64 value);
G_INLINE_FUNC guint gskb_string_get_packed_size(const char * value);
#define gskb_float32_get_packed_size(value) 4
#define gskb_float64_get_packed_size(value) 8

/* declare pack_slab() */
#define DECLARE_PACK_SLAB(name, maybe_const) \
  G_INLINE_FUNC guint gskb_##name##_pack_slab (maybe_const gskb_##name value, \
                                        guint8     *slab);
GSKB_FOREACH_FUNDAMENTAL_TYPE(DECLARE_PACK_SLAB)
#undef DECLARE_PACK_SLAB

/* declare validate_partial() */
#define DECLARE_VALIDATE_PARTIAL(name, maybe_const) \
  guint gskb_##name##_validate_partial(guint          len, \
                                       const guint8  *data, \
                                       GError       **error);
GSKB_FOREACH_FUNDAMENTAL_TYPE(DECLARE_VALIDATE_PARTIAL)
#undef DECLARE_VALIDATE
G_INLINE_FUNC guint gskb_uint8_validate_unpack  (guint len,
                                                 const guint8 *data,
                                                 guint8 *value_out,
                                                 GError **error);
G_INLINE_FUNC guint gskb_uint16_validate_unpack (guint len,
                                                 const guint8 *data,
                                                 guint16 *value_out,
                                                 GError **error);
G_INLINE_FUNC guint gskb_uint32_validate_unpack (guint len,
                                                 const guint8 *data,
                                                 guint32 *value_out,
                                                 GError **error);
G_INLINE_FUNC guint gskb_uint_validate_unpack   (guint len,
                                                 const guint8 *data,
                                                 guint32 *value_out,
                                                 GError **error);

#define DECLARE_UNPACK(name, maybe_const) \
  G_INLINE_FUNC guint gskb_##name##_unpack (const guint8 *data, gskb_##name *value_out);
GSKB_FOREACH_FUNDAMENTAL_TYPE(DECLARE_UNPACK);
#undef DECLARE_UNPACK

/* common formats */
#define gskb_int_format_generic(int_type) \
             ((GskbFormat *) &(gskb_format_ints_array[int_type].base))
#define gskb_int8_format    gskb_int_format_generic(GSKB_FORMAT_INT_INT8)
#define gskb_int16_format   gskb_int_format_generic(GSKB_FORMAT_INT_INT16)
#define gskb_int32_format   gskb_int_format_generic(GSKB_FORMAT_INT_INT32)
#define gskb_int64_format   gskb_int_format_generic(GSKB_FORMAT_INT_INT64)
#define gskb_uint8_format   gskb_int_format_generic(GSKB_FORMAT_INT_UINT8)
#define gskb_uint16_format  gskb_int_format_generic(GSKB_FORMAT_INT_UINT16)
#define gskb_uint32_format  gskb_int_format_generic(GSKB_FORMAT_INT_UINT32)
#define gskb_uint64_format  gskb_int_format_generic(GSKB_FORMAT_INT_UINT64)
#define gskb_int_format     gskb_int_format_generic(GSKB_FORMAT_INT_INT)
#define gskb_uint_format    gskb_int_format_generic(GSKB_FORMAT_INT_UINT)
#define gskb_long_format    gskb_int_format_generic(GSKB_FORMAT_INT_LONG)
#define gskb_ulong_format   gskb_int_format_generic(GSKB_FORMAT_INT_ULONG)
#define gskb_bit_format     gskb_int_format_generic(GSKB_FORMAT_INT_BIT)

#define gskb_float_format_generic(float_type) \
             ((GskbFormat *) &(gskb_format_floats_array[float_type].base))
extern GskbFormatFloat gskb_format_floats_array[GSKB_N_FORMAT_FLOAT_TYPES];
#define gskb_float32_format gskb_float_format_generic(GSKB_FORMAT_FLOAT_FLOAT32)
#define gskb_float64_format gskb_float_format_generic(GSKB_FORMAT_FLOAT_FLOAT64)

extern GskbFormatString gskb_string_format_instance;
#define gskb_string_format ((GskbFormat *)&gskb_string_format_instance)

/* implementations */
#if defined(G_CAN_INLINE) || defined(GSKB_INTERNAL_IMPLEMENT_INLINES)
#include "../gskerror.h"
/* helper function for fixed length validation */
G_INLINE_FUNC guint
gskb_simple_fixed_length_validate_partial (const char *type_name,
                                           guint length,
                                           guint exp_length,
                                           GError **error)
{
  if (length < exp_length)
    {
      g_set_error (error, GSK_G_ERROR_DOMAIN, GSK_ERROR_TOO_SHORT,
                   "too short validating %s (expected %u bytes, got %u)",
                   type_name, exp_length, length);
      return 0;
    }
  return exp_length;
}
G_INLINE_FUNC void
gskb_int8_pack (gskb_int8 value, GskbAppendFunc func, gpointer data)
{
  guint8 v = value;
  func (1, &v, data);
}
G_INLINE_FUNC guint
gskb_int8_pack_slab (gskb_int8 value, guint8 *out)
{
  *out = (guint8) value;
  return 1;
}
G_INLINE_FUNC guint
gskb_int8_validate_partial (guint length,
                            const guint8 *data,
                            GError **error)
{
  return gskb_simple_fixed_length_validate_partial ("int8", length, 1, error);
}
G_INLINE_FUNC guint
gskb_int8_unpack (const guint8 *data, gskb_int8 *value_out)
{
  *value_out = (gint8) *data;
  return 1;
}
G_INLINE_FUNC void
gskb_int16_pack (gskb_int16 value, GskbAppendFunc func, gpointer data)
{
#if GSKB_OPTIMIZE_LITTLE_ENDIAN
  func (2, (guint8 *) &value, data);
#else
  guint8 slab[2] = { (guint8) value, (guint8) (value>>8) };
  func (2, slab, data);
#endif
}
G_INLINE_FUNC guint
gskb_int16_pack_slab (gskb_int16 value, guint8 *out)
{
#if GSKB_OPTIMIZE_LITTLE_ENDIAN
  memcpy (out, &value, 2);
#else
  out[0] = value;
  out[1] = value >> 8;
#endif
  return 2;
}
G_INLINE_FUNC guint
gskb_int16_validate_partial (guint length,
                            const guint8 *data,
                            GError **error)
{
  return gskb_simple_fixed_length_validate_partial ("int16", length, 2, error);
}
G_INLINE_FUNC guint
gskb_int16_unpack (const guint8 *data, gskb_int16 *value_out)
{
#if GSKB_OPTIMIZE_LITTLE_ENDIAN
  memcpy (value_out, data, 2);
#else
  *value_out = (gint16)data[0]
             | ((gint16)data[1] << 8);
#endif
  return 2;
}
G_INLINE_FUNC void
gskb_int32_pack (gskb_int32 value, GskbAppendFunc func, gpointer data)
{
#if GSKB_OPTIMIZE_LITTLE_ENDIAN
  func (4, (guint8 *) &value, data);
#else
  guint8 slab[4] = { (guint8) value, (guint8) (value>>8),
                     (guint8) (value>>16), (guint8) (value>>24) };
  func (4, slab, data);
#endif
}
G_INLINE_FUNC guint
gskb_int32_pack_slab (gskb_int32 value, guint8 *out)
{
#if GSKB_OPTIMIZE_LITTLE_ENDIAN
  memcpy (out, &value, 4);
#else
  out[0] = value;
  out[1] = value >> 8;
  out[2] = value >> 16;
  out[3] = value >> 24;
#endif
  return 4;
}
G_INLINE_FUNC guint
gskb_int32_validate_partial (guint length,
                            const guint8 *data,
                            GError **error)
{
  return gskb_simple_fixed_length_validate_partial ("int32", length, 4, error);
}
G_INLINE_FUNC guint
gskb_int32_unpack (const guint8 *data, gskb_int32 *value_out)
{
#if GSKB_OPTIMIZE_LITTLE_ENDIAN
  memcpy (value_out, data, 4);
#else
  *value_out = (gint32)data[0]
             | ((gint32)data[1] << 8)
             | ((gint32)data[2] << 16)
             | ((gint32)data[3] << 24);
#endif
  return 4;
}
G_INLINE_FUNC void
gskb_int64_pack (gskb_int64 value, GskbAppendFunc func, gpointer data)
{
#if GSKB_OPTIMIZE_LITTLE_ENDIAN
  func (8, (guint8 *) &value, data);
#else
  guint8 slab[8];
  gskb_int32_pack_slab ((gskb_int32)value, slab);
  gskb_int32_pack_slab ((gskb_int32)(value>>32), slab+4);
  func (8, slab, data);
#endif
}
G_INLINE_FUNC guint
gskb_int64_pack_slab (gskb_int64 value, guint8 *out)
{
#if GSKB_OPTIMIZE_LITTLE_ENDIAN
  memcpy (out, (guint8 *) &value, 8);
#else
  guint32 hi = value >> 32;
  guint32 lo = (guint32) value;
  gskb_int32_pack_slab (lo, out);
  gskb_int32_pack_slab (hi, out+4);
#endif
  return 8;
}
G_INLINE_FUNC guint
gskb_int64_validate_partial (guint length,
                            const guint8 *data,
                            GError **error)
{
  return gskb_simple_fixed_length_validate_partial ("int64", length, 8, error);
}
G_INLINE_FUNC guint
gskb_int64_unpack (const guint8 *data, gskb_int64 *value_out)
{
#if GSKB_OPTIMIZE_LITTLE_ENDIAN
  memcpy (value_out, data, 8);
#else
  gint32 lo, hi;
  gskb_int32_unpack (data, &lo);
  gskb_int32_unpack (data+4, &hi);
  *value_out = (((gint64)hi) << 32) | (gint64) (guint32)lo;
#endif
  return 8;
}
G_INLINE_FUNC void
gskb_uint8_pack (gskb_uint8 value, GskbAppendFunc func, gpointer data)
{
  guint8 v = value;
  func (1, &v, data);
}
G_INLINE_FUNC guint
gskb_uint8_pack_slab (gskb_uint8 value, guint8 *out)
{
  *out = value;
  return 1;
}
G_INLINE_FUNC guint
gskb_uint8_validate_partial (guint length,
                            const guint8 *data,
                            GError **error)
{
  return gskb_simple_fixed_length_validate_partial ("uint8", length, 1, error);
}
G_INLINE_FUNC guint
gskb_uint8_unpack (const guint8 *data, gskb_uint8 *value_out)
{
  *value_out = *data;
  return 1;
}
G_INLINE_FUNC guint
gskb_uint8_validate_unpack  (guint length,
                             const guint8 *data,
                             guint8 *out,
                             GError **error)
{
  if (!gskb_simple_fixed_length_validate_partial ("uint8", length, 1, error))
    return 0;
  *out = data[0];
  return 1;
}
G_INLINE_FUNC void
gskb_uint16_pack (gskb_uint16 value, GskbAppendFunc func, gpointer data)
{
#if GSKB_OPTIMIZE_LITTLE_ENDIAN
  func (2, (const guint8 *) &value, data);
#else
  guint8 slab[2] = { (guint8) value, (guint8) (value>>8) };
  func (2, slab, data);
#endif
}
G_INLINE_FUNC guint
gskb_uint16_pack_slab (gskb_uint16 value, guint8 *out)
{
#if GSKB_OPTIMIZE_LITTLE_ENDIAN
  memcpy (out, &value, 2);
#else
  out[0] = value;
  out[1] = value >> 8;
#endif
  return 2;
}
G_INLINE_FUNC guint
gskb_uint16_validate_partial (guint length,
                            const guint8 *data,
                            GError **error)
{
  return gskb_simple_fixed_length_validate_partial ("uint16", length, 2, error);
}
G_INLINE_FUNC guint
gskb_uint16_unpack (const guint8 *data, gskb_uint16 *value_out)
{
#if GSKB_OPTIMIZE_LITTLE_ENDIAN
  memcpy (value_out, data, 2);
#else
  *value_out = (guint16)data[0]
             | ((guint16)data[1] << 8);
#endif
  return 2;
}
G_INLINE_FUNC guint
gskb_uint16_validate_unpack  (guint length,
                             const guint8 *data,
                             guint16 *out,
                             GError **error)
{
  if (!gskb_simple_fixed_length_validate_partial ("uint16", length, 2, error))
    return 0;
  return gskb_uint16_unpack (data, out);
}
G_INLINE_FUNC void
gskb_uint32_pack (gskb_uint32 value, GskbAppendFunc func, gpointer data)
{
#if GSKB_OPTIMIZE_LITTLE_ENDIAN
  func (4, (guint8 *) &value, data);
#else
  guint8 slab[4] = { (guint8) value, (guint8) (value>>8),
                     (guint8) (value>>16), (guint8) (value>>24) };
  func (4, slab, data);
#endif
}
G_INLINE_FUNC guint
gskb_uint32_pack_slab (gskb_uint32 value, guint8 *out)
{
#if GSKB_OPTIMIZE_LITTLE_ENDIAN
  memcpy (out, &value, 4);
#else
  out[0] = value;
  out[1] = value >> 8;
  out[2] = value >> 16;
  out[3] = value >> 24;
#endif
  return 4;
}
G_INLINE_FUNC guint
gskb_uint32_validate_partial (guint length,
                            const guint8 *data,
                            GError **error)
{
  return gskb_simple_fixed_length_validate_partial ("uint32", length, 4, error);
}
G_INLINE_FUNC guint
gskb_uint32_unpack (const guint8 *data, gskb_uint32 *value_out)
{
#if GSKB_OPTIMIZE_LITTLE_ENDIAN
  memcpy (value_out, data, 4);
#else
  *value_out = (guint32)data[0]
             | ((guint32)data[1] << 8)
             | ((guint32)data[2] << 16)
             | ((guint32)data[3] << 24);
#endif
  return 4;
}
G_INLINE_FUNC guint
gskb_uint32_validate_unpack  (guint length,
                             const guint8 *data,
                             guint32 *out,
                             GError **error)
{
  if (!gskb_simple_fixed_length_validate_partial ("uint32", length, 4, error))
    return 0;
  return gskb_uint32_unpack (data, out);
}
G_INLINE_FUNC void
gskb_uint64_pack (gskb_uint64 value, GskbAppendFunc func, gpointer data)
{
#if GSKB_OPTIMIZE_LITTLE_ENDIAN
  func (8, (guint8 *) &value, data);
#else
  guint8 slab[8];
  gskb_uint32_pack_slab ((gskb_uint32)value, slab);
  gskb_uint32_pack_slab ((gskb_uint32)(value>>32), slab+4);
  func (8, slab, data);
#endif
}
G_INLINE_FUNC guint
gskb_uint64_pack_slab (gskb_uint64 value, guint8 *out)
{
#if GSKB_OPTIMIZE_LITTLE_ENDIAN
  memcpy (out, &value, 8);
#else
  guint32 hi = (guint32) (value >> 32);
  guint32 lo = (guint32) value;
  gskb_uint32_pack_slab (lo, out);
  gskb_uint32_pack_slab (hi, out+4);
#endif
  return 8;
}
G_INLINE_FUNC guint
gskb_uint64_validate_partial (guint length,
                            const guint8 *data,
                            GError **error)
{
  return gskb_simple_fixed_length_validate_partial ("uint64", length, 8, error);
}
G_INLINE_FUNC guint
gskb_uint64_unpack (const guint8 *data, gskb_uint64 *value_out)
{
#if GSKB_OPTIMIZE_LITTLE_ENDIAN
  memcpy (value_out, data, 8);
#else
  guint32 lo, hi;
  gskb_uint32_unpack (data, &lo);
  gskb_uint32_unpack (data+4, &hi);
  *value_out = (((gint64)hi) << 32) | (gint64) (guint32)lo;
#endif
  return 8;
}
G_INLINE_FUNC guint
gskb_int_get_packed_size (gskb_int value)
{
  return (-(1<<6) <= value && value < (1<<6)) ? 1
       : (-(1<<13) <= value && value < (1<<13)) ? 2
       : (-(1<<20) <= value && value < (1<<20)) ? 3
       : (-(1<<27) <= value && value < (1<<27)) ? 4
       : 5;
}
G_INLINE_FUNC guint
gskb_int_pack_slab (gskb_int value, guint8 *out)
{
  if (-(1<<6) <= value && value < (1<<6))
    {
      out[0] = value & 0x7f;
      return 1;
    }
  else if (-(1<<13) <= value && value < (1<<13))
    {
      out[0] = value | 0x80;
      out[1] = (value >> 7) & 0x7f;
      return 2;
    }
  else if (-(1<<20) <= value && value < (1<<20))
    {
      out[0] = value | 0x80;
      out[1] = (value >> 7) | 0x80;
      out[2] = (value >> 14) & 0x7f;
      return 3;
    }
  else if (-(1<<27) <= value && value < (1<<27))
    {
      out[0] = value | 0x80;
      out[1] = (value >> 7) | 0x80;
      out[2] = (value >> 14) | 0x80;
      out[3] = (value >> 21) & 0x7f;
      return 4;
    }
  else
    {
      out[0] = value | 0x80;
      out[1] = (value >> 7) | 0x80;
      out[2] = (value >> 14) | 0x80;
      out[3] = (value >> 21) | 0x80;
      out[4] = (value >> 28) & 0x7f;
      return 5;
    }
}
#define GSKB_INT_MAX_PACKED_SIZE 5
G_INLINE_FUNC void
gskb_int_pack (gskb_int value, GskbAppendFunc func, gpointer data)
{
  guint8 slab[GSKB_INT_MAX_PACKED_SIZE];
  func (gskb_int_pack_slab (value, slab), slab, data);
}
G_INLINE_FUNC guint
gskb_int_validate_partial (guint length,
                           const guint8 *data,
                           GError **error)
{
  /* TODO: verify this returns the shortest encoding possible? */
  if ((data[0] & 0x80) == 0) return 1;
  if ((data[1] & 0x80) == 0) return 2;
  if ((data[2] & 0x80) == 0) return 3;
  if ((data[3] & 0x80) == 0) return 4;
  if ((data[4] & 0x80) == 0) return 5;
  g_set_error (error, GSK_G_ERROR_DOMAIN, GSK_ERROR_BAD_FORMAT,
               "validating data of type 'int': too long");
  return 0;
}
G_INLINE_FUNC guint
gskb_int_unpack (const guint8 *data, gskb_int *value_out)
{
  if ((data[0] & 0x80) == 0)
    {
      gint8 v = data[0] << 1;
      *value_out = v >> 1;
      return 1;
    }
  else if ((data[1] & 0x80) == 0)
    {
      gint16 v = ((data[0]&0x7f) << 2)
               | ((gint16) data[1] << 9);
      *value_out = v >> 2;
      return 2;
    }
  else if ((data[2] & 0x80) == 0)
    {
      gint32 v = ((gint32)(data[0]&0x7f) << 11)
               | ((gint32)(data[1]&0x7f) << 18)
               | ((gint32)(data[2]     ) << 25);
      *value_out = v >> 11;
      return 3;
    }
  else if ((data[3] & 0x80) == 0)
    {
      gint32 v = ((gint32)(data[0]&0x7f) << 4)
               | ((gint32)(data[1]&0x7f) << 11)
               | ((gint32)(data[2]&0x7f) << 18)
               | ((gint32)(data[3]     ) << 25);
      *value_out = v >> 4;
      return 4;
    }
  else
    {
      gint32 v = ((gint32)(data[0]&0x7f))
               | ((gint32)(data[1]&0x7f) << 7)
               | ((gint32)(data[2]&0x7f) << 14)
               | ((gint32)(data[3]&0x7f) << 21)
               | ((gint32)(data[4]     ) << 28);
      *value_out = v;
      return 5;
    }
}
G_INLINE_FUNC guint
gskb_uint_get_packed_size (gskb_uint value)
{
  return (value < (1<<7)) ? 1
       : (value < (1<<14)) ? 2
       : (value < (1<<21)) ? 3
       : (value < (1<<28)) ? 4
       : 5;
}
G_INLINE_FUNC guint
gskb_uint_pack_slab (gskb_uint value, guint8 *out)
{
  if (value < (1<<7))
    {
      out[0] = value;
      return 1;
    }
  else if (value < (1<<14))
    {
      out[0] = value | 0x80;
      out[1] = (value >> 7);
      return 2;
    }
  else if (value < (1<<21))
    {
      out[0] = value | 0x80;
      out[1] = (value >> 7) | 0x80;
      out[2] = (value >> 14);
      return 3;
    }
  else if (value < (1<<28))
    {
      out[0] = value | 0x80;
      out[1] = (value >> 7) | 0x80;
      out[2] = (value >> 14) | 0x80;
      out[3] = (value >> 21);
      return 4;
    }
  else
    {
      out[0] = value | 0x80;
      out[1] = (value >> 7) | 0x80;
      out[2] = (value >> 14) | 0x80;
      out[3] = (value >> 21) | 0x80;
      out[4] = (value >> 28);
      return 5;
    }
}
#define GSKB_UINT_MAX_PACKED_SIZE 5
G_INLINE_FUNC void
gskb_uint_pack (gskb_uint value, GskbAppendFunc func, gpointer data)
{
  guint8 slab[GSKB_UINT_MAX_PACKED_SIZE];
  func (gskb_uint_pack_slab (value, slab), slab, data);
}
G_INLINE_FUNC guint
gskb_uint_validate_partial (guint length,
                           const guint8 *data,
                           GError **error)
{
  /* TODO: verify this returns the shortest encoding possible? */
  if ((data[0] & 0x80) == 0) return 1;
  if ((data[1] & 0x80) == 0) return 2;
  if ((data[2] & 0x80) == 0) return 3;
  if ((data[3] & 0x80) == 0) return 4;
  if ((data[4] & 0x80) == 0) return 5;
  g_set_error (error, GSK_G_ERROR_DOMAIN, GSK_ERROR_BAD_FORMAT,
               "validating data of type 'uint': too long");
  return 0;
}
G_INLINE_FUNC guint
gskb_uint_validate_unpack  (guint length,
                            const guint8 *data,
                            guint32 *out,
                            GError **error)
{
  /* TODO: verify this returns the shortest encoding possible? */
  guint i;
  guint32 o = 0;
  guint shift = 0;
  for (i = 0; i < 5; i++)
    {
      if (i == length)
        {
          g_set_error (error, GSK_G_ERROR_DOMAIN, GSK_ERROR_TOO_SHORT,
                       "too short parsing uint: input length is %u", length);
          return 0;
        }
      if (data[i] & 0x80)
        {
          o |= ((guint32) (data[i] & 0x7f) << shift);
          shift += 7;
        }
      else
        {
          *out = o | ((guint32) (data[i]) << shift);
          return o + 1;
        }
    }
  g_set_error (error, GSK_G_ERROR_DOMAIN, GSK_ERROR_TOO_SHORT,
               "invalid uint encoding (too many bytes)");
  return 0;
}

G_INLINE_FUNC guint
gskb_uint_unpack (const guint8 *data, gskb_uint *value_out)
{
  if ((data[0] & 0x80) == 0)
    {
      *value_out = data[0];
      return 1;
    }
  else if ((data[1] & 0x80) == 0)
    {
      *value_out = (guint32) (data[0] & 0x7f)
                 | (guint32) (data[1] << 7);
      return 2;
    }
  else if ((data[2] & 0x80) == 0)
    {
      *value_out = (guint32) (data[0] & 0x7f)
                 | ((guint32) (data[1] & 0x7f) << 7)
                 | (guint32) (data[2] << 14);
      return 3;
    }
  else if ((data[3] & 0x80) == 0)
    {
      *value_out = (guint32) (data[0] & 0x7f)
                 | ((guint32) (data[1] & 0x7f) << 7)
                 | ((guint32) (data[2] & 0x7f) << 14)
                 | (guint32) (data[3] << 21);
      return 4;
    }
  else
    {
      *value_out = (guint32) (data[0] & 0x7f)
                 | ((guint32) (data[1] & 0x7f) << 7)
                 | ((guint32) (data[2] & 0x7f) << 14)
                 | ((guint32) (data[3] & 0x7f) << 21)
                 | (guint32) (data[4] << 28);
      return 5;
    }
}
G_INLINE_FUNC guint
gskb_long_get_packed_size (gskb_long value)
{
  if ((1LL<<31) <= value && value <= ((1LL<<31)-1))
    return gskb_int_get_packed_size (value);
  if (-(1LL<<34) <= value && value < (1LL<<34))
    return 5;
  return gskb_int_get_packed_size (value>>35) + 5;
}
G_INLINE_FUNC guint
gskb_long_pack_slab (gskb_long value, guint8 *out)
{
  if ((1LL<<31) <= value && value <= ((1LL<<31)-1))
    return gskb_int_pack_slab (value, out);
  if (-(1LL<<34) <= value && value < (1LL<<34))
    {
      out[0] = value | 0x80;
      out[1] = (value >> 7) | 0x80;
      out[2] = (value >> 14) | 0x80;
      out[3] = (value >> 21) | 0x80;
      out[4] = (value >> 28) & 0x7f;
      return 5;
    }
  else if (-(1LL<<41) <= value && value < (1LL<<41))
    {
      out[0] = value | 0x80;
      out[1] = (value >> 7) | 0x80;
      out[2] = (value >> 14) | 0x80;
      out[3] = (value >> 21) | 0x80;
      out[4] = (value >> 28) | 0x80;
      out[5] = (value >> 35) & 0x7f;
      return 6;
    }
  else if (-(1LL<<48) <= value && value < (1LL<<48))
    {
      out[0] = value | 0x80;
      out[1] = (value >> 7) | 0x80;
      out[2] = (value >> 14) | 0x80;
      out[3] = (value >> 21) | 0x80;
      out[4] = (value >> 28) | 0x80;
      out[5] = (value >> 35) | 0x80;
      out[6] = (value >> 42) & 0x7f;
      return 7;
    }
  else if (-(1LL<<55) <= value && value < (1LL<<55))
    {
      out[0] = value | 0x80;
      out[1] = (value >> 7) | 0x80;
      out[2] = (value >> 14) | 0x80;
      out[3] = (value >> 21) | 0x80;
      out[4] = (value >> 28) | 0x80;
      out[5] = (value >> 35) | 0x80;
      out[6] = (value >> 42) | 0x80;
      out[7] = (value >> 49) & 0x7f;
      return 8;
    }
  else if (-(1LL<<62) <= value && value < (1LL<<62))
    {
      out[0] = value | 0x80;
      out[1] = (value >> 7) | 0x80;
      out[2] = (value >> 14) | 0x80;
      out[3] = (value >> 21) | 0x80;
      out[4] = (value >> 28) | 0x80;
      out[5] = (value >> 35) | 0x80;
      out[6] = (value >> 42) | 0x80;
      out[7] = (value >> 49) | 0x80;
      out[8] = (value >> 56) & 0x7f;
      return 9;
    }
  else
    {
      out[0] = value | 0x80;
      out[1] = (value >> 7) | 0x80;
      out[2] = (value >> 14) | 0x80;
      out[3] = (value >> 21) | 0x80;
      out[4] = (value >> 28) | 0x80;
      out[5] = (value >> 35) | 0x80;
      out[6] = (value >> 42) | 0x80;
      out[7] = (value >> 49) | 0x80;
      out[8] = (value >> 56) | 0x80;
      out[9] = (value >> 63) & 0x7f;
      return 10;
    }
}
#define GSKB_LONG_MAX_PACKED_SIZE 10
G_INLINE_FUNC void
gskb_long_pack (gskb_long value, GskbAppendFunc func, gpointer data)
{
  guint8 slab[GSKB_LONG_MAX_PACKED_SIZE];
  func (gskb_long_pack_slab (value, slab), slab, data);
}
G_INLINE_FUNC guint
gskb_long_unpack (const guint8 *data, gskb_long *value_out)
{
  gint32 ff;
  if ((data[0] & 0x80) == 0
   || (data[1] & 0x80) == 0
   || (data[2] & 0x80) == 0
   || (data[3] & 0x80) == 0)
    {
      gskb_int v;
      guint rv = gskb_int_unpack (data, &v);
      *value_out = v;
      return rv;
    }
  ff = ((gint32)(data[0]&0x7f))
     | ((gint32)(data[1]&0x7f) << 7)
     | ((gint32)(data[2]&0x7f) << 14)
     | ((gint32)(data[3]&0x7f) << 21);
  if ((data[4] & 0x80) == 0)
    {
      *value_out = (gint64) ff 
                 | ((gint64)data[4] << 28);
      return 5;
    }
  else if ((data[5] & 0x80) == 0)
    {
      *value_out = (gint64)ff
                 | ((gint64)(data[4]&0x7f) << 28)
                 | ((gint64)(data[5]     ) << 35);
      return 6;
    }
  else if ((data[6] & 0x80) == 0)
    {
      *value_out = (gint64)ff
                 | ((gint64)(data[4]&0x7f) << 28)
                 | ((gint64)(data[5]&0x7f) << 35)
                 | ((gint64)(data[6]     ) << 42);

      return 7;
    }
  else if ((data[7] & 0x80) == 0)
    {
      *value_out = (gint64)ff
                 | ((gint64)(data[4]&0x7f) << 28)
                 | ((gint64)(data[5]&0x7f) << 35)
                 | ((gint64)(data[6]&0x7f) << 42)
                 | ((gint64)(data[7]     ) << 49);
      return 8;
    }
  else if ((data[7] & 0x80) == 0)
    {
      *value_out = (gint64)ff
                 | ((gint64)(data[4]&0x7f) << 28)
                 | ((gint64)(data[5]&0x7f) << 35)
                 | ((gint64)(data[6]&0x7f) << 42)
                 | ((gint64)(data[7]     ) << 49);
      return 8;
    }
  else if ((data[8] & 0x80) == 0)
    {
      *value_out = (gint64)ff
                 | ((gint64)(data[4]&0x7f) << 28)
                 | ((gint64)(data[5]&0x7f) << 35)
                 | ((gint64)(data[6]&0x7f) << 42)
                 | ((gint64)(data[7]&0x7f) << 49)
                 | ((gint64)(data[8]     ) << 56);
      return 9;
    }
  else
    {
      *value_out = (gint64)ff
                 | ((gint64)(data[4]&0x7f) << 28)
                 | ((gint64)(data[5]&0x7f) << 35)
                 | ((gint64)(data[6]&0x7f) << 42)
                 | ((gint64)(data[7]&0x7f) << 49)
                 | ((gint64)(data[8]&0x7f) << 56)
                 | ((gint64)(data[9]     ) << 63);
      return 10;
    }
}
G_INLINE_FUNC guint
gskb_long_validate_partial (guint length,
                           const guint8 *data,
                           GError **error)
{
  /* TODO: verify this returns the shortest encoding possible? */
  guint i;
  for (i = 0; i < 10; i++)
    if ((data[0] & 0x80) == 0)
      return i + 1;
  g_set_error (error, GSK_G_ERROR_DOMAIN, GSK_ERROR_BAD_FORMAT,
               "validating data of type 'long': too long");
  return 0;
}
G_INLINE_FUNC guint
gskb_ulong_get_packed_size (gskb_ulong value)
{
  if (value <= G_MAXUINT32)
    return gskb_uint_get_packed_size (value);
  if (value < (1ULL<<35))
    return 5;
  return gskb_uint_get_packed_size (value>>35) + 5;
}
G_INLINE_FUNC guint
gskb_ulong_pack_slab (gskb_ulong value, guint8 *out)
{
  if (value <= (guint64) G_MAXUINT)
    return gskb_uint_pack_slab (value, out);
  out[0] = value | 0x80;
  out[1] = (value >> 7) | 0x80;
  out[2] = (value >> 14) | 0x80;
  out[3] = (value >> 21) | 0x80;
  if (value < (1ULL<<35))
    {
      out[4] = (value >> 28);
      return 5;
    }
  else if (value < (1ULL<<42))
    {
      out[4] = (value >> 28) | 0x80;
      out[5] = (value >> 35) & 0x7f;
      return 6;
    }
  else if (value < (1ULL<<49))
    {
      out[4] = (value >> 28) | 0x80;
      out[5] = (value >> 35) | 0x80;
      out[6] = (value >> 42) & 0x7f;
      return 7;
    }
  else if (value < (1ULL<<56))
    {
      out[4] = (value >> 28) | 0x80;
      out[5] = (value >> 35) | 0x80;
      out[6] = (value >> 42) | 0x80;
      out[7] = (value >> 49) & 0x7f;
      return 8;
    }
  else if (value < (1ULL<<63))
    {
      out[4] = (value >> 28) | 0x80;
      out[5] = (value >> 35) | 0x80;
      out[6] = (value >> 42) | 0x80;
      out[7] = (value >> 49) | 0x80;
      out[8] = (value >> 56) & 0x7f;
      return 9;
    }
  else
    {
      out[4] = (value >> 28) | 0x80;
      out[5] = (value >> 35) | 0x80;
      out[6] = (value >> 42) | 0x80;
      out[7] = (value >> 49) | 0x80;
      out[8] = (value >> 56) | 0x80;
      out[9] = (value >> 63) & 0x7f;
      return 10;
    }
}
G_INLINE_FUNC void
gskb_ulong_pack (gskb_ulong value, GskbAppendFunc func, gpointer data)
{
  guint8 slab[5];
  func (gskb_ulong_pack_slab (value, slab), slab, data);
}
G_INLINE_FUNC guint
gskb_ulong_unpack (const guint8 *data, gskb_ulong *value_out)
{
  if ((data[0] & 0x80) == 0)
    {
      *value_out = data[0];
      return 1;
    }
  else if ((data[1] & 0x80) == 0)
    {
      *value_out = (guint32) (data[0] & 0x7f)
                 | (guint32) (data[1] << 7);
      return 2;
    }
  else if ((data[2] & 0x80) == 0)
    {
      *value_out = (guint32) (data[0] & 0x7f)
                 | ((guint32) (data[1] & 0x7f) << 7)
                 | (guint32) (data[2] << 14);
      return 3;
    }
  else if ((data[3] & 0x80) == 0)
    {
      *value_out = (guint32) (data[0] & 0x7f)
                 | ((guint32) (data[1] & 0x7f) << 7)
                 | ((guint32) (data[2] & 0x7f) << 14)
                 | (guint32) (data[3] << 21);
      return 4;
    }
  else
    {
      *value_out = (guint32) (data[0] & 0x7f)
                 | ((guint32) (data[1] & 0x7f) << 7)
                 | ((guint32) (data[2] & 0x7f) << 14)
                 | ((guint32) (data[3] & 0x7f) << 21)
                 | (guint32) (data[4] << 28);
      return 5;
    }
}
G_INLINE_FUNC guint
gskb_ulong_validate_partial (guint length,
                             const guint8 *data,
                             GError **error)
{
  /* TODO: verify this returns the shortest encoding possible? */
  guint i;
  for (i = 0; i < 10; i++)
    if ((data[0] & 0x80) == 0)
      return i + 1;
  g_set_error (error, GSK_G_ERROR_DOMAIN, GSK_ERROR_BAD_FORMAT,
               "validating data of type 'ulong': too long");
  return 0;
}
G_INLINE_FUNC void
gskb_bit_pack (gskb_bit value, GskbAppendFunc func, gpointer data)
{
  guint8 v = value;
  func (1, &v, data);
}
G_INLINE_FUNC guint
gskb_bit_pack_slab (gskb_bit value, guint8 *out)
{
  *out = value;
  return 1;
}
G_INLINE_FUNC guint
gskb_bit_unpack (const guint8 *data, gskb_bit *value_out)
{
  *value_out = *data;
  return 1;
}

/* float */
G_INLINE_FUNC void
gskb_float32_pack (gskb_float32 value, GskbAppendFunc func, gpointer data)
{
#if GSKB_OPTIMIZE_LITTLE_ENDIAN
  func (4, (const guint8 *) &value, data);
#else
  union {
    gskb_float32 f;
    guint32 i;
    guint8 bytes[4];
  } u = { value };
  u.i = GUINT32_TO_LE (u.i);
  func (4, u.bytes, data);
#endif
}
G_INLINE_FUNC guint
gskb_float32_pack_slab (gskb_float32 value, guint8 *out)
{
#if GSKB_OPTIMIZE_LITTLE_ENDIAN
  memcpy (out, &value, 4);
#else
  union {
    gskb_float32 f;
    guint32 i;
    guint8 bytes[4];
  } u = { value };
  u.i = GUINT32_TO_LE (u.i);
  memcpy (out, u.bytes, 4);
#endif
  return 4;
}
G_INLINE_FUNC guint
gskb_float32_unpack (const guint8 *data, gskb_float32 *value_out)
{
#if GSKB_OPTIMIZE_LITTLE_ENDIAN
  memcpy (value_out, data, 4);
#else
  union {
    gskb_float32 f;
    guint32 i;
    guint8 bytes[4];
  } u;
  memcpy (u.bytes, data, 4);
  u.i = GUINT32_FROM_LE (u.i);
  *value_out = u.f;
#endif
  return 4;
}
G_INLINE_FUNC guint
gskb_float32_validate_partial (guint length,
                               const guint8 *data,
                               GError **error)
{
  return gskb_simple_fixed_length_validate_partial ("float32", length, 4, error);
}

G_INLINE_FUNC void
gskb_float64_pack (gskb_float64 value, GskbAppendFunc func, gpointer data)
{
#if GSKB_OPTIMIZE_LITTLE_ENDIAN
  func (8, (const guint8 *) &value, data);
#else
  union {
    gskb_float64 f;
    guint64 i;
    guint8 bytes[8];
  } u = { value };
  u.i = GUINT64_TO_LE (u.i);
  func (8, u.bytes, data);
#endif
}
G_INLINE_FUNC guint
gskb_float64_pack_slab (gskb_float64 value, guint8 *out)
{
#if GSKB_OPTIMIZE_LITTLE_ENDIAN
  memcpy (out, &value, 8);
#else
  union {
    gskb_float64 f;
    guint64 i;
    guint8 bytes[8];
  } u = { value };
  u.i = GUINT64_TO_LE (u.i);
  memcpy (out, u.bytes, 8);
#endif
  return 8;
}
G_INLINE_FUNC guint
gskb_float64_unpack (const guint8 *data, gskb_float64 *value_out)
{
#if GSKB_OPTIMIZE_LITTLE_ENDIAN
  memcpy (value_out, data, 8);
#else
  union {
    gskb_float64 f;
    guint64 i;
    guint8 bytes[8];
  } u;
  memcpy (u.bytes, data, 8);
  u.i = GUINT64_FROM_LE (u.i);
  *value_out = u.f;
#endif
  return 8;
}
G_INLINE_FUNC guint
gskb_float64_validate_partial (guint length,
                               const guint8 *data,
                               GError **error)
{
  return gskb_simple_fixed_length_validate_partial ("float64", length, 8, error);
}

G_INLINE_FUNC guint
gskb_string_get_packed_size (const char *value)
{
  return strlen (value) + 1;
}
G_INLINE_FUNC void
gskb_string_pack (gskb_string value, GskbAppendFunc func, gpointer data)
{
  func (strlen (value) + 1, (guint8 *) value, data);
}
G_INLINE_FUNC guint
gskb_string_pack_slab (gskb_string value, guint8 *out)
{
  return g_stpcpy ((char*) out, value) + 1 - (char *) out;
}
G_INLINE_FUNC guint
gskb_string_unpack (const guint8 *data, gskb_string *value_out)
{
  guint len;
  for (len = 0; data[len] != 0; len++)
    ;
  *value_out = g_malloc (len + 1);
  memcpy (*value_out, data, len + 1);
  return len + 1;
}
G_INLINE_FUNC guint
gskb_string_validate_partial (guint length,
                              const guint8 *data,
                              GError **error)
{
  const guint8 *nul = memchr (data, 0, length);
  if (nul == NULL)
    {
      g_set_error (error, GSK_G_ERROR_DOMAIN, GSK_ERROR_TOO_SHORT,
                   "no NUL to terminate string");
      return 0;
    }
  return (nul - data) + 1;
}

#endif          /* G_CAN_INLINE */

#undef GSKB_FOREACH_FUNDAMENTAL_TYPE
