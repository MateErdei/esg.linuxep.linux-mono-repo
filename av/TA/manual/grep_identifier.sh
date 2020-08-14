#!/bin/bash

STARTINGDIR=$(pwd)
cd ${0%/*}
BASE=$(pwd)
cd ..
cd ..

I="$1"
I2=$(echo "$I" | sed -e's/ /{SP}/g' -e's/_/{SP}/g' | sed -e's/{SP}/[ _]/g')
#echo "$I2"
exec grep -riI "$I2" TA/
