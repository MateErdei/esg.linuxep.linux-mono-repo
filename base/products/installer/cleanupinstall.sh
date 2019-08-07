#!/bin/bash

[[ -n "$SOPHOS_INSTALL" ]] || SOPHOS_INSTALL=/opt/sophos-spl

function generate_manifest_diff()
{
    local DIST=$1
    local BASE_MANIFEST_DIFF=${SOPHOS_INSTALL}/base/bin/manifestdiff

    if [[ -f $DIST/files/base/bin/manifestdiff ]]
    then
        MANIFEST_DIFF=$DIST/files/base/bin/manifestdiff
    else
        MANIFEST_DIFF=${BASE_MANIFEST_DIFF}
    fi

    #Find all manifest files in DIST
    for F in $(find $DIST -name "*manifest.dat")
    do
        local NEW_MANIFEST=${F}
        local BASE_NAME=$(basename ${F})
        local PROD_NAME=$(dirname ${F} | xargs basename)
        local OLD_MANIFEST_DIR=${SOPHOS_INSTALL}/base/update/$PROD_NAME
        local OLD_MANIFEST=${OLD_MANIFEST_DIR}/${BASE_NAME}
        mkdir -p ${SOPHOS_INSTALL}/tmp/${PROD_NAME}

        echo ${NEW_MANIFEST}
        echo ${OLD_MANIFEST}

        "${MANIFEST_DIFF}" \
            --old="${OLD_MANIFEST}" \
            --new="${NEW_MANIFEST}" \
            --added="${SOPHOS_INSTALL}/tmp/${PROD_NAME}/addedFiles_${BASE_NAME}" \
            --removed="${SOPHOS_INSTALL}/tmp/${PROD_NAME}/removedFiles_${BASE_NAME}" \
            --diff="${SOPHOS_INSTALL}/tmp/${PROD_NAME}/changedFiles_${BASE_NAME}"
            #--new="${NEW_MANIFEST}" \
    done
}

function copy_manifests()
{
    local DIST=$1
    for F in $(find $DIST -name "*manifest.dat")
    do
        local NEW_MANIFEST=${F}
        local BASE_NAME=$(basename ${F})
        local PROD_NAME=$(dirname ${F} | xargs basename)
        local OLD_MANIFEST_DIR=${SOPHOS_INSTALL}/base/update/$PROD_NAME
        mkdir -p ${OLD_MANIFEST_DIR}
        cp ${NEW_MANIFEST} ${OLD_MANIFEST_DIR}
        chmod 600 ${OLD_MANIFEST_DIR}/${BASE_NAME}
    done
}

function changedOrAdded()
{
    local TARGET="$1"
    local DIST="$2"
    for F in $(find $DIST -name "*manifest.dat")
    do
        local NEW_MANIFEST=${F}
        local BASE_NAME=$(basename ${F})
        local PROD_NAME=$(dirname ${F} | xargs basename)
        echo grep -q "^${TARGET}\$" "${SOPHOS_INSTALL}/tmp/${PROD_NAME}/addedFiles_${BASE_NAME}" "${SOPHOS_INSTALL}/tmp/${PROD_NAME}/changedFiles_${BASE_NAME}"
        if [[ $(grep -q "^${TARGET}\$" "${SOPHOS_INSTALL}/tmp/${PROD_NAME}/addedFiles_${BASE_NAME}" "${SOPHOS_INSTALL}/tmp/${PROD_NAME}/changedFiles_${BASE_NAME}" >/dev/null) ]]
        then
            return 1
        fi
    done
    return 0
}

function manifestsChanged()
{
    local DIST=$1
    for F in $(find $DIST -name "*manifest.dat")
    do
        local NEW_MANIFEST=${F}
        local BASE_NAME=$(basename ${F})
        local PROD_NAME=$(dirname ${F} | xargs basename)
        if [ -s "${SOPHOS_INSTALL}/tmp/${PROD_NAME}/addedFiles_${BASE_NAME}" ] || \
           [ -s "${SOPHOS_INSTALL}/tmp/${PROD_NAME}/changedFiles_${BASE_NAME}" ] || \
            [ -s "${SOPHOS_INSTALL}/tmp/${PROD_NAME}/removedFiles_${BASE_NAME}" ]
        then
            return 0
        fi
        done
    return 1
}