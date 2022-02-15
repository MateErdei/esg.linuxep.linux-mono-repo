#!/bin/bash

set -x


env

REDIST=./redist
export LIBRARY_PATH="${LIBRARY_PATH}:/usr/lib/x86_64-linux-gnu"
tar xvzf redist/gcc/gcc-8.1.0-linux.tar.gz -C redist/gcc/
export LD_LIBRARY_PATH="${REDIST}/gcc/gcc/lib64/:/usr/lib/x86_64-linux-gnu:${LD_LIBRARY_PATH}"
#objdump -x binary-or-library |grep RPATH
#readelf -d binary-or-library |head -20

## debug
ls -l /usr/lib/x86_64-linux-gnu
find /lib64/
find / -name "libstdc++.so*"

SDDS3_BUILDER=${REDIST}/sdds3/sdds3-builder
chmod +x ${SDDS3_BUILDER}

ldd "$SDDS3_BUILDER"

OUTPUT=./output
rm -rf ${OUTPUT}
REPO=${OUTPUT}/repo

PACKAGE_OUTPUT=${REPO}/package
SUITE_OUTPUT=${REPO}/suite

LAUNCH_DARKLY=${OUTPUT}/launchdarkly

PACKAGE_INPUT=${REDIST}/package
SUPPLEMENT_INPUT=${REDIST}/package
SUITE_INPUT=./sdds3/suite

#move packages to repo
find ${PACKAGE_INPUT}/ -name SDDS-Import.xml | xargs -i ${SDDS3_BUILDER} --build-package --package-dir ${PACKAGE_OUTPUT} \
                                                                  --sdds-import {} --nonce DNR

#create suites
find ${SUITE_INPUT}/ -type f | xargs -i ${SDDS3_BUILDER} --build-suite --xml {} --dir ${SUITE_OUTPUT} --nonce DNR



#create launchdarkly files
cp -r sdds3/launchdarkly ${LAUNCH_DARKLY}