dnl configure.ac
dnl
dnl Copyright (C) 2012 Thien-Thi Nguyen
dnl
dnl This file is part of GNU Alive.
dnl
dnl GNU Alive is free software; you can redistribute it and/or modify
dnl it under the terms of the GNU General Public License as published by
dnl the Free Software Foundation; either version 3, or (at your option)
dnl any later version.
dnl
dnl GNU Alive is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl GNU General Public License for more details.
dnl
dnl You should have received a copy of the GNU General Public License
dnl along with GNU Alive.  If not, see <http://www.gnu.org/licenses/>.

AC_INIT([GNU Alive],[2.0.1],[bug-alive@gnu.org])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([build-aux])
AM_INIT_AUTOMAKE([1.12.2 gnu no-dist-gzip dist-lzip dist-xz])

SNUGGLE_PROGS

AC_SUBST([EXTRAMODS],[])dnl for now

AC_PATH_PROG([PING],[ping])
AS_IF([test -z "$PING"],
[AC_MSG_ERROR([missing required program: ping(1)])])

AC_CACHE_CHECK([if the ping program accepts ‘-n -c 1’],[alive_cv_nice_ping],
[AS_IF(["$PING" -n -c 1 localhost 1>/dev/null 2>&1],
 [alive_cv_nice_ping=yes],
 [alive_cv_nice_ping=no])])

dnl Luckily both inetutils-ping and iputils-ping are nice.
AS_IF([test xno = x$alive_cv_nice_ping],
[AC_MSG_ERROR([$PING does not support ‘-n -c 1’.
Please install a ping program that does, and retry.])])

AC_CONFIG_FILES([
  Makefile
  src/Makefile
  doc/Makefile
])

AC_CONFIG_FILES([src/alive:src/body.scm],[chmod +x src/alive])

AC_OUTPUT

dnl configure.ac ends here
