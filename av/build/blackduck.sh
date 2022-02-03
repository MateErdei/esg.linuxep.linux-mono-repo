#!/bin/bash

# Blackduck scan should take minutes.
# If it takes longer than 1 hour it's probably stuck
# Look in ~/blackduck/runs/<date-time>/scan/BlackDuckScanOutput/<date-time>/CLI_Output.txt

set -ex

if [[ -z $NO_BUILD ]]
then
    bash ./build.sh --setup
    bash ./build.sh --dev --no-unpack
fi

# MTUyNTdjMmItYWU0NS00MTM2LWIzN2QtNmFiZmI5NDVkMWJmOjExMDI0NDE5LWM1ZmMtNDRmNC05NDBhLTY2YzRiMDg2MmZmZA==

BLACKDUCK_API_TOKEN="${BLACKDUCK_API_TOKEN:-$1}"
VERSION=${VERSION:-develop}


exec bash <(curl -s -L https://detect.synopsys.com/detect.sh) \
    --blackduck.url=https://sophos.app.blackduck.com \
    --blackduck.api.token="${BLACKDUCK_API_TOKEN}" \
    --detect.project.name='ESG\ -\ EP\ -\ AV\ Plugin\ SSPL' \
    --detect.project.version.name="${VERSION}" \
    --detect.blackduck.signature.scanner.exclusion.name.patterns=tapvenv,file_samples,base-sdds,tests,TA,tools,pipeline \
    $EXTRA_ARGS

#    --detect.detector.search.exclusion.paths=TA/resources/file_samples,tapvenv


