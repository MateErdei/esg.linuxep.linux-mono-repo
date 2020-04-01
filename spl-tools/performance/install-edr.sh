#!/usr/bin/env bash

[[ -z $1 ]] || echo "Please pass in the Central test account password."

date
# wait until all existing installs are removed. This script may slightly overlap with an uninstall.
while [[ -d /opt/sophos-spl ]]
do
  sleep 1
done

cp /root/certs/qa_region_certs/hmr-qa-sha256.pem /tmp/
export MCS_CA=/tmp/hmr-qa-sha256.pem
chmod a+r $MCS_CA

/root/performance/edr-installer.sh --allow-override-mcs-ca
#touch /opt/sophos-spl/base/mcs/certs/ca_env_override_flag
#/root/performance/edr-mtr-installer.sh --allow-override-mcs-ca
sleep 60
CENTRAL_PASSWORD=$1
python3 /root/performance/cloudClient.py --region q --email testEUCentral-perf@savlinux.xmas.testqa.com --password "$CENTRAL_PASSWORD" move_machine_to_edr_eap "$HOSTNAME"



