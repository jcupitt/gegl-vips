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
 * Copyright 2006 Philip Lafleur
 */

/* TODO: reenable different samplers (through the sampling mechanism
 *       of GeglBuffer intitially) */
/* TODO: only calculate pixels inside transformed polygon */
/* TODO: should hard edges always be used when only scaling? */
/* TODO: make rect calculations depend on the sampling kernel of the
 *       interpolation filter used */

#include "config.h"
#include <glib/gi18n-lib.h>


#include <math.h>
#include <gegl.h>
#include <gegl-plugin.h>
#include <gegl-utils.h>
#include "buffer/gegl-sampler.h"
#include <graph/gegl-pad.h>
#include <graph/gegl-node.h>
#include <graph/gegl-connection.h>

#include <stdio.h>

#include "affine.h"
#include "module.h"

enum
{
  PROP_ORIGIN_X = 1,
  PROP_ORIGIN_Y,
  PROP_FILTER,
  PROP_HARD_EDGES,
  PROP_LANCZOS_WIDTH
};


static void          gegl_affine_get_property              (GObject              *object,
                                                            guint                 prop_id,
                                                            GValue               *value,
                                                            GParamSpec           *pspec);
static void          gegl_affine_set_property              (GObject              *object,
                                                            guint                 prop_id,
                                                            const GValue         *value,
                                                            GParamSpec           *pspec);
static void          gegl_affine_bounding_box              (gdouble              *points,
                                                            gint                  num_points,
                                                            GeglRectangle        *output);
static gboolean      gegl_affine_is_intermediate_node      (OpAffine             *affine);
static gboolean      gegl_affine_is_composite_node         (OpAffine             *affine);
static void          gegl_affine_get_source_matrix         (OpAffine             *affine,
                                                            GeglMatrix3           output);
static GeglRectangle gegl_affine_get_bounding_box          (GeglOperation        *op);
static GeglRectangle gegl_affine_get_invalidated_by_change (GeglOperation        *operation,
                                                            const gchar          *input_pad,
                                                            const GeglRectangle  *input_region);
static GeglRectangle gegl_affine_get_required_for_output   (GeglOperation        *self,
                                                            const gchar          *input_pad,
                                                            const GeglRectangle  *region);
#if 0
static gboolean      gegl_affine_process                   (GeglOperation       *op,
                                                            GeglBuffer          *input,
                                                            GeglBuffer          *output,
                                                            const GeglRectangle *result);
#endif
static gboolean      gegl_affine_process                   (GeglOperation        *operation,
                                                            GeglOperationContext *context,
                                                            const gchar          *output_prop,
                                                            const GeglRectangle  *result);
static GeglNode    * gegl_affine_detect                    (GeglOperation        *operation,
                                                            gint                  x,
                                                            gint                  y);


/* ************************* */

static void     op_affine_init          (OpAffine      *self);
static void     op_affine_class_init    (OpAffineClass *klass);
static gpointer op_affine_parent_class = NULL;

static void
op_affine_class_intern_init (gpointer klass)
{
  op_affine_parent_class = g_type_class_peek_parent (klass);
  op_affine_class_init ((OpAffineClass *) klass);
}

GType
op_affine_get_type (void)
{
  static GType g_define_type_id = 0;
  if (G_UNLIKELY (g_define_type_id == 0))
    {
      static const GTypeInfo g_define_type_info =
        {
          sizeof (OpAffineClass),
          (GBaseInitFunc) NULL,
          (GBaseFinalizeFunc) NULL,
          (GClassInitFunc) op_affine_class_intern_init,
          (GClassFinalizeFunc) NULL,
          NULL,   /* class_data */
          sizeof (OpAffine),
          0,      /* n_preallocs */
          (GInstanceInitFunc) op_affine_init,
          NULL    /* value_table */
        };

      g_define_type_id =
        gegl_module_register_type (affine_module_get_module (),
                                   GEGL_TYPE_OPERATION_FILTER,
                                   "GeglOpPlugIn-affine",
                                   &g_define_type_info, 0);
    }
  return g_define_type_id;
}


GType
gegl_sampler_type_from_interpolation (GeglInterpolation interpolation);

/* ************************* */
static GeglSampler *
op_affine_sampler (OpAffine *self)
{
  Babl                 *format;
  GeglSampler          *sampler;
  GType                 desired_type;
  GeglInterpolation     interpolation;

  format = babl_format ("RaGaBaA float");

  interpolation = gegl_buffer_interpolation_from_string (self->filter);
  desired_type = gegl_sampler_type_from_interpolation (interpolation);

  if (interpolation == GEGL_INTERPOLATION_LANCZOS)
    {
      sampler = g_object_new (desired_type,
                              "format", format,
                              "lanczos_width",  self->lanczos_width,
                              NULL);
    }
  else
    {
      sampler = g_object_new (desired_type,
                              "format", format,
                              NULL);
    }
  return sampler;
}
/* XXX: keep a pool of samplers */

#if 0
static void
op_affine_sampler_init (OpAffine *self)
{
  GType                 desired_type;
  GeglInterpolation     interpolation;

  interpolation = gegl_buffer_interpolation_from_string (self->filter);
  desired_type = gegl_sampler_type_from_interpolation (interpolation);

  if (self->sampler != NULL &&
      !G_TYPE_CHECK_INSTANCE_TYPE (self->sampler, desired_type))
    {
      self->sampler->buffer=NULL;
      g_object_unref(self->sampler);
      self->sampler = NULL;
    }

  self->sampler = op_affine_sampler (self);
}
#endif

static void
gegl_affine_create_matrix (OpAffine    *affine,
                           GeglMatrix3  matrix)
{
  gegl_matrix3_identity (matrix);

  if (OP_AFFINE_GET_CLASS (affine))
    OP_AFFINE_GET_CLASS (affine)->create_matrix (affine, matrix);
}

static void
gegl_affine_create_composite_matrix (OpAffine    *affine,
                                     GeglMatrix3  matrix)
{
  gegl_affine_create_matrix (affine, matrix);

  if (affine->origin_x || affine->origin_y)
    gegl_matrix3_originate (matrix, affine->origin_x, affine->origin_y);

  if (gegl_affine_is_composite_node (affine))
    {
      GeglMatrix3 source;

      gegl_affine_get_source_matrix (affine, source);
      gegl_matrix3_multiply (source, matrix, matrix);
    }
}

static void
gegl_affine_prepare (GeglOperation * operation)
{
  OpAffine *self = OP_AFFINE (operation);
  GeglNode *node = operation->node;

  GeglMatrix3 matrix;
  GeglNode *input;
  guint64 hash;

  printf ("gegl_affine_prepare: 1\n");

  input = gegl_operation_get_source_node (operation, "input");
  if (!input || !input->vips_image)
    return;

  gegl_affine_create_composite_matrix (self, matrix);

  hash = 0;
  hash = GEGL_VIPS_HASH_POINTER (hash, input->vips_image);
  hash = GEGL_VIPS_HASH_DOUBLE (hash, matrix[0][0]);
  hash = GEGL_VIPS_HASH_DOUBLE (hash, matrix[0][1]);
  hash = GEGL_VIPS_HASH_DOUBLE (hash, matrix[1][0]);
  hash = GEGL_VIPS_HASH_DOUBLE (hash, matrix[1][1]);

  if (!node->vips_image || node->vips_hash != hash)
    {
      VipsImage *image;
      int i, j;

      if (!(image = vips_image_new ("p")))
	{
	  gegl_vips_error ("affine");
	  return;
	}
      if (im_affinei_all (input->vips_image, image,
			  vips_interpolate_bilinear_static (),
			  matrix[0][0], matrix[0][1],
			  matrix[1][0], matrix[1][1], 0, 0))
	{
	  gegl_vips_error ("affine");
	  g_object_unref (image);
	  return;
	}
      node->vips_image = image;
      node->vips_hash = hash;

      printf ("gegl_affine_prepare:\n");
      printf ("\torigin_x = %g\n", self->origin_x);
      printf ("\torigin_y = %g\n", self->origin_y);
      printf ("\tfilter = %s\n", self->filter);
      printf ("\thard_edges = %d\n", self->hard_edges);
      printf ("\tlanczos_width = %d\n", self->lanczos_width);

      for (j = 0; j < 3; j++)
	{
	  for (i = 0; i < 3; i++)
	    printf (" %g", matrix[i][j]);
	  printf ("\n");
	}
    }
}

