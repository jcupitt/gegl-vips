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
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright 2006, 2007 Øyvind Kolås <pippin@gimp.org>
 */

#include "config.h"

#include <glib.h>

#include "glib-object.h"
#include "gegl-buffer-types.h"
#include "gegl-tile-handler-chain.h"
#include "gegl-tile-handler-cache.h"


G_DEFINE_TYPE (GeglTileHandlerChain, gegl_tile_handler_chain, GEGL_TYPE_TILE_HANDLER)

static void
gegl_tile_handler_chain_nuke_cache (GeglTileHandlerChain *tile_handler_chain)
{
  GSList *iter;

  while (gegl_tile_handler_chain_get_first (tile_handler_chain, GEGL_TYPE_TILE_HANDLER_CACHE))
    {
      iter = tile_handler_chain->chain;
      while (iter)
        {
          if (GEGL_IS_TILE_HANDLER_CACHE (iter->data))
            {
              g_object_unref (iter->data);
              tile_handler_chain->chain = g_slist_remove (tile_handler_chain->chain, iter->data);
              break;
            }
          iter = iter->next;
        }
    }
}

static void
gegl_tile_handler_chain_dispose (GObject *object)
{
  GeglTileHandlerChain *tile_handler_chain = GEGL_TILE_HANDLER_CHAIN (object);
  GSList       *iter;
  GSList       *chain;

  /* Get rid of the cache before any further parts of the deconstruction of the
   * TileStore chain, unwritten tiles need a living TileStore for their
   * deconstruction.
   */
  gegl_tile_handler_chain_nuke_cache (tile_handler_chain);

  /* While we destroy tile_handler_chain->chain, don't have
   * tile_handler_chain->chain set, because it might otherwise be used
   * by other objects in the dispose phase
   */
  chain = tile_handler_chain->chain;
  tile_handler_chain->chain = NULL;
  
  iter = chain;
  while (iter)
    {
      if (iter->data)
        {
          if (G_IS_OBJECT (iter->data))
            g_object_unref (iter->data);
          else
            g_free (iter->data);
        }
      iter = iter->next;
    }

  if (chain)
    g_slist_free (chain);
  chain = NULL;

  G_OBJECT_CLASS (gegl_tile_handler_chain_parent_class)->dispose (object);
}


static void
gegl_tile_handler_chain_finalize (GObject *object)
{
  G_OBJECT_CLASS (gegl_tile_handler_chain_parent_class)->finalize (object);
}

static gpointer
gegl_tile_handler_chain_command (GeglTileSource *tile_store,
                                 GeglTileCommand command,
                                 gint            x,
                                 gint            y,
                                 gint            z,
                                 gpointer        data)
{
  GeglTileHandlerChain *tile_handler_chain = (GeglTileHandlerChain *) tile_store;
  GeglTileSource *source = ((GeglTileHandler *) tile_store)->source;

  if (tile_handler_chain->chain != NULL)
    return gegl_tile_source_command ((GeglTileSource *)(tile_handler_chain->chain->data),
                                  command, x, y, z, data);
  else if (source)
    return gegl_tile_source_command (source, command, x, y, z, data);
  else
    g_assert (0);

  return FALSE;
}

static void
gegl_tile_handler_chain_class_init (GeglTileHandlerChainClass *class)
{
  GObjectClass      *gobject_class;
  gobject_class    = (GObjectClass *) class;

  gobject_class->finalize = gegl_tile_handler_chain_finalize;
  gobject_class->dispose  = gegl_tile_handler_chain_dispose;
}

static void
gegl_tile_handler_chain_init (GeglTileHandlerChain *self)
{
  ((GeglTileSource*)self)->command = gegl_tile_handler_chain_command;
  self->chain = NULL;
}

void
gegl_tile_handler_chain_bind (GeglTileHandlerChain *tile_handler_chain)
{
  GSList *iter;

  iter = tile_handler_chain->chain;
  while (iter)
    {
      GeglTileHandler  *handler;
      GeglTileSource *source = NULL;

      handler = iter->data;
      if (iter->next)
        {
          source = iter->next->data;
        }
      else
        {
          source = gegl_tile_handler_get_source (tile_handler_chain);
        }
      gegl_tile_handler_set_source (handler, source);
      iter = iter->next;
    }
}

GeglTileHandler *
gegl_tile_handler_chain_add (GeglTileHandlerChain *tile_handler_chain,
                             GeglTileHandler      *handler)
{
  tile_handler_chain->chain = g_slist_prepend (tile_handler_chain->chain, handler);
  /*gegl_tile_handler_chain_bind (tile_handler_chain);*/
  return handler;
}

/*
 * return the first handler of a given type
 */
GeglTileHandler *
gegl_tile_handler_chain_get_first (GeglTileHandlerChain *tile_handler_chain,
                                   GType                 type)
{
  GSList *iter;

  iter = tile_handler_chain->chain;
  while (iter)
    {
      if ((G_TYPE_CHECK_INSTANCE_TYPE ((iter->data), type)))
        {
          return iter->data;
        }
      iter = iter->next;
    }
  return NULL;
}
