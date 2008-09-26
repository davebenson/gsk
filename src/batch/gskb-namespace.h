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


struct _GskbNamespace
{
  guint ref_count;
  GHashTable *name_to_format;

  guint n_formats;
  GskbFormat **formats;
  guint formats_alloced;
};

GskbNamespace *gskb_namespace_new          (void);
gboolean       gskb_namespace_parse_string (GskbNamespace *ns,
                                            const char    *str,
                                            GError       **error);
gboolean       gskb_namespace_parse_file   (GskbNamespace *ns,
                                            const char    *filename,
                                            GError       **error);
GskbFormat    *gskb_namespace_parse_format (GskbNamespace *ns,
                                            const char    *str,
                                            GError       **error);
