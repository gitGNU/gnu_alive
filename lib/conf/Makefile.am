# Makefile.am - Run automake to construct the Makefile.in
# that file is then used as a template for configure to
# construct the regular Makefile ...
#
# The existance of this file is simply because I would like
# it that the GNU autotools would allow me to logically
# contain separate units like this. This is however not so
# easily done. 
#
# I would like for the libconf.a library to be placed in ../
# when built and this file ONLY to know of how to build
# libconf.a -- the ../Makefile.am should only do the recursive
# descent, nothing more...
#
# Until I figure out how to actually do this without resorting
# to plain old make the ../Makefile.am is responsible for
# building ../libconf.a -- it's ugly but it works.
#
#AM_YFLAGS         = -d

#noinst_LIBRARIES  = ../libconf.a

#libconf_a_SOURCES = conf.c parser.y tokens.l

