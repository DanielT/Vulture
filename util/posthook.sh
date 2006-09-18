#!/bin/sh
lockfile=/tmp/vultures_posthook
timeout=0
while [ -f "$lockfile" ]; do
    lockpid=`cat $lockfile`
    echo "$$ waiting for $lockpid ... $timeout"
    sleep 15
    timeout=$(($timeout+1))
    if [ $timeout -eq "100" ]
    then
        echo " * timeout"
        exit 1
    fi
done
echo $$ > $lockfile

make -C doc posthook
gmake snapshot
find ./snapshot -type f -iregex ".*/vultures-snapshot-.*-full\.tar.\gz.*" -ctime +2d -delete

rm -f $lockfile
