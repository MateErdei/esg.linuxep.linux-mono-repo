#!/bin/bash
set -e
#Fetch inputs using python build scripts and then build full product passing all arguments

rm -rf ./input
python3 -m build_scripts.artisan_fetch build/release-package.xml
# tap fetch is the more "modern" way of doing this. But requires at least python3.7, which not all our test templates have.
#tap fetch sspl_base
./build.sh $@
