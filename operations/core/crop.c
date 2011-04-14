/* This file is part of GEGL
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
 * Copyright 2006 Øyvind Kolås
 */

#include "config.h"
#include <glib/gi18n-lib.h>


#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_double (x,      _("X"),      -G_MAXFLOAT, G_MAXFLOAT,  0.0, _("X"))
gegl_chant_double (y,      _("Y"),      -G_MAXFLOAT, G_MAXFLOAT,  0.0, _("Y"))
gegl_chant_double (width,  _("Width"),  -G_MAXFLOAT, G_MAXFLOAT, 10.0, _("Width"))
gegl_chant_double (height, _("Height"), -G_MAXFLOAT, G_MAXFLOAT, 10.0, _("Height"))

#else

#define GEGL_CHANT_TYPE_FILTER
#define GEGL_CHANT_C_FILE       "crop.c"

#include "gegl-chant.h"
#include "graph/gegl-node.h"
#include <math.h>

#include <stdio.h>

static GeglNode *
gegl_crop_detect (GeglOperation *operation,
                  gint           x,
                  gint           y)
{
  GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);
  GeglNode   *input_node;

  input_node = gegl_operation_get_source_node (operation, "input");

  if (input_node)
    return gegl_operation_detect (input_node->operation,
                                  x - floor (o->x),
                                  y - floor (o->y));

  return operation->node;
}


static GeglRectangle
gegl_crop_get_bounding_box (GeglOperation *operation)
{
  GeglChantO    *o = GEGL_CHANT_PROPERTIES (operation);
  GeglRectangle *in_rect = gegl_operation_source_get_bounding_box (operation, "input");
  GeglRectangle  result  = { 0, 0, 0, 0 };

  if (!in_rect)
    return result;

  result.x = o->x;
  result.y = o->y;
  result.width  = o->width;
  result.height = o->height;

  return result;
}

static GeglRectangle
gegl_crop_get_invalidated_by_change (GeglOperation       *operation,
                                     const gchar         *input_pad,
                                     const GeglRectangle *input_region)
{
  GeglChantO   *o = GEGL_CHANT_PROPERTIES (operation);
  GeglRectangle result;

  result.x = o->x;
  result.y = o->y;
  result.width = o->width;
  result.height = o->height;

  gegl_rectangle_intersect (&result, &result, input_region);

  return result;
}

static GeglRectangle
gegl_crop_get_required_for_output (GeglOperation       *operation,
                                   const gchar         *input_pad,
                                   const GeglRectangle *roi)
{
  GeglChantO   *o = GEGL_CHANT_PROPERTIES (operation);
  GeglRectangle result;

  result.x = o->x;
  result.y = o->y;
  result.width = o->width;
  result.height = o->height;

  gegl_rectangle_intersect (&result, &result, roi);
  return result;
}

static void
gegl_crop_prepare (GeglOperation * operation)
{
  GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);
  GeglNode *node = operation->node;

  GeglNode *input;
  guint64 hash;

  printf ("gegl_crop_prepare: 1\n");

  input = gegl_operation_get_source_node (operation, "input");
  if (!input || !input->vips_image)
    return;

  hash = 0;
  hash = GEGL_VIPS_HASH_POINTER (hash, input->vips_image);
  hash = GEGL_VIPS_HASH_DOUBLE (hash, o->x);
  hash = GEGL_VIPS_HASH_DOUBLE (hash, o->y);
  hash = GEGL_VIPS_HASH_DOUBLE (hash, o->width);
  hash = GEGL_VIPS_HASH_DOUBLE (hash, o->height);

  if (!node->vips_image || node->vips_hash != hash)
    {
      VipsImage *image;

      image = vips_image_new ("p");
      if (im_extract_area (input->vips_image, image,
			   o->x, o->y, o->width, o->height))
	{
	  gegl_vips_error ("crop");
	  g_object_unref (image);
	  return;
	}
      node->vips_image = image;
      node->vips_hash = hash;

      printf ("gegl_crop_prepare:\n");
      printf ("\tx = %g\n", o->x);
      printf ("\ty = %g\n", o->y);
      printf ("\twidth = %g\n", o->width);
      printf ("\theight = %g\n", o->height);
    }
}

static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationClass *operation_class;

  operation_class = GEGL_OPERATION_CLASS (klass);

  operation_class->prepare                   = gegl_crop_prepare;
  operation_class->get_bounding_box          = gegl_crop_get_bounding_box;
  operation_class->detect                    = gegl_crop_detect;
  operation_class->get_invalidated_by_change = gegl_crop_get_invalidated_by_change;
  operation_class->get_required_for_output   = gegl_crop_get_required_for_output;

  operation_class->name        = "gegl:crop";
  operation_class->categories  = "core";
  operation_class->description = _("Crop a buffer");

  operation_class->no_cache = TRUE;
}

#endif
