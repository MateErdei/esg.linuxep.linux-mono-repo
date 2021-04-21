#!/usr/bin/env bash
echo "Download specified file/folder from Allegro in /tmp/allegro"

#sspl-plugin-edr-taptest/sspl-plugin-edr-tap.cov
[[ -n ${FILESTODOWNLOAD} ]] || exitFailure 4 "Files to download not specified"

SCRIPT_DIR=$(cd "${0%/*}"; echo "$PWD")
[[ -n ${BASE} ]] || BASE=${SCRIPT_DIR}/../..

PRIVATE_KEY=/opt/test/inputs/bullseye_files/private.key
[[ -f ${PRIVATE_KEY} ]] || PRIVATE_KEY=${BASE}/build/bullseye/private.key
[[ -f ${PRIVATE_KEY} ]] || exitFailure 3 "Unable to find private key for download"

## Ensure ssh won't complain about private key permissions:
chmod 600 ${PRIVATE_KEY}

mkdir /tmp/allegro

rsync -va --rsh="ssh -i ${PRIVATE_KEY} -o StrictHostKeyChecking=no" --delete \
    upload@allegro.eng.sophos:public_html/bullseye/"${FILESTODOWNLOAD}" \
    /tmp/allegro
