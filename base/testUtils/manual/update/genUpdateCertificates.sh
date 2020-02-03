#!/usr/bin/env bash

DIRECTORYOFSCRIPT="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

SYSTEMPRODUCTTESTSPATH="${DIRECTORYOFSCRIPT}/../.."

pushd ${SYSTEMPRODUCTTESTSPATH}
SYSTEMPRODUCTTESTSPATH=$(pwd)
PYTHONPATH=${SYSTEMPRODUCTTESTSPATH}/libs/:$PYTHONPATH python -c '
import UpdateServer
print "Running update certificates"
updateServer = UpdateServer.UpdateServer()
updateServer.generate_update_certs()
'
popd
