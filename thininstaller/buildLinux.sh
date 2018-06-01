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
INPUT=$BASE/input
[ -d $INPUT ] || failure "Can't find input"

REDIST=$BASE/redist
rm -rf $REDIST
mkdir -p $REDIST
pushd $REDIST
tar xf $INPUT/boost.tar 2>&1 | tee -a "$LOG" || failure "Unable to unpack boost"
tar xf $INPUT/curl.tar 2>&1 | tee -a "$LOG" || failure "Unable to unpack curl"
tar xf $INPUT/expat.tar 2>&1 | tee -a "$LOG" || failure "Unable to unpack expat"
tar xf $INPUT/openssl.tar 2>&1 | tee -a "$LOG" || failure "Unable to unpack openssl"
tar xf $INPUT/SUL.tar 2>&1 | tee -a "$LOG" || failure "Unable to unpack SUL"
tar xf $INPUT/zlib.tar 2>&1 | tee -a "$LOG" || failure "Unable to unpack zlib"
## Setup redist symlinks
for BITS in 32 64
do
    pushd openssl/lib${BITS}
    for lib in ssl crypto
    do
        ln -snf lib${lib}.so.1.* "lib${lib}.so.1"
        ln -snf lib${lib}.so.1.* "lib${lib}.so"
    done
    popd
    pushd expat/lib${BITS}
    ln -snf libexpat.so.1.* libexpat.so.1
    ln -snf libexpat.so.1.* libexpat.so
    popd
done

tar zxf $INPUT/sav-linux-x86-precid-*.tgz 2>&1 | tee -a "$LOG" || failure "Unable to unpack savlinux tarfile"

popd

LIBRARIES="-lSUL -lboost_date_time -lboost_filesystem -lboost_system -lboost_thread -lretailer_lib -lxmlcpp -laesclib -lsaucrypto -lcurl -lexpat -lcrypto -lssl -lz -lssp"

g++ -std=c++11 -g -DLINUX64 installer.cpp -Iredist/SUL/include -Iredist/curl/include64 -Lredist/curl/lib64 -Lredist/SUL/lib64 -Lredist/boost/lib64 -Lredist/openssl/lib64 -Lredist/zlib/lib64 -Lredist/expat/lib64 $LIBRARIES -o installer64 2>&1 | tee -a "$LOG" || failure "Failure to compile 64 bit version!"

g++ -std=c++11 -m32 -g -DLINUX32 installer.cpp -Iredist/SUL/include -Iredist/curl/include32 -Lredist/curl/lib32 -Lredist/SUL/lib32 -Lredist/boost/lib32 -Lredist/openssl/lib32 -Lredist/zlib/lib32 -Lredist/expat/lib32 $LIBRARIES -o installer32 2>&1 | tee -a "$LOG" || failure "Failure to compile 32 bit version!"

mkdir -p installer/bin64
mkdir -p installer/bin32

cp installer64 installer/bin64/installer || failure "Failure to copy 64 bit version!"
cp -a redist/SUL/lib64/*.so* installer/bin64/
cp -a redist/curl/lib64/*.so* installer/bin64/
cp -a redist/boost/lib64/*.so* installer/bin64/
cp -a redist/openssl/lib64/*.so* installer/bin64/
cp -a redist/expat/lib64/*.so* installer/bin64/
cp -a redist/zlib/lib64/*.so* installer/bin64/
cp /opt/toolchain/lib64/libstdc++.so.6 installer/bin64/ || failure "Failure to copy 64 bit libstdc++!"
cp /opt/toolchain/lib64/libgcc_s.so.1 installer/bin64/ || failure "Failure to copy 64 bit libgcc!"
cp -a redist/sophos-av/sav-linux/x86/64/engine/versig installer/bin64/
strip installer/bin64/*
cp *rootca* installer/

cp installer32 installer/bin32/installer || failure "Failure to copy 32 bit version!"
cp -a redist/SUL/lib32/*.so* installer/bin32/
cp -a redist/curl/lib32/*.so* installer/bin32/
cp -a redist/boost/lib32/*.so* installer/bin32/
cp -a redist/openssl/lib32/*.so* installer/bin32/
cp -a redist/expat/lib32/*.so* installer/bin32/
cp -a redist/zlib/lib32/*.so* installer/bin32/
cp /opt/toolchain/lib32/libstdc++.so.6 installer/bin32/ || cp /opt/toolchain/lib/libstdc++.so.6 installer/bin32/ || failure "Failure to copy 32 bit libstdc++!"
cp /opt/toolchain/lib32/libgcc_s.so.1 installer/bin32/ || cp /opt/toolchain/lib/libgcc_s.so.1 installer/bin32/ || failure "Failure to copy 32 bit libgcc!"
cp -a redist/sophos-av/sav-linux/x86/32/engine/versig installer/bin32/
strip installer/bin32/*

mkdir -p output
tar cf installer.tar installer/*
rm -f installer.tar.gz
gzip installer.tar
cp installer.tar.gz output/

cat installer_header.sh credentials.txt installer.tar.gz > output/installer.sh
cat installer_header.sh credentials_with_windows_line_endings.txt installer.tar.gz > output/installer_with_windows_line_endings.sh
cat installer_header.sh credentials_with_extra_lines.txt installer.tar.gz > output/installer_with_extra_lines.sh
cat installer_header.sh credentials_with_windows_line_endings_and_extra_lines.txt installer.tar.gz > output/installer_with_windows_line_endings_and_extra_lines.sh

chmod u+x output/installer*.sh
cp installer_header.sh output/
cp credentials.txt output/