static void
op_affine_class_init (OpAffineClass *klass)
{
  GObjectClass             *gobject_class = G_OBJECT_CLASS (klass);
  /*GeglOperationFilterClass *filter_class  = GEGL_OPERATION_FILTER_CLASS (klass);*/
  GeglOperationClass       *op_class      = GEGL_OPERATION_CLASS (klass);

  gobject_class->set_property         = gegl_affine_set_property;
  gobject_class->get_property         = gegl_affine_get_property;

  op_class->get_invalidated_by_change = gegl_affine_get_invalidated_by_change;
  op_class->get_bounding_box          = gegl_affine_get_bounding_box;
  op_class->get_required_for_output   = gegl_affine_get_required_for_output;
  op_class->detect                    = gegl_affine_detect;
  op_class->process                   = gegl_affine_process;
  op_class->categories                = "transform";
  op_class->prepare                   = gegl_affine_prepare;
  op_class->no_cache                  = TRUE;

  /*filter_class->process             = gegl_affine_process;*/

  klass->create_matrix = NULL;

  g_object_class_install_property (gobject_class, PROP_ORIGIN_X,
                                   g_param_spec_double (
                                     "origin-x",
                                     _("Origin-x"),
                                     _("X-coordinate of origin"),
                                     -G_MAXDOUBLE, G_MAXDOUBLE,
                                     0.,
                                     G_PARAM_CONSTRUCT | G_PARAM_READWRITE));
  g_object_class_install_property (gobject_class, PROP_ORIGIN_Y,
                                   g_param_spec_double (
                                     "origin-y",
                                     _("Origin-y"),
                                     _("Y-coordinate of origin"),
                                     -G_MAXDOUBLE, G_MAXDOUBLE,
                                     0.,
                                     G_PARAM_CONSTRUCT | G_PARAM_READWRITE));
  g_object_class_install_property (gobject_class, PROP_FILTER,
                                   g_param_spec_string (
                                     "filter",
                                     _("Filter"),
                                     _("Filter type (nearest, linear, lanczos, cubic, downsharp, downsize, downsmooth, upsharp, upsize, upsmooth)"),
                                     "linear",
                                     G_PARAM_CONSTRUCT | G_PARAM_READWRITE));
  g_object_class_install_property (gobject_class, PROP_HARD_EDGES,
                                   g_param_spec_boolean (
                                     "hard-edges",
                                     _("Hard-edges"),
                                     _("Hard edges"),
                                     FALSE,
                                     G_PARAM_CONSTRUCT | G_PARAM_READWRITE));
  g_object_class_install_property (gobject_class, PROP_LANCZOS_WIDTH,
                                   g_param_spec_int (
                                     "lanczos-width",
                                     _("Lanczos-width"),
                                     _("Width of lanczos function"),
                                     3, 6, 3,
                                     G_PARAM_CONSTRUCT | G_PARAM_READWRITE));
}


static void
op_affine_init (OpAffine *self)
{
}

