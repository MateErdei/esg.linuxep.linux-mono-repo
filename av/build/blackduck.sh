#!/bin/bash

set -ex

# MTUyNTdjMmItYWU0NS00MTM2LWIzN2QtNmFiZmI5NDVkMWJmOjExMDI0NDE5LWM1ZmMtNDRmNC05NDBhLTY2YzRiMDg2MmZmZA==

BLACKDUCK_API_TOKEN="${BLACKDUCK_API_TOKEN:-$1}"
VERSION=${VERSION:-develop}

exec bash <(curl -s -L https://detect.synopsys.com/detect.sh) \
    --blackduck.url=https://sophos.app.blackduck.com \
    --blackduck.api.token="${BLACKDUCK_API_TOKEN}" \
    --detect.project.name='SSPL-AV' \
    --detect.project.version.name="${VERSION}"
