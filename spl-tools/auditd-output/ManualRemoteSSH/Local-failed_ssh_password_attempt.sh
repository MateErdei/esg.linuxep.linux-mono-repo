#!/bin/bash
sshpass -p badpassword ssh -o StrictHostKeyChecking=no testuser@${1}
