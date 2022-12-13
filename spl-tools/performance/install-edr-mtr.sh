#!/usr/bin/env bash

if [[ -z $1 ]]
then
  echo "Please pass in the Central test account password."
  exit 1
fi

date
# wait until all existing installs are removed. This script may slightly overlap with an uninstall.
while [[ -d /opt/sophos-spl ]]
do
  sleep 1
done

# For QA use these lines
#cp /root/certs/qa_region_certs/hmr-qa-sha256.pem /tmp/
#export MCS_CA=/tmp/hmr-qa-sha256.pem
#chmod a+r $MCS_CA
#/root/performance/edr-mtr-installer.sh --allow-override-mcs-ca
#sleep 60
#CENTRAL_PASSWORD=$1
#python3 $CLOUD_CLIENT_SCRIPT --region q --email testEUCentral-Perf-MTR@savlinux.xmas.testqa.com --password "$CENTRAL_PASSWORD" move_machine_to_edr_eap "$HOSTNAME"

/root/performance/edr-mtr-installer.sh
sleep 60
CENTRAL_PASSWORD=$1
python3 "$CLOUD_CLIENT_SCRIPT" --region p --email LicenceDarwin@gmail.com --password "$CENTRAL_PASSWORD" move_machine_to_edr_eap "$HOSTNAME"


## re-register to get correct policy as there is a bug in Central.
#sleep 10
#/root/performance/edr-mtr-installer.sh --allow-override-mcs-ca
