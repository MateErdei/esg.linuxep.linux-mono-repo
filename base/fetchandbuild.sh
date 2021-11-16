#!/bin/bash
set -e
#Fetch inputs using python build scripts and then build full product passing all arguments

rm -rf ./input
#python3 -m build_scripts.artisan_fetch build/release-package.xml
tap fetch sspl_base
./build.sh $@
