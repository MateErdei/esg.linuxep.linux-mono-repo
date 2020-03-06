#!/bin/bash

while [ ! -e /tmp/stop ]
do
	echo Waiting
	sleep 1
done
