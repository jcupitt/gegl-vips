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
 * Copyright 2008 Øyvind Kolås <pippin@gimp.org>
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_double (x1,        _("x1"),  -1000.0, 1000.0, 25.0, "")
gegl_chant_double (y1,        _("y1"),  -1000.0, 1000.0, 25.0, "")
gegl_chant_double (x2,        _("x2"),  -1000.0, 1000.0, 150.0, "")
gegl_chant_double (y2,        _("y2"),  -1000.0, 1000.0, 150.0, "")
gegl_chant_color (color1,   _("Color"), "black",
                  _("one end of gradient"))
gegl_chant_color (color2,   _("Other color"), "white",
                  _("one end of gradient"))

#else

#define GEGL_CHANT_TYPE_POINT_RENDER
#define GEGL_CHANT_C_FILE       "linear-gradient.c"

#include "gegl-chant.h"

#include <math.h>

static void
prepare (GeglOperation *operation)
{
  gegl_operation_set_format (operation, "output", babl_format ("RGBA float"));
}

static GeglRectangle
get_bounding_box (GeglOperation *operation)
{
  GeglRectangle result = {-10000000, -10000000, 20000000, 20000000};
  return result;
}

static gfloat dist(gfloat x1, gfloat y1, gfloat x2, gfloat y2)
{
  return sqrt(  (x1-x2)*(x1-x2)+(y1-y2)*(y1-y2));
}

static gboolean
process (GeglOperation       *operation,
         void                *out_buf,
         glong                n_pixels,
         const GeglRectangle *roi)
{
  GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);
  gfloat     *out_pixel = out_buf;
  gfloat      color1[4];
  gfloat      color2[4];
  gint        x, y;
  gfloat      dx, dy;

  gfloat length = dist (o->x1, o->y1, o->x2, o->y2);

  gegl_color_get_rgba4f (o->color1, color1);
  gegl_color_get_rgba4f (o->color2, color2);


  x= roi->x;
  y= roi->y;

  dx = (o->x2-o->x1)/length;
  dy = (o->y2-o->y1)/length;

  while (n_pixels--)
    {
      gfloat v;
      gint c;

      if (length == 0.0)
        v = 0.5;
      else
        {
          v = (dx * x + dy * y) / length;

          if (v < 0.0)
            v = 0.0;
          else if (v>1.0)
            v = 1.0;
        }

      for (c=0;c<4;c++)
        out_pixel[c]=color1[c] * v + color2[c] * (1.0-v);

      out_pixel += 4;

      /* update x and y coordinates */
      if (++x>=roi->x + roi->width)
        {
          x=roi->x;
          y++;
      }
    }

  return  TRUE;
}


static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationClass            *operation_class;
  GeglOperationPointRenderClass *point_render_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  point_render_class = GEGL_OPERATION_POINT_RENDER_CLASS (klass);

  point_render_class->process = process;
  operation_class->get_bounding_box = get_bounding_box;
  operation_class->prepare = prepare;
  operation_class->no_cache = TRUE;

  operation_class->name        = "gegl:linear-gradient";
  operation_class->categories  = "render";
  operation_class->description = _("linear gradient renderer");
}

#endif
