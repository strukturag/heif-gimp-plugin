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
 *
 * You should have received a copy of the GNU General Public License
 * along with gimp-libheif-plugin.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __INTERFACE_H__
#define __INTERFACE_H__


#include "libheif/heif.h"


/*  Public functions  */

gboolean   dialog (struct heif_context* heif,
                   uint32_t* selected_image);


struct save_parameters
{
  gint quality;
  gboolean lossless;
};

gboolean   save_dialog (struct save_parameters* params);

#endif /* __INTERFACE_H__ */
