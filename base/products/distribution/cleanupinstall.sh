#!/bin/bash

# Copyright 2019 Sophos Limited.  All rights reserved.

[[ -n "$SOPHOS_INSTALL" ]] || SOPHOS_INSTALL=/opt/sophos-spl

function generate_manifest_diff()
{
    # Function used to generate a set of files based on diff'ing 2 manifest.dat file.
    # These files will be used to determine changes to the product and if there are any files
    # which need to be removed after the upgrade.

    local WORKING_DIST="$1"
    local PRODUCT_RIGID_NAME="$2"
    local BASE_MANIFEST_DIFF="${SOPHOS_INSTALL}/base/bin/manifestdiff"

    if [[ -f $WORKING_DIST/files/base/bin/manifestdiff ]]
    then
        MANIFEST_DIFF=$WORKING_DIST/files/base/bin/manifestdiff
    else
        MANIFEST_DIFF=${BASE_MANIFEST_DIFF}
    fi

    #Find all manifest files in WORKING_DIST
    for CID_MANIFEST_FILE in $(find "$WORKING_DIST"/ -name '*manifest.dat')
    do
        local NEW_MANIFEST=${CID_MANIFEST_FILE}
        local BASE_NAME=$(basename ${CID_MANIFEST_FILE})
        local OLD_MANIFEST_DIR=${SOPHOS_INSTALL}/base/update/${PRODUCT_RIGID_NAME}
        local OLD_MANIFEST=${OLD_MANIFEST_DIR}/${BASE_NAME}
        mkdir -p ${SOPHOS_INSTALL}/tmp/${PRODUCT_RIGID_NAME}

        "${MANIFEST_DIFF}" \
            --old="${OLD_MANIFEST}" \
            --new="${NEW_MANIFEST}" \
            --added="${SOPHOS_INSTALL}/tmp/${PRODUCT_RIGID_NAME}/addedFiles_${BASE_NAME}" \
            --removed="${SOPHOS_INSTALL}/tmp/${PRODUCT_RIGID_NAME}/removedFiles_${BASE_NAME}" \
            --diff="${SOPHOS_INSTALL}/tmp/${PRODUCT_RIGID_NAME}/changedFiles_${BASE_NAME}"
    done
}

function copy_manifests()
{
    # Function will copy manifest into install location which will be diff'ed on the next update / install execution.

    local WORKING_DIST="$1"
    local PRODUCT_RIGID_NAME="$2"

    for CID_MANIFEST_FILE in $(find "$WORKING_DIST"/ -name "*manifest.dat")
    do
        local NEW_MANIFEST=${CID_MANIFEST_FILE}
        local BASE_NAME=$(basename ${CID_MANIFEST_FILE})
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
    local PRODUCT_RIGID_NAME="$3"
    local return_value=1

    for CID_MANIFEST_FILE in $(find $WORKING_DIST/ -name "*manifest.dat")
    do
        local NEW_MANIFEST=${CID_MANIFEST_FILE}
        local BASE_NAME=$(basename ${CID_MANIFEST_FILE})
        grep -q "^${TARGET}\$" "${SOPHOS_INSTALL}/tmp/${PRODUCT_RIGID_NAME}/addedFiles_${BASE_NAME}" "${SOPHOS_INSTALL}/tmp/${PRODUCT_RIGID_NAME}/changedFiles_${BASE_NAME}" >/dev/null

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

    local WORKING_DIST="$1"
    local PRODUCT_RIGID_NAME="$2"

    for CID_MANIFEST_FILE in $(find $WORKING_DIST/ -name "*manifest.dat")
    do
        local NEW_MANIFEST=${CID_MANIFEST_FILE}
        local BASE_NAME=$(basename ${CID_MANIFEST_FILE})

        if [ -s "${SOPHOS_INSTALL}/tmp/${PRODUCT_RIGID_NAME}/addedFiles_${BASE_NAME}" ] || \
           [ -s "${SOPHOS_INSTALL}/tmp/${PRODUCT_RIGID_NAME}/changedFiles_${BASE_NAME}" ] || \
            [ -s "${SOPHOS_INSTALL}/tmp/${PRODUCT_RIGID_NAME}/removedFiles_${BASE_NAME}" ]
        then
            return 0
        fi
    done

    return 1
}

