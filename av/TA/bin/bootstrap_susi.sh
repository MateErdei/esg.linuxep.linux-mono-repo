#!/bin/bash

set -ex

function failure()
{
    echo "$*"
    ls -lR "${SUSI_UPDATE_SRC}"
    exit 32
}

function set_permissions_on_susi_update_source()
{
    chown -R ${THREAT_DETECTOR_USER}:${GROUP_NAME} "${SUSI_UPDATE_SRC}"
    chmod 710 "${SUSI_UPDATE_SRC}"
    chmod -R g-w,o= "${SUSI_UPDATE_SRC}"
    chmod -R u=rwx,g=,o= \
        "${SUSI_UPDATE_SRC}/susicore" \
        "${SUSI_UPDATE_SRC}/libglobalrep" \
        "${SUSI_UPDATE_SRC}/libsophtainer" \
        "${SUSI_UPDATE_SRC}/lrlib" \
        "${SUSI_UPDATE_SRC}/libsavi" \
        "${SUSI_UPDATE_SRC}/libupdater"

    chmod -R u=rwX,g=,o= \
        "${SUSI_UPDATE_SRC}/rules"

    # AV Plugin needs to be able to read files in the following directories for telemetry
    chmod -R g=rX,o= "${SUSI_UPDATE_SRC}/vdl/"
    chmod -R g=rX,o= "${SUSI_UPDATE_SRC}/reputation/"
    chmod -R g=rX,o= "${SUSI_UPDATE_SRC}/model/"
}

function generate_susi_package_manifest()
{
    local nonsupplement_manifest="${SUSI_UPDATE_SRC}/nonsupplement_manifest.txt"
    local temp_manifest="${SUSI_UPDATE_SRC}/temp_manifest.txt"
    local package_manifest="${SUSI_UPDATE_SRC}/package_manifest.txt"
    cp ${nonsupplement_manifest} ${temp_manifest}

    local DATASETA_MANIFEST=$SUSI_UPDATE_SRC/vdl/manifestdata.dat
    vdl_hash=$(sha256sum "$DATASETA_MANIFEST" | awk '{print $1}')
    echo "vdl ${vdl_hash}" >> ${temp_manifest}

    model_hash=$(sha256sum "${SUSI_UPDATE_SRC}/model/manifest.dat" | awk '{print $1}')
    echo "model ${model_hash}" >> ${temp_manifest}

    lrdata_hash=$(cat ${SUSI_UPDATE_SRC}/reputation/filerep.dat ${SUSI_UPDATE_SRC}/reputation/signerrep.dat | sha256sum | awk '{print $1}')
    echo "reputation ${lrdata_hash}" >> ${temp_manifest}

    libupdater_hash=$(sha256sum "${SUSI_UPDATE_SRC}/libupdater/libupdater.so" | awk '{print $1}')
    [[ -n "$libupdater_hash" ]] || failure "Unable to hash ${SUSI_UPDATE_SRC}/libupdater/libupdater.so"
    echo "libupdater ${libupdater_hash}" >> ${temp_manifest}

    manifest_hash=$(sha256sum "${temp_manifest}" | awk '{print $1}')
    echo $manifest_hash > $package_manifest
    cat $temp_manifest >> $package_manifest

    rm -f ${temp_manifest}

    set_permissions_on_susi_update_source
}

function bootstrap_susi_from_update_source()
{
    set_permissions_on_susi_update_source
    ${PLUGIN_INSTALL}/sbin/susi_bootstrapper
    chown -R ${THREAT_DETECTOR_USER}:${GROUP_NAME} "${SUSI_DIST_VERS}"
}

function main()
{
    THREAT_DETECTOR_USER=sophos-spl-threat-detector
    GROUP_NAME=sophos-spl-group
    PLUGIN_INSTALL=${SOPHOS_INSTALL}/plugins/av

    SUSI_UPDATE_SRC="$PLUGIN_INSTALL/chroot/susi/update_source"
    SUSI_DIST_VERS="$PLUGIN_INSTALL/chroot/susi/distribution_version"

    ls -lR ${SUSI_UPDATE_SRC}
    pushd ${SUSI_UPDATE_SRC}/libupdater
    ldconfig -lNv "libupdater.so".*
    ln -s libupdater.so.? "libupdater.so"
    popd
    ldconfig -lNv ${SUSI_UPDATE_SRC}/{susicore,libsavi,libglobalrep}/*.so.*
    pushd ${SUSI_UPDATE_SRC}/libglobalrep
    ln -s libglobalrep.so.? libglobalrep.so
    popd
    ls -lR ${SUSI_UPDATE_SRC}

    generate_susi_package_manifest
    chmod 700 "${PLUGIN_INSTALL}/sbin/susi_bootstrapper"
    bootstrap_susi_from_update_source
}

SOPHOS_INSTALL=${SOPHOS_INSTALL:-/opt/sophos-spl}
cd ${SOPHOS_INSTALL}
main </dev/null
