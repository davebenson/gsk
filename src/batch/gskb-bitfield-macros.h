

/* declare bitfields so they are little endian,
   regardless of the local endianness.
   You must use the right version of this.
   Note that any padding must be given explicitly.
   Zero-length padding must not be given. */
#if GSKB_BITFIELD_ENDIANNESS == G_LITTLE_ENDIAN
#define GSKB_LE_BITFIELDS_DECLARE1(f1) guint8 f1;
#define GSKB_LE_BITFIELDS_DECLARE2(f1,f2) guint8 f1; guint8 f2;
#define GSKB_LE_BITFIELDS_DECLARE3(f1,f2,f3) guint8 f1; guint8 f2; guint8 f3;
#define GSKB_LE_BITFIELDS_DECLARE4(f1,f2,f3,f4) guint8 f1; guint8 f2; guint8 f3; guint8 f4;
#define GSKB_LE_BITFIELDS_DECLARE5(f1,f2,f3,f4,f5) guint8 f1; guint8 f2; guint8 f3; guint8 f4; guint8 f5;
#define GSKB_LE_BITFIELDS_DECLARE6(f1,f2,f3,f4,f5,f6) guint8 f1; guint8 f2; guint8 f3; guint8 f4; guint8 f5; guint8 f6;
#define GSKB_LE_BITFIELDS_DECLARE7(f1,f2,f3,f4,f5,f6,f7) guint8 f1; guint8 f2; guint8 f3; guint8 f4; guint8 f5; guint8 f6; guint8 f7;
#define GSKB_LE_BITFIELDS_DECLARE8(f1,f2,f3,f4,f5,f6,f7,f8) guint8 f1; guint8 f2; guint8 f3; guint8 f4; guint8 f5; guint8 f6; guint8 f7; guint8 f8;
#elif GSKB_BITFIELD_ENDIANNESS == G_BIG_ENDIAN
#define GSKB_LE_BITFIELDS_DECLARE1(f1) guint8 f1;
#define GSKB_LE_BITFIELDS_DECLARE2(f1,f2) guint8 f2; guint8 f1;
#define GSKB_LE_BITFIELDS_DECLARE3(f1,f2,f3) guint8 f3; guint8 f2; guint8 f1;
#define GSKB_LE_BITFIELDS_DECLARE4(f1,f2,f3,f4) guint8 f4; guint8 f3; guint8 f2; guint8 f1;
#define GSKB_LE_BITFIELDS_DECLARE5(f1,f2,f3,f4,f5) guint8 f5; guint8 f4; guint8 f3; guint8 f2; guint8 f1;
#define GSKB_LE_BITFIELDS_DECLARE6(f1,f2,f3,f4,f5,f6) guint8 f6; guint8 f5; guint8 f4; guint8 f3; guint8 f2; guint8 f1;
#define GSKB_LE_BITFIELDS_DECLARE7(f1,f2,f3,f4,f5,f6,f7) guint8 f7; guint8 f6; guint8 f5; guint8 f4; guint8 f3; guint8 f2; guint8 f1;
#define GSKB_LE_BITFIELDS_DECLARE8(f1,f2,f3,f4,f5,f6,f7,f8) guint8 f8; guint8 f7; guint8 f6; guint8 f5; guint8 f4; guint8 f3; guint8 f2; guint8 f1;
#else
#error "only big and little endian bit-field layouts are supported"
/* for ($i = 2; $i <= 8; $i++) {
    print "#define GSKB_BITFIELDS_DECLARE$i(";
    for ($j = 1; $j <= $i; $j++) {print "f$j";if ($j < $i) {print ","}}
    print ") ";
    for ($j = $i; $j >= $1; $j--) {print "guint8 f$j;";if ($j < $i) {print " "}}
    print "\n";
} */
#endif