static void
gegl_affine_get_property (GObject    *object,
                          guint       prop_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
  OpAffine *self = OP_AFFINE (object);

  switch (prop_id)
    {
    case PROP_ORIGIN_X:
      g_value_set_double (value, self->origin_x);
      break;
    case PROP_ORIGIN_Y:
      g_value_set_double (value, self->origin_y);
      break;
    case PROP_FILTER:
      g_value_set_string (value, self->filter);
      break;
    case PROP_HARD_EDGES:
      g_value_set_boolean (value, self->hard_edges);
      break;
    case PROP_LANCZOS_WIDTH:
      g_value_set_int (value, self->lanczos_width);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gegl_affine_set_property (GObject      *object,
                          guint         prop_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
  OpAffine *self = OP_AFFINE (object);

  switch (prop_id)
    {
    case PROP_ORIGIN_X:
      self->origin_x = g_value_get_double (value);
      break;
    case PROP_ORIGIN_Y:
      self->origin_y = g_value_get_double (value);
      break;
    case PROP_FILTER:
      g_free (self->filter);
      self->filter = g_strdup (g_value_get_string (value));
      break;
    case PROP_HARD_EDGES:
      self->hard_edges = g_value_get_boolean (value);
      break;
    case PROP_LANCZOS_WIDTH:
      self->lanczos_width = g_value_get_int (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gegl_affine_bounding_box (gdouble       *points,
                          gint           num_points,
                          GeglRectangle *output)
{
  gint    i;
  gdouble min_x,
          min_y,
          max_x,
          max_y;

  if (num_points < 1)
    return;
  num_points = num_points << 1;

  min_x = max_x = points [0];
  min_y = max_y = points [1];

  for (i = 2; i < num_points;)
    {
      if (points [i] < min_x)
        min_x = points [i];
      else if (points [i] > max_x)
        max_x = points [i];
      i++;

      if (points [i] < min_y)
        min_y = points [i];
      else if (points [i] > max_y)
        max_y = points [i];
      i++;
    }

  output->x = floor (min_x);
  output->y = floor (min_y);
  output->width  = (gint) ceil (max_x) - output->x;
  output->height = (gint) ceil (max_y) - output->y;
}

static gboolean
gegl_affine_is_intermediate_node (OpAffine *affine)
{
  GSList        *connections;
  GeglOperation *op = GEGL_OPERATION (affine);

  connections = gegl_pad_get_connections (gegl_node_get_pad (op->node,
                                                             "output"));
  if (! connections)
    return FALSE;

  do
    {
      GeglOperation *sink;

      sink = gegl_connection_get_sink_node (connections->data)->operation;

      if (! IS_OP_AFFINE (sink) ||
          strcmp (affine->filter, OP_AFFINE (sink)->filter))
        return FALSE;
    }
  while ((connections = g_slist_next (connections)));

  return TRUE;
}

static gboolean
gegl_affine_is_composite_node (OpAffine *affine)
{
  GSList        *connections;
  GeglOperation *op = GEGL_OPERATION (affine);
  GeglOperation *source;

  connections = gegl_pad_get_connections (gegl_node_get_pad (op->node,
                                                             "input"));
  if (! connections)
    return FALSE;

  source = gegl_connection_get_source_node (connections->data)->operation;

  return (IS_OP_AFFINE (source) &&
          ! strcmp (affine->filter, OP_AFFINE (source)->filter));
}

static void
gegl_affine_get_source_matrix (OpAffine    *affine,
                               GeglMatrix3  output)
{
  GSList        *connections;
  GeglOperation *op = GEGL_OPERATION (affine);
  GeglOperation *source;

  connections = gegl_pad_get_connections (gegl_node_get_pad (op->node,
                                                             "input"));
  g_assert (connections);

  source = gegl_connection_get_source_node (connections->data)->operation;
  g_assert (IS_OP_AFFINE (source));

  gegl_affine_create_composite_matrix (OP_AFFINE (source), output);
  /*gegl_matrix3_copy (output, OP_AFFINE (source)->matrix);*/
}

static GeglRectangle
gegl_affine_get_bounding_box (GeglOperation *op)
{
  OpAffine      *affine  = OP_AFFINE (op);
  GeglMatrix3    matrix;
  GeglRectangle  in_rect = {0,0,0,0},
                 have_rect;
  gdouble        have_points [8];
  gint           i;

  GeglRectangle  context_rect;
  GeglSampler   *sampler;

  sampler = op_affine_sampler (OP_AFFINE (op));
  context_rect = sampler->context_rect;
  g_object_unref (sampler);

  if (gegl_operation_source_get_bounding_box (op, "input"))
    in_rect = *gegl_operation_source_get_bounding_box (op, "input");

  gegl_affine_create_composite_matrix (affine, matrix);

  if (gegl_affine_is_intermediate_node (affine) ||
      gegl_matrix3_is_identity (matrix))
    {
      return in_rect;
    }

  in_rect.x      += context_rect.x;
  in_rect.y      += context_rect.y;
  in_rect.width  += context_rect.width;
  in_rect.height += context_rect.height;

  have_points [0] = in_rect.x;
  have_points [1] = in_rect.y;

  have_points [2] = in_rect.x + in_rect.width ;
  have_points [3] = in_rect.y;

  have_points [4] = in_rect.x + in_rect.width ;
  have_points [5] = in_rect.y + in_rect.height ;

  have_points [6] = in_rect.x;
  have_points [7] = in_rect.y + in_rect.height ;

  for (i = 0; i < 8; i += 2)
    gegl_matrix3_transform_point (matrix,
                             have_points + i, have_points + i + 1);

  gegl_affine_bounding_box (have_points, 4, &have_rect);
  return have_rect;
}

static GeglNode *
gegl_affine_detect (GeglOperation *operation,
                    gint           x,
                    gint           y)
{
  OpAffine    *affine      = OP_AFFINE (operation);
  GeglNode    *source_node = gegl_operation_get_source_node (operation, "input");
  GeglMatrix3  inverse;
  gdouble      need_points [2];
  gint         i;

  if (gegl_affine_is_intermediate_node (affine) ||
      gegl_matrix3_is_identity (inverse))
    {
      return gegl_operation_detect (source_node->operation, x, y);
    }

  need_points [0] = x;
  need_points [1] = y;

  gegl_affine_create_matrix (affine, inverse);
  gegl_matrix3_invert (inverse);

  for (i = 0; i < 2; i += 2)
    gegl_matrix3_transform_point (inverse,
                             need_points + i, need_points + i + 1);

  return gegl_operation_detect (source_node->operation,
                                need_points[0], need_points[1]);
}

static GeglRectangle
gegl_affine_get_required_for_output (GeglOperation       *op,
                                     const gchar         *input_pad,
                                     const GeglRectangle *region)
{
  OpAffine      *affine = OP_AFFINE (op);
  GeglMatrix3    inverse;
  GeglRectangle  requested_rect,
                 need_rect;
  GeglRectangle  context_rect;
  GeglSampler   *sampler;
  gdouble        need_points [8];
  gint           i;

  requested_rect = *region;
  sampler = op_affine_sampler (OP_AFFINE (op));
  context_rect = sampler->context_rect;
  g_object_unref (sampler);

  gegl_affine_create_composite_matrix (affine, inverse);
  gegl_matrix3_invert (inverse);

  if (gegl_affine_is_intermediate_node (affine) ||
      gegl_matrix3_is_identity (inverse))
    {
      return requested_rect;
    }

  need_points [0] = requested_rect.x;
  need_points [1] = requested_rect.y;

  need_points [2] = requested_rect.x + requested_rect.width ;
  need_points [3] = requested_rect.y;

  need_points [4] = requested_rect.x + requested_rect.width ;
  need_points [5] = requested_rect.y + requested_rect.height ;

  need_points [6] = requested_rect.x;
  need_points [7] = requested_rect.y + requested_rect.height ;

  for (i = 0; i < 8; i += 2)
    gegl_matrix3_transform_point (inverse,
                             need_points + i, need_points + i + 1);
  gegl_affine_bounding_box (need_points, 4, &need_rect);

  need_rect.x      += context_rect.x;
  need_rect.y      += context_rect.y;
  need_rect.width  += context_rect.width;
  need_rect.height += context_rect.height;
  return need_rect;
}

static GeglRectangle
gegl_affine_get_invalidated_by_change (GeglOperation       *op,
                                       const gchar         *input_pad,
                                       const GeglRectangle *input_region)
{
  OpAffine          *affine = OP_AFFINE (op);
  GeglMatrix3        matrix;
  GeglRectangle      affected_rect;
  GeglRectangle      context_rect;
  GeglSampler       *sampler;
  gdouble            affected_points [8];
  gint               i;
  GeglRectangle      region = *input_region;

  sampler = op_affine_sampler (OP_AFFINE (op));
  context_rect = sampler->context_rect;
  g_object_unref (sampler);

  gegl_affine_create_matrix (affine, matrix);

  if (affine->origin_x || affine->origin_y)
    gegl_matrix3_originate (matrix, affine->origin_x, affine->origin_y);

  if (gegl_affine_is_composite_node (affine))
    {
      GeglMatrix3 source;

      gegl_affine_get_source_matrix (affine, source);
      gegl_matrix3_multiply (source, matrix, matrix);
    }

  if (gegl_affine_is_intermediate_node (affine) ||
      gegl_matrix3_is_identity (matrix))
    {
      return region;
    }

  region.x      += context_rect.x;
  region.y      += context_rect.y;
  region.width  += context_rect.width;
  region.height += context_rect.height;

  affected_points [0] = region.x;
  affected_points [1] = region.y;

  affected_points [2] = region.x + region.width ;
  affected_points [3] = region.y;

  affected_points [4] = region.x + region.width ;
  affected_points [5] = region.y + region.height ;

  affected_points [6] = region.x;
  affected_points [7] = region.y + region.height ;

  for (i = 0; i < 8; i += 2)
    gegl_matrix3_transform_point (matrix,
                             affected_points + i, affected_points + i + 1);

  gegl_affine_bounding_box (affected_points, 4, &affected_rect);
  return affected_rect;
}

void  gegl_sampler_prepare     (GeglSampler *self);
  /*XXX: Eeeek, obsessive avoidance of public headers, the API needed to
   *     satisfy this use case should probably be provided.
   */

static gboolean
gegl_affine_process (GeglOperation        *operation,
                     GeglOperationContext *context,
                     const gchar          *output_prop,
                     const GeglRectangle  *result)
{
  return TRUE;
}

