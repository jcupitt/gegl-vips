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

gegl_chant_double (radius, _("Radius"), 0.0, 200.0, 4.0,
  _("Radius of square pixel region (width and height will be radius*2+1)"))

#else

#define GEGL_CHANT_TYPE_AREA_FILTER
#define GEGL_CHANT_C_FILE       "box-max.c"

#include "gegl-chant.h"
#include <stdio.h>
#include <math.h>

static inline gfloat
get_max_component (gfloat *buf,
                   gint    buf_width,
                   gint    buf_height,
                   gint    x0,
                   gint    y0,
                   gint    width,
                   gint    height,
                   gint    component)
{
  gint    x, y;
  gfloat max=-1000000000.0;
  gint    count=0;

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
            count++;
          }
        offset+=4;
      }
      offset+= (buf_width * 4) - 4 * width;
    }
   return max;
}

static void
hor_max (GeglBuffer          *src,
         const GeglRectangle *src_rect,
         GeglBuffer          *dst,
         const GeglRectangle *dst_rect,
         gint                 radius)
{
  gint u,v;
  gint offset;
  gfloat *src_buf;
  gfloat *dst_buf;

  src_buf = g_new0 (gfloat, src_rect->width * src_rect->height * 4);
  dst_buf = g_new0 (gfloat, dst_rect->width * dst_rect->height * 4);

  gegl_buffer_get (src, 1.0, src_rect, babl_format ("RGBA float"), src_buf, GEGL_AUTO_ROWSTRIDE);

  offset = 0;
  for (v=0; v<dst_rect->height; v++)
    for (u=0; u<dst_rect->width; u++)
      {
        gint i;

        for (i=0; i<4; i++)
          dst_buf [offset++] = get_max_component (src_buf,
                               src_rect->width,
                               src_rect->height,
                               u + radius - radius,
                               v + radius,
                               1 + radius*2,
                               1,
                               i);
      }

  gegl_buffer_set (dst, dst_rect, babl_format ("RGBA float"), dst_buf,
                   GEGL_AUTO_ROWSTRIDE);
  g_free (src_buf);
  g_free (dst_buf);
}


static void
ver_max (GeglBuffer *src,
         const GeglRectangle *src_rect,
         GeglBuffer          *dst,
         const GeglRectangle *dst_rect,
         gint                 radius)
{
  gint u,v;
  gint offset;
  gfloat *src_buf;
  gfloat *dst_buf;

  src_buf = g_new0 (gfloat, src_rect->width * src_rect->height * 4);
  dst_buf = g_new0 (gfloat, dst_rect->width * src_rect->height * 4);

  gegl_buffer_get (src, 1.0, src_rect, babl_format ("RGBA float"), src_buf, GEGL_AUTO_ROWSTRIDE);

  offset=0;
  for (v=0; v<dst_rect->height; v++)
    for (u=0; u<dst_rect->width; u++)
      {
        gint c;

        for (c=0; c<4; c++)
          dst_buf [offset++] =
           get_max_component (src_buf,
                              src_rect->width,
                              src_rect->height,
                              u,
                              v - radius,
                              1,
                              1 + radius * 2,
                              c);
      }

  gegl_buffer_set (dst, dst_rect, babl_format ("RGBA float"), dst_buf,
                   GEGL_AUTO_ROWSTRIDE);
  g_free (src_buf);
  g_free (dst_buf);
}

static void prepare (GeglOperation *operation)
{
  GeglOperationAreaFilter *area = GEGL_OPERATION_AREA_FILTER (operation);
  area->left  =
  area->right =
  area->top   =
  area->bottom = GEGL_CHANT_PROPERTIES (operation)->radius;
  gegl_operation_set_format (operation, "output", babl_format ("RGBA float"));
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *result)
{
  GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);
  GeglRectangle input_rect = gegl_operation_get_required_for_output (operation, "input", result);

  hor_max ( input, &input_rect, output, result, o->radius);
  ver_max (output,      result, output, result, o->radius);

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

  operation_class->name        = "gegl:box-max";
  operation_class->categories  = "misc";
  operation_class->description =
        _("Sets the target pixel to the value of the maximum value in a box surrounding the pixel.");
}

#endif
