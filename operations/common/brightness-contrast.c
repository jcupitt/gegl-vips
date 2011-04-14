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

/* Followed by this #if ... */
#ifdef GEGL_CHANT_PROPERTIES
/* ... are the properties of the filter, these are all scalar values
 * (doubles), the the parameters are:
 *       property name,   min,   max, default, "description of property"
 */

gegl_chant_double (contrast, _("Contrast"), -5.0, 5.0, 1.0,
                   _("Range scale factor"))
gegl_chant_double (brightness, _("Brightness"), -3.0, 3.0, 0.0,
                   _("Amount to increase brightness"))

/* this will create the instance structure for our use, and register the
 * property with the given ranges, default values and a comment for the
 * documentation/tooltip.
 */
#else

/* Specify the base class we're building our operation on, the base
 * class provides a lot of the legwork so we do not have to. For
 * brightness contrast the best base class is the POINT_FILTER base
 * class.
 */
#define GEGL_CHANT_TYPE_POINT_FILTER

/* We specify the file we're in, this is needed to make the code
 * generation for the properties work.
 */
#define GEGL_CHANT_C_FILE       "brightness-contrast.c"

/* Including gegl-chant.h creates most of the GObject boiler plate
 * needed, creating a GeglChant instance structure a GeglChantClass
 * structure for our operation, as well as the needed code to register
 * our new gobject with GEGL.
 */
#include "gegl-chant.h"


/* prepare() is called on each operation providing data to a node that
 * is requested to provide a rendered result. When prepare is called
 * all properties are known. For brightness contrast we use this
 * opportunity to dictate the formats of the input and output buffers.
 */
static void prepare (GeglOperation *operation)
{
  gegl_operation_set_format (operation, "input", babl_format ("RGBA float"));
  gegl_operation_set_format (operation, "output", babl_format ("RGBA float"));
}

/* For GeglOperationPointFilter subclasses, we operate on linear
 * buffers with a pixel count.
 */
static gboolean
process (GeglOperation       *op,
         void                *in_buf,
         void                *out_buf,
         glong                n_pixels,
         const GeglRectangle *roi)
{
  /* Retrieve a pointer to GeglChantO structure which contains all the
   * chanted properties
   */
  GeglChantO *o = GEGL_CHANT_PROPERTIES (op);
  gfloat     * GEGL_ALIGNED in_pixel;
  gfloat     * GEGL_ALIGNED out_pixel;
  gfloat      brightness, contrast;
  glong       i;

  in_pixel   = in_buf;
  out_pixel  = out_buf;

  brightness = o->brightness;
  contrast   = o->contrast;

  for (i=0; i<n_pixels; i++)
    {
      out_pixel[0] = (in_pixel[0] - 0.5f) * contrast + brightness + 0.5;
      out_pixel[1] = (in_pixel[1] - 0.5f) * contrast + brightness + 0.5;
      out_pixel[2] = (in_pixel[2] - 0.5f) * contrast + brightness + 0.5;
      out_pixel[3] = in_pixel[3]; /* copy the alpha */
      in_pixel  += 4;
      out_pixel += 4;
    }
  return TRUE;
}

/*
 * The class init function sets up information needed for this operations class
 * (template) in the GObject OO framework.
 */
static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationClass            *operation_class;
  GeglOperationPointFilterClass *point_filter_class;

  operation_class    = GEGL_OPERATION_CLASS (klass);
  point_filter_class = GEGL_OPERATION_POINT_FILTER_CLASS (klass);

  /* override the prepare methods of the GeglOperation class */
  operation_class->prepare = prepare;
  /* override the process method of the point filter class (the process methods
   * of our superclasses deal with the handling on their level of abstraction)
   */
  point_filter_class->process = process;

  /* specify the name this operation is found under in the GUI/when
   * programming/in XML
   */
  operation_class->name        = "gegl:brightness-contrast";

  /* a colon separated list of categories/tags for this operations */
  operation_class->categories  = "color";

  /* a description of what this operations does */
  operation_class->description = _("Changes the light level and contrast.");
}

#endif /* closing #ifdef GEGL_CHANT_PROPERTIES ... else ... */
