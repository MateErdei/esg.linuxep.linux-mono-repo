#!/bin/bash
set -e
#Fetch inputs using python build scripts and then build full product passing all arguments
tap fetch sspl_base
./build.sh $@
