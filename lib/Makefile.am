# yacc -d, defines - also produce a header file
AM_YFLAGS         = -d

noinst_LIBRARIES  = libconf.a libost.a

libconf_a_SOURCES = conf/conf.c     \
                    conf/parser.y   \
                    conf/tokens.l   \
                    ../include/conf.h

# Generated files we want to be cleaned
CLEANFILES        = parser.c \
                    parser.h \
                    tokens.c


libost_a_SOURCES = strcasecmp.c \
	           getopt.c     \
		   getopt1.c    \
                   ../include/getopt.h
