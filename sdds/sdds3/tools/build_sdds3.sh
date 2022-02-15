#!/bin/bash

set -ex


env

REDIST=./redist

SDDS3_BUILDER=${REDIST}/sdds3/sdds3-builder
chmod +x ${SDDS3_BUILDER}

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