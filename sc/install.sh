#!/bin/bash

sc_ext_dir=~/.local/share/SuperCollider/Extensions
norns_ext_dir=$sc_ext_dir/norns
rm -rf $norns_ext_dir
mkdir $norns_ext_dir


link() {
    base=`basename $1`
    src=`pwd`/$1
    dst=$norns_ext_dir/$base
    ln -sf $src $dst
}

for file in $(find . -name '*.sc'); do
    link $file
done

for file in $(find ../build/sc -name '*.so'); do
    link $file
done


# user extensions supplied by "dust"
dust_ext_dir=$sc_ext_dir/dust
rm -rf $dust_ext_dir
mkdir $dust_ext_dir
ln -sf ~/dust/sc $dust_ext_dir
