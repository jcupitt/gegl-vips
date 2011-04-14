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

gegl_chant_double   (amount, _("Amount"),  0.0, 1.0, 0.2,
                  _("Amount of noise to add."))
gegl_chant_double   (saturation, _("Saturation"), 0.0, 1.0, 0.0,
                  _("Color saturation of the noise"))
#else

#define GEGL_CHANT_TYPE_POINT_FILTER
#define GEGL_CHANT_C_FILE       "add-noise.c"

#include "gegl-chant.h"

static void
prepare (GeglOperation *operation)
{
  gegl_operation_set_format (operation, "input", babl_format ("RGBA float"));
  gegl_operation_set_format (operation, "output", babl_format ("RGBA float"));
}

static gboolean
process (GeglOperation       *operation,
         void                *in_buf,
         void                *out_buf,
         glong                n_pixels,
         const GeglRectangle *roi)
{
  GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);
  gfloat     *in_pixel = in_buf;
  gfloat     *out_pixel = out_buf;

  GRand* noise_gr = g_rand_new();

  gdouble amount = o->amount;
  gdouble saturation = o->saturation;

  while (n_pixels--)
    {
      gdouble noise[3];
      gdouble monochrome;
      gint c;
      for (c=0; c<3; ++c)
         noise[c] = g_rand_double_range(noise_gr, -1.0, 1.0);

      monochrome = noise[0];
      for (c=0; c<3; ++c)
        noise[c] = saturation*(noise[c]-monochrome) + monochrome;
      
      /* should these be clamped 0-1? */
      for (c=0; c<3; ++c)
        {
          out_pixel[c] = in_pixel[c] + amount*noise[c];
          out_pixel[c] = MAX(0.0, out_pixel[c]);
        }
      
      out_pixel[3] = in_pixel[3];
    
      in_pixel += 4;
      out_pixel += 4;
    }

  g_rand_free(noise_gr);

  return  TRUE;
}


static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationClass            *operation_class;
  GeglOperationPointFilterClass *point_filter_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  point_filter_class = GEGL_OPERATION_POINT_FILTER_CLASS (klass);

  point_filter_class->process = process;
  operation_class->prepare = prepare;

  operation_class->name        = "gegl:add-noise";
  operation_class->categories  = "noise";
  operation_class->description = _("Add Random noise");
}

#endif
