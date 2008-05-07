
/* description of data format */

struct _GskbFormatAny
{
  GskbFormatType type;		/* must be first */
  guint ref_count;
};

typedef union _GskbFormat GskbFormat;
union _GskbFormat
{
  GskbFormatType   type;
  GskbFormatAny    any;

  GskbFormatInt    v_int;
  GskbFormatFloat  v_float;
  GskbFormatString v_string;
  GskbFormatArray  v_array;
  GskbFormatStruct v_struct;
  GskbFormatUnion  v_union;
  GskbFormatEnum   v_enum;
};
