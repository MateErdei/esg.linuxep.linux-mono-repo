#!/bin/bash

STARTINGDIR=$(pwd)
cd ${0%/*}
BASE=$(pwd)
cd ../../..
echo "Running in $(pwd)"

I2=$(echo "$1" | sed -e's/[ _]/[ _]/g')
# echo "$I2"
exec grep -riI "$I2" --include='*.py' --include='*.robot' base/testUtils common/TA av/TA edr/TA sdds/TA

