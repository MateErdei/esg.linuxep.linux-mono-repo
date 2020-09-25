#!/bin/bash

STARTINGDIR=$(pwd)
cd ${0%/*}
BASE=$(pwd)
cd ..
cd ..

I2=$(echo "$1" | sed -e's/[ _]/[ _]/g')
# echo "$I2"
exec grep -riI "$I2" testUtils/
