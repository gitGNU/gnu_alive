#!/bin/sh
# Does all the funky stuff of creating every generated file of
# the GNU Configure & Build System.
#

rm -rf autom4te.cache
gettextize --intl -f -c && sed "s/\(^INCLUDES = -I\..*$\)/\1 -I../include/
s/\(^$(OBJECTS): ..\/\)\(config\.h.*$\)/\1include\/\2/" <intl/Makefile.in >intl/Makefile.new && mv intl/Makefile.new intl/Makefile.in && echo "intl/Makefile.in modified successfully for GNU Alive."
autoreconf -f -i -W portability
