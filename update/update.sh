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
sudo rm -rf /home/we/bin/maiden-repl
sudo cp -a /home/we/norns/build/maiden-repl/maiden-repl /home/we/bin/

# version/changelog
cp version.txt /home/we/
cp changelog.txt /home/we/

# remove logging
sudo apt -y remove rsyslog
sudo cp config/logrotate.conf /etc/
sudo cp config/journald.conf /etc/systemd/
sudo rm -rf /var/log/journal
sudo rm -rf /var/log/daemon.log
sudo rm -rf /var/log/user.log

# disable hciuart
sudo systemctl disable hciuart 

# update jack systemd
sudo cp --remove-destination config/norns-jack.service /etc/systemd/system/norns-jack.service

# scrub invisibles
find ~/dust -name .DS_Store -delete
find ~/dust -name ._.DS_Store -delete

# get common audio if not present
if [ ! -d /home/we/dust/audio/common ]; then
	echo "does not exist, downloading"
	cd /home/we/dust/audio
	wget https://github.com/monome/norns/releases/download/v2.7.1/dust-audio-common.tgz
	tar xzvf dust-audio-common.tgz
	rm dust-audio-common.tgz
fi

# set alsa volume
amixer --device hw:sndrpimonome set Master 100% on
sudo alsactl store

# change boot/cmdline for screen
sudo sed -e '/dtoverlay=ssd1322-spi/ s/^#*/#/' -i /boot/config.txt
sudo sed -e '/spidev.bufsiz/! s/$/ spidev.bufsiz=8192/' -i /boot/cmdline.txt

# install packages
sudo dpkg -i package/*.deb

# clean slate
rm /home/we/matronrc.lua

# maiden project setup
cd /home/we/maiden
./project-setup.sh


# cleanup
rm -rf ~/update/*
