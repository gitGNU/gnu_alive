sbin_PROGRAMS = alive

alive_SOURCES = config.c  ../include/config.h  \
                daemon.c  ../include/daemon.h  \
                http.c    ../include/http.h    \
                lock.c    ../include/lock.h    \
                log.c     ../include/log.h     \
                process.c ../include/process.h \
                main.c    ../include/settings.h.in

alive_LDFLAGS = -L$(top_builddir)/lib
alive_LDADD   = -lconf -lost @LIBINTL@ @ALLOCA@
