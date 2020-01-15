#!/usr/bin/env bash

currentDir=$(pwd)
scratch=/d/GBS-ROOT5-5/local/BUILD-ROOTS/scratch.armv7l.0/
gbsDaliDir=/d/GBS-ROOT5-5/local/BUILD-ROOTS/scratch.armv7l.0/home/dali2/dali

cd ${gbsDaliDir}

tar -cvzf dali-layers.tgz ${currentDir}/../dali-layers
rm -rf ${gbsDaliDir}/dali-layers
tar -xvzf dali-layers.tgz

cat << EOF | sudo chroot ${scratch}
cd /home/dali2/dali/
. setenv-local
cd dali-layers/build
mkdir build
cd build
cmake ../
make -j8 install
EOF

cd ${gbsDaliDir}/dali-env/opt
sdb push ./lib/*layer*.so /usr/lib/
sdb push ./share/dali/layers/*.json /usr/share/dali/layers/



