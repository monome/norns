#!/bin/bash

sc_ext_dir=~/.local/share/SuperCollider/Extensions
norns_ext_dir=$sc_ext_dir/norns
rm -rf $norns_ext_dir
mkdir -p $norns_ext_dir


link() {
    base=`basename $1`
    src=`pwd`/$1
    dst=$norns_ext_dir/$base
    ln -sf $src $dst
}

for file in $(find . -name '*.sc'); do
    link $file
done

# FIXME: some faust-generated ugens are just precompiled in the repo here :/
for file in $(find . -name '*.so'); do
    link $file
done

for file in $(find ../build/sc -name '*.so'); do
    link $file
done


# user extensions supplied by "dust"
dust_ext_dir=$sc_ext_dir/dust
rm -rf $dust_ext_dir
mkdir -p $dust_ext_dir
ln -sf ~/dust/lib/sc $dust_ext_dir
