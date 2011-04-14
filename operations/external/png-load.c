/* This file is an image processing operation for GEGL
 *
 * GEGL is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * GEGL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GEGL; if not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright 2006 Øyvind Kolås <pippin@gimp.org>
 *           2006 Dominik Ernst <dernst@gmx.de>
 *           2006 Kevin Cozens <kcozens@cvs.gnome.org>
 */

#include "config.h"
#include <glib/gi18n-lib.h>


#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_file_path (path, _("File"), "", _("Path of file to load."))
#else

#define GEGL_CHANT_TYPE_SOURCE
#define GEGL_CHANT_C_FILE       "png-load.c"

#include "gegl-chant.h"
#include "graph/gegl-node.h"

static GeglRectangle
get_bounding_box (GeglOperation * operation)
{
  GeglRectangle result = { 0, 0, 0, 0 };
  gint width, height;

  printf( "gegl_png_load_get_bounding_box\n" );

  if (!operation->node->vips_image)
    {
      width = 10;
      height = 10;
    }
  else
    {
      width = operation->node->vips_image->Xsize;
      height = operation->node->vips_image->Ysize;
    }

  // do we need this?
  gegl_operation_set_format (operation, "output", babl_format ("R'G'B' u8"));

  result.width = width;
  result.height = height;

  return result;
}

static void
gegl_png_load_prepare (GeglOperation * operation)
{
  GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);

  VipsImage *vips_image;

  printf ("gegl_png_load_prepare: 1\n");

  if (!operation->node->vips_image)
    {
      if (!(vips_image = vips_image_new_from_file (o->path, "r")))
	{
	  g_warning ("%s failed to open file %s for reading.",
		     G_OBJECT_TYPE_NAME (operation), o->path);
	  return;
	}
      operation->node->vips_image = vips_image;

      printf ("gegl_png_load_prepare:\n");
      printf ("\tpath = %s\n", o->path);
      printf ("\tattaching to node = %p\n", operation->node);
    }

  return;
}

static GeglRectangle
get_cached_region (GeglOperation * operation, const GeglRectangle * roi)
{
  return get_bounding_box (operation);
}

static void
gegl_chant_class_init (GeglChantClass * klass)
{
  GeglOperationClass *operation_class;
  GeglOperationSourceClass *source_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  source_class = GEGL_OPERATION_SOURCE_CLASS (klass);

  operation_class->prepare = gegl_png_load_prepare;
  operation_class->get_bounding_box = get_bounding_box;
  operation_class->get_cached_region = get_cached_region;

  operation_class->name = "gegl:png-load";
  operation_class->categories = "hidden";
  operation_class->description = _("PNG image loader.");

/*  static gboolean done=FALSE;
    if (done)
      return; */
  gegl_extension_handler_register (".png", "gegl:png-load");
/*  done = TRUE; */
}

#endif
