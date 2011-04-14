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

gegl_chant_double (radius, _("Radius"), 0.0, 50.0, 4.0,
  _("Radius of square pixel region (width and height will be radius*2+1)"))

#else

#define GEGL_CHANT_TYPE_AREA_FILTER
#define GEGL_CHANT_C_FILE       "kuwahara-min.c"

#include "gegl-chant.h"
#include <math.h>

static inline void
compute_rectangle (gfloat *buf,
                   gint    buf_width,
                   gint    buf_height,
                   gint    x0,
                   gint    y0,
                   gint    width,
                   gint    height,
                   gint    component,
                   gfloat *pmin,
                   gfloat *pmax,
                   gfloat *pmean,
                   gfloat *pvariance)
{
  gint    x, y;
  gfloat  max   = -1000000000.0;
  gfloat  min   =  1000000000.0;
  gfloat  mean  =  0.0;
  glong   count =  0;

  gint offset = (y0 * buf_width + x0) * 4 + component;

  for (y=y0; y<y0+height; y++)
    {
    for (x=x0; x<x0+width; x++)
      {
        if (x>=0 && x<buf_width &&
            y>=0 && y<buf_height)
          {
            if (buf [offset] > max)
              max = buf[offset];
            if (buf [offset] < min)
              min = buf[offset];
            mean += buf[offset];
            count++;
          }
        offset+=4;
      }
      offset+= (buf_width * 4) - 4 * width;
    }
  if (pmin)
    *pmin = min;
  if (pmax)
    *pmax = max;
  if (pmean && count)
    *pmean = mean/count;
  if (pvariance)
    *pvariance = max-min;
}

static void
kuwahara (GeglBuffer *src,
          GeglBuffer *dst,
          gint        radius)
{
  gint u,v;
  gint offset;
  gfloat *src_buf;
  gfloat *dst_buf;

  src_buf = g_new0 (gfloat, gegl_buffer_get_pixel_count (src) * 4);
  dst_buf = g_new0 (gfloat, gegl_buffer_get_pixel_count (dst) * 4);

  gegl_buffer_get (src, 1.0, NULL, babl_format ("RGBA float"), src_buf, GEGL_AUTO_ROWSTRIDE);

  offset = 0;
  for (v=0; v<gegl_buffer_get_height (dst); v++)
    for (u=0; u<gegl_buffer_get_width (dst); u++)
      {
        gint component;

        for (component=0; component<3; component++)
          {
            gfloat value=0.0;
            gfloat best=1000000.0;

            gfloat min = 0.0;
            gfloat variance = 0.0;

            compute_rectangle (src_buf,
                               gegl_buffer_get_width (src),
                               gegl_buffer_get_height (src),
                               u - radius - 1,
                               v - radius - 1,
                               1 + radius,
                               1 + radius,
                               component,
                               &min, /* min */
                               NULL, /* max */
                               NULL,
                               &variance);
            if (variance<best)
              {
                value = min;
                best = variance;
              }

            compute_rectangle (src_buf,
                               gegl_buffer_get_width (src),
                               gegl_buffer_get_height (src),
                               u,
                               v - radius - 1,
                               1 + radius,
                               1 + radius,
                               component,
                               &min, /* min */
                               NULL, /* max */
                               NULL,
                               &variance);
            if (variance<best)
              {
                value = min;
                best = variance;
              }

            compute_rectangle (src_buf,
                               gegl_buffer_get_width (src),
                               gegl_buffer_get_height (src),
                               u - radius - 1,
                               v,
                               1 + radius,
                               1 + radius,
                               component,
                               &min, /* min */
                               NULL, /* max */
                               NULL,
                               &variance);
            if (variance<best)
              {
                value = min;
                best = variance;
              }

            compute_rectangle (src_buf,
                               gegl_buffer_get_width (src),
                               gegl_buffer_get_height (src),
                               u,
                               v,
                               1 + radius,
                               1 + radius,
                               component,
                               &min, /* min */
                               NULL, /* max */
                               NULL,
                               &variance);

            if (variance<best)
              {
                value = min;
                best = variance;
              }
            dst_buf [offset++] = value;
          }
          dst_buf [offset] = src_buf[offset];
          offset++;
      }

  gegl_buffer_set (dst, NULL, babl_format ("RGBA float"), dst_buf, GEGL_AUTO_ROWSTRIDE);
  g_free (src_buf);
  g_free (dst_buf);
}

static void prepare (GeglOperation *operation)
{
  GeglOperationAreaFilter *area = GEGL_OPERATION_AREA_FILTER (operation);

  area->left = area->right = area->top = area->bottom =
      ceil (GEGL_CHANT_PROPERTIES (operation)->radius);
  gegl_operation_set_format (operation, "output", babl_format ("RGBA float"));
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *result)
{
  GeglChantO   *o = GEGL_CHANT_PROPERTIES (operation);
  GeglBuffer   *temp_in;
  GeglRectangle compute = gegl_operation_get_required_for_output (operation, "input", result);

  temp_in = gegl_buffer_create_sub_buffer (input, &compute);

  kuwahara (temp_in, output, o->radius);
  g_object_unref (temp_in);

  return  TRUE;
}


static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationFilterClass *filter_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  filter_class->process = process;
  operation_class->prepare = prepare;

  operation_class->name        = "gegl:kuwahara-min";
  operation_class->categories  = "misc";
  operation_class->description = _("Edge preserving min filter");
}

#endif
