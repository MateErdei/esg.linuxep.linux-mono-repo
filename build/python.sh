#!/bin/bash

# This is a wrapper around Python, which is needed because various build environments have different Python versions
# available, and otherwise there isn't a single command that can be run to run Python >=3.7

if [ ! -z "${VIRTUAL_ENV}" ]
then
  python $@
elif [ -f /usr/bin/python3.7 ]
then
  /usr/bin/python3.7 $@
else
  python3 $@
fi
