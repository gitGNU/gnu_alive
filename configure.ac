dnl Process this file with autoconf to produce a configure script.

dnl Every other copy of the package version number gets its value from here
AC_INIT(GNU Alive, 1.4.0,
        [http://savannah.gnu.org/bugs/?group=alive])

dnl create a config.h file (Automake will add -DHAVE_CONFIG_H)
AM_CONFIG_HEADER(include/settings.h)
AC_CONFIG_SRCDIR(configure.ac)

AM_INIT_AUTOMAKE(alive, AC_PACKAGE_VERSION)

AC_SUBST(VERSION)

ISODATE=`date +%Y-%m-%d`
AC_SUBST(ISODATE)

AC_CANONICAL_HOST

dnl Checks for programs.
AC_PROG_INSTALL
AC_PROG_CC
dnl Flex will soon be needed to build fully reentrant parsers with Bison
AM_PROG_LEX
dnl if test "$LEX" != flex; then
dnl    AC_MSG_FAILURE([Cannot find Flex, aborting.
dnl GNU Alive, or rather libconf, needs Bison to generate reentrant code.])
dnl fi
dnl Bison will soon be needed to build fully reentrant parsers.
AC_PROG_YACC
dnl if test "$YACC" != "bison -y"; then
dnl    AC_MSG_FAILURE([Cannot find Bison, aborting.
dnl GNU Alive, or rather libconf, needs Bison to generate reentrant code.])
dnl fi

dnl Add _GNU_SOURCE to enable features and activate legacy API.
CFLAGS="${CFLAGS} -D_GNU_SOURCE"

dnl Here is a check for GCC which sets all necessary flags, most notably
dnl CFLAGS and CXXFLAGS. Checks for other compilers should do the same.
if test "$GCC" = "yes" ; then
    # set GCC compiler options that should work with all versions
    CFLAGS="${CFLAGS} -Wall -fpic -Wno-long-long"
    SHAREDLIB_LDFLAGS="-shared ${SHAREDLIB_LDFLAGS}"
    DEPEND_CCFLAGS="-MM"

    # -std=c99 is only supported in later releases of GCC and causes
    # Linux headers to go into strict ANSI mode, dropping non-ANSI functions
    # so we have to fall back to -std=gnu99 if c99 doesn't work
    save_CFLAGS="${CFLAGS}"
    CFLAGS="${CFLAGS} -std=c99 -Werror-implicit-function-declaration"
    AC_MSG_CHECKING([whether GNU cc -std=c99 option works])
    AC_COMPILE_IFELSE([ AC_LANG_PROGRAM([[#include <string.h> char *a; a = strdup("foo");]])],
	AC_MSG_RESULT([yes]) ; CFLAGS="$save_CFLAGS -std=c99",
	AC_MSG_RESULT([no])
	CFLAGS="$save_CFLAGS -std=gnu99 -Werror-implicit-function-declaration"
	AC_MSG_CHECKING([whether GNU cc -std=gnu99 option works])
	AC_COMPILE_IFELSE([ AC_LANG_PROGRAM([[#include <string.h> char *a; a = strdup("foo");]]) ],
		AC_MSG_RESULT([yes]) ; CFLAGS="$save_CFLAGS -std=gnu99",
		AC_MSG_RESULT([no]) ; CFLAGS="$save_CFLAGS"))

    # Override autoconf default addition of -O2 and let the
    # --enable-optimizer-flags decide whether to set it or not
    OPTIMIZER_CFLAGS="-O2"
    OPTIMIZER_CXXFLAGS="-O2"
    CFLAGS=`echo ${CFLAGS} | sed 's/-O2//'`
    CXXFLAGS=`echo ${CXXFLAGS} | sed 's/-O2//'`
fi

dnl Checks for libraries.
AC_PROG_RANLIB

dnl Enable/disable stuff
AC_ARG_ENABLE(debug,
              AC_HELP_STRING([--enable-debug], [Enable debugging with GDB [default=no]]),[
CFLAGS="$CFLAGS -g -DYYDEBUG=1 -DYYERROR_VERBOSE"],[
CFLAGS="$CFLAGS $OPTIMIZER_CFLAGS"
LDFLAGS="-s"
LDLIBS="-lefence"])

AC_ARG_ENABLE(pedantic-compiler,
              AC_HELP_STRING([--enable-pedantic-compiler], [Enable pedantic checks in GCC [default=no]]),
              [CFLAGS="$CFLAGS -pedantic -Wformat=2 -Wunreachable-code -Winline"])


dnl Checks for standard header files:
dnl ANSI C header files, sys/types.h, sys/stat.h, stdlib.h, string.h,
dnl memory.h, strings.h, inttypes.h, stdint.h, unistd.h
AC_HEADER_STDC
dnl Check for GNU Alive specific includes
AC_CHECK_HEADERS(errno.h features.h netdb.h stdio.h syslog.h)

dnl jm_CHECK_TYPE_STRUCT_UTIMBUF
dnl AC_HEADER_MAJOR
AC_FUNC_ALLOCA
AC_STRUCT_TM
dnl AC_STRUCT_ST_BLOCKS
dnl AC_FUNC_CLOSEDIR_VOID

AH_TOP([/* settings.h - Created by configure */
#ifndef __SETTINGS_H__
#define __SETTINGS_H__])
AH_BOTTOM([#endif /* __SETTINGS_H__ */])

AH_VERBATIM([_GNU_SOURCE], [
/* Enable GNU extensions on systems that have them.  */
#ifndef _GNU_SOURCE
# define _GNU_SOURCE
#endif])

dnl If not found, replace with components from -lost
AC_REPLACE_FUNCS(rindex strcasecmp getopt getopt_long)

dnl Checks for typedefs, structures, and compiler characteristics.

dnl Checks for library functions.
dnl AC_CHECK_FUNCS()

dnl Check for various --with-??? directives

default_globalconf="/etc/${PACKAGE}.conf"
AC_ARG_WITH(global-cfg,
[
GNU Alive Features:
  --with-global-cfg=FILE  If set will change the name and location of the
                          global configuration file used by GNU Alive.
                          Defaults to /etc/alive.conf],
globalconf="$withval", globalconf="auto")
if test "x$globalconf" = "xauto" || test "x$globalconf" = "xyes" || \
	test "x$globalconf" = "xno"; then  dnl yes/no sanity checks
	globalconf="$default_globalconf"
fi
AC_DEFINE_UNQUOTED(GLOBAL_CONF, "$globalconf", [Define this to the location of the global config file])

default_userconf="~/.${PACKAGE}rc"
AC_ARG_WITH(user-cfg,
[  --with-user-cfg=FILE    If set will change the name and location of the
                          user-specific configuration file used by GNU Alive.
                          Defaults to ~/.aliverc],
userconf="$withval", userconf="auto")
if test "x$userconf" = "xauto" || test "x$userconf" = "xyes" || \
	test "x$userconf" = "xno"; then  dnl yes/no sanity checks
	userconf="$default_userconf"
fi
AC_DEFINE_UNQUOTED(USER_CONF, "$userconf", [Define this to the location of the user config file])

default_pidfile="/var/run/alive.pid"
AC_ARG_WITH(pidfile,
[  --with-pidfile=FILE     If set will change the name and location of the file
                          used by qadsl to store the PID of its daemon process.
			  Defaults to /var/run/alive.pid],
pidfile="$withval", pidfile="auto")
if test "x$pidfile" = "xauto" || test "x$pidfile" = "xyes" || \
	test "x$pidfile" = "xno"; then  dnl yes/no sanity checks
	pidfile="$default_pidfile"
fi
AC_DEFINE_UNQUOTED(PID_FILE, "$pidfile", [Define this to the location of the user config file])

default_port=80
AC_ARG_WITH(port,
[  --with-port=NUMBER      Set the HTTP port number, defaults to standard port 80.],
port="$withval", port="auto")
if test "x$port" = "xauto" || test "x$port" = "xyes" || \
	test "x$port" = "xno"; then  dnl yes/no sanity checks
	port="$default_port"
fi
AC_DEFINE_UNQUOTED(PORT, "$port", [Define this to the HTTP port number.])


dnl internationalization macros
AM_NLS
AM_GNU_GETTEXT
AM_GNU_GETTEXT_VERSION

dnl XXX - These should be removed ASAP! /Joachim
AC_DEFINE(MAXDATASIZE, 4091, [I am a hack, please send patch to remove me.])
AC_DEFINE(MAXLEN, 256, [I am a hack, please send patch to remove me.])


dnl Create all the makefiles and stuff
AC_OUTPUT([
	Makefile
	alive.lsm
	alive.spec
        intl/Makefile
        po/Makefile.in
        m4/Makefile
	lib/Makefile
	src/Makefile
	etc/Makefile
	man/Makefile
])
