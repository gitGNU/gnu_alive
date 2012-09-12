#!/bin/sh
#
# Copyright (C) 2012 Thien-Thi Nguyen
#
# This file is part of GNU Alive.
#
# GNU Alive is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3, or (at your option)
# any later version.
#
# GNU Alive is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with GNU Alive.  If not, see <http://www.gnu.org/licenses/>.

##
# Usage: sh autogen.sh
##

for p in guile snuggle gnulib-tool autoconf automake ; do
    $p --version | sed 's/^/using: /;1q'
done

snuggle m4 build-aux/

autoreconf --verbose --install --symlink --force -Wall

# These override what ‘autoreconf --install’ creates.
# Another way is to use gnulib's config/srclist-update.
actually ()
{
    gnulib-tool --copy-file $1 $2
}
actually build-aux/texinfo.tex
actually doc/INSTALL.UTF-8 INSTALL
actually doc/fdl.texi

# We aren't really interested in the backup files.
rm -f INSTALL~ build-aux/*~

# autogen.sh ends here
