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
 * License along with GEGL; if not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright 2006, 2010 Øyvind Kolås <pippin@gimp.org>
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_double(std_dev, _("Std. Dev."), 0.0, 100.0, 1.0,
                  _("Standard deviation (spatial scale factor)"))
gegl_chant_double(scale,  _("Scale"), 0.0, 100.0, 1.0, _("Scale, strength of effect."))

#else

#define GEGL_CHANT_TYPE_FILTER
#define GEGL_CHANT_C_FILE       "unsharp-mask.c"

#include "gegl-chant.h"
#include "graph/gegl-node.h"

#include <stdio.h>

static DOUBLEMASK *
im_gauss_dmask_sep (const char *filename, double sigma, double min_ampl)
{
  DOUBLEMASK *im;
  DOUBLEMASK *im2;
  int i;
  double sum;

  if (!(im = im_gauss_dmask (filename, sigma, min_ampl)))
    return (NULL);
  if (!(im2 = im_create_dmask (filename, im->xsize, 1)))
    {
      im_free_dmask (im);
      return (NULL);
    }

  sum = 0.0;
  for (i = 0; i < im->xsize; i++)
    {
      im2->coeff[i] = im->coeff[i + im->xsize * (im->ysize / 2)];
      sum += im2->coeff[i];
    }
  im2->scale = sum;

  im_free_dmask (im);

  return (im2);
}

static void
gegl_unsharp_prepare (GeglOperation * operation)
{
  GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);
  GeglNode *node = operation->node;

  GeglNode *input;
  guint64 hash;

  printf ("gegl_unsharp_prepare: 1\n");
      printf ("\tstd_dev = %g\n", o->std_dev);
      printf ("\tscale = %g\n", o->scale);

  input = gegl_operation_get_source_node (operation, "input");
  if (!input || !input->vips_image)
    return;

  hash = 0;
  hash = GEGL_VIPS_HASH_POINTER (hash, input->vips_image);
  hash = GEGL_VIPS_HASH_DOUBLE (hash, o->std_dev);
  hash = GEGL_VIPS_HASH_DOUBLE (hash, o->scale);

  if (!node->vips_image || node->vips_hash != hash)
    {

	    /*
	 // 3x3 sharpen, like the others in vips bench

         VipsImage *image;
         INTMASK *mask;

         mask = im_create_imaskv ("untitled", 3, 3, 
		 -1, -1, -1, -1, 16, -1, -1, -1, -1);
         mask->scale = 8;

         image = vips_image_new ("p");
         if (im_conv (input->vips_image, image, mask))
           {
		 gegl_vips_error ("unsharp");
		 g_object_unref (image);
		 im_free_imask (mask);
		 return;
          }
        node->vips_image = image;
        node->vips_hash = hash;
	 */

	    /*
	 */
	    // 7x1 sep conv, what the gegl usharp is equivalent to
         VipsImage *image;
         INTMASK *mask;

         mask = im_gauss_imask_sep ("untitled", o->std_dev, 0.1);
         mask->scale = 8;

         image = vips_image_new ("p");
         if (im_convsep (input->vips_image, image, mask))
           {
		 gegl_vips_error ("unsharp");
		 g_object_unref (image);
		 im_free_imask (mask);
		 return;
          }
        node->vips_image = image;
        node->vips_hash = hash;

	    /*
	       float conv

	       // gegl-like usharp

      VipsImage *image;
      VipsImage *t[4];
      DOUBLEMASK *mask;

      image = vips_image_new ("p");

      if (!(mask = im_local_dmask (image,
				   im_gauss_dmask_sep ("untitled", o->std_dev,
						       0.1)))
	  || vips_image_new_array (VIPS_OBJECT(image), t, 4)
	  || im_convsepf (input->vips_image, t[0], mask)
	  || im_subtract (input->vips_image, t[0], t[1])
	  || im_lintra (o->scale, t[1], 0, t[2])
	  || im_add (input->vips_image, t[2], image))
	{
	  gegl_vips_error ("unsharp");
	  g_object_unref (image);
	  return;
	}
	 */

	    /*

	       	int conv

	       // gegl-like usharp

      VipsImage *image;
      VipsImage *t[4];
      INTMASK *mask;

      image = vips_image_new ("p");

      if (!(mask = im_local_imask (image,
				   im_gauss_imask_sep ("untitled", o->std_dev,
						       0.1)))
	  || vips_image_new_array (VIPS_OBJECT(image), t, 4)
	  || im_convsep (input->vips_image, t[0], mask)
	  || im_subtract (input->vips_image, t[0], t[1])
	  || im_lintra (o->scale, t[1], 0, t[2])
	  || im_add (input->vips_image, t[2], t[3])
	  || im_clip2fmt (t[3], image, input->vips_image->BandFmt))
	{
	  gegl_vips_error ("unsharp");
	  g_object_unref (image);
	  return;
	}
 */

      node->vips_image = image;
      node->vips_hash = hash;

      {
      int i;
      printf ("\t");
      for (i = 0; i < mask->xsize; i++)
	printf ("%g ", mask->coeff[i]);
      printf ("\n");
      }
    }
}

static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GObjectClass       *object_class;
  GeglOperationClass *operation_class;

  object_class    = G_OBJECT_CLASS (klass);
  operation_class = GEGL_OPERATION_CLASS (klass);
  operation_class->prepare = gegl_unsharp_prepare;

  operation_class->name        = "gegl:unsharp-mask";
  operation_class->categories  = "enhance";
  operation_class->description =
        _("Performs an unsharp mask on the input buffer (sharpens an image by "
          "adding false mach-bands around edges).");
}

#endif
