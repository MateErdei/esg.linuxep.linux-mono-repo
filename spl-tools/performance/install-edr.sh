#!/usr/bin/env bash

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

