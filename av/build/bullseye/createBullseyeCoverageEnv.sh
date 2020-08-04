#!/bin/bash
echo "COVFILE=$1" >/tmp/BullseyeCoverageEnv.txt
echo "COVSRCDIR=$2" >>/tmp/BullseyeCoverageEnv.txt
chmod 0644 /tmp/BullseyeCoverageEnv.txt
chmod 0666 "$1"

