#!/bin/bash

pushd /root/gcc-build-test || exit 1
rm -rf ./build
mkdir build
cd build || exit 1
../gcc-11.2.0/configure --enable-languages=c,c++ --disable-multilib
make -j4 || exit 1
