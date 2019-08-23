#!/bin/bash

cd /home/ubuntu/g/nova
echo 'setting up dns'
# Change the DNS container so it uses the host as a DNS server.
# Fixes issue where the hub container cannot access dci.sophosupd.com
IP_ADDRESS=$(ip addr | grep ens | sed -n "s_.*inet \([^/]*\).*_\1_p")
[[ -z $IP_ADDRESS ]] && IP_ADDRESS=$(ip addr | grep eth | sed -n "s_.*inet \([^/]*\).*_\1_p")
  
DOCKER_COMPOSE=/home/ubuntu/g/nova/docker-compose.yml
DNSMASQ_CONF=/home/ubuntu/g/nova/dnsmasq/dnsmasq.conf
  
echo "Replacing DNS IP Address with \"$IP_ADDRESS\""
  
if ! grep SUBSTITUTE_ME_WITH_HOST_IP $DOCKER_COMPOSE
then
        echo "WARNING - Cannot fix dns file, substitution token not added"
fi
  
cp $DOCKER_COMPOSE "${DOCKER_COMPOSE}.bak"
cp $DNSMASQ_CONF "${DNSMASQ_CONF}.bak"
sed -i "s/SUBSTITUTE_ME_WITH_HOST_IP/$IP_ADDRESS/" $DOCKER_COMPOSE
sed -i "s_\(sophos\.com/\)10\.104\.65\.1_\1${IP_ADDRESS}_" $DNSMASQ_CONF

echo 'starting Nova'
su ubuntu /home/ubuntu/novaScripts/buildNova.sh

find /var/lib/docker/containers/ -name "resolv.conf" | xargs -n 1 cp -v /home/ubuntu/novaScripts/resolv.conf

su ubuntu ./gradlew startNova 
echo 'finished starting nova exit code: ' $?

mv "${DOCKER_COMPOSE}.bak" ${DOCKER_COMPOSE}
mv "${DNSMASQ_CONF}.bak" ${DNSMASQ_CONF}
