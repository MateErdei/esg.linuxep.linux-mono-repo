#!/usr/bin/env bash

clean_up_installed_certs()
{
	if [ -n "$(which apt-get)" ]
	then
		clean_up_certs_ubuntu || ( echo "cert clean-up failed" && exit 1 )
	elif [ -n "$(which yum)" ]
	then
		clean_up_certs_rhel || ( echo "cert clean-up failed" && exit 1 )
  elif [ -n "$(which zypper)" ]
    then
      clean_up_certs_sles || ( echo "cert clean-up failed" && exit 1 )
  else
    echo "System is not rhel-based, sles-based or ubuntu-based"
		exit 1
	fi
}

clean_up_certs_rhel()
{
	rm -rf /etc/pki/ca-trust/source/anchors/SOPHOS*
	update-ca-trust extract
}

clean_up_certs_ubuntu()
{
	rm -rf /usr/local/share/ca-certificates/SOPHOS*
	update-ca-certificates -f
}

clean_up_certs_sles()
{
	rm -rf /usr/share/pki/trust/anchors/SOPHOS*
	update-ca-certificates -f
}

clean_up_installed_certs