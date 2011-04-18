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
 */

#include "config.h"
#include <glib/gi18n-lib.h>


#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_file_path (path, _("File"), "", _("Path of file to load."))

#else

#define GEGL_CHANT_TYPE_SOURCE
#define GEGL_CHANT_C_FILE       "load.c"

#include "gegl-chant.h"
#include "graph/gegl-node.h"

#include <math.h>

static GeglRectangle
gegl_load_get_bounding_box (GeglOperation * operation)
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
gegl_load_prepare (GeglOperation * operation)
{
  GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);
  GeglNode *node = operation->node;

  guint64 hash;

  printf ("gegl_load_prepare: 1\n");

  hash = 0;
  hash = GEGL_VIPS_HASH_STRING( hash, o->path );

  if (!node->vips_image || node->vips_hash != hash)
    {
      VipsImage *image;
      VipsImage *t[10];

      image = vips_image_new ("p");

      /*
      // load as linear float with a LUT
      if (vips_image_new_array (VIPS_OBJECT(image), t, 4) ||
        !(t[3] = vips_image_new_from_file (o->path, "r")) ||
	 im_identity (t[0], 1) ||
	 im_powtra (t[0], t[1], 1.0 / 2.4) ||
	 im_lintra (255.0 / pow (255.0, 1.0 / 2.4), t[1], 0.0, t[2]) ||
	 im_maplut (t[3], image, t[2]))
	{
	  gegl_vips_error ("load");
	  g_object_unref (image);
	  return;
	}
		 */

      /*
      // load as linear float with a LUT, add an alpha channel
		 */
      if (vips_image_new_array (VIPS_OBJECT(image), t, 10) ||
        !(t[0] = vips_image_new_from_file (o->path, "r")) ||
	 im_black (t[1], 1, 1, 1) ||
	 im_lintra (1, t[1], 255.0, t[2]) ||
	 im_clip2fmt (t[2], t[3], VIPS_FORMAT_UCHAR) ||
	 im_embed (t[3], t[4], 1, 0, 0, t[0]->Xsize, t[0]->Ysize) ||
	 im_bandjoin (t[0], t[4], t[5]) ||
	 im_identity (t[6], 1) ||
	 im_powtra (t[6], t[7], 1.0 / 2.4) ||
	 im_lintra (255.0 / pow (255.0, 1.0 / 2.4), t[7], 0.0, t[8]) ||
	 im_maplut (t[5], image, t[8]))
	{
	  gegl_vips_error ("load");
	  g_object_unref (image);
	  return;
	}

      /*
      // load as linear float, no LUT
      if (vips_image_new_array (VIPS_OBJECT(image), t, 4) ||
        !(t[0] = vips_image_new_from_file (o->path, "r")) ||
	 im_lintra (1.0 / 255.0, t[0], 0.0, t[1]) ||
	 im_powtra (t[1], image, 1.0 / 2.4))
	{
	  gegl_vips_error ("load");
	  g_object_unref (image);
	  return;
	}
	 */

      /*
      // load as linear float, no LUT, add an alpha channel
      if (vips_image_new_array (VIPS_OBJECT(image), t, 8) ||
        !(t[0] = vips_image_new_from_file (o->path, "r")) ||
	 im_black (t[2], 1, 1, 1) ||
	 im_lintra (1, t[2], 255.0, t[3]) ||
	 im_clip2fmt (t[3], t[4], VIPS_FORMAT_UCHAR) ||
	 im_embed (t[4], t[5], 1, 0, 0, t[0]->Xsize, t[0]->Ysize) ||
	 im_bandjoin (t[0], t[5], t[6]) ||
	 im_lintra (1.0 / 255.0, t[6], 0.0, t[7]) ||
	 im_powtra (t[7], image, 1.0 / 2.4))
	{
	  gegl_vips_error ("load");
	  g_object_unref (image);
	  return;
	}
	*/

      /*

	 int version

	if (!(image = vips_image_new_from_file (o->path, "r")))
	  {
	    gegl_vips_error ("load");
	    return;
	  }
       */


      node->vips_image = image;
      node->vips_hash = hash;

      printf ("gegl_load_prepare:\n");
      printf ("\tpath = %s\n", o->path);
      printf ("\tattaching to node = %p\n", node);
    }

  return;
}

static GeglRectangle
gegl_load_get_cached_region (GeglOperation * operation, const GeglRectangle * roi)
{
  return gegl_load_get_bounding_box (operation);
}


static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationClass *operation_class;
  GeglOperationSourceClass *source_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  source_class = GEGL_OPERATION_SOURCE_CLASS (klass);

  operation_class->prepare = gegl_load_prepare;
  operation_class->get_bounding_box = gegl_load_get_bounding_box;
  operation_class->get_cached_region = gegl_load_get_cached_region;

  operation_class->name = "gegl:load";
  operation_class->categories = "input";
  operation_class->description = _("image loader.");

/*  static gboolean done=FALSE;
    if (done)
      return; */
/*  done = TRUE; */
}

#endif

