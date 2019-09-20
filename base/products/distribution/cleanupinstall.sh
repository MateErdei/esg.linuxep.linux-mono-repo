#!/bin/bash

# Copyright 2019 Sophos Limited.  All rights reserved.

[[ -n "$SOPHOS_INSTALL" ]] || SOPHOS_INSTALL=/opt/sophos-spl

function generate_manifest_diff()
{
    # Function used to generate a set of files based on diff'ing 2 manifest.dat file.
    # These files will be used to determine changes to the product and if there are any files
    # which need to be removed after the upgrade.

    local WORKING_DIST=$1
    local BASE_MANIFEST_DIFF=${SOPHOS_INSTALL}/base/bin/manifestdiff

    if [[ -f $WORKING_DIST/files/base/bin/manifestdiff ]]
    then
        MANIFEST_DIFF=$WORKING_DIST/files/base/bin/manifestdiff
    else
        MANIFEST_DIFF=${BASE_MANIFEST_DIFF}
    fi

    #Find all manifest files in WORKING_DIST
    for CID_MANIFEST_FILE in $(find $WORKING_DIST -name "*manifest.dat")
    do
        local NEW_MANIFEST=${CID_MANIFEST_FILE}
        local BASE_NAME=$(basename ${CID_MANIFEST_FILE})
        local PRODUCT_FOLDER=$(dirname ${CID_MANIFEST_FILE} | xargs basename)
        local OLD_MANIFEST_DIR=${SOPHOS_INSTALL}/base/update/$PRODUCT_FOLDER
        local OLD_MANIFEST=${OLD_MANIFEST_DIR}/${BASE_NAME}
        mkdir -p ${SOPHOS_INSTALL}/tmp/${PRODUCT_FOLDER}

        # handle old manifest file location.
        if [[ ! -f $OLD_MANIFEST ]]
        then
            OLD_MANIFEST=${SOPHOS_INSTALL}/base/update/${BASE_NAME}
        fi

        "${MANIFEST_DIFF}" \
            --old="${OLD_MANIFEST}" \
            --new="${NEW_MANIFEST}" \
            --added="${SOPHOS_INSTALL}/tmp/${PRODUCT_FOLDER}/addedFiles_${BASE_NAME}" \
            --removed="${SOPHOS_INSTALL}/tmp/${PRODUCT_FOLDER}/removedFiles_${BASE_NAME}" \
            --diff="${SOPHOS_INSTALL}/tmp/${PRODUCT_FOLDER}/changedFiles_${BASE_NAME}"
    done
}

function copy_manifests()
{
    # Function will copy manifest into install location which will be diff'ed on the next update / install execution.

    local WORKING_DIST=$1
    local PRODUCT_RIGID_NAME=$2

    for CID_MANIFEST_FILE in $(find $WORKING_DIST -name "*manifest.dat")
    do
        local NEW_MANIFEST=${CID_MANIFEST_FILE}
        local BASE_NAME=$(basename ${CID_MANIFEST_FILE})
        local PRODUCT_FOLDER=$(dirname ${CID_MANIFEST_FILE} | xargs basename)
        local OLD_MANIFEST_DIR=${SOPHOS_INSTALL}/base/update/${PRODUCT_RIGID_NAME}
        mkdir -p ${OLD_MANIFEST_DIR}
        cp ${NEW_MANIFEST} ${OLD_MANIFEST_DIR}
        chmod 600 ${OLD_MANIFEST_DIR}/${BASE_NAME}
    done
}

function changed_or_added()
{
    # Function used to generate a set of files used to determine if the component install has changed any files.

    local TARGET="$1"
    local WORKING_DIST="$2"
    local return_value=1

    for CID_MANIFEST_FILE in $(find $WORKING_DIST -name "*manifest.dat")
    do
        local NEW_MANIFEST=${CID_MANIFEST_FILE}
        local BASE_NAME=$(basename ${CID_MANIFEST_FILE})
        local PRODUCT_FOLDER=$(dirname ${CID_MANIFEST_FILE} | xargs basename)
        grep -q "^${TARGET}\$" "${SOPHOS_INSTALL}/tmp/${PRODUCT_FOLDER}/addedFiles_${BASE_NAME}" "${SOPHOS_INSTALL}/tmp/${PRODUCT_FOLDER}/changedFiles_${BASE_NAME}" >/dev/null

        if [[ $? -eq 0 ]]
        then
            return_value=0
        fi
    done

    return $return_value
}

function software_changed()
{
    # Function used to detect if files are been added changed or removed.

    local WORKING_DIST=$1

    for CID_MANIFEST_FILE in $(find $WORKING_DIST -name "*manifest.dat")
    do
        local NEW_MANIFEST=${CID_MANIFEST_FILE}
        local BASE_NAME=$(basename ${CID_MANIFEST_FILE})
        local PRODUCT_FOLDER=$(dirname ${CID_MANIFEST_FILE} | xargs basename)

        if [ -s "${SOPHOS_INSTALL}/tmp/${PRODUCT_FOLDER}/addedFiles_${BASE_NAME}" ] || \
           [ -s "${SOPHOS_INSTALL}/tmp/${PRODUCT_FOLDER}/changedFiles_${BASE_NAME}" ] || \
            [ -s "${SOPHOS_INSTALL}/tmp/${PRODUCT_FOLDER}/removedFiles_${BASE_NAME}" ]
        then
            return 0
        fi
    done

    return 1
}

function perform_cleanup()
{
    # function used to clean up / remove known files no longer used after upgrade.

    local WORKING_DIST=$1
    # get list of folders to search
    pushd ${SOPHOS_INSTALL}/tmp/
    local DATA_FOLDERS=$(ls -d */)
    popd

    PRODUCT_FOLDER=$(basename ${WORKING_DIST})

    # remove all files listed for component
    # the content of the remove files will have a file path matching the CID not the installer
    # "expr {} : '[^/]*/\(.*\)'" removes the first folder from the given path
    # i.e files/base/etc/logger.conf will become base/etc/logger.conf

    for FILES_TO_REMOVE_DATA_FILE in $(find ${SOPHOS_INSTALL}/tmp/${PRODUCT_FOLDER} -name "removedFiles*")
    do
        for FILES_TO_DELETE in $(cat ${FILES_TO_REMOVE_DATA_FILE} | xargs -ri expr {} : '[^/]*/\(.*\)' | xargs -ri echo ${SOPHOS_INSTALL}/{})
        do
                rm -f ${FILES_TO_DELETE}*
        done

    done

    # clean up all broken symlinks created by deleting installed files
    find ${SOPHOS_INSTALL} -xtype l -delete

    # delete files created by the product after install that are no longer required which are defined in the
    # component filestodelete.dat file.
    # this can include files which we want to force regeneration after upgrade.

    # Note files my no longer exist, so sending output to dev null.

    if [[ -f ${WORKING_DIST}/filestodelete.dat ]]
    then
        for SPECIFIC_FILE_TO_DELETE in $(cat ${WORKING_DIST}/filestodelete.dat)
        do
            if [[ ${SPECIFIC_FILE_TO_DELETE} != */* ]]
            then
                find ${SOPHOS_INSTALL} -name ${SPECIFIC_FILE_TO_DELETE} | xargs -r rm -f >/dev/null
            else
                rm -f ${SOPHOS_INSTALL}/${SPECIFIC_FILE_TO_DELETE} >/dev/null
            fi
        done
    fi

}