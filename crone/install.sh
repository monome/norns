#!/bin/bash

mkdir -p ~/.local/share/SuperCollider/Extensions/norns
ln -s ~/norns/sc ~/.local/share/SuperCollider/Extensions/norns/sc
cp -Rf classes/* ~/.local/share/SuperCollider/Extensions/norns/
