#!/bin/bash

PRODUCT=thininstaller
cd ${0%/*}
BASE=$(pwd)
[ -f "$BASE"/pathmgr.sh ] || { echo "Failed to find $BASE/bin/build/pathmgr.sh" | tee log/build.log ; exit 1 ; }
source "$BASE"/pathmgr.sh
source "$BASE"/common.sh

ORIG_PATH=$PATH
ORIG_LD_LIBRARY_PATH=$LD_LIBRARY_PATH

rm -rf $BASE/output $BASE/installer

## Create redist from input
#INPUT=$BASE/input
#[ -d $INPUT ] || failure "Can't find input"

#REDIST=$BASE/redist
#rm -rf $REDIST
#mkdir -p $REDIST
#pushd $REDIST
#tar xf $INPUT/boost.tar 2>&1 | tee -a "$LOG" || failure "Unable to unpack boost"
#tar xf $INPUT/curl.tar 2>&1 | tee -a "$LOG" || failure "Unable to unpack curl"
#tar xf $INPUT/expat.tar 2>&1 | tee -a "$LOG" || failure "Unable to unpack expat"
#tar xf $INPUT/openssl.tar 2>&1 | tee -a "$LOG" || failure "Unable to unpack openssl"
#tar xf $INPUT/SUL.tar 2>&1 | tee -a "$LOG" || failure "Unable to unpack SUL"
#tar xf $INPUT/zlib.tar 2>&1 | tee -a "$LOG" || failure "Unable to unpack zlib"

## Setup redist symlinks

#pushd openssl/lib64
#for lib in ssl crypto
#do
#    ln -snf lib${lib}.so.1.* "lib${lib}.so.1"
#    ln -snf lib${lib}.so.1.* "lib${lib}.so"
#done
#popd

#pushd expat/lib64
#ln -snf libexpat.so.1.* libexpat.so.1
#ln -snf libexpat.so.1.* libexpat.so
#popd

#tar zxf $INPUT/sav-linux-x86-precid-*.tgz 2>&1 | tee -a "$LOG" || failure "Unable to unpack savlinux tarfile"
#popd

cmake --build ./cmake-build-release --target thininstaller

#LIBRARIES="-lSUL -lboost_date_time -lboost_filesystem -lboost_system -lboost_thread -lretailer_lib -lxmlcpp -laesclib -lsaucrypto -lcurl -lexpat -lcrypto -lssl -lz -lssp"
#g++ -std=c++11 -g -DLINUX64 installer.cpp -Iredist/SUL/include -Iredist/curl/include64 -Lredist/curl/lib64 -Lredist/SUL/lib64 -Lredist/boost/lib64 -Lredist/openssl/lib64 -Lredist/zlib/lib64 -Lredist/expat/lib64 $LIBRARIES -o installer64 2>&1 | tee -a "$LOG" || failure "Failure to compile 64 bit version!"

#Target directories for copying
installer_dir="installer"
bin_dir="$installer_dir/bin"
libs_dir="$installer_dir/libs"
installer_binary="$bin_dir/installer"

mkdir -p "$installer_dir"
mkdir -p "$libs_dir"
mkdir -p "$bin_dir"

# bin files
cp cmake-build-release/thininstaller "$installer_binary" || failure "Failure to copy installer binary"

# TODO LINUXEP-6202 change this to an artisan prod build location
cp -a /redist/binaries/everest/versig_temp/versig "$bin_dir/"

# lib files
cp -a libs/SUL/lib64/*.so* "$libs_dir"
cp -a libs/curl/lib64/*.so* "$libs_dir"
cp -a libs/boost/lib64/*.so* "$libs_dir"
cp -a libs/openssl/lib64/*.so* "$libs_dir"
cp -a libs/expat/lib64/*.so* "$libs_dir"
cp -a libs/zlib/lib64/*.so* "$libs_dir"

#TODO LINUXEP-6203 which libs should we ship?
#cp /opt/toolchain/lib64/libstdc++.so.6 installer/bin64/ || failure "Failure to copy 64 bit libstdc++!"
#cp /opt/toolchain/lib64/libgcc_s.so.1 installer/bin64/ || failure "Failure to copy 64 bit libgcc!"
#cp -a redist/sophos-av/sav-linux/x86/64/engine/versig installer/bin64/

# strip files
strip $bin_dir/*
strip $libs_dir/*

cp *rootca* installer/

mkdir -p output
tar cf installer.tar installer/*
rm -f installer.tar.gz
gzip installer.tar

output_install_script="SophosSetup.sh"
cat installer_header.sh installer.tar.gz > output/${output_install_script}
pushd output
chmod u+x ${output_install_script}
python ../generateManifestDat.py .
tar cf installer.tar *
gzip installer.tar
sha256=$(sha256sum installer.tar.gz)
sha256="${sha256%% *}"
mv installer.tar.gz "${sha256}.tar.gz"
echo "{\"name\": \"${sha256}\"}" > latest.json
rm manifest.dat
rm ${output_install_script}
popd