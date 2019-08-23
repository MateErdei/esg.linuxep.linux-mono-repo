#!/bin/bash

set -x 
rm -rf ~/using_stale_wars
if aws s3 ls s3://sspl-nova/novarepo/
then
        aws s3 cp s3://sspl-nova/novarepo/nova.tar.gz /tmp
else
        touch ~/using_stale_wars
fi
if aws s3 ls s3://sspl-nova/ui/
then
        aws s3 cp s3://sspl-nova/ui/uibuild.tar.gz /tmp 
else
        touch ~/using_stale_wars
fi

NEWUI=/tmp/uibuild.tar.gz
NEWNOVA=/tmp/nova.tar.gz
OLDUI=~/g/uibuild.tar.gz
OLDNOVA=~/g/nova.tar.gz

UINEWSUM=$(sha256sum $NEWUI | cut -d " " -f 1 )
NOVANEWSUM=$(sha256sum $NEWNOVA | cut -d " " -f 1 )
UIOLDSUM=$(sha256sum $OLDUI | cut -d " " -f 1 )
NOVAOLDSUM=$(sha256sum $OLDNOVA | cut -d " " -f 1 )

if [[ $UINEWSUM != $UIOLDSUM ]]; then
	rm $OLDUI
	cp $NEWUI $OLDUI
	pushd ~/g/cloud/ui/build
	rm -rf webroot
	tar -xzf $OLDUI
fi

if [[ $NOVANEWSUM != $NOVAOLDSUM ]]; then
	rm $OLDNOVA
	cp $NEWNOVA $OLDNOVA
	pushd ~/g/
	rm -rf nova
	tar -xzf $OLDNOVA
	cp /home/ubuntu/novaScripts/docker-compose.yml nova/docker-compose.yml
	cp /home/ubuntu/novaScripts/nova-ubuntu-install.yml nova/nova-ubuntu-install.yml
fi
