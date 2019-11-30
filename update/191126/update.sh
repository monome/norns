#!/usr/bin/env bash
cd "$(dirname "$0")"

# basic repo updates
sudo rm -rf /home/we/norns
cp -a norns /home/we/
sudo rm -rf /home/we/maiden
cp -a maiden /home/we/
#sudo rm -rf /home/we/norns-image
#cp -a norns-image /home/we/

# version/changelog
cp version.txt /home/we/
cp changelog.txt /home/we/

# packages
#sudo dpkg -i package/*.deb

# kernel
#sudo tar xzvf kernel/kernel-4.14.52-14-g00c5424.tar.gz -C /

# norns-image
#cd /home/we/norns-image
#./setup.sh

# sc?
#cd /home/we/norns/sc
#rm -rf /home/we/.local/share/SuperCollider/Extensions/*
#./install.sh

# webdav
cp -a webdav /home/we/
sudo cp webdav/webdav.service /etc/systemd/system/
sudo systemctl enable webdav.service
# maiden project setup

cd /home/we/maiden
./project-setup.sh

