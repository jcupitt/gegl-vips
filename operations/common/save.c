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
 * Copyright 2010 Danny Robson <danny@blubinc.net>
 */

#include "config.h"
#include <glib/gi18n-lib.h>


#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_file_path (path, _("File"), "", _("Path of file to save."))

#else

#define GEGL_CHANT_TYPE_SINK
#define GEGL_CHANT_C_FILE       "save.c"

#include "gegl-chant.h"
#include <graph/gegl-node.h>

#include <stdio.h>
#include <math.h>

static void
gegl_save_prepare (GeglOperation * operation)
{
  GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);
  GeglNode *node = operation->node;

  GeglNode *input;
  guint64 hash;

  printf ("gegl_save_prepare: 1\n");

  input = gegl_operation_get_source_node (operation, "input");
  if (!input || !input->vips_image)
    return;

  hash = 0;
  hash = GEGL_VIPS_HASH_POINTER (hash, input->vips_image);
  hash = GEGL_VIPS_HASH_STRING (hash, o->path);

  if (node->vips_hash != hash)
    {
	    /*

	       just-save version

      VipsImage *image;

      if (!(image = vips_image_new_from_file (o->path, "w")))
	{
	  gegl_vips_error ("save");
	  return;
	}
      if (im_copy (input->vips_image, image))
	{
	  gegl_vips_error ("save");
	  g_object_unref (image);
	  return;
	}
      g_object_unref (image);
      	*/

	    /*

	       float to 8-bit int version

      VipsImage *image;
      VipsImage *t[5];

      if (!(image = vips_image_new_from_file (o->path, "w")))
	{
	  gegl_vips_error ("save");
	  return;
	}

      // convert from linear float back to 8-bit int
      if (vips_image_new_array (VIPS_OBJECT(image), t, 5) ||
	 im_identity (t[0], 1) ||
	 im_lintra (log (255.0) / 255.0, t[0], 0.0, t[1]) ||
	 im_exptra (t[1], t[2]) ||
	 im_clip2fmt (t[2], t[3], VIPS_FORMAT_UCHAR) ||
	 im_clip2fmt (input->vips_image, t[4], VIPS_FORMAT_UCHAR) ||
	 im_maplut (t[4], image, t[3]))
	{
	  gegl_vips_error ("save");
	  g_object_unref (image);
	  return;
	}
      g_object_unref (image);
       */

	    /*

	       float to 16-bit int version, no LUT
       */
      VipsImage *image;
      VipsImage *t[7];

      if (!(image = vips_image_new_from_file (o->path, "w")))
	{
	  gegl_vips_error ("save");
	  return;
	}

      // convert from linear float back to 16-bit int
      if (vips_image_new_array (VIPS_OBJECT(image), t, 7) ||
	 im_lintra (log (255.0) / 65535.0, input->vips_image, 0.0, t[0]) ||
	 im_exptra (t[0], t[1]) ||
	 im_clip2fmt (t[1], image, VIPS_FORMAT_USHORT))
	{
	  gegl_vips_error ("save");
	  g_object_unref (image);
	  return;
	}
      g_object_unref (image);

	    /*

	       float to 16-bit int version
      VipsImage *image;
      VipsImage *t[7];

      if (!(image = vips_image_new_from_file (o->path, "w")))
	{
	  gegl_vips_error ("save");
	  return;
	}

      // convert from linear float back to 16-bit int
      if (vips_image_new_array (VIPS_OBJECT(image), t, 7) ||
	 im_identity_ushort (t[0], 1, 65536) ||
	 im_lintra (log (255.0) / 65535.0, t[0], 0.0, t[1]) ||
	 im_exptra (t[1], t[2]) ||
	 im_lintra (256.0, t[2], 0.0, t[3]) ||
	 im_clip2fmt (t[3], t[4], VIPS_FORMAT_USHORT) ||
	 im_lintra (256.0, input->vips_image, 0.0, t[5]) ||
	 im_clip2fmt (t[5], t[6], VIPS_FORMAT_USHORT) ||
	 im_maplut (t[6], image, t[4]))
	{
	  gegl_vips_error ("save");
	  g_object_unref (image);
	  return;
	}
      g_object_unref (image);
       */

      node->vips_hash = hash;

      printf ("gegl_save_prepare:\n");
      printf ("\tpath = %s\n", o->path);
    }
}

static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationSinkClass *sink_class      = GEGL_OPERATION_SINK_CLASS (klass);
  GeglOperationClass     *operation_class = GEGL_OPERATION_CLASS (klass);

  operation_class->prepare = gegl_save_prepare;

  sink_class->needs_full = TRUE;

  operation_class->name        = "gegl:save";
  operation_class->categories  = "output";
  operation_class->description = _("Multipurpose file saver.");
}

#endif

