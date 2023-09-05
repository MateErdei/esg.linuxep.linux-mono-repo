#!/bin/bash
set -xe

#
# wrapper script to set an ubuntu template up for cloning (to be ran directly in a jenkins job)
#

if [[ -z "${VSPHERE_IP}" ]]
then
	echo "Didn't get IP address for ${VM_NAME}" >2
	exit 1
fi
spl-tools/jenkins/runScriptOnRemoteMachine.sh deleteDifferentMachineId.sh root@${VSPHERE_IP}
sleep 120