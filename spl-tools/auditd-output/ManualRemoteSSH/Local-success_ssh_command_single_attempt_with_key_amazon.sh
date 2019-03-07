#!/bin/bash
ssh -i "id_rsa_vagrant" -o StrictHostKeyChecking=no ec2-user@${1} 'ls /'
