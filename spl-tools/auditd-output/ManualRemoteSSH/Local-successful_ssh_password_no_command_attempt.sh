#!/bin/bash
sshpass -p linuxpassword ssh -o StrictHostKeyChecking=no testuser@${1}
