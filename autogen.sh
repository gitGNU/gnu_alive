#!/bin/sh
# Does all the funky stuff of creating every generated file of
# the GNU Configure & Build System.
#
# Add libtoolize and friends when we move to that.
#

rm -rf autom4te.cache
gettextize --intl -f -c
autoreconf -f -i -s -W gnu

