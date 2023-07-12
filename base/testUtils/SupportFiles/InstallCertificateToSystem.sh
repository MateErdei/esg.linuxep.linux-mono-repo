#!/usr/bin/env bash
set -x

install_cert()
{
  certificate=$1

  if [ -n "$(which apt-get)" ]
  then
    install_cert_ubuntu $certificate || ( echo "cert install failed" && exit 1 )
  elif [ -n "$(which yum)" ]
  then
    install_cert_rhel $certificate || ( echo "cert install failed" && exit 1 )
  elif [ -n "$(which zypper)" ]
  then
    install_cert_sles $certificate || ( echo "cert install failed" && exit 1 )
  else
    echo "System is not rhel-based, sles-based or ubuntu-based, cannot install certs"
    exit 1
  fi
}

update_ca_certs_with_retry()
{
  local attempt
  for (( attempt=1; attempt <= 10; ++attempt ))
  do
    update-ca-certificates -f
    if [ $? -eq 0 ]
    then
      echo Successfully updated CA certificates
      return 0
    fi
    echo Attempt to update CA certificates $attempt failed - will retry in 1 second
    sleep 1
  done
  return 1
}

install_cert_rhel()
{
  cp $1 /etc/pki/ca-trust/source/anchors/SOPHOS$(basename $1) || { echo "Failed to copy cert"; exit 1; }
  update-ca-trust extract
}

install_cert_ubuntu()
{
  cp $1 /usr/local/share/ca-certificates/SOPHOS$(basename $1) || { echo "Failed to copy cert"; exit 1; }
  update_ca_certs_with_retry
}

install_cert_sles()
{
  cp $1 /usr/share/pki/trust/anchors/SOPHOS$(basename $1) || { echo "Failed to copy cert"; exit 1; }
  update_ca_certs_with_retry
}

CERT=$1
echo args: $@
if [ -f $CERT ]
then
    install_cert $1
else
    echo "usage: InstallCertificateToSystem.sh  <path_to_certificate>" && exit 1
fi