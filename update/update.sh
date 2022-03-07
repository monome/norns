# MUST CHECK VERSION FIRST
# to ensure we're not trying to update an image pre 220306

#!/usr/bin/env bash
cd "$(dirname "$0")"

# basic repo updates
sudo rm -rf /home/we/norns
cp -a norns /home/we/
sudo rm -rf /home/we/maiden
cp -a maiden /home/we/

# version/changelog
cp version.txt /home/we/
cp changelog.txt /home/we/

# scrub invisibles
find ~/dust -name .DS_Store -delete
find ~/dust -name ._.DS_Store -delete

# logs
sudo rm -rf /var/log/*

# maiden project setup
cd /home/we/maiden
./project-setup.sh

# cleanup
rm -rf ~/update/*
