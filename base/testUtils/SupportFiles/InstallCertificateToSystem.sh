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
	else
		echo "System is not rhel-based or ubuntu, cannot install certs"
		exit 1
	fi
}

install_cert_rhel()
{
	cp $1 /etc/pki/ca-trust/source/anchors/SOPHOS$(basename $1) || ( echo "Failed to copy cert" && exit 1 )
	update-ca-trust extract
}

install_cert_ubuntu()
{
	cp $1 /usr/local/share/ca-certificates/SOPHOS$(basename $1) || ( echo "Failed to copy cert" && exit 1 )
	update-ca-certificates -f
}

CERT=$1
echo args: $@
if [ -f $CERT ]; then
    install_cert $1
else
    echo "usage: InstallCertificateToSystem.sh  <path_to_certificate>" && exit 1
fi