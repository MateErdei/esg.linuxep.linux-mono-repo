#!/bin/bash

# This is needed because bazel-tools exported from monorepo are exported on a Windows machine,
# so some things are not quite as they should be to work on Linux

sed -i 's/\r//' tools/aspects/wrapper.sh
chmod +x tools/aspects/wrapper.sh
chmod +x imports/internal/sdds3_utils/sdds3-builder
chmod +x tools/launch_py3.py
chmod +x tools/src/bullseye/shim_ar.sh tools/src/bullseye/shim_gcc_compile.sh tools/src/bullseye/shim_gcc_link.sh

# sophlib is not part of bazel-tools, however this fix is necessary because of bazel-tools.
# sophlib is exported so that e.g. sdds3 is accessible as //common/sophlib:sdds3
# but bazel-tools requires it to be accessible as //common/sophlib/sdds3
# So we need to create some aliases to allow that to work
for lib in sdds3 string zip rapidjson crypto hostname time raii file pugixml
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

rm -rf imports/thirdparty/python/x64
rm -rf imports/thirdparty/python/arm64
mkdir -p imports/thirdparty/python/x64
mkdir -p imports/thirdparty/python/arm64
tar -zxf $(ls imports/thirdparty/python/cpython-*-x86_64-*.tar.gz) --strip-components=1 -C imports/thirdparty/python/x64
tar -zxf $(ls imports/thirdparty/python/cpython-*-aarch64-*.tar.gz) --strip-components=1 -C imports/thirdparty/python/arm64

if ! ( git apply --check --reverse build/bazel_tools.patch 2>/dev/null && echo "OK - patch already applied" )
then
  git apply --verbose build/bazel_tools.patch
fi

if [[ "$CI" == "true" ]]
then
  echo "Detected that this is a CI build"
else
  echo "Detected that this is a non-CI (local) build"
fi
