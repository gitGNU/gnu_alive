## Process this file with automake to produce Makefile.in

# Dive in and do what these dirs tell you.
SUBDIRS    = intl m4 po lib src man etc

# Stuff that would otherwise not be distributed.
# .lsm:  Linux Software Map - http://lsm.execpc.com/lsm/
# .spec: RedHat Package Manager template
# debian/ Debian maintainer scripts for building .deb, .dsc etc.
EXTRA_DIST = config.rpath  alive.lsm.in alive.spec.in \
             CREDITS \
             HACKING \
             RELATED \
             debian/README.Debian \
             debian/changelog     \
             debian/control       \
             debian/copyright     \
             debian/dirs          \
             debian/docs          \
             debian/init.d        \
             debian/postinst      \
             debian/rules         \
	     debian/config        \
	     debian/config.dia    \
	     debian/config.eps    \
	     debian/config.tex    \
	     debian/templates     

ACLOCAL_AMFLAGS = -I m4
