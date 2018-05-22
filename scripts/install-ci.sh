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
    echo "Adding PPA strukturag/libde265 ..."
    sudo add-apt-repository -y ppa:strukturag/libde265
    sudo apt-get update -qq
    INSTALL_PACKAGES="$INSTALL_PACKAGES \
        libde265-dev \
        "
fi

if [ ! -z "$CHECK_LICENSES" ]; then
    # For licensecheck
    sudo apt-get update
    INSTALL_PACKAGES="$INSTALL_PACKAGES \
        devscripts \
        "
fi

if [ -z "$CHECK_LICENSES" ]; then
    INSTALL_PACKAGES="$INSTALL_PACKAGES \
        intltool \
        libgimp2.0-dev \
        "
fi

if [ ! -z "$INSTALL_PACKAGES" ]; then
    echo "Installing packages $INSTALL_PACKAGES ..."
    sudo apt-get install -qq $INSTALL_PACKAGES
fi

if [ ! -z "$REMOVE_PACKAGES" ]; then
    echo "Removing packages $REMOVE_PACKAGES ..."
    sudo apt-get remove $REMOVE_PACKAGES
fi

if [ -z "$CHECK_LICENSES" ]; then
    echo "Installing libheif ..."
    git clone --depth 1 -b master https://github.com/strukturag/libheif.git
    pushd libheif
    ./autogen.sh
    ./configure \
        --prefix=$BUILD_ROOT/libheif/dist
    make && make install
    popd
fi
