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
 * Copyright 2007 Mark Probst <mark.probst@gmail.com>
 */

#include "config.h"
#include <glib/gi18n-lib.h>


#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_int (sampling_points, _("Sample points"), 0, 65536, 0,
                _("Number of curve sampling points.  0 for exact calculation."))
gegl_chant_curve (curve, _("Curve"), _("The contrast curve."))

#else

#define GEGL_CHANT_TYPE_POINT_FILTER
#define GEGL_CHANT_C_FILE  "contrast-curve.c"

#include "gegl-chant.h"

static void prepare (GeglOperation *operation)
{
  Babl *format = babl_format ("YA float");

  gegl_operation_set_format (operation, "input", format);
  gegl_operation_set_format (operation, "output", format);
}

static gboolean
process (GeglOperation       *op,
         void                *in_buf,
         void                *out_buf,
         glong                samples,
         const GeglRectangle *roi)
{
  GeglChantO *o = GEGL_CHANT_PROPERTIES (op);
  gint        num_sampling_points;
  GeglCurve  *curve;
  gint i;
  gfloat  *in  = in_buf;
  gfloat  *out = out_buf;
  gdouble *xs, *ys;

  num_sampling_points = o->sampling_points;
  curve = o->curve;

  if (num_sampling_points > 0)
  {
    xs = g_new(gdouble, num_sampling_points);
    ys = g_new(gdouble, num_sampling_points);

    gegl_curve_calc_values(o->curve, 0.0, 1.0, num_sampling_points, xs, ys);

    g_free(xs);

    for (i=0; i<samples; i++)
    {
      gint x = in[0] * num_sampling_points;
      gfloat y;

      if (x < 0)
       y = ys[0];
      else if (x >= num_sampling_points)
       y = ys[num_sampling_points - 1];
      else
       y = ys[x];

      out[0] = y;
      out[1]=in[1];

      in += 2;
      out+= 2;
    }

    g_free(ys);
  }
  else
    for (i=0; i<samples; i++)
    {
      gfloat u = in[0];

      out[0] = gegl_curve_calc_value(curve, u);
      out[1]=in[1];

      in += 2;
      out+= 2;
    }

  return TRUE;
}


static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationClass            *operation_class;
  GeglOperationPointFilterClass *point_filter_class;

  operation_class    = GEGL_OPERATION_CLASS (klass);
  point_filter_class = GEGL_OPERATION_POINT_FILTER_CLASS (klass);

  point_filter_class->process = process;
  operation_class->prepare = prepare;

  operation_class->name        = "gegl:contrast-curve";
  operation_class->categories  = "color";
  operation_class->description =
        _("Adjusts the contrast of the image according to a curve.");
}

#endif
