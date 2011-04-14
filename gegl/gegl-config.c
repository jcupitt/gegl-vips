/* This file is part of GEGL.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GEGL; if not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright 2006, 2007 Øyvind Kolås <pippin@gimp.org>
 */
#include <glib.h>
#include <glib-object.h>
#include <string.h>
#include <glib/gprintf.h>
#include "config.h"
#include "gegl.h"
#include "gegl-types-internal.h"
#include "gegl-config.h"

G_DEFINE_TYPE (GeglConfig, gegl_config, G_TYPE_OBJECT)

static GObjectClass * parent_class = NULL;

enum
{
  PROP_0,
  PROP_QUALITY,
  PROP_CACHE_SIZE,
  PROP_CHUNK_SIZE,
  PROP_SWAP,
  PROP_BABL_TOLERANCE,
  PROP_TILE_WIDTH,
  PROP_TILE_HEIGHT,
  PROP_THREADS
};

static void
gegl_config_get_property (GObject    *gobject,
                          guint       property_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
  GeglConfig *config = GEGL_CONFIG (gobject);

  switch (property_id)
    {
      case PROP_CACHE_SIZE:
        g_value_set_int (value, config->cache_size);
        break;

      case PROP_CHUNK_SIZE:
        g_value_set_int (value, config->chunk_size);
        break;

      case PROP_TILE_WIDTH:
        g_value_set_int (value, config->tile_width);
        break;

      case PROP_TILE_HEIGHT:
        g_value_set_int (value, config->tile_height);
        break;

      case PROP_QUALITY:
        g_value_set_double (value, config->quality);
        break;

      case PROP_BABL_TOLERANCE:
        g_value_set_double (value, config->babl_tolerance);
        break;

      case PROP_SWAP:
        g_value_set_string (value, config->swap);
        break;

      case PROP_THREADS:
        g_value_set_int (value, config->threads);
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
        break;
    }
}

static void
gegl_config_set_property (GObject      *gobject,
                          guint         property_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
  GeglConfig *config = GEGL_CONFIG (gobject);

  switch (property_id)
    {
      case PROP_CACHE_SIZE:
        config->cache_size = g_value_get_int (value);
        break;
      case PROP_CHUNK_SIZE:
        config->chunk_size = g_value_get_int (value);
        break;
      case PROP_TILE_WIDTH:
        config->tile_width = g_value_get_int (value);
        break;
      case PROP_TILE_HEIGHT:
        config->tile_height = g_value_get_int (value);
        break;
      case PROP_QUALITY:
        config->quality = g_value_get_double (value);
        return;
      case PROP_BABL_TOLERANCE:
          {
            gchar buf[256];
            config->babl_tolerance = g_value_get_double (value);
            g_sprintf (buf, "%f", config->babl_tolerance);
            g_setenv ("BABL_TOLERANCE", buf, 0);
            /* babl picks up the babl error through the environment, babl
             * caches valid conversions though so this needs to be set
             * before any processing is done
             */
          }
        return;
      case PROP_SWAP:
        if (config->swap)
         g_free (config->swap);
        config->swap = g_value_dup_string (value);
        break;
      case PROP_THREADS:
        config->threads = g_value_get_int (value);
        return;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
        break;
    }
}

static void
gegl_config_finalize (GObject *gobject)
{
  GeglConfig *config = GEGL_CONFIG (gobject);

  if (config->swap)
    g_free (config->swap);

  G_OBJECT_CLASS (gegl_config_parent_class)->finalize (gobject);
}

static void
gegl_config_class_init (GeglConfigClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  parent_class  = g_type_class_peek_parent (klass);

  gobject_class->set_property = gegl_config_set_property;
  gobject_class->get_property = gegl_config_get_property;
  gobject_class->finalize     = gegl_config_finalize;


  g_object_class_install_property (gobject_class, PROP_TILE_WIDTH,
                                   g_param_spec_int ("tile-width", "Tile width", "default tile width for created buffers.",
                                                     0, G_MAXINT, 64,
                                                     G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_TILE_HEIGHT,
                                   g_param_spec_int ("tile-height", "Tile height", "default tile height for created buffers.",
                                                     0, G_MAXINT, 64,
                                                     G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_CACHE_SIZE,
                                   g_param_spec_int ("cache-size", "Cache size", "size of cache in bytes",
                                                     0, G_MAXINT, 256*1024*1024,
                                                     G_PARAM_READWRITE));


  g_object_class_install_property (gobject_class, PROP_CHUNK_SIZE,
                                   g_param_spec_int ("chunk-size", "Chunk size",
                                     "the number of pixels processed simultaneously by GEGL.",
                                                     1, G_MAXINT, 256*300,
                                                     G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_QUALITY,
                                   g_param_spec_double ("quality", "Quality", "quality/speed trade off 1.0 = full quality, 0.0=full speed",
                                                     0.0, 1.0, 1.0,
                                                     G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_BABL_TOLERANCE,
                                   g_param_spec_double ("babl-tolerance", "babl error", "the error tolerance babl operates with",
                                                     0.0, 0.2, 0.0001,
                                                     G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_SWAP,
                                   g_param_spec_string ("swap", "Swap", "where gegl stores it's swap files", NULL,
                                                     G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_THREADS,
                                   g_param_spec_int ("threads", "Number of concurrent evaluation threads", "default tile height for created buffers.",
                                                     0, 16, 1,
                                                     G_PARAM_READWRITE));
}

static void
gegl_config_init (GeglConfig *self)
{
  self->swap        = NULL;
  self->quality     = 1.0;
  self->cache_size  = 256 * 1024 * 1024;
  self->chunk_size  = 512 * 512;
  self->tile_width  = 128;
  self->tile_height = 64;
  self->threads = 1;
}
