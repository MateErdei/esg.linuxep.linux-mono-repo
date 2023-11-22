#!/bin/bash

trap "exit" INT

if [ -z "$VIRTUAL_ENV" ]
then
  echo "This script needs to be run from a TAP venv"
  exit 1
fi

cd "${0%/*}" || exit

./install_sdds3_server_certs.sh
python ../common/TA/libs/SDDS3server.py --sdds3 ../bazel-out/k8-dbg/bin/sdds/sdds3 --certpath ../common/TA/utils/server_certs/server.crt
