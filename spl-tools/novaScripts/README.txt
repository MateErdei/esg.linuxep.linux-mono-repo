scripts for building and running nova on AWS

Setting up an fresh install of AWS nova copy the folder novascripts into the /home/ubuntu directory

cp the docker-compose.yml from the nova repo and replace the dns address
    networks:
      novanet:
        ipv4_address: 172.16.199.101
    dns:
      - 172.16.199.101

with
    networks:
      novanet:
        ipv4_address: 172.16.199.101
    dns:
      - SUBSTITUTE_ME_WITH_HOST_IP

cp the nova-ubuntu-intsall.yml from the nova repo and comment out the 'Manage DNS resolving by NetworkManager' task