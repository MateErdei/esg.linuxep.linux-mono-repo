#!/bin/bash

PRODUCT=thininstaller
BULLSEYE=0
cd ${0%/*}
BASE=$(pwd)
[ -f "$BASE"/pathmgr.sh ] || { echo "Failed to find $BASE/bin/build/pathmgr.sh" | tee log/build.log ; exit 1 ; }
source "$BASE"/pathmgr.sh
source "$BASE"/common.sh

ORIG_PATH=$PATH
ORIG_LD_LIBRARY_PATH=$LD_LIBRARY_PATH

DO_32=1

while [ $# -ge 1 ]; do
    case $1 in
        --bullseye)
            BULLSEYE=1
            DO_32=0
            ;;
        --no-32)
            DO_32=0
            ;;
        --sav)
            shift
            SAV_DIST=$1
            ;;
        *)
            echo "Bad argument: $1"
            exit 2
            ;;
    esac
    shift
done

rm -rf $BASE/output $BASE/installer

REDIST=/redist/binaries/linux/input
[ -z $SAV_DIST ] && SAV_DIST=~/build64-sav-dlcl/dist/full
[ -d $SAV_DIST ] || SAV_DIST=~/build64-sav-debug/dist/full

BULLSEYE_FLAGS=""
if [[ $BULLSEYE == 1 ]]
then
    ## In the src directory
    SAV_DIST=$BASE/../../build/dist/full
    export PATH=/usr/local/bullseye/bin:/usr/bin:$PATH
    echo "New PATH=$PATH" | tee -a $LOG | tee -a /tmp/build.log
    BULLSEYE_FLAGS="-L${SAV_DIST}/sophos-av/sav-linux/x86/64/lib64"
    [[ -d $SAV_DIST ]] || failure "Unable to find SAV_DIST=$SAV_DIST"
    pushd "${SAV_DIST}/sophos-av/sav-linux/x86/64/lib64/" || failure "Unable to pushd to ${SAV_DIST}/sophos-av/sav-linux/x86/64/lib64/"
    [ -f libssp.so ] || ln -s libssp.so.0 libssp.so
    popd
fi

[ -f libssp.so ] || ln -s $REDIST/openssl/lib64/libssp.so.0 libssp.so

LIBRARIES="-lSUL -lboost_date_time -lboost_filesystem -lboost_system -lboost_thread -lretailer_lib -lxmlcpp -laesclib -lsaucrypto -lcurl -lexpat -lcrypto -lssl -lz -lssp"

g++ -std=c++11 -g -DLINUX64 installer.cpp \
    -I$REDIST/SUL/include -I$REDIST/curl/include64 \
    -L$REDIST/curl/lib64 -L$REDIST/SUL/lib64 -L$REDIST/boost/lib64 -L$REDIST/openssl/lib64 \
    -L$REDIST/zlib/lib64 -L$REDIST/expat/lib64 -L.\
    $LIBRARIES $BULLSEYE_FLAGS \
    -o installer64 2>&1 | tee -a "$LOG" || failure "Failure to compile 64 bit version!"

if [[ $DO_32 == 1 ]]
then
    mkdir -p lib32
    ln -sf $REDIST/openssl/lib32/libssp.so.0 lib32/libssp.so
    g++ -std=c++11 -m32 -g -DLINUX32 installer.cpp -I$REDIST/SUL/include -I$REDIST/curl/include32 \
        -L$REDIST/curl/lib32 -L$REDIST/SUL/lib32 -L$REDIST/boost/lib32 -L$REDIST/openssl/lib32 \
        -L$REDIST/zlib/lib32 -L$REDIST/expat/lib32 -Llib32 \
        $LIBRARIES -o installer32 2>&1 | tee -a "$LOG" || failure "Failure to compile 32 bit version!"
fi

STD_LIB=/opt/toolchain/lib64
if [[ ! -d $STD_LIB ]]
then
    STD_LIB=/usr/lib/x86_64-linux-gnu
fi

GCC_S_LIB=/opt/toolchain/lib64
if [[ ! -d $GCC_S_LIB ]]
then
    GCC_S_LIB=/lib/x86_64-linux-gnu
fi

mkdir -p installer/bin64
mkdir -p installer/bin32

cp installer64 installer/bin64/installer || failure "Failure to copy 64 bit version!"
cp -a $REDIST/SUL/lib64/*.so* installer/bin64/
cp -a $REDIST/curl/lib64/*.so* installer/bin64/
cp -a $REDIST/boost/lib64/*.so* installer/bin64/
cp -a $REDIST/openssl/lib64/*.so* installer/bin64/
cp -a $REDIST/expat/lib64/*.so* installer/bin64/
cp -a $REDIST/zlib/lib64/*.so* installer/bin64/
cp $STD_LIB/libstdc++.so.6 installer/bin64/ || failure "Failure to copy 64 bit libstdc++!"
cp $GCC_S_LIB/libgcc_s.so.1 installer/bin64/ || failure "Failure to copy 64 bit libgcc!"
cp -a $SAV_DIST/sophos-av/sav-linux/x86/64/engine/versig installer/bin64/
strip installer/bin64/*
cp *rootca* installer/

if [[ $DO_32 == 1 ]]
then
    STD_LIB=/opt/toolchain/lib32
    [[ -d $STD_LIB ]] || STD_LIB=/opt/toolchain/lib
    [[ -d $STD_LIB ]] || STD_LIB=/usr/lib32

    GCC_S_LIB=/opt/toolchain/lib32
    [[ -d $GCC_S_LIB ]] || GCC_S_LIB=/opt/toolchain/lib
    [[ -d $GCC_S_LIB ]] || GCC_S_LIB=/usr/lib32

    cp installer32 installer/bin32/installer || failure "Failure to copy 32 bit version!"
    cp -a $REDIST/SUL/lib32/*.so* installer/bin32/
    cp -a $REDIST/curl/lib32/*.so* installer/bin32/
    cp -a $REDIST/boost/lib32/*.so* installer/bin32/
    cp -a $REDIST/openssl/lib32/*.so* installer/bin32/
    cp -a $REDIST/expat/lib32/*.so* installer/bin32/
    cp -a $REDIST/zlib/lib32/*.so* installer/bin32/
    cp $STD_LIB/libstdc++.so.6 installer/bin32/ || failure "Failure to copy 32 bit libstdc++ from $STD_LIB!"
    cp $GCC_S_LIB/libgcc_s.so.1 installer/bin32/ || failure "Failure to copy 32 bit libgcc from $GCC_S_LIB!"
    cp -a $SAV_DIST/sophos-av/sav-linux/x86/32/engine/versig installer/bin32/
    strip installer/bin32/*
fi

mkdir -p output
tar cf installer.tar installer/*
rm -f installer.tar.gz
gzip installer.tar
cp installer.tar.gz output/
cp $BASE/create.sh output/
chmod 700 output/create.sh

cat installer_header.sh credentials.txt installer.tar.gz > output/installer.sh
cat installer_header.sh credentials_with_windows_line_endings.txt installer.tar.gz > output/installer_with_windows_line_endings.sh
cat installer_header.sh credentials_with_extra_lines.txt installer.tar.gz > output/installer_with_extra_lines.sh
cat installer_header.sh credentials_with_windows_line_endings_and_extra_lines.txt installer.tar.gz > output/installer_with_windows_line_endings_and_extra_lines.sh

chmod u+x output/installer*.sh
cp installer_header.sh output/
cp credentials.txt output/
