#!/bin/bash
cp -a /home/ec2-user/.ssh/authorized_keys /home/ec2-user/.ssh/authorized_keys.bak
cat id_rsa_vagrant.pub >>  /home/ec2-user/.ssh/authorized_keys
