
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
typedef gchararray gskb_string;

#define GSKB_FOREACH_FUNDAMENTAL_TYPE(macro) \
 macro(int8) \
 macro(int16) \
 macro(int32) \
 macro(int64) \
 macro(uint8) \
 macro(uint16) \
 macro(uint32) \
 macro(uint64) \
 macro(int) \
 macro(uint) \
 macro(long) \
 macro(ulong) \
 macro(bit) \
 macro(float32) \
 macro(float64) \
 macro(string)


/* declare pack() */
#define GSKB_DECLARE_PACK(lctypename) \
GSKB_FUNDAMENTAL_INLINE void  gskb_##lctypename##_pack (ctype value, \
                                                        GskbAppendFunc append, \
                                                        gpointer append_data);

GSKB_FOREACH_FUNDAMENTAL_TYPE(GSKB_DECLARE_PACK)
#undef GSKB_DECLARE_PACK

/* declare get_packed_size() */
#define gskb_int8_get_packed_size(value)    1
#define gskb_int16_get_packed_size(value)   2
#define gskb_int32_get_packed_size(value)   4
#define gskb_int64_get_packed_size(value)   8
#define gskb_uint8_get_packed_size(value)   1
#define gskb_uint16_get_packed_size(value)  2
#define gskb_uint32_get_packed_size(value)  4
#define gskb_uint64_get_packed_size(value)  8
GSKB_FUNDAMENTAL_INLINE guint gskb_int_get_packed_size(gint32 value);
GSKB_FUNDAMENTAL_INLINE guint gskb_long_get_packed_size(gint64 value);
GSKB_FUNDAMENTAL_INLINE guint gskb_uint_get_packed_size(guint32 value);
GSKB_FUNDAMENTAL_INLINE guint gskb_ulong_get_packed_size(guint64 value);
GSKB_FUNDAMENTAL_INLINE guint gskb_string_get_packed_size(gchararray value);
#define gskb_float32_get_packed_size(value) 4
#define gskb_float64_get_packed_size(value) 8

/* declare pack_slab() */
#define DECLARE_PACK_SLAB(name) \
  GSKB_FUNDAMENTAL_INLINE guint name##_pack_slab (gskb_##name value, \
                          guint8     *slab);
FOREACH_FUNDAMENTAL_TYPE(DECLARE_PACK_SLAB)
#undef DECLARE_PACK_SLAB

/* declare validate() */
#define DECLARE_VALIDATE(name) \
  guint name##_validate  (guint          len, \
                          const guint8  *data, \
                          guint         *n_used_out, \
                          GError       **error);
FOREACH_FUNDAMENTAL_TYPE(DECLARE_VALIDATE)
#undef DECLARE_VALIDATE

