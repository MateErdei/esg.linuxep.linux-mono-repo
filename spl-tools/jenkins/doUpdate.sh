#!/bin/bash -xe

if [[ -z "${VSPHERE_IP}" ]]

then
	echo "Didn't get IP address for ${VM_NAME}" >2
	exit 1
fi
/bin/bash jenkins/runScriptOnRemoteMachine.sh update.sh root@${VSPHERE_IP}
sleep 120