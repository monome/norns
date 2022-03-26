#!/usr/bin/env bash
cd "$(dirname "$0")"

# only update new disk image
version=$(cat ~/version.txt)
if [ $version -lt 220306 ];
then
	echo "needs new disk image, aborting."
	exit;
fi;

# basic repo updates
sudo rm -rf /home/we/norns
cp -a norns /home/we/
sudo rm -rf /home/we/maiden
cp -a maiden /home/we/

# version/changelog
cp version.txt /home/we/
cp changelog.txt /home/we/

# rewrite journalctl
sudo cp config/journald.conf /etc/systemd/
sudo mkdir -p /var/log/journal

# scrub invisibles
find ~/dust -name .DS_Store -delete
find ~/dust -name ._.DS_Store -delete

# maiden project setup
cd /home/we/maiden
./project-setup.sh

# cleanup
rm -rf ~/update/*
