#!/bin/sh

### BEGIN INIT INFO
# Provides:          AFT
# Required-Start:    $syslog
# Required-Stop:     $syslog
# Default-Start:     2 3 4 5
# Default-Stop:      0 1 6
### END INIT INFO

DAEMON="/opt/jhbuild/build/evd/sbin/AFT"
CONFIG_FILE="/opt/jhbuild/build/evd/etc/AFT/AFT-tls-devel.conf"
PID_FILE="/var/run/AFT.pid"

case "$1" in
  start)
    echo "Starting AFT server daemon"
    $DAEMON -c $CONFIG_FILE -D
    ;;
  stop)
    echo "Stopping AFT server daemon"
    kill `cat $PID_FILE`
    ;;
  *)
    echo "Usage: /etc/init.d/AFT {start|stop}"
    exit 1
    ;;
esac

exit 0
