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

BUILD_ROOT=$TRAVIS_BUILD_DIR

if [ ! -z "$CHECK_LICENSES" ]; then
    echo "Checking licenses ..."
    ./scripts/check-licenses.sh
fi

if [ ! -z "$BUILD" ]; then
    echo "Building libheif ..."
    make
fi

if [ ! -z "$TARBALL" ]; then
    VERSION=$(grep "^VERSION " Makefile | sed -r 's/^VERSION *= *([0-9]+\.[0-9]+\.[0-9]+).*/\1/g')
    echo "Creating tarball for version $VERSION ..."
    make dist

    export PKG_CONFIG_PATH=$BUILD_ROOT/libheif/dist/lib/pkgconfig/

    echo "Building from tarball ..."
    tar xf gimp-heif-plugin-$VERSION.tar*
    pushd gimp-heif-plugin-$VERSION
    ./configure
    make
    popd
fi
