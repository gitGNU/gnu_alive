#!/bin/sh
# GNU Alive init script
#
# Written by Miquel van Smoorenburg <miquels()cistron!nl>.
# Modified for Debian GNU/Linux by Ian Murdock <imurdock()gnu!ai!mit!edu>.
# Further modified for GNU Alive by Joachim Nilsson <joachim!nilsson()member!fsf!org>.
#
# Version:	@(#)skeleton  1.8  03-Mar-1998  miquels@cistron.nl
#

PATH=/usr/local/sbin:/usr/local/bin:/sbin:/bin:/usr/sbin:/usr/bin
DAEMON=/usr/sbin/alive
# PIDFILE=/var/run/alive.pid
CONFIG=/etc/alive.conf
NAME=alive
DESC=alive
ACTIVE=no

[ "$ACTIVE" = "yes" ] || { echo "Please edit $0" ; exit; }

test -f $DAEMON || exit 0

set -e

case "$1" in
  start)
	echo -n "Starting $DESC"
	$DAEMON -l
	echo "."
	;;
  stop)
	echo -n "Stopping $DESC"
	$DAEMON -o
	echo "."
	;;
  #reload)
	#
	#	If the daemon can reload its config files on the fly
	#	for example by sending it SIGHUP, do it here.
	#
	#	If the daemon responds to changes in its config file
	#	directly anyway, make this a do-nothing entry.
	#
	# echo "Reloading $DESC configuration files."
	# start-stop-daemon --stop --signal 1 --quiet --pidfile \
	#	/var/run/$NAME.pid --exec $DAEMON
  #;;
  restart|force-reload)
	#
	#	If the "reload" option is implemented, move the "force-reload"
	#	option to the "reload" entry above. If not, "force-reload" is
	#	just the same as "restart".
	#
	echo -n "Restarting $DESC"
	$DAEMON -o
	sleep 1
	$DAEMON -l
	echo "."
	;;
  *)
	echo "Usage: /etc/init.d/$NAME {start|stop|restart|force-reload}" >&2
	exit 1
	;;
esac

exit 0
