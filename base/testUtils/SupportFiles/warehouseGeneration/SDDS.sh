#!/bin/bash

source common.sh
source replacements.sh
source AUCommon.sh


function failMessage()
{
    echo FAILURE: ${@}
}

## Test failed setting up its environment
function setupFailure()
{
    failure 11 "$@"
}

## Test operation failed or assertion was incorrect
function testFailure()
{
    failure 1 "$@"
}

# cleanup, display a failure message, and exit with a specific exit code.
# @param EXITCODE    exit code for the test
# @param "$@"    Failure message
# @param "$@"    Failure message
function failure()
{
    local EXITCODE=$1
    shift

    failMessage "$@"

    ## break out of any subshells (Bash 3.0+ only)
    if (( $BASH_SUBSHELL >= 1 ))
    then
        #echo $EXITCODE > $TMPROOT/.failcode
        #echo "$@" > $TMPROOT/.failtext
        kill -USR1 $$
    fi

    exit ${EXITCODE}
}

function runPython()
{
    announceFunction $FUNCNAME "$@"
    python  "$@"
    local RETCODE=$?
    return $RETCODE
}

function _SDDS_sh_init()
{
    announceFunction $FUNCNAME "$@"

    local updateFromSophos=$1

    __sdds_latest_download_flag_file="downloadedbylatest"
    __sdds_recommended_download_flag_file="downloadedbyrecommended"
    __sdds_conservative_download_flag_file="downloadedbyconservative"
    __sdds_hibernate_download_flag_file="downloadedbyhibernate"
    __sdds_fixed_download_flag_file="downloadedbyfixed"
    __sdds_previous_download_flag_file="downloadedbyprevious"
    __sdds_oldest_download_flag_file="downloadedbyoldest"
    __sdds_resubscription_download_flag_file="downloadedbyresubscription"
    __sdds_retired_download_flag_file="downloadedbyretired"
    __sdds_not_retired_download_flag_file="downloadedbynotretired"
    __sdds_oldest2_download_flag_file="downloadedbyoldest2"
    __sdds_previous2_download_flag_file="downloadedbyprevious2"
    __sdds_recommended2_download_flag_file="downloadedbyrecommended2"
    __sdds_preview_download_flag_file="downloadedbypreview"
    __sdds_extended_download_flag_file="downloadedbyextended"
    __sdds_extended_previous_download_flag_file="downloadedbyextendedprevious"

    # SDDS Tool Stuff
    sdds_tool="runPython ./SDDS-WH-GEN-V-2.0/sdds_wh_gen_main.py"
    sdds_dictionary_files="./SDDS-WH-GEN-V-2.0/DictionaryFiles"
    sdds_config_templates="./SDDS-WH-GEN-V-2.0/Templates"

    sdds_sophos_customer_file_port=1233

    if [[ ${updateFromSophos} == "1" ]]
    then
        sdds_sophos_main_port=1234
        sdds_sophos_main_url="http://localhost:${sdds_sophos_main_port}"
    else
        # create update cache
        sdds_sophos_main_port=1236
        sdds_sophos_main_url="http://localhost:${sdds_sophos_main_port}/sophos/warehouse"
    fi

    sdds_sophos_customer_file_url="http://localhost:${sdds_sophos_customer_file_port}/"


    # customer file variables
    sdds_sophos_timestamp="2013-01-01T00:00:00Z"

    [[ -n ${REGRESSIONUSERNAME} ]] || export REGRESSIONUSERNAME=regruser
    DEFAULT_REGRESSIONUSERNAME=$REGRESSIONUSERNAME
    [[ -n ${REGRESSIONPASSWORD} ]] || export REGRESSIONPASSWORD=regrABC123pass
    #export REGRESSIONPREHASHED=$(echo -n "$REGRESSIONUSERNAME:$REGRESSIONPASSWORD" | md5sum | cut -f1 -d' ')
    [[ -n ${REGRESSIONPREHASHED} ]] || export REGRESSIONPREHASHED="9539d7d1f36a71bbac1259db9e868231"

    SED=$(which sed)
    if [[ $? == 1 ]]
    then
        setupFailure "No sed installed"
    fi
    echo "Using sed: $SED"
}

#------------------------------------------------------------------------------
#
# Require Functions
#
#------------------------------------------------------------------------------
function requireSophosWarehousesHttps()
{
    announceFunction $FUNCNAME "$@"

    local warehouse_name=$1
    local updateFromSophos=$2


    _SDDS_sh_init ${updateFromSophos}

    #XX_SDDS_MAIN_RIGID_NAME=$2

    # Used as the home folder
    XX_INSTALL_PATH=${HOME_FOLDER:-"sophos-spl"}

    local opensslBin=$(readlink -f "./bin/openssl")
    local opensslLdLibraryPath=$(readlink -f "./lib")
    export TEST_OPENSSL=${opensslBin}
    export OPENSSL_LD_LIBRARY_PATH=${opensslLdLibraryPath}

    #sdds_customer_files="${directory}/customer_files"

    generateSophosWarehouse ${warehouse_name}
}

function getRigidNameVersion()
{
    announceFunction $FUNCNAME "$@"

    local rigidname=$1
    local __version=$2

    # versions are purely for testing and should not need to be exact.
    # however the first version digit (base version) must match with up with the
    # base version defined in the config.json file
    # created via SulDownloader.py

    if [[ $rigidname == "SSPL-RIGIDNAME" ]]
    then
        eval ${__version}="0.5.0"
    elif [[ $rigid_name = *"ServerProtectionLinux-Base"* ]]
    then
        eval ${__version}="0.5.0"
    elif [[ $rigid_name = *"ServerProtectionLinux-Plugin"* ]]
    then
        eval ${__version}="0.0.1"
    elif [[ $rigidname == "SSPL-RIGIDNAME-2" ]]
    then
        eval ${__version}="0.5.0"
    elif [[ $rigidname == "PLUGIN-1" ]]
    then
        eval ${__version}="0.0.1"
    else
        eval ${__version}="0.5.0"
    fi
}


