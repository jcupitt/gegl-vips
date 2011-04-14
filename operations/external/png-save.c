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
 */

#include "config.h"
#include <glib/gi18n-lib.h>


#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_string (path, _("File"), "",
		   _("Target path and filename, use '-' for stdout."))
gegl_chant_int (compression, _("Compression"),
		1, 9, 1, _("PNG compression level from 1 to 9"))
gegl_chant_int (bitdepth, _("Bitdepth"),
		8, 16, 16,
		_("8 and 16 are amongst the currently accepted values."))

#else

#define GEGL_CHANT_TYPE_SINK
#define GEGL_CHANT_C_FILE       "png-save.c"

#include "gegl-chant.h"
#include <graph/gegl-node.h>

#include <png.h>
#include <stdio.h>

static void
gegl_png_save_prepare (GeglOperation * operation)
{
  GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);
  GeglNode *input;

  printf ("gegl_png_save_prepare:\n");
  printf ("\tpath = %s\n", o->path);

  input = gegl_operation_get_source_node (operation, "input");
  if (!input || !input->vips_image)
    {
      g_warning ("gegl_png_save_prepare: no input image\n");
      return;
    }

  if (im_vips2png (input->vips_image, o->path)) {
      g_warning ("gegl_png_save_prepare: save failed\n");
    return;
   }
}


static void
gegl_chant_class_init (GeglChantClass * klass)
{
  GeglOperationClass *operation_class;
  GeglOperationSinkClass *sink_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  sink_class = GEGL_OPERATION_SINK_CLASS (klass);

  operation_class->prepare = gegl_png_save_prepare;

  operation_class->name = "gegl:png-save";
  operation_class->categories = "output";
  operation_class->description =
    _("PNG image saver (passes the buffer through, saves as a side-effect.)");

  gegl_extension_handler_register_saver (".png", "gegl:png-save");
}

#endif