function can_delete()
{
   # function to check if a file can be deleted based on the realm allowed file list.
   # ensures the file clean up only occurs within the allowed directory structure.
   # Returns 1 if can delete, any other value should be treated as false.

   local FILE_PATH="$1"
   local WORKING_DIST="$2"

   INCLUDED_PATH=0
   EXCLUDED_PATH=0

   # The cleanup realm file will contain a list of folder paths from the install directory onwards.
   # a + sign in front of the path indecates that the path is in the included list of paths in which files can be
   # deleted from.
   # a - sign in front of the path indecates explicitly that files in that path cannot be deleted.  This is for excluding
   # some subpaths is the update/cache folder.

   if [[ -f ${WORKING_DIST}/cleanuprealm.dat ]]
   then
       for REALM_PATH in $(cat ${WORKING_DIST}/cleanuprealm.dat)
       do
         if [[ ${REALM_PATH} == +* ]]
         then
            if [[ ${FILE_PATH} == ${SOPHOS_INSTALL}/${REALM_PATH:1}* ]]
            then
              INCLUDED_PATH=1
            fi
         elif [[ ${REALM_PATH} == -* ]]
         then
            if [[ ${FILE_PATH} == ${SOPHOS_INSTALL}/${REALM_PATH:1}* ]]
            then
              EXCLUDED_PATH=2
            fi
         else
            #default to prevent deletions for mis-configuration
            EXCLUDED_PATH=2
         fi
       done
   fi

   echo $(( ${INCLUDED_PATH} + ${EXCLUDED_PATH} ))
}


function perform_cleanup()
{
    # function used to clean up / remove known files no longer used after upgrade.

    local WORKING_DIST="$1"
    local PRODUCT_RIGID_NAME="$2"
    # get list of folders to search
    pushd ${SOPHOS_INSTALL}/tmp/
    local DATA_FOLDERS=$(ls -d */)
    popd

    FILES_OR_DIRECTORIES_DELETED=""

    # remove all files listed for component
    # the content of the remove files will have a file path matching the CID not the installer
    # "expr {} : '[^/]*/\(.*\)'" removes the first folder from the given path
    # i.e files/base/etc/logger.conf will become base/etc/logger.conf

    for FILES_TO_REMOVE_DATA_FILE in $(find ${SOPHOS_INSTALL}/tmp/${PRODUCT_RIGID_NAME} -name "removedFiles*")
    do
        for FILE_TO_DELETE in $(cat ${FILES_TO_REMOVE_DATA_FILE} | xargs -ri expr {} : '[^/]*/\(.*\)' | xargs -ri echo ${SOPHOS_INSTALL}/{})
        do
            if [[ $(can_delete ${FILE_TO_DELETE} ${WORKING_DIST}) == 1 ]]
            then
                FILES_OR_DIRECTORIES_DELETED+=", ${FILE_TO_DELETE}"
                rm -f ${FILE_TO_DELETE}*
            fi
        done

    done

    # delete files created by the product after install that are no longer required which are defined in the
    # component filestodelete.dat file.
    # this can include files which we want to force regeneration after upgrade.

    # Note files my no longer exist, so sending output to dev null.
    # if entry contains a path seporator and the path is to a directory the directory will be removed.

    if [[ -f ${WORKING_DIST}/filestodelete.dat ]]
    then
        for SPECIFIC_FILE_TO_DELETE in $(cat ${WORKING_DIST}/filestodelete.dat)
        do
            if [[ ${SPECIFIC_FILE_TO_DELETE} != */* ]]
            then
                for FILE_FOUND in $(find ${SOPHOS_INSTALL} -name ${SPECIFIC_FILE_TO_DELETE})
                do
                    if [[ $(can_delete ${FILE_FOUND} ${WORKING_DIST}) == 1 ]]
                    then
                        FILES_OR_DIRECTORIES_DELETED+=", ${FILE_FOUND}"
                        rm -f ${FILE_FOUND} >/dev/null
                    fi
                done
            else
                if [[ $(can_delete ${SOPHOS_INSTALL}/${SPECIFIC_FILE_TO_DELETE} ${WORKING_DIST}) == 1 ]]
                then
                    if [[ -d ${SOPHOS_INSTALL}/${SPECIFIC_FILE_TO_DELETE} ]] || [[ -f ${SOPHOS_INSTALL}/${SPECIFIC_FILE_TO_DELETE} ]]
                    then
                        FILES_OR_DIRECTORIES_DELETED+=", ${SOPHOS_INSTALL}/${SPECIFIC_FILE_TO_DELETE}"
                        rm -rf ${SOPHOS_INSTALL}/${SPECIFIC_FILE_TO_DELETE} >/dev/null
                    fi
                fi
            fi
        done
    fi

    # clean up all broken symlinks created by deleting installed files
    for dirname in base plugins bin; do
        find ${SOPHOS_INSTALL}/${dirname} -xtype l -delete
    done
    
    if [[ ! -z "${FILES_OR_DIRECTORIES_DELETED}" ]]
    then
      echo "List of files or directories removed during upgrade${FILES_OR_DIRECTORIES_DELETED}"
    fi
}