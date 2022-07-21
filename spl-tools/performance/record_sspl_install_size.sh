#!/bin/bash

#PLUGINS_DIR="/opt/sophos-spl/plugins"
SSPL_DIR="/opt/sophos-spl/plugins"
[[ -d "$SSPL_DIR" ]] || exit 1
HOSTNAME=$(hostname)
DATETIME=$(date -u +"%Y/%m/%d %H:%M:%S")


# Get install size
# Ensure 1kB block size (don't rely on different OSs defaulting 1kB)
BLOCK_SIZE=1000
SIZE=$(du -B $BLOCK_SIZE -sx --exclude /opt/sophos-spl/var/sophos-spl-comms /opt/sophos-spl | cut  -f 1)

# Basic check to make sure we do have something installed before polluting test data with a bad setup.
BASIC_CHECK_LIMIT_KB=10000
if (( $SIZE < BASIC_CHECK_LIMIT_KB ))
then
    # Something went wrong, the install can't be less than this small
    echo "Check the product install, it's less than ${BASIC_CHECK_LIMIT_KB}kB"
    exit 1
fi


curl -X POST "http://sspl-perf-mon:9200/sspl_install_size/_doc" -H "Content-Type: application/json" -d \
    '{"hostname":"'$HOSTNAME'", "size_kB":'$SIZE', "datetime":"'"$DATETIME"'"}'


