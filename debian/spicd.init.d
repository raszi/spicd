#! /bin/sh
#
# Modified from skeleton by KARASZI Istvan <spicd-debian@spam.raszi.hu>.
#
#		Written by Miquel van Smoorenburg <miquels@cistron.nl>.
#		Modified for Debian GNU/Linux
#		by Ian Murdock <imurdock@gnu.ai.mit.edu>.

PATH=/usr/local/sbin:/usr/local/bin:/sbin:/bin:/usr/sbin:/usr/bin
DAEMON=/usr/sbin/spicd
NAME=spicd
DESC="Sony Vaio SPIC daemon"

test -x $DAEMON || exit 0

set -e

case "$1" in
  start)
	echo -n "Starting $DESC: $NAME"
	start-stop-daemon --start --quiet --pidfile /var/run/$NAME.pid \
		--exec $DAEMON
	echo "."
	;;
  stop)
	echo -n "Stopping $DESC: $NAME "
	start-stop-daemon --stop --quiet --pidfile /var/run/$NAME.pid \
		--exec $DAEMON
	echo "."
	;;
  restart|force-reload)
	echo -n "Restarting $DESC: $NAME"
	start-stop-daemon --stop --quiet --pidfile \
		/var/run/$NAME.pid --exec $DAEMON
	sleep 1
	start-stop-daemon --start --quiet --pidfile \
		/var/run/$NAME.pid --exec $DAEMON
	echo "."
	;;
  *)
	N=/etc/init.d/$NAME
	echo "Usage: $N {start|stop|restart|force-reload}" >&2
	exit 1
	;;
esac

exit 0
