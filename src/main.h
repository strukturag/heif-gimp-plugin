/*
 * GIMP HEIF loader / write plugin.
 * Copyright (c) 2018 struktur AG, Dirk Farin <farin@struktur.de>
 *
 * This file is part of gimp-libheif-plugin.
 *
 * gimp-libheif-plugin is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * gimp-libheif-plugin is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *                                                                                                                                                                                                                  * You should have received a copy of the GNU General Public License
 * along with gimp-libheif-plugin.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __MAIN_H__
#define __MAIN_H__


typedef struct
{
  gint      dummy1;
  gint      dummy2;
  gint      dummy3;
  guint     seed;
  gboolean  random_seed;
} PlugInVals;

typedef struct
{
  gint32    image_id;
} PlugInImageVals;

typedef struct
{
  gint32    drawable_id;
} PlugInDrawableVals;

typedef struct
{
  gboolean  chain_active;
} PlugInUIVals;


typedef struct
{
  int selected_image;
} UIResult;


/*  Default values  */

extern const PlugInVals         default_vals;
extern const PlugInImageVals    default_image_vals;
extern const PlugInDrawableVals default_drawable_vals;
extern const PlugInUIVals       default_ui_vals;


#endif /* __MAIN_H__ */
