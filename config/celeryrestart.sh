#!/bin/bash
echo -n "Restarting celery worker... "
celery multi stopwait worker1 --pidfile=/var/run/%n.pid
sleep 3
celery multi restart worker1 -A analysystem --pidfile=/var/run/%n.pid --logfile=/home/analy/analysystem/logs/celery/%n%I.log --loglevel=INFO --workdir=/home/analy/analysystem