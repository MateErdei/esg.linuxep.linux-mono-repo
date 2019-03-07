#!/bin/bash
ssh -i "id_rsa_vagrant" -o StrictHostKeyChecking=no testuser@${1} 'ls /'
