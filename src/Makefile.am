sbin_PROGRAMS = qadsl

qadsl_SOURCES = config.c  ../include/config.h  \
                daemon.c  ../include/daemon.h  \
                http.c    ../include/http.h    \
                lock.c    ../include/lock.h    \
                log.c     ../include/log.h     \
                process.c ../include/process.h \
                main.c    ../include/settings.h.in

qadsl_LDADD   = $(top_builddir)/lib/libconf.a
