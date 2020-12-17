#!/usr/bin/env bash
set -x

failure()
{
  echo "$@"
  exit 1
}

install_cert()
{
	certificate=$1

	if [ -n "$(which apt-get)" ]
	then
		install_cert_ubuntu $certificate || failure "cert install failed"
	elif [ -n "$(which yum)" ]
	then
		install_cert_rhel $certificate || failure "cert install failed"
	else
		echo "System is not rhel-based or ubuntu, cannot install certs"
		exit 1
	fi
}

install_cert_rhel()
{
	cp $1 /etc/pki/ca-trust/source/anchors/SOPHOS$(basename $1) || failure "Failed to copy cert"
	update-ca-trust extract
}

install_cert_ubuntu()
{
	cp $1 /usr/local/share/ca-certificates/SOPHOS$(basename $1) || failure "Failed to copy cert"
	update-ca-certificates -f
}

echo args: $@
for CERT in $@
do
  if [ -f $CERT ]
  then
      install_cert $1
  else
      failure "usage: InstallCertificateToSystem.sh  <path_to_certificate>+"
  fi
done
