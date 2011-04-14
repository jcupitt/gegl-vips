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
 * Copyright 2003 Calvin Williamson
 *           2006 Øyvind Kolås
 */

#ifndef __GEGL_TYPES_INTERNAL_H__
#define __GEGL_TYPES_INTERNAL_H__

G_BEGIN_DECLS

typedef struct _GeglCRVisitor        GeglCRVisitor;
typedef struct _GeglDebugRectVisitor GeglDebugRectVisitor;
typedef struct _GeglEvalMgr          GeglEvalMgr;
typedef struct _GeglEvalVisitor      GeglEvalVisitor;
typedef struct _GeglFinishVisitor    GeglFinishVisitor;
typedef struct _GeglGraph            GeglGraph;
typedef struct _GeglHaveVisitor      GeglHaveVisitor;
typedef struct _GeglNeedVisitor      GeglNeedVisitor;
typedef struct _GeglDotVisitor       GeglDotVisitor;
#ifndef __GEGL_PLUGIN_H__
typedef struct _GeglPad              GeglPad;
typedef struct _GeglOperation        GeglOperation;
typedef struct _GeglOperationContext GeglOperationContext;
typedef struct _GeglConnection       GeglConnection;
#endif
typedef struct _GeglPrepareVisitor   GeglPrepareVisitor;
typedef struct _GeglLockVisitor      GeglLockVisitor;
typedef struct _GeglUnlockVisitor    GeglUnlockVisitor;
typedef struct _GeglVisitable        GeglVisitable; /* dummy typedef */
typedef struct _GeglVisitor          GeglVisitor;

typedef struct _GeglPoint            GeglPoint;
typedef struct _GeglDimension        GeglDimension;

struct _GeglPoint
{
  gint x;
  gint y;
};

struct _GeglDimension
{
  gint width;
  gint height;
};


G_END_DECLS

#endif /* __GEGL_TYPES_INTERNAL_H__ */
