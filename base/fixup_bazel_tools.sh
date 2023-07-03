#!/bin/bash

# This is needed because bazel-tools exported from monorepo are exported on a Windows machine,
# so some things are not quite as they should be to work on Linux

sed -i 's/\r\n$/\n/' tools/aspects/wrapper.sh
chmod +x tools/aspects/wrapper.sh
