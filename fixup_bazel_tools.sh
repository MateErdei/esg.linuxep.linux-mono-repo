#!/bin/bash

# This is needed because bazel-tools exported from monorepo are exported on a Windows machine,
# so some things are not quite as they should be to work on Linux

sed -i 's/\r//' tools/aspects/wrapper.sh
chmod +x tools/aspects/wrapper.sh
chmod +x imports/internal/sdds3_utils/sdds3-builder

# sophlib is not part of bazel-tools, however this fix is necessary because of bazel-tools.
# sophlib is exported so that e.g. sdds3 is accessible as //common/sophlib:sdds3
# but bazel-tools requires it to be accessible as //common/sophlib/sdds3
# So we need to create some aliases to allow that to work
for lib in sdds3 string zip rapidjson crypto hostname
do
  mkdir -p common/sophlib/$lib
  cat >common/sophlib/$lib/BUILD.bazel <<EOF
alias(
    name = "$lib",
    actual = "//common/sophlib:$lib",
    visibility = ["//visibility:public"],
)
EOF
done
