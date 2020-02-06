#!/bin/bash

hostname
ip addr
ls /usr/local/share/ca-certificates/
which cov-commit-defects

STARTINGDIR=${BASE}
EDR_PLUGIN="$STARTINGDIR"/sspl-edr-plugin-build
EDR_PLUGIN_REDIST="$EDR_PLUGIN"/redist

cd "$EDR_PLUGIN"

rm -rf build64/
mkdir -p build64
cd build64

export PATH=/usr/local/bin:"$EDR_PLUGIN_REDIST"/cmake/bin:${PATH}

cmake ../

cov-configure --gcc

cov-build --dir covdir make -j2

cov-import-scm --dir covdir --scm git --filename-regex ${EDR_PLUGIN}/\.\*

cov-analyze --dir covdir --security --concurrency --enable-constraint-fpp --enable-virtual --strip-path "${EDR_PLUGIN}"

cov-format-errors --dir covdir --html-output cov-html --include-files ${EDR_PLUGIN} --strip-path $EDR_PLUGIN -X

echo $(pwd)
ll ../build/coveritycoverity.key
chmod 600 ../build/coveritycoverity.key
cov-commit-defects --dir covdir --host abn-coverity1.green.sophos --https-port 8443 --ssl --auth-key-file ../build/coverity/coverity.key \
    --stream "SSP-Linux-Plugin-EDR" --strip-path "${EDR_PLUGIN}" --on-new-cert trust --scm git --certs ../build/coverity/sophos-certs.crt \
    --description "$BUILD_TAG"
