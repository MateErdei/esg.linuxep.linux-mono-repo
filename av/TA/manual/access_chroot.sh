#!/bin/bash

CHROOT=/opt/sophos-spl/plugins/av/chroot
mkdir -p ${CHROOT}/bin
cp /bin/bash ${CHROOT}/bin/
cp /lib64/ld-linux-x86-64.so.2 ${CHROOT}/lib64
mkdir -p ${CHROOT}/lib/x86_64-linux-gnu
cp /lib/x86_64-linux-gnu/libtinfo.so.6 ${CHROOT}/lib/x86_64-linux-gnu/
cp /lib/x86_64-linux-gnu/libdl.so.2 ${CHROOT}/lib/x86_64-linux-gnu/
cp /lib/x86_64-linux-gnu/libc.so.6 ${CHROOT}/lib/x86_64-linux-gnu/

cp /usr/bin/ldd ${CHROOT}/bin/

exec chroot ${CHROOT}