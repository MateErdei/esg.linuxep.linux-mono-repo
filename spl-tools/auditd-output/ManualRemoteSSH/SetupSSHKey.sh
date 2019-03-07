#!/bin/bash
cp -a /home/testuser/.ssh/authorized_keys /home/testuser/.ssh/authorized_keys.bak
cat id_rsa_vagrant.pub >> /home/testuser/.ssh/authorized_keys
