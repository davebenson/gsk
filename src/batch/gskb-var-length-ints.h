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


gboolean gskb_parse_int32_var_length    (guint8 *data,
                                         gint32 *out,
                                         guint  *n_used_out);
gboolean gskb_validate_int32_var_length (guint max_len,
                                         guint8 *data,
                                         gint32 *out,
                                         guint  *n_used_out);
guint    gskb_get_packed_size_int32_var_length (gint32  in);
guint    gskb_pack_int32_var_length     (gint32  in,
                                         guint8 *out);
gboolean gskb_parse_uint32_var_length   (guint8 *data,
                                         guint32 *out,
                                         guint  *n_used_out);
gboolean gskb_validate_uint32_var_length(guint max_len,
                                         guint8 *data,
                                         guint32 *out,
                                         guint  *n_used_out);
guint    gskb_get_packed_size_uint32_var_length (guint32  in);
guint    gskb_pack_uint32_var_length    (guint32  in,
                                         guint8 *out);
gboolean gskb_parse_int64_var_length    (guint8 *data,
                                         gint64 *out,
                                         guint  *n_used_out);
gboolean gskb_validate_int64_var_length (guint max_len,
                                         guint8 *data,
                                         gint64 *out,
                                         guint  *n_used_out);
guint    gskb_get_packed_size_int64_var_length (gint64  in);
guint    gskb_pack_int64_var_length     (gint64  in,
                                         guint8 *out);
gboolean gskb_parse_uint64_var_length   (guint8 *data,
                                         guint64 *out,
                                         guint  *n_used_out);
gboolean gskb_validate_uint64_var_length(guint max_len,
                                         guint8 *data,
                                         guint64 *out,
                                         guint  *n_used_out);
guint    gskb_get_packed_size_uint64_var_length (guint64  in);
guint    gskb_pack_uint64_var_length    (guint64  in,
                                         guint8 *out);
