#!/bin/sh
# Does all the funky stuff of creating every generated file of
# the GNU Configure & Build System.
#

rm -rf autom4te.cache
gettextize --intl -f -c
autoreconf -f -i -W portability