function SDDS_createMainPackageSDDSImportFile()
{

    announceFunction $FUNCNAME "$@"

    local sdds_main_import_dir=$1/$2
    local rigidname=$2

    getRigidNameVersion ${rigidname} version

    declare main_version=${version}
    declare main_decode_path


    createSDDSImportFile "$main_version" "$XX_INSTALL_PATH"

}

function _SDDSTool_add_componentSuite()
{
    announceFunction $FUNCNAME "$@"

    declare config_path=$1
    declare release_tags=$2
    declare component_suite_name=$3

    declare -a component_suite_record_to_process

    get_component_suite ${component_suite_name}

    IFS=, read -a component_suite_config <<< ${component_suite_record_to_process[@]}

    declare component_id="Main${_XX_warehouse_instance_counter}Package"

    component_suite_import=${component_suite_config[4]}/${component_suite_config[2]}/sdds/SDDS-Import.xml

    [[ -n $MAJOR_VERSION ]] || MAJOR_VERSION=3
    [[ -n $MINOR_VERSION ]] || MINOR_VERSION=2
    [[ -n $TIMESTAMP ]] || TIMESTAMP=
    [[ -n $LIFE_STAGE ]] || LIFE_STAGE=APPROVED

    [[ -n $SUITE_MAJOR_VERSION ]] || SUITE_MAJOR_VERSION=$MAJOR_VERSION
    [[ -n $SUITE_MINOR_VERSION ]] || SUITE_MINOR_VERSION=$MINOR_VERSION
    [[ -n $SUITE_TIMESTAMP ]] || SUITE_TIMESTAMP=$TIMESTAMP
    [[ -n $SUITE_LIFE_STAGE ]] || SUITE_LIFE_STAGE=$LIFE_STAGE

    [[ -n $COMPONENT_MAJOR_VERSION ]] || COMPONENT_MAJOR_VERSION=$MAJOR_VERSION
    [[ -n $COMPONENT_MINOR_VERSION ]] || COMPONENT_MINOR_VERSION=$MINOR_VERSION
    [[ -n $COMPONENT_TIMESTAMP ]] || COMPONENT_TIMESTAMP=$TIMESTAMP
    [[ -n $COMPONENT_LIFE_STAGE ]] || COMPONENT_LIFE_STAGE=$LIFE_STAGE

    declare tmp_instance_config_path=${TESTTMP}/${component_id}.tmp
    cp "$sdds_config_templates/template_product_instance.xml" "$tmp_instance_config_path" \
        || setupFailure "Unable to copy $sdds_config_templates/template_product_instance.xml to $tmp_instance_config_path"

    _SDDSTool_modify_component_tags "$tmp_instance_config_path" "$component_id" \
            "$component_suite_import" \
            "$SUITE_MAJOR_VERSION" "$SUITE_MINOR_VERSION" "$SUITE_TIMESTAMP" "$SUITE_LIFE_STAGE"

    _replaceStringInFile $tmp_instance_config_path "<!-- DecodePath -->" "."

    local release_tag
    local tag base
    while [[ -n $release_tags ]]
    do
        release_tag=${release_tags%%|*}
        release_tags=${release_tags#$release_tag}
        release_tags=${release_tags#|}
        IFS='#' read tag base <<EOF
$release_tag
EOF
        if [[ -n $base ]]
        then
            echo "DEBUG:: +++ Inserting TAG $tag $base"
            _insertStringIntoFile "$tmp_instance_config_path" "<!-- ReleaseTag -->" "<ReleaseTag><Tag>$tag</Tag><Base>$base</Base></ReleaseTag>"
        else
            echo "DEBUG:: +++ Inserting TAG $tag NOBASE"
            _insertStringIntoFile "$tmp_instance_config_path" "<!-- ReleaseTag -->" "<ReleaseTag><Tag>$tag</Tag></ReleaseTag>"
        fi
    done

    declare -a list_of_components

    get_components_for_parent ${component_suite_config[2]}
    echo "List of Components for Component suite: ''${component_suite_config[2]}'', Components : '${list_of_components[@]}' "

    for sub_component in ${list_of_components[@]}
    do
        IFS=, read -a component_config <<< ${sub_component[@]}

        declare warehouse_name=${component_config[0]}
        declare component_parent=${component_config[1]}
        declare rigidname=${component_config[2]}
        declare source_directory=${component_config[3]}
        declare target_directory=${component_config[4]}
        declare product_type=${component_config[5]}

        product_component_id=${component_id}-COMPONENT

        declare product_tmp_instance_config_path=$TESTTMP/${product_component_id}.tmp

        cp "$sdds_config_templates/template_product_instance.xml" "${product_tmp_instance_config_path}" \
            || setupFailure "Unable to copy ${sdds_config_templates}/template_product_instance.xml to ${product_tmp_instance_config_path}"

        _SDDSTool_modify_component_tags "$product_tmp_instance_config_path" "$product_component_id" \
                "${component_config[4]}/${component_config[2]}/sdds/SDDS-Import.xml" \
                "$COMPONENT_MAJOR_VERSION" "$COMPONENT_MINOR_VERSION" "$COMPONENT_TIMESTAMP" "$COMPONENT_LIFE_STAGE"

        _replaceStringInFile $product_tmp_instance_config_path "<!-- Component -->" ""

        _replaceStringInFile $product_tmp_instance_config_path "<!-- DecodePath -->" "."

        _insertFileIntoFile $tmp_instance_config_path "<!-- Component -->" $product_tmp_instance_config_path

        rm $product_tmp_instance_config_path

    done

     #insert component suite instance.
     _insertFileIntoFile $config_path "<!-- ProductInstance -->" $tmp_instance_config_path

    rm "$tmp_instance_config_path"
    _XX_warehouse_instance_counter=$(( $_XX_warehouse_instance_counter + 1 ))

}


function requireSDDSPackages()
{
    announceFunction $FUNCNAME "$@"

    local sdds_root=$1
    CreateSharedCID $sdds_root

    echo "---------Creating main import file..."
    SDDS_createMainPackageSDDSImportFile "$sdds_root"
}

#------------------------------------------------------------------------------
#
# HighLevel Generate Warehouses
#
#------------------------------------------------------------------------------

function copy_customer_files_to_update_cache_directory()
{
    local directory=$1
    local update_cache_customer_files=${directory}/warehouse_root/sophos/customer
    mkdir -p "$update_cache_customer_files"

    if [[ -d ${directory}/customer_files ]]
    then
        cp -r ${directory}/customer_files/* ${update_cache_customer_files} || true
    fi
}

# Generates Update Cache "Warehouse".
function generateUpdateCacheWarehouse()
{
    announceFunction $FUNCNAME "$@"

    local directory=$1
    local warehouse_name=$2

    updateFromSophos="0"

    requireSophosWarehousesHttps ${warehouse_name} ${updateFromSophos}

    local update_cache_warehouse=${directory}/warehouse_root/sophos/warehouse
    mkdir -p "$update_cache_warehouse"
    cp -r ${directory}/warehouses/sophosmain/* ${update_cache_warehouse}

    copy_customer_files_to_update_cache_directory "${directory}"
}

# Generates default Sophos Warehouses.
function generateSophosWarehouse()
{
    announceFunction $FUNCNAME "$@"

    local warehouse_name=$1

    declare -a list_of_all_warehouse_components

    get_all_components_for_warehouse  ${warehouse_name}

    declare -a sdds_import_component

    for component_record in ${list_of_all_warehouse_components[@]}
    do
        sdds_import_component=${component_record[@]}

        IFS=, read -a component_config_values <<< ${component_record[@]}

        local rigidName=${component_config_values[2]}
        local directory=${component_config_values[4]}

        local sdds="$directory/$rigidName/sdds"
        mkdir -p "$sdds"

        CreateSharedCID ${directory}/$rigidName

        if [[ "${CORRUPTINSTALL}" == "yes" ]]
        then
            chmod +w ${sdds}/install.sh
            echo "CORRUPTING install.sh! CORRUPTINSTALL==yes"
            echo "#!/bin/bash\necho THIS IS BAD ;\nexit 77" >${sdds}/install.sh
        fi

        SDDS_createMainPackageSDDSImportFile ${directory}  $rigidName

    done

    _generateSophosWarehouse ${directory} ${warehouse_name}

}


function _generateSophosWarehouse()
{
    announceFunction $FUNCNAME "$@"

    local directory=$1
    local SDDS_WAREHOUSE_NAME=$2


    local ROOT_SDDS_WAREHOUSE_NAME=${SDDS_WAREHOUSE_NAME}

    [[ -n "$SDDS_WAREHOUSE_NAME" ]] && SDDS_WAREHOUSE_NAME=sdds.$SDDS_WAREHOUSE_NAME

    [[ -n "$SDDS_WAREHOUSE_NAME" ]] || SDDS_WAREHOUSE_NAME=sdds.live

    ## Also create the default DCI if we aren't doing that anyway
    if [[ "$REGRESSIONUSERNAME" != "$DEFAULT_REGRESSIONUSERNAME" ]]
    then
        echo "CREATING SSDS customer file probs not needed as we have it above."
        (
            REGRESSIONUSERNAME=$DEFAULT_REGRESSIONUSERNAME
            createCustomerFile "$sdds_sophos_main_url" "sdds.$warehouse_name"
        )
    fi

    _new_warehouse "$SDDS_WAREHOUSE_NAME" "$sdds_sophos_main_warehouse" "$sdds_sophos_main_url" $directory

    declare -a list_of_all_top_level_warehouse_components
    get_all_top_level_components_for_warehouse ${ROOT_SDDS_WAREHOUSE_NAME}

    echo "Top Level Components: ${list_of_all_top_level_warehouse_components[@]}"

    for component_record in ${list_of_all_top_level_warehouse_components[@]}
    do
        IFS=, read -a component_config  <<< ${component_record[@]}

        SDDS_COMPONENT_SUITE_CONFIG=$_XX_warehouse_main_config_file

        [[ -f "$SDDS_COMPONENT_SUITE_CONFIG" ]] || setupFailure "Failed to create $SDDS_COMPONENT_SUITE_CONFIG"

        if [[ ${component_config[5]} == "true" ]]
        then
            _SDDSTool_add_componentSuite "$SDDS_COMPONENT_SUITE_CONFIG" "RECOMMENDED#${DIST_MAJOR}" ${component_config[2]}
        else
            [[ -n "$DIST_MAJOR" ]] || DIST_MAJOR=0
            if [[ -n "$RemoveBaseVersion" ]] ; then
                _add_component_instance_to_warehouse "" "RECOMMENDED" "$directory/${component_config[2]}"
            else
                _add_component_instance_to_warehouse "" "RECOMMENDED#${DIST_MAJOR}" "$directory/${component_config[2]}"
            fi
        fi

    done

    if [[ -n $XX_SHA384_ERRORS ]]
    then
        _insertStringIntoFile "$_XX_warehouse_main_config_file" "<!-- DeliberateError -->" "$XX_SHA384_ERRORS"
    fi

    _publish_warehouse $directory
}




#------------------------------------------------------------------------------
#
# LowerLevel Generate Warehouses
#
#------------------------------------------------------------------------------

# Global variables to store data between functions calls
_XX_warehouse_instance_counter=

_XX_warehouse_main_name=
_XX_warehouse_main_url=
_XX_warehouse_main_config_file=
_XX_warehouse_main_output=

function _new_warehouse()
{
    announceFunction $FUNCNAME "$@"

    _XX_warehouse_main_name=$1
    _XX_warehouse_main_output=${2:-$sdds_sum_warehouse}
    _XX_warehouse_main_url=$3

    local directory=$4

    _createMainWarehouse "$_XX_warehouse_main_name" $directory

}

function _createMainWarehouse()
{

    announceFunction $FUNCNAME "$@"
    _XX_warehouse_main_name=$1

    local directory=$2

    [[ -z $_XX_warehouse_main_name ]] \
        && setupFailure "_new_warehouse called without a warehouse name"

    # Remove possible trailing slashes
    while [[ "${_XX_warehouse_main_output%/}" != "${_XX_warehouse_main_output}" ]]
    do
        _XX_warehouse_main_output=${_XX_warehouse_main_output%/}
    done

    _XX_warehouse_instance_counter=0
    _XX_warehouse_main_config_file="$directory/sdds/mainconfig.xml"
    _SDDSTool_create_config "$_XX_warehouse_main_config_file"
}

function _add_component_instance_to_warehouse()
{
    announceFunction $FUNCNAME "$@"

    declare main_version=$1
    declare main_release_tag=$2
    local cidDir=$3

    declare main_import_dir="$cidDir/sdds"

    [[ -f "$main_import_dir/SDDS-Import.xml" ]] \
        || echo ${main_import_dir/SDDS-Import.xml} doesnt exits

    declare SDDSImport_main_package="$main_import_dir/SDDS-Import${_XX_warehouse_instance_counter}.xml"

    cp "$main_import_dir/SDDS-Import.xml" "$SDDSImport_main_package"

    if [[ -n $main_version ]]
    then
        _replaceStringInFile "$SDDSImport_main_package" "<Version>.*</Version>" "<Version>$main_version</Version>"
    fi

    declare instance_package_name="Main${_XX_warehouse_instance_counter}Package"

    _SDDSTool_add_product_instance "$_XX_warehouse_main_config_file" "$instance_package_name" \
        "$SDDSImport_main_package" "$(( $_XX_warehouse_instance_counter + 3 ))" "2"  "" "$main_release_tag" "" "" "$SDDS_RESUBSCRIPTION" "$SDDS_LIFESTAGE"

    _XX_warehouse_instance_counter=$(( $_XX_warehouse_instance_counter + 1 ))
}

function _publish_warehouse()
{
    announceFunction $FUNCNAME "$@"
    local directory=$1
    _publish_main_warehouse $directory
}


function _publish_main_warehouse()
{

    announceFunction $FUNCNAME "$@"
    local directory=$1

    local output="$directory/warehouses/sophosmain"

    _SDDSTool_publish_config "$_XX_warehouse_main_config_file" "$output" "$_XX_warehouse_main_name"
}


#------------------------------------------------------------------------------
#
# Customer Files
#
#------------------------------------------------------------------------------

# Creates a customer file
#
# @param address(es)    Warehouse URL to redirect to
# @param catalogue      Warehouse name to redirect to (e.g. sdds.live)
function createOldCustomerFile()
{
    local addresses
    addresses=( $1 )
    declare catalogue=$2

    announceFunction $FUNCNAME "$@"

    #   For now if required override globals in subshell then call function :-)
    local _username=$REGRESSIONUSERNAME
    local _password=$REGRESSIONPASSWORD
    local _destination=$sdds_customer_files
    local _timestamp=$sdds_sophos_timestamp

    runPython "./createCustomerFile.py" \
            --destination "$_destination" \
            ${addresses[@]/#/--address } \
            --catalogue "$catalogue" \
            --username "$_username" \
            --password "$_password" \
            --timestamp "$_timestamp" \
            --redirect "$REDIRECT" \
        || setupFailure "${FUNCNAME}(): failed to create customer file"


    safermrf "$_destination/update"
    ln -s . "$_destination/update"
}

# Creates a new customer file with multiple warehouses
#
# @param address(es)    Warehouse URL to redirect to
# @param warehouse(s)   Warehouse name to redirect to (e.g. sdds.linux)

function createNewMultiWarehouseCustomerFile()
{
    _SDDS_sh_init "1"

    local target_directory=$1
    shift
    local main_url=$1
    shift
    local warehouses=$@

    local opensslBin=$(readlink -f "./bin/openssl")
    local opensslLdLibraryPath=$(readlink -f "./lib")
    export TEST_OPENSSL=${opensslBin}
    export OPENSSL_LD_LIBRARY_PATH=${opensslLdLibraryPath}

    sdds_customer_files="${target_directory}/customer_files"

    SSLPRIVATEKEY=./certs/testkey.pem
    COMBINED_CERTIFICATE=./certs/combined.crt

    if [[ ${main_url} = "defaultUrl" ]]
    then
        createNewCustomerFile ${sdds_sophos_main_url} ${target_directory} ${warehouses[@]}
    else
        createNewCustomerFile ${main_url} ${target_directory} ${warehouses[@]}
    fi
}

function createNewMultiWarehouseCustomerFileForUpdateCache()
{
    announceFunction $FUNCNAME "$@"

    local target_directory=$1
    shift
    local warehouses=$@

    sdds_sophos_customer_file_url="http://dci.sophosupd.com/cloudupdate http://dci.sophosupd.net/cloudupdate"
    sdds_sophos_main_url="http://d1.sophosupd.com/update"

    # Note update-C refers to A,B,C releases.  Either one is fine to use as it has not specific meaning for testing
    REDIRECT="d2.sophosupd.net/update=d2.sophosupd.net/update-C"
    REDIRECT="${REDIRECT};d2.sophosupd.com/update=d2.sophosupd.com/update-C"
    REDIRECT="${REDIRECT};d3.sophosupd.net/update=d3.sophosupd.net/update-C"
    REDIRECT="${REDIRECT};d3.sophosupd.com/update=d3.sophosupd.com/update-C"

    createNewMultiWarehouseCustomerFile ${target_directory} ${sdds_sophos_main_url} ${warehouses[@]}

    copy_customer_files_to_update_cache_directory "${target_directory}"
}

# Creates a new customer file
#
# @param address(es)    Warehouse URL to redirect to
# @param warehouse(s)   Warehouse name to redirect to (e.g. sdds.linux)
function createNewCustomerFile()
{
    announceFunction $FUNCNAME "$@"

    local addresses
    addresses=( $1 )
    shift
    local directory=$1
    shift
    local warehouses
    warehouses=( $@ )

    #   For now if required override globals in subshell then call function :-)
    local _username=${REGRESSIONUSERNAME}
    local _password=${REGRESSIONPASSWORD}



    # NOTE this was one of the global wars that was removed - the lcoation now may be wrong....
    #local _destination=$sdds_customer_files

    local _destination="$directory/customer_files"
    local _timestamp=${sdds_sophos_timestamp}

    ## Private key for bottom level certificate
    local key=$SSLPRIVATEKEY
    local passphrase=$(_keyPassword)
    ## Combined bottom and intermediate level certificates
    local cert=${COMBINED_CERTIFICATE}

    OPENSSL_LD_LIBRARY_PATH=./lib:${LD_LIBRARY_PATH} \
        OPENSSL_PATH=${TEST_OPENSSL} \
        OPENSSL_CONF=./openssl.cnf \
        runPython "./createCustomerFile.py" \
            --destination "$_destination" \
            ${addresses[@]/#/--address } \
            ${warehouses[@]/#/--warehouse } \
            --username "$_username" \
            --password "$_password" \
            --timestamp "$_timestamp" \
            --signing-keyfile "$key" \
            --signing-passphrase "$passphrase" \
            --signing-certificate "$cert" \
            --redirect "$REDIRECT" \
            $EXTRA_CUSTOMER_FILE_OPTIONS \
        || setupFailure "${FUNCNAME}(): failed to create customer file"
}

# Creates a customer file
#
# @param address        Warehouse URL to redirect to
# @param catalogue      Warehouse name to redirect to (e.g. sdds.live)
function createCustomerFile()
{
    announceFunction $FUNCNAME "$@"

    createOldCustomerFile "$@"
    createNewCustomerFile "$@"
}


#------------------------------------------------------------------------------
#
# SDDS Tool Helper Functions
#
#------------------------------------------------------------------------------

# Find and replace a string in a file
# This function is in case there is a more efficient way of find/replace than using sed.
# Also it can be adapted for incase sed isn't cross-platform
# @param file_path:     File path
# @param token:         String to find
# @param replacement:   String to insert
function _replaceStringInFile()
{

    announceFunction $FUNCNAME "$@"

    declare file_path=$1
    declare token=$2
    declare replacement=$3
    declare tmp_file_path=$1.tmp

    declare -x LC_ALL=C
    [[ -s "$file_path" ]] || setupFailure "_replaceStringInFile $* input file is empty"
    ## echo is there to ensure a new-line at the end of $file_path contents, as
    echo "$(<$file_path)" | $SED "s^$token^$replacement^g"  >"$tmp_file_path" \
        || setupFailure "_replaceStringInFile $* sed s^$token^$replacement^g $file_path has failed: $?"
    [[ -s "$tmp_file_path" ]] || setupFailure "_replaceStringInFile $* has produced an empty file"
    rm -f "$file_path"
    mv "$tmp_file_path" "$file_path"
}

# Insert file contents into a file at marker point
# This function is in case there is a more efficient way of find/replace than using sed.
# Also it can be adapted for incase sed isn't cross-platform
# @param file_path:         File path
# @param marker:            Marker point
# @param insert_file_path:  File to insert
function _insertFileIntoFile()
{
    declare file_path=$1
    declare marker=$2
    declare insert_file_path=$3
    declare tmp_file_path=$file_path.tmp

    declare -x LC_ALL=C
    $SED -e "/$marker/r $insert_file_path" "$file_path" >"$tmp_file_path" \
        || setupFailure "_insertFileIntoFile $* sed has failed: $?"
    [[ -s "$tmp_file_path" ]] \
        || setupFailure "_insertFileIntoFile $* has produced an empty file"
    rm -f "$file_path"
    mv "$tmp_file_path" "$file_path"
}

# Insert a string into a file at marker point
# This function is in case there is a more efficient way of find/replace than using sed.
# Also it can be adapted for incase sed isn't cross-platform
# @param file_path: File path
# @param marker:    Marker point
# @param string:    String to insert
function _insertStringIntoFile()
{

    announceFunction $FUNCNAME "$@"

    declare file_path=$1
    declare marker=$2
    declare string=$3
    declare tmp_file_path=$file_path.tmp

    declare -x LC_ALL=C

    $SED -e "/$marker/a\\
$string" "$file_path" >"$tmp_file_path" || setupFailure "Unable to insert string into $file_path"
    [[ -s "$tmp_file_path" ]] || setupFailure "_insertStringIntoFile $* has produced an empty file"
    rm -f "$file_path"
    mv "$tmp_file_path" "$file_path"
}

# Creates a new config file for use with the SDDS warehouse generation tool
# @param config_path:   File path to save the config file
function _SDDSTool_create_config()
{

    announceFunction $FUNCNAME "$@"

    declare config_path=$1
    declare config_dir=${config_path%/*}
    [[ -d $config_dir ]] \
        || mkdir -p "$config_dir" \
        || setupFailure "Could not create parent directory for config_path: $config_path"
    cp "$sdds_config_templates/template_config.xml" "$config_path"
    local dictionary_file

    for dictionary_file in $(find "$sdds_dictionary_files" -name "*.xml")
    do
        _insertStringIntoFile "$config_path" "<!-- DictionaryFile -->" "<DictionaryFile>$dictionary_file</DictionaryFile>"
    done
}

# Creates a SDDS warehouse
# @param config_path            File path to SDDS tool config file
# @param warehouse_location     Output directory for the warehouse
# @param warehouse_name         Warehouse name (e.g. sdds.live)
function _SDDSTool_publish_config()
{
    announceFunction $FUNCNAME "$@"
    declare config_path=$1
    declare warehouse_location=$2
    declare warehouse_name=$3

    [[ -d "$warehouse_location" ]] || mkdir -p "$warehouse_location"
    (
        #cd "$sdds_root"
        #cd "$sdds"
        export OPENSSL_LD_LIBRARY_PATH=./lib:${LD_LIBRARY_PATH}
#        export OPENSSL_PATH=$(which openssl)
        export OPENSSL_CONF=./openssl.cnf
        ${sdds_tool} "$warehouse_location" "$warehouse_name" "$config_path"
    ) || setupFailure "Unable to _SDDSTool_publish_config $*"
}

# Adds a new product instance to the SDDS tool config file.
# @param 1  config_path            File path to SDDS tool config file
# @param 2  component_id           Unique ID to identify this product instance
#                               (allows you to add supplements/resubscriptions etc.) the correct product.
#                               Note: this is not a required field of the SDDS tool, but used by functions below.
# @param 3  component_import       File path to the SDDSImport file for this product instance.
# @param 4  major_release          Major release [optional]
# @param 5  minor_release          Minor Release [optional]
# @param 6  time_stamp             Time stamp [optional]
# @param 7  release_tags           Release tags (multiple release tags can be specified separated by |)
#                               e.g. LATEST|LATEST#9|RECOMMENDED#9
# @param 8  feature_suppressions   Features to suppress from the SDDSImport file
#                               (can specify multiple features separated by |) e.g. SAU|RMS
# @param 9  platform_suppressions  Platforms to suppress from the SDDSImport file
#                               (can specify multiple features separated by |) e.g. WINXP|MACOS
# @param 10 resubscriptions        Resubscription information - Line,version,baseversion separated by |
# @param 11 lifestage              LifeStage for the product instance
# The last feature_suppressions and platform_suppressions are not used by the linux product (currently)
function _SDDSTool_add_product_instance()
{

    announceFunction $FUNCNAME "$@"

    declare config_path=$1
    declare component_id=$2
    declare release_tags=$7
    declare feature_suppressions=$8
    declare platform_suppressions=$9
    local resubscriptions=${10}
    local lifestage=${11}

    echo ">>>> Config_path [$config_path] import file [$3] args $1 $2 $3 $4 $5 $6 $7 $8 $9 $10 $11"

    [[ -f "$config_path" ]] || setupFailure "_SDDSTool_add_product_instance: config_path doesn't exist: $config_path"
    [[ -f "$3" ]] || setupFailure "_SDDSTool_add_product_instance: component_import doesn't exist: $3"

    declare tmp_instance_config_path=./${component_id}.tmp
    cp "$sdds_config_templates/template_product_instance.xml" "$tmp_instance_config_path" \
        || setupFailure "Unable to copy $sdds_config_templates/template_product_instance.xml to $tmp_instance_config_path"

    _SDDSTool_modify_component_tags "$tmp_instance_config_path" "$2" "$3" "$4" "$5" "$6" "${lifestage}"

    local release_tag
    local tag base
    while [[ -n ${release_tags} ]]
    do
        release_tag=${release_tags##*|}
        release_tags=${release_tags%$release_tag}
        release_tags=${release_tags%|}
        IFS='#' read tag base <<EOF
${release_tag}
EOF
        if [[ -n $base ]]
        then
            echo "DEBUG:: +++ Inserting TAG $tag $base"
            _insertStringIntoFile "$tmp_instance_config_path" "<!-- ReleaseTag -->" "<ReleaseTag><Tag>$tag</Tag><Base>$base</Base></ReleaseTag>"
        else
            echo "DEBUG:: +++ Inserting TAG $tag NOBASE"
            _insertStringIntoFile "$tmp_instance_config_path" "<!-- ReleaseTag -->" "<ReleaseTag><Tag>$tag</Tag></ReleaseTag>"
        fi
    done

    local resubscription
    local line version baseversion
    while [[ -n "$resubscriptions" ]]
    do
        resubscription=${resubscriptions##*|}
        resubscriptions=${resubscriptions%$resubscription}
        resubscriptions=${resubscriptions%|}
        IFS=',' read line version baseversion <<EOF
$resubscription
EOF
        echo "DEBUG:: +++ Inserting RESUB $line $version $baseversion"
        _insertStringIntoFile "$tmp_instance_config_path" "<!-- $component_id-Resubscription -->" \
            "<Resubscription><Line>$line</Line><Version>$version</Version><BaseVersion>$baseversion</BaseVersion></Resubscription>"

        #~ cat "$tmp_instance_config_path"
    done

    local feature_suppression
    while [[ -n ${feature_suppressions} ]]
    do
        feature_suppression=${feature_suppressions##*|}
        feature_suppressions=${feature_suppressions%$feature_suppression}
        feature_suppressions=${feature_suppressions%|}
        _insertStringIntoFile "$tmp_instance_config_path" "<!-- Feature -->" "<Feature>$feature_suppression</Feature>"
    done

    local platform_suppression
    while [[ -n ${platform_suppressions} ]]
    do
        platform_suppression=${platform_suppressions##*|}
        platform_suppressions=${platform_suppressions%$platform_suppression}
        platform_suppressions=${platform_suppressions%|}
        _insertStringIntoFile "$tmp_instance_config_path" "<!-- Platform -->" "<Platform>$platform_suppression</Platform>"
    done

    _insertFileIntoFile ${config_path} "<!-- ProductInstance -->" ${tmp_instance_config_path}
    rm "$tmp_instance_config_path"
}

# These config file operations will be used by both _SDDSTool_add_product_instance and _SDDSTool_add_component
# (they have the same set of XML tags)
#
# @param 1 config_path        File path to SDDS tool config file
# @param 2 component_id       Unique ID to identify the product instance/component
# @param 3 component_import   File path to the SDDSImport file for this product instance.
# @param 4 major_release      Major release [optional]
# @param 5 minor_release      Minor Release [optional]
# @param 6 time_stamp         Time stamp [optional]
# @param 7 lifestage          Life stage [optional]
function _SDDSTool_modify_component_tags()
{
    declare component_config_path=$1
    declare component_id=$2
    declare component_import="$3"

    [[ -n $4 ]] && declare major_release="<MajorRelease>${4}</MajorRelease>" || declare major_release=""
    [[ -n $5 ]] && declare minor_release="<MinorRelease>${5}</MinorRelease>" || declare minor_release=""
    [[ -n $6 ]] && declare time_stamp="<TimeStamp>${6}</TimeStamp>" || declare time_stamp=""
    [[ -n $7 ]] && local lifestage="<LifeStage>${7}</LifeStage>" || local lifestage=""

    _replaceStringInFile "$component_config_path" "<!-- ComponentImport -->" "$component_import"
    _replaceStringInFile "$component_config_path" "<!-- MajorRelease -->" "$major_release"
    _replaceStringInFile "$component_config_path" "<!-- MinorRelease -->" "$minor_release"
    _replaceStringInFile "$component_config_path" "<!-- TimeStamp -->" "$time_stamp"
    _replaceStringInFile "$component_config_path" "<!-- LifeStage -->" "$lifestage"
    _replaceStringInFile "$component_config_path" "<!-- Supplement -->" "<!-- $component_id-Supplement -->"
    _replaceStringInFile "$component_config_path" "<!-- Resubscription -->" "<!-- $component_id-Resubscription -->"
}


#------------------------------------------------------------------------------
#
# SDDS Import Functions
#
# - These are temporary functions until the build script for creating
#   SDDSImport files is ready for use with the regression suite
#
#------------------------------------------------------------------------------

function get_values_from_config_file()
{
    IFS=, read -a warehouse_config_values < /tmp/config_file.csv
}

function get_component_suite()
{
    announceFunction $FUNCNAME "$@"

    look_for=$1
    component_suite_record_to_process=()
    while IFS='' read -r line || [[ -n "$line" ]]; do
        IFS=, read -a values <<< $line
        if [[ ${values[2]} == $look_for ]]
        then
            component_suite_record_to_process+=($line)
        fi
    done < /tmp/config_file.csv
}

function get_components_for_parent()
{
    announceFunction $FUNCNAME "$@"

    look_for=$1
    list_of_components=()
    while IFS='' read -r line || [[ -n "$line" ]]; do
        IFS=, read -a values <<< $line
        if [[ ${values[1]} == $look_for && ${values[1]} != ${values[2]} ]]
        then
            list_of_components+=($line)
        fi
    done < /tmp/config_file.csv
}

function get_all_top_level_components_for_warehouse()
{
    announceFunction $FUNCNAME "$@"

    local warehouse_name=$1
    list_of_all_top_level_warehouse_components=()
    while read -r line || [[ -n "$line" ]]; do
        IFS=, read -a values <<< $line

        if [[ ${values[0]} == ${warehouse_name}  &&  ${values[1]} == ${values[2]}  ]]
        then
            list_of_all_top_level_warehouse_components+=($line)
        fi
    done < /tmp/config_file.csv
}

function get_all_components_for_warehouse()
{

    announceFunction $FUNCNAME "$@"

    local warehouse_name=$1
    list_of_all_warehouse_components=()
    while read -r line || [[ -n "$line" ]]; do
        IFS=, read -a values <<< $line

        if [[ ${values[0]} == ${warehouse_name} ]]
        then
            list_of_all_warehouse_components+=($line)
        fi
    done < /tmp/config_file.csv
}

function createSDDSImportFile()
{
    announceFunction $FUNCNAME "$@"

    declare version=$1
    declare home_folder=$2

    IFS=, read -a component_config_values <<< ${sdds_import_component[@]}

    declare rigid_name=${component_config_values[2]}
    declare directory=${component_config_values[4]}/${rigid_name}
    declare component=${component_config_values[5]}

    # cannot pass string with spaces in array easily so need to convert values here.
    if [[ ${component} == "true" ]]
    then
        component="Component Suite"
    else
        component="Component"
    fi

    local sdds="$directory/sdds"

    local shortName="SSP-$version"
    local longName="Sophos Server Protection for Linux $rigid_name v$version"

    local line_short_name="Linux"
    local line_long_name=${rigid_name}
    # make the product name the rigid_name minus ServerProtectionLinux-[Base|Plugin]-
    local product_name=${line_long_name/ServerProtectionLinux-/}
    product_name=${product_name/Base-/}
    product_name=${product_name/Plugin-/}
    product_name=${product_name//-/_}
    local feature_name="${product_name}_FEATURES"
    #extract the environment variable <Product>_FEATURES
    local features=${!feature_name}
    echo "Checking if environment variable ${feature_name} is setup to rigidname ${rigid_name} -> ${features}"

    if [[ $rigid_name = *"SSPL-RIGIDNAME"* ]] || [[ $rigid_name = *"PLUGIN"* ]] \
        || [[ $rigid_name = *"ServerProtectionLinux-Base"* ]] || [[ $rigid_name = *"ServerProtectionLinux-Plugin"* ]]
    then
      line_long_name="Sophos Server Protection for Linux $rigid_name"
    fi

#    local line_short_name="Linux"
#    local line_long_name="Sophos Server Protection for Linux"

    declare sdds_import_file_path="${sdds}/SDDS-Import.xml"
    [[ -d $sdds ]] || testFailure "Cannot access directory: $sdds; to create SDDS import"
    declare tmp_sdds_import_file_path="$sdds/TmpSDDSImport.xml"
    [[ -f $sdds_import_file_path ]] && rm $sdds_import_file_path
    {
        echo -n "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
        echo -n "<ComponentData>"
        echo -n "<Component>"
        echo -n "<Name>${rigid_name}#${version}</Name>"
        echo -n "<RigidName>${rigid_name}</RigidName>"
        echo -n "<Version>${version}</Version>"
        echo -n "<Build>${rigid_name}</Build>"
        echo -n "<ProductType>${component}</ProductType>"
        echo -n "<DefaultHomeFolder>${home_folder}</DefaultHomeFolder>"
        echo -n "<TargetTypes />"
        echo -n "<Roles />"
        echo -n "<Platforms />"
        if [[ -n $features ]]; then
          echo -n '<Features>'
          for feature in $features; do
            echo -n "<Feature Name=\"$feature\" />"
          done
          echo -n '</Features>'
        else
          echo -n '<Features><Feature Name="CORE"/></Features>'
        fi
        echo -n "<FileList>"

        # use separate variable declaration to avoid Bash 3.0 word-splitting bug
        local -a files
        local OLD_IFS="$IFS"
        local IFS=$'\n'

        # Must ignore "TmpSDDSImport.xml" and "SDDS-Import0.xml"
        files=( $(find "$sdds" -type f ! -name "*SDDS*Import*.xml") )
        IFS="$OLD_IFS"

        local file
        local last=$SECONDS
        local i
        for (( i = 0 ; i < ${#files[@]}; i++ ))
        do
            file="${files[$i]}"
            fileRelative="${file#$sdds}"

            if (( $SECONDS > $last + 30 ))
            then
                echo " - $(( ( $i * 100 ) / ${#files[@]} ))%" >&2
                last=$SECONDS
            fi
            _generateSDDSImportFileTag "$file" "$fileRelative"
        done

        echo -n "</FileList>"
        echo -n "</Component>"
        echo -n "<Dictionary>"
        echo -n "<Name>"
        echo -n "<Label>"
        echo -n "<Token>${rigid_name}#${version}</Token>"
        echo -n "<Language lang=\"en\">"
        echo -n "<ShortDesc>${shortName}</ShortDesc>"
        echo -n "<LongDesc>${longName}</LongDesc>"
        echo -n "</Language>"
        echo -n "</Label>"
        echo -n "</Name>"
        echo -n "<Line>"
        echo -n "<Label>"
        echo -n "<Token>${rigid_name}</Token>"
        echo -n "<Language lang=\"en\">"
        echo -n "<ShortDesc>${line_short_name}</ShortDesc>"
        echo -n "<LongDesc>${line_long_name}</LongDesc>"
        echo -n "</Language>"
        echo -n "</Label>"
        echo -n "</Line>"
        echo -n "</Dictionary>"
        echo -n "</ComponentData>"
    } > "$tmp_sdds_import_file_path"

    [[ -f "$tmp_sdds_import_file_path" ]] || setupFailure "Unable to construct temp SDDS-Import.xml: $tmp_sdds_import_file_path"
    mkdir -p "${sdds}"
    mv "$tmp_sdds_import_file_path" "$sdds_import_file_path" \
        || setupFailure "Unable to move $tmp_sdds_import_file_path to $sdds_import_file_path"
}

function _generateSDDSImportFileTag()
{
    local file="$1"
    local file_relative="$2"

    [[ -f "$file" ]] || setupFailure "_generateSDDSImportFileTag: $file is not a valid file"


    local file_name="${file##*/}"
    local file_path="${file%/*}"
    local offset="${file_relative%$file_name}"
    offset="${offset#/}"

    local file_size=$(fileSize "$file")
    local file_md5=$(_getMd5sum "$file")

    echo -n "<File MD5=\"$file_md5\" Name=\"$file_name\" Offset=\"$offset\" Size=\"$file_size\" />"
}
