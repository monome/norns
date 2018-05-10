#!/bin/bash

MOUNTS_FILE=$HOME/mounts.sync
LABELS_FILE=$HOME/labels.sync

function mount_scan {
    mount | awk '/media\/usb?/{printf("%s %s\n",$1,$3)}' > $MOUNTS_FILE
}

function label_scan {
    blkid | sed -nr 's/^(.*): LABEL=\"(\w+)\".*$/\1 \2/p' > $LABELS_FILE
}

function targets {
    mount_scan;
    label_scan;
    join $MOUNTS_FILE $LABELS_FILE
}

if [ -d $1 ]; then
    echo usage:
    echo ./sync.sh targets
    echo ./sync.sh to mount_point
    echo ./sync.sh from mount_point
    echo ./sync.sh sync mount_point
elif [ $1 = "targets" ]; then
    targets;
elif [ $1 = "to" ]; then
    echo to
elif [ $1 = "from" ]; then
    echo from
elif [ $1 = "sync" ]; then
    echo sync
else
    echo invalid command: $1
    exit 1
fi
