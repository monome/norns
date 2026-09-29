#!/usr/bin/env bash
# @name pack.sh
# @brief create a norns update package
# @description
#   usage:
#     pack.sh <release name> <maiden tag>      create an update package with the 
#                                              given release name (e.g. 260909) 
#                                              with the contents of the current
#                                              working tree, plus the specified
#                                              maiden release tag from GitHub
#                                              (e.g. "v1.1.5")

readonly RELEASE_NAME=$1
readonly MAIDEN_TAG=$2

# use advanced glob syntax, and include dotfiles and folders (for .git)
shopt -s extglob dotglob

# create the release folder structure
mkdir -p ${RELEASE_NAME}/norns
mkdir -p ${RELEASE_NAME}/config
mkdir -p ${RELEASE_NAME}/lib
mkdir -p ${RELEASE_NAME}/package

# copy update files into release root
cp update/update.sh ${RELEASE_NAME}
cp update/changelog.txt ${RELEASE_NAME}
cp update/version.txt ${RELEASE_NAME}

# copy entire norns tree into norns subfolder
cp -r !(${RELEASE_NAME}) ${RELEASE_NAME}/norns

# copy config files into release root
cp -r image/config/* ${RELEASE_NAME}/config

# download and copy maiden release with the given tag
curl -LO https://github.com/monome/maiden/releases/download/${MAIDEN_TAG}/maiden-${MAIDEN_TAG}.tgz
tar xzvf maiden-${MAIDEN_TAG}.tgz
mv maiden ${RELEASE_NAME}
rm maiden-${MAIDEN_TAG}.tgz

tar czvf norns${RELEASE_NAME}.tgz ${RELEASE_NAME}/
sha256sum norns${RELEASE_NAME}.tgz > norns${RELEASE_NAME}.sha256