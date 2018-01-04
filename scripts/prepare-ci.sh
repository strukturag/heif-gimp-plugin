#!/bin/bash
set -e
#
# GIMP HEIF loader / write plugin.
# Copyright (c) 2018 struktur AG, Joachim Bauch <bauch@struktur.de>
#
# This file is part of gimp-libheif-plugin.
#
# gimp-libheif-plugin is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# gimp-libheif-plugin is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with gimp-libheif-plugin.  If not, see <http://www.gnu.org/licenses/>.
#

INSTALL_PACKAGES=
REMOVE_PACKAGES=
BUILD_ROOT=$TRAVIS_BUILD_DIR

if [ -z "$CHECK_LICENSES" ]; then
    export PKG_CONFIG_PATH=$BUILD_ROOT/libheif/dist/lib/pkgconfig/
    ./autogen.sh
fi
