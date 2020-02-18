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

# norns-image
#cd /home/we/norns-image
#./setup.sh

# sc?
#cd /home/we/norns/sc
#rm -rf /home/we/.local/share/SuperCollider/Extensions/*
#./install.sh

# webdav
#cp -a webdav /home/we/
#sudo cp webdav/webdav.service /etc/systemd/system/
#sudo systemctl enable webdav.service
if [ -d /home/we/webdav ]; then
  sudo rm -rf /home/we/webdav
  sudo systemctl disable webdav.service
  sudo rm /etc/systemd/system/webdav.service
fi

# samba
SAMBA=$(dpkg -s samba | grep "install ok")
if [ "$SAMBA" == "" ]; then
  sudo dpkg -i packages/*.deb
  (echo "sleep"; echo "sleep") | sudo smbpasswd -s -a we
fi
sudo cp config/smb.conf /etc/samba/
sudo /etc/init.d/samba restart

# scrub invisibles
find ~/dust -name .DS_Store -delete
find ~/dust -name ._.DS_Store -delete

# kernel
sudo rm /boot/kernel-*
sudo find /lib/modules/* -path "/lib/modules/$(uname -r)" -o -type d -exec rm -rf {} +
sudo cp -r kernel/boot /
sudo cp -r kernel/lib /
HW=$(sudo cat /sys/firmware/devicetree/base/model)
if [[ "$HW" == *"Compute"* ]]; then
  echo "CM3"
else
  echo "SHIELD"
  sudo cp -r kernel/shield/* /boot/
fi

# logs
sudo rm -rf /var/log/*
sudo apt-get remove -y rsyslog

# maiden project setup
cd /home/we/maiden
./project-setup.sh

# cleanup
rm -rf ~/update/*
