#!/bin/bash
#
# sync.sh -- basic script to push/pull data from ~pi/dust
#
# this script assumes:
#   (1) the 'usbmount' package is installed
#       apt-get install usbmount
#
#   (2) MountFlags has been tweaked in systemd-udevd.service
#       https://www.raspberrypi.org/forums/viewtopic.php?t=205016
#       (change MountFlags=slave to MountFlags=shared)
#
#   (3) the invoking user can sudo w/o providing a password
#

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

function mount_for_label {
    targets | grep $1 | cut -d' ' -f2
}

function prep_target {
    target=$1
    if [ -e $target ]; then
        sudo mkdir -pv $target/{dust,wind}
        #sudo chown pi:pi $target/{dust,wind}
    else
        echo "target $target does not exist"
        exit -1
    fi
}

function sync_to_target {
    # must be root since vfat has no sense of perms
    sudo rsync --recursive --links --verbose --update $HOME/dust/ $1/dust
    sudo sync
}

function sync_from_target {
    # must *not* be root since we want stuff owned by pi
    rsync --recursive --links --verbose --update $1/dust/ $HOME/dust
    sudo sync
}

if [ -d $1 ]; then
    echo usage:
    echo ./sync.sh targets
    echo ./sync.sh labels
    echo ./sync.sh to LABEL
    echo ./sync.sh from LABEL

elif [ $1 = "targets" ]; then
    targets;

elif [ $1 = "labels" ]; then
    targets | cut -f3 -d' '

elif [ $1 = "to" ]; then
    if [ -z "$2" ]; then
        echo "usage: ./sync.sh to LABEL"
        exit
    fi
    target=$(mount_for_label $2)
    if [ -z "$target" ]; then
        echo "error: unable to match label $2 to mount"
        exit
    fi
    echo to $target
    prep_target $target
    sync_to_target $target

elif [ $1 = "from" ]; then
    echo from
    if [ -z "$2" ]; then
        echo "usage: ./sync.sh from LABEL"
        exit
    fi
    target=$(mount_for_label $2)
    if [ -z "$target" ]; then
        echo "error: unable to match label $2 to mount"
        exit
    fi
    echo from $target
    sync_from_target $target

else
    echo invalid command: $1
    exit 1
fi
