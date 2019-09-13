#!/bin/bash
#Fetch inputs using python build scripts and then build full product passing all arguments
python3 -m build_scripts.artisan_fetch build/release-package.xml
./build.sh $@
