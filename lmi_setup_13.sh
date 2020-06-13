#!/bin/sh

# Create a chroot for cross-building "Let me illustrate...".
#
# Copyright (C) 2016, 2017, 2018, 2019, 2020 Gregory W. Chicares.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation,
# Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
#
# http://savannah.nongnu.org/projects/lmi
# email: <gchicares@sbcglobal.net>
# snail: Chicares, 186 Belle Woods Drive, Glastonbury CT 06033, USA

. ./lmi_setup_inc.sh
. /tmp/schroot_env

set -evx

assert_su
assert_not_chrooted

# For caveats, see:
#    https://lists.nongnu.org/archive/html/lmi/2020-05/msg00040.html
mkdir -p /srv/cache_for_lmi
du   -sb /srv/chroot/"${CHRTNAME}"/srv/cache_for_lmi || echo "Okay."
mkdir -p /srv/chroot/"${CHRTNAME}"/srv/cache_for_lmi
mount --bind /srv/cache_for_lmi /srv/chroot/"${CHRTNAME}"/srv/cache_for_lmi

stamp=$(date -u +'%Y%m%dT%H%M%SZ')
echo "$stamp $0: Mounted cache_for_lmi."  | tee /dev/tty