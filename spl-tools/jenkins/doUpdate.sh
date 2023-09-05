#!/bin/bash
set -xe

#
# wrapper script to perform a yum/apt update on a jenkins template (to be ran directly in a jenkins job)
#

if [[ -z "${VSPHERE_IP}" ]]
then
	echo "Didn't get IP address for ${VM_NAME}" >2
	exit 1
fi
spl-tools/jenkins/runScriptOnRemoteMachine.sh update.sh root@${VSPHERE_IP}
sleep 120