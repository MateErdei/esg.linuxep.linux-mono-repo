#!/bin/bash

umask 077

echo "This software is governed by the terms and conditions of a licence agreement with Sophos Limited"

VERSION=@PRODUCT_VERSION@
PRODUCT_NAME="Sophos Protection for Linux"
INSTALL_FILE=$0

EXITCODE_SUCCESS=0
EXITCODE_NOT_LINUX=1
EXITCODE_NOT_ROOT=2
EXITCODE_NO_CENTRAL=3
EXITCODE_NOT_ENOUGH_MEM=4
EXITCODE_NOT_ENOUGH_SPACE=5
EXITCODE_FAILED_REGISTER=6
EXITCODE_ALREADY_INSTALLED=7
EXITCODE_SAV_INSTALLED=8
EXITCODE_NOT_64_BIT=9
EXITCODE_DOWNLOAD_FAILED=10
EXITCODE_FAILED_TO_UNPACK=11
EXITCODE_CANNOT_MAKE_TEMP=12
EXITCODE_VERIFY_INSTALLER_FAILED=13
EXITCODE_SYMLINKS_FAILED=14
EXITCODE_CHMOD_FAILED=15
EXITCODE_NOEXEC_TMP=16
EXITCODE_DELETE_INSTALLER_ARCHIVE_FAILED=17
EXITCODE_BASE_INSTALL_FAILED=18
EXITCODE_BAD_INSTALL_PATH=19
EXITCODE_INSTALLED_BUT_NO_PATH=20
EXIT_FAIL_WRONG_LIBC_VERSION=21
EXIT_FAIL_COULD_NOT_FIND_LIBC_VERSION=22
EXITCODE_UNEXPECTED_ARGUMENT=23
EXITCODE_BAD_GROUP_NAME=24
EXITCODE_GROUP_NAME_EXCEEDS_MAX_SIZE=25
EXITCODE_DUPLICATE_ARGUMENTS_GIVEN=26
EXITCODE_BAD_PRODUCT_SELECTED=27
EXITCODE_INVALID_CUSTOM_ID_GIVEN=28
EXITCODE_NO_SYSTEMD=29
EXITCODE_MISSING_PACKAGE=30
EXITCODE_INVALID_CA_PATHS=31
EXITCODE_INVALID_MR_UC_GIVEN=32
EXITCODE_COMPATIBILITY_CHECKS_FAILED=33
EXITCODE_MALFORMED_THININSTALLER=34
EXITCODE_REGISTRATION_FAILED=51
EXITCODE_AUTHENTICATION_FAILED=52
EXITCODE_ALC_POLICY_TRANSLATION_FAILED=53

PROXY_CREDENTIALS=

MAX_GROUP_NAME_SIZE=1024
VALID_PRODUCTS=("antivirus" "mdr" "xdr")
REQUEST_NO_PRODUCTS="none"

@COMMON_SPL_COMPATIBILITY_FUNCTIONS@

# Ensure override is unset so that it can only be set when explicitly passed in by the user with --allow-override-mcs-ca
unset ALLOW_OVERRIDE_MCS_CA

CREATED_INSTALL_DIRECTORY=0

function get_help_text() {
  echo "${PRODUCT_NAME} Installer, help:"
  echo "Note: Please refer to https://docs.sophos.com/central/customer/help/en-us/index.html?contextId=protect-devices-server-CLI-Linux for advanced options"
  echo "Usage: [options]"
  echo "Valid options are:"
  echo -e "--help [-h]\t\t\tDisplay this summary"
  echo -e "--version [-v]\t\t\tDisplay version of installer"
  echo -e "--force\t\t\t\tForce re-install"
  echo -e "--group=<group>\t\t\tAdd this endpoint into the Sophos Central group specified"
  echo -e "--group=<path to sub group>\tAdd this endpoint into the Sophos Central nested\n\t\t\t\tgroup specified where path to the nested group\n\t\t\t\tis each group separated by a backslash\n\t\t\t\ti.e. --group=<top-level group>\\\\\<sub-group>\\\\\<bottom-level-group>\n\t\t\t\tor --group='<top-level group>\\\<sub-group>\\\<bottom-level-group>'"
  echo -e "--products='<products>'\t\tComma separated list of products to install\n\t\t\t\ti.e. --products=antivirus,mdr,xdr"
  echo -e "--uninstall-sav\t\t\tUninstall Sophos Anti-Virus if installed"
  echo -e "--message-relays=<host1>:<port1>,<host2>:<port2>\n\t\t\t\tSpecify message relays used for registration.\n\t\t\t\t To specify no message relays use --message-relays=none"
  echo -e "--update-caches=<host1>:<port1>,<host2>:<port2>\n\t\t\t\tSpecify update caches used for install.\n\t\t\t\t To specify no update caches use --update-caches=none"
  echo -e "--test\t\t\t\tOnly run pre-installation checks and do not install SPL"
  echo -e "--notest\t\t\tDon't run any pre-installation checks and proceed with installing SPL"
}


function cleanup_and_exit() {
    code=$1

    if [[ "${code}" -eq "${EXITCODE_SUCCESS}" ]]; then
        [[ -z "${OVERRIDE_INSTALLER_CLEANUP}" ]] && rm -rf "${SOPHOS_TEMP_DIRECTORY}"

        # Start product if it is not running
        [[ $(systemctl is-active --quiet sophos-spl) ]] || systemctl start sophos-spl 2>/dev/null
    fi

    exit "${code}"
}

function failure() {
    code=$1
    removeinstall=1
    if [[ -n "$3" ]]; then
        removeinstall=0
    fi
    if [[ $CREATED_INSTALL_DIRECTORY == 0 ]]; then
        removeinstall=0
    fi

    echo "$2" >&2
    if [[ -s "${SOPHOS_INSTALL}/logs/base/suldownloader.log" ]]; then
        echo "-- Output from suldownloader log:"
        cat "${SOPHOS_INSTALL}/logs/base/suldownloader.log"
        cat "${SOPHOS_INSTALL}/logs/base/suldownloader_sync.log"
    fi

    # only remove files if we didnt get far enough through the process to install or
    # the install directory existed already and we didnt create it
    if [[ ${removeinstall} -eq 1 ]] && ! is_sspl_installed; then
        if [[ -n "${SOPHOS_INSTALL}" ]]; then
            if [[ -d ${SOPHOS_TEMP_DIRECTORY} ]]; then
                echo "Copying SPL logs to ${SOPHOS_TEMP_DIRECTORY}/logs"
                cp -Lr "${SOPHOS_INSTALL}/logs" "${SOPHOS_TEMP_DIRECTORY}" 2>/dev/null
                mkdir -p "${SOPHOS_TEMP_DIRECTORY}/logs/plugins"
                cp -Lr "${SOPHOS_INSTALL}/plugins/*/log/*" "${SOPHOS_TEMP_DIRECTORY}/logs/plugins" 2>/dev/null
            fi
            echo "Removing ${SOPHOS_INSTALL}"
            rm -rf "${SOPHOS_INSTALL}"
        fi
    fi
    cleanup_and_exit ${code}
}

function handle_installer_errorcodes() {
    errcode=$1

    if [[ ${errcode} -eq 0 ]]; then
        echo "Successfully installed product"
    elif [[ ${errcode} -eq 103 ]]; then
        failure ${EXITCODE_BASE_INSTALL_FAILED} "Failed to install at least one of the packages, see logs for details"
    elif [[ ${errcode} -eq 107 ]]; then
        failure ${EXITCODE_BASE_INSTALL_FAILED} "Failed to install: Download failure"
    elif [[ ${errcode} -eq 111 ]]; then
        failure ${EXITCODE_BASE_INSTALL_FAILED} "Failed to install: Package source missing from repository"
    elif [[ ${errcode} -eq 112 ]]; then
        failure ${EXITCODE_BASE_INSTALL_FAILED} "Failed to install: Connection error - please check your network connections"
    else
        failure ${EXITCODE_BASE_INSTALL_FAILED} "Failed to install (Error code = ${errcode})"
    fi
}

function handle_register_errorcodes() {
    errcode=$1
    if [[ ${errcode} -eq 0 ]]; then
        echo "Successfully registered with Sophos Central"
    elif [[ ${errcode} -eq 44 ]]; then
        failure ${EXITCODE_NO_CENTRAL} "Cannot connect to Sophos Central - please check your network connections"
    elif [[ ${errcode} -eq ${EXITCODE_REGISTRATION_FAILED} ]]; then # Error code 51
        failure ${EXITCODE_REGISTRATION_FAILED} "Failed to register with Sophos Central, aborting installation"
    elif [[ ${errcode} -eq ${EXITCODE_AUTHENTICATION_FAILED} ]]; then # Error code 52
        failure ${EXITCODE_AUTHENTICATION_FAILED} "Failed to authenticate with Sophos Central, aborting installation"
    else
        failure ${EXITCODE_DOWNLOAD_FAILED} "Failed to register with Central (Error code = ${errcode})"
    fi
}

function check_SAV_installed() {
    local path=$1
    local sav_instdir=$(readlink ${path} | sed 's/bin\/savscan//g')
    if [[ "$sav_instdir" == "" ]] || [[ ! -d ${sav_instdir} ]]; then
        # Sophos Anti-Virus not found at this location
        return
    fi
    echo "Found an existing installation of Sophos Anti-Virus in $sav_instdir."
    echo "This product cannot be run alongside Sophos Anti-Virus."
    if ((FORCE_UNINSTALL_SAV != 1)); then
        failure ${EXITCODE_SAV_INSTALLED} \
            "Found an existing installation of SAV in $sav_instdir. This product cannot be run alongside Sophos Anti-Virus. Re-run installer with --uninstall-sav to remove Sophos Anti-Virus"
    fi

    echo "Sophos Anti-Virus will be uninstalled:"
    $sav_instdir/uninstall.sh --force ||
        failure ${EXITCODE_SAV_INSTALLED} "Unable to uninstall Sophos Anti-Virus from $sav_instdir: $?"
    echo "Sophos Anti-Virus has been uninstalled."
}

#
# Use mktemp if it is available, or try to create unique
# directory name and use mkdir instead.
#
# Pass directory name prefix as a parameter.
#
function sophos_mktempdir() {
    _mktemp=$(which mktemp 2>/dev/null)
    if [ -x "${_mktemp}" ]; then
        # mktemp exists - use it
        _tmpdirTemplate="${TMPDIR:-/tmp}/$1_XXXXXXX"
        _tmpdir=$(${_mktemp} -d "${_tmpdirTemplate}")
        [ $? = 0 ] || {
            echo "Could not create temporary directory" 1>&2
            exit 1
        }
    else
        _od=$(which od 2>/dev/null)
        if [ -x "${_od}" ]; then
            # mktemp not available - use /dev/urandom, od and mkdir
            _random=/dev/urandom
            [ -f "${_random}" ] || _random=/dev/random
            _tmpdir="${TMPDIR:-/tmp}"/$1_$("${_od}" -An -N16 -tu2 "${_random}" | tr -d " \t\r\n").$$
        else
            # mktemp not available - /dev/urandom not available - use $RANDOM and mkdir
            _tmpdir="${TMPDIR:-/tmp}"/$1_${RANDOM-0}.${RANDOM-0}.${RANDOM-0}.$$
        fi
        [ -d "${_tmpdir}" ] && {
            echo "Temporary directory already exists" 1>&2
            exit 1
        }
        (umask 077 && mkdir "${_tmpdir}") || {
            echo "Could not create temporary directory" 1>&2
            exit 1
        }
    fi

    if [ ! -d "${_tmpdir}" ]; then
        echo "Could not create temporary directory" 1>&2
        exit ${EXITCODE_CANNOT_MAKE_TEMP}
    fi

    echo "${_tmpdir}"
}

function is_sspl_installed() {
    systemctl list-unit-files | grep -q sophos-spl
}

function check_for_duplicate_arguments() {
    declare -a checked_arguments
    for argument in "$@"; do
        argument_name="${argument%=*}"
        [[ "${checked_arguments[@]}" =~ (^| )"$argument_name"($| ) ]] && failure ${EXITCODE_DUPLICATE_ARGUMENTS_GIVEN} "Error: Duplicate argument given: $argument_name --- aborting install"
        checked_arguments+=("$argument_name")
    done
}

function validate_group_name() {
    [ -z "$1" ] && failure ${EXITCODE_BAD_GROUP_NAME} "Error: Group name not passed with '--group=' argument --- aborting install"
    [[ ${#1} -gt ${MAX_GROUP_NAME_SIZE} ]] && failure ${EXITCODE_GROUP_NAME_EXCEEDS_MAX_SIZE} "Error: Group name exceeds max size of: ${MAX_GROUP_NAME_SIZE} --- aborting install"
    is_string_valid_for_xml "$1" || failure ${EXITCODE_BAD_GROUP_NAME} "Error: Group name contains one of the following invalid characters: < & > ' \" --- aborting install"
}

function is_string_valid_for_xml() {
    [[ ! $1 =~ .*[\>\<\&\'\"].* ]]
}

function check_selected_products_are_valid() {
    local products="${1}"
    [[ "${products: -1}" == "," ]] && failure ${EXITCODE_BAD_PRODUCT_SELECTED} "Error: Products passed with trailing comma --- aborting install"
    [[ "${products}" == "${REQUEST_NO_PRODUCTS}" ]] && return 0
    [[ -n "${products}" && "${products}" != " " ]] || failure ${EXITCODE_BAD_PRODUCT_SELECTED} "Error: Products not passed with '--products=' argument --- aborting install"
    declare -a selected_products
    IFS=',' read -ra PRODUCTS_ARRAY <<<"${products}"
    [[ -n "${PRODUCTS_ARRAY}" && "$PRODUCTS_ARRAY" != " " ]] || failure ${EXITCODE_BAD_PRODUCT_SELECTED} "Error: Products not passed with '--products=' argument --- aborting install"
    for product in "${PRODUCTS_ARRAY[@]}"; do
        [[ -n "${product}" && "${product}" != " " ]] || failure ${EXITCODE_BAD_PRODUCT_SELECTED} "Error: Product cannot be whitespace"
        [[ ! "${VALID_PRODUCTS[*]}" =~ (^| )"${product}"($| ) ]] && failure ${EXITCODE_BAD_PRODUCT_SELECTED} "Error: Invalid product selected: ${product} --- aborting install."
        [[ "${selected_products[*]}" =~ (^| )"${product}"($| ) ]] && failure ${EXITCODE_BAD_PRODUCT_SELECTED} "Error: Duplicate product given: ${product} --- aborting install."
        selected_products+=("${product}")
    done
}

function validate_MR_and_UC_addresses() {
    local addresses="${1}"
    local server_type="${2}"

    [[ "${addresses: -1}" == "," ]] && failure ${EXITCODE_INVALID_MR_UC_GIVEN} "Error: ${server_type}s passed with trailing comma --- aborting install"
    [[ -n "${addresses}" && "${addresses}" != " " ]] || failure ${EXITCODE_INVALID_MR_UC_GIVEN} "Error: ${server_type}s not passed with '--${server_type/ /-}s=' argument --- aborting install"

    declare -a valid_addresses
    IFS=',' read -ra ADDRESS_ARRAY <<<"${addresses}"
    [[ -n "${ADDRESS_ARRAY}" && "${ADDRESS_ARRAY}" != " " ]] || failure ${EXITCODE_INVALID_MR_UC_GIVEN} "Error: ${server_type}s not passed with '--${server_type/ /-}s=' argument --- aborting install"
    for address in "${ADDRESS_ARRAY[@]}"; do
        [[ -n "${address}" && "${address}" != " " ]] || failure ${EXITCODE_INVALID_MR_UC_GIVEN} "Error: ${server_type} cannot be whitespace"
        [[ "${address}" =~ ^[a-zA-Z0-9._-]+:[0-9]+$ ]] || failure ${EXITCODE_INVALID_MR_UC_GIVEN} "Error: Requested ${server_type} address not valid: ${address} --- aborting install"
        [[ "${valid_addresses[*]}" =~ (^| )"${address}"($| ) ]] && failure ${EXITCODE_INVALID_MR_UC_GIVEN} "Error: Duplicate ${server_type} given: ${address} --- aborting install."
        valid_addresses+=("${address}")
    done
}

function check_custom_ids_are_valid() {
    local ids="${1}"

    [[ "${ids: -1}" == "," ]] && failure ${EXITCODE_INVALID_CUSTOM_ID_GIVEN} "Error: Requested group/user ids to configure passed with trailing comma --- aborting install"
    [[ -n "${ids}" && "${ids}" != " " ]] || failure ${EXITCODE_INVALID_CUSTOM_ID_GIVEN} "Error: Requested group/user ids to configure not passed with argument --- aborting install"

    declare -a valid_ids
    IFS=',' read -ra IDS_ARRAY <<<"${ids}"
    [[ -n "${IDS_ARRAY}" && "${IDS_ARRAY}" != " " ]] || failure ${EXITCODE_INVALID_CUSTOM_ID_GIVEN} "Error: Requested group/user ids to configure not passed with argument --- aborting install"
    for id in "${IDS_ARRAY[@]}"; do
        [[ -n "${id}" && "${id}" != " " ]] || failure ${EXITCODE_INVALID_CUSTOM_ID_GIVEN} "Error: Requested ID cannot contain whitespace: ${id}  --- aborting install"
        [[ "${id}" =~ ^sophos-spl-?(.*):[0-9]+$ ]] || failure ${EXITCODE_INVALID_CUSTOM_ID_GIVEN} "Error: Requested group/user id to configure is not valid: ${id} --- aborting install"
        [[ "${valid_ids[*]%%:*}" =~ (^| )"${id%%:*}"($| ) ]] && failure ${EXITCODE_INVALID_CUSTOM_ID_GIVEN} "Error: Duplicate user name given: ${id} --- aborting install"
        [[ "${valid_ids[*]#*:}" =~ (^| )"${id#*:}"($| ) ]] && failure ${EXITCODE_INVALID_CUSTOM_ID_GIVEN} "Error: Duplicate id given: ${id} --- aborting install"
        valid_ids+=("${id}")
    done
}

function pre_install_checks() {
    echo -e "\nPerforming pre-installation checks to verify whether SPL can be installed on this machine"

    # Check running as root
    [[ $(id -u) -ne 0 ]] && failure ${EXITCODE_NOT_ROOT} "Please run this installer as root"

    # Run common SPL compatibility checks
    verify_system_requirements

    # Get baked in Central URL and any configured Message Relays/Update Caches
    CENTRAL_URL=$(sed -n -e "/^URL=/ s/.*\= *//p" "${INSTALL_FILE}")

    if [[ "${MESSAGE_RELAYS_OVERRIDE}" != "none" ]]; then
        if [[ -n "${MESSAGE_RELAYS_OVERRIDE}" ]]; then
          message_relays="${MESSAGE_RELAYS_OVERRIDE}"
          IFS=',' read -ra message_relay_array <<<"${message_relays}"
        else
          message_relays="$(sed -n -e "/^MESSAGE_RELAYS=[^$]/ s/.*\= *//p" "${INSTALL_FILE}")"
          IFS=';' read -ra message_relay_array <<<"${message_relays}"
        fi
        for message_relay in "${message_relay_array[@]}"; do
            MESSAGE_RELAYS+=("${message_relay%%,*}")
        done
    fi

    if [[ "${UPDATE_CACHES_OVERRIDE}" != "none" ]]; then
        if [[ -n "${UPDATE_CACHES_OVERRIDE}" ]]; then
          update_caches="${UPDATE_CACHES_OVERRIDE}"
          IFS=',' read -ra update_cache_array <<<"${update_caches}"
        else
          update_caches="$(sed -n -e "/^UPDATE_CACHES=[^$]/ s/.*\= *//p" "${INSTALL_FILE}")"
          IFS=';' read -ra update_cache_array <<<"${update_caches}"
        fi
        for update_cache in "${update_cache_array[@]}"; do
            UPDATE_CACHES+=("${update_cache%%,*}")
        done
    fi

    # Read SDDS3 SUS URL from credentials file, e.g. SDDS3_SUS_URL=sus.sophosupd.com
    EXTRACTED_SUS_URL=$(sed -n -e "/^SDDS3_SUS_URL=/ s/.*\= *//p" "${INSTALL_FILE}")
    if [[ "$EXTRACTED_SUS_URL" != "" && $EXTRACTED_SUS_URL != https://* ]]
    then
        EXTRACTED_SUS_URL="https://${EXTRACTED_SUS_URL}"
    fi
    # Read SDDS3 CDN URLs from credentials file, e.g. SDDS3_CONTENT_URLS=sdds3.sophosupd.com;sdds3.sophosupd.net
    cdn_urls="$(sed -n -e "/^SDDS3_CONTENT_URLS=[^$]/ s/.*\= *//p" "${INSTALL_FILE}")"
    IFS=';' read -ra cdn_urls_array <<<"${cdn_urls}"
    for cdn_url in "${cdn_urls_array[@]}"; do
         if [[ $cdn_url != https://* ]]
        then
            cdn_url="https://${cdn_url}"
        fi
        EXTRACTED_CDN_URLS+=("${cdn_url%%,*}")
    done

    # Verify network connections made during installation to Central, SUS and CDN servers
    verify_network_connections "${MESSAGE_RELAYS[*]}" "${UPDATE_CACHES[*]}" "${EXTRACTED_CDN_URLS[*]}" "$EXTRACTED_SUS_URL"

    if [[ -n "${COMPATIBILITY_ERROR_FOUND}" ]]; then
        failure ${EXITCODE_COMPATIBILITY_CHECKS_FAILED} "SPL cannot be installed on this system, the pre-installation checks found some critical issues. Please review the logs and address the issues before attempting to reinstall"
    elif [[ -n "${COMPATIBILITY_WARNING_FOUND}" ]]; then
        echo "SPL can be installed on this system, however, the pre-installation checks found some non-critical issues. Please review and address these"
    else
        echo "SPL can be installed on this system based on the pre-installation checks"
    fi
    echo
}

function make_tmp_dir()
{
    # Create Sophos temp directory (unless overridden with an existing dir)
    if [ -z "${SOPHOS_TEMP_DIRECTORY}" ]; then
        SOPHOS_TEMP_DIRECTORY=$(sophos_mktempdir SophosCentralInstall) || failure ${EXITCODE_CANNOT_MAKE_TEMP} "Could not generate name for temp directory in ${TMPDIR:-/tmp}"
    fi

    mkdir -p "${SOPHOS_TEMP_DIRECTORY}"
}

declare -a INSTALL_OPTIONS_ARGS
# Handle arguments
check_for_duplicate_arguments "$@"
FORCE_UNINSTALL_SAV=0
FORCE_INSTALL=0
UNEXPECTED_ARGUMENT=0
HELP_FLAG=0
VERSION_FLAG=0
for i in "$@"; do
    case $i in
    --install-dir=*)
        SOPHOS_INSTALL="${i#*=}"
        shift
        ;;
    --update-source-creds=*)
        export OVERRIDE_SOPHOS_CREDS="${i#*=}"
        shift
        ;;
    --proxy-credentials=*)
        export PROXY_CREDENTIALS="${i#*=}"
        ;;
    --products=*)
        check_selected_products_are_valid "${i#*=}"
        PRODUCT_ARGUMENTS="--products ${i#*=}"
        CMCSROUTER_PRODUCT_ARGUMENTS="${i}"
        shift
        ;;
    --allow-override-mcs-ca)
        export ALLOW_OVERRIDE_MCS_CA=--allow-override-mcs-ca
        shift
        ;;
    --group=*)
        validate_group_name "${i#*=}"
        REGISTRATION_GROUP_ARGS="--central-group=${i#*=}"
        INSTALL_OPTIONS_ARGS+=("$i")
        shift
        ;;
    --user-ids-to-configure=*)
        check_custom_ids_are_valid "${i#*=}"
        INSTALL_OPTIONS_ARGS+=("$i")
        shift
        ;;
    --group-ids-to-configure=*)
        check_custom_ids_are_valid "${i#*=}"
        INSTALL_OPTIONS_ARGS+=("$i")
        shift
        ;;
    --disable-auditd)
        INSTALL_OPTIONS_ARGS+=("$i")
        shift
        ;;
    --do-not-disable-auditd)
        INSTALL_OPTIONS_ARGS+=("$i")
        shift
        ;;
    --message-relays=*)
        [[ "${i#*=}" == "none" ]] || validate_MR_and_UC_addresses "${i#*=}" "message relay"
        MESSAGE_RELAYS_OVERRIDE="${i#*=}"
        shift
        ;;
    --update-caches=*)
        [[ "${i#*=}" == "none" ]] || validate_MR_and_UC_addresses "${i#*=}" "update cache"
        export UPDATE_CACHES_OVERRIDE="${i#*=}"
        shift
        ;;
    --uninstall-sav)
        FORCE_UNINSTALL_SAV=1
        ;;
    --force)
        FORCE_INSTALL=1
        shift
        ;;
    --test)
        echo "Found '--test' is used, will only do pre-install checks and will not install SPL"
        PRE_INSTALL_TEST_ONLY=1
        ;;
    --notest)
        echo "Found '--notest' is used, will not perform pre-install checks but will proceed with installing SPL"
        NO_PRE_INSTALL_TESTS=1
        ;;
    --help|-h)
        HELP_FLAG=1
        ;;
    --version|-v)
        VERSION_FLAG=1
        ;;
    *)
        UNEXPECTED_ARGUMENT=1
        UNEXPECTED_ARG_VAL=$i
        ;;
    esac
done

if [[ ${HELP_FLAG} == 1 ]]
then
    get_help_text
    exit 0
fi

if [[ ${VERSION_FLAG} == 1 ]]
then
    echo "${PRODUCT_NAME} Installer, version: $VERSION"
    exit 0
fi

if [[ ${UNEXPECTED_ARGUMENT} == 1 ]]
then
  failure ${EXITCODE_UNEXPECTED_ARGUMENT} "Error: Unexpected argument given: $UNEXPECTED_ARG_VAL --- aborting install. Please see '--help' output for list of valid arguments"
fi

if [[ ${NO_PRE_INSTALL_TESTS} != 1 ]]
then
    pre_install_checks
else
    # Check machine architecture
    if [[ $(uname -m) = "x86_64" || $(uname -m) = "aarch64" ]]
    then
        BIN="bin"
    else
        failure ${EXITCODE_NOT_64_BIT} "ERROR: SPL installation will fail as it can only be installed on a x86_64 or aarch64 system"
    fi
fi

verify_install_directory

make_tmp_dir
# Check that the tmp directory we're using allows execution
echo "exit 0" >"${SOPHOS_TEMP_DIRECTORY}/exectest" 2>/dev/null && chmod +x "${SOPHOS_TEMP_DIRECTORY}/exectest"
$SOPHOS_TEMP_DIRECTORY/exectest 2>/dev/null || failure ${EXITCODE_NOEXEC_TMP} "Cannot execute files within ${TMPDIR:-/tmp} directory. Please see KBA 131783 http://www.sophos.com/kb/131783"

function test_installation_executable()
{
    # Don't attempt execution test if install dir already exists
    [[ -d "$SOPHOS_INSTALL" ]] && return
    # ensure parent directories of install dir have the correct permissions by setting umask
    umask 066
    mkdir -p "$SOPHOS_INSTALL"
    umask 077
    echo "exit 0" >"${SOPHOS_INSTALL}/exectest" 2>/dev/null && chmod +x "${SOPHOS_INSTALL}/exectest"
    $SOPHOS_INSTALL/exectest 2>/dev/null || {
      rm -r "$SOPHOS_INSTALL"
      failure ${EXITCODE_NOEXEC_TMP} "Cannot execute files within ${SOPHOS_INSTALL} directory. Please see KBA 131783 http://www.sophos.com/kb/131783"
    }
    rm -r "$SOPHOS_INSTALL"
}

test_installation_executable

if [[ -n ${PRE_INSTALL_TEST_ONLY} ]]
then
    cleanup_and_exit ${EXITCODE_SUCCESS}
fi

echo "Installing to ${SOPHOS_INSTALL}"

# Check if SAV is installed.
## We check everything on $PATH, and always /usr/local/bin and /usr/bin
## This should catch everywhere SAV might have installed the sweep symlink
SWEEP=$(which sweep 2>/dev/null)
[ -x "$SWEEP" ] && check_SAV_installed "$SWEEP"
check_SAV_installed '/usr/local/bin/sweep'
check_SAV_installed '/usr/bin/sweep'

# Check to see if the Sophos credentials override is being used.
[ -n "$OVERRIDE_SOPHOS_CREDS" ] && {
    echo "Overriding Sophos credentials with $OVERRIDE_SOPHOS_CREDS"
}

# Get line numbers for the each of the sections
MIDDLEBIT=$(awk '/^__MIDDLE_BIT__/ {print NR + 1; exit 0; }' "${INSTALL_FILE}")
UC_CERTS=$(awk '/^__UPDATE_CACHE_CERTS__/ {print NR + 1; exit 0; }' "${INSTALL_FILE}")
ARCHIVE=$(awk '/^__ARCHIVE_BELOW__/ {print NR + 1; exit 0; }' "${INSTALL_FILE}")

if [[ -z "${MIDDLEBIT}" ]]; then
  failure ${EXITCODE_MALFORMED_THININSTALLER} "Thin installer is malformed it is missing the __MIDDLE_BIT__ seperator."
fi

# If we have __UPDATE_CACHE_CERTS__ section then the middle section ends there, else it ends at the ARCHIVE marker.
if [[ -n "${UC_CERTS}" ]]; then
    mkdir -p "${SOPHOS_TEMP_DIRECTORY}/installer"
    UPDATE_CACHE_CERT="${SOPHOS_TEMP_DIRECTORY}/installer/uc_certs.crt"
    MIDDLEBIT_SIZE=$((UC_CERTS - MIDDLEBIT - 1))
    UC_CERTS_SIZE=$((ARCHIVE - UC_CERTS - 1))
    tail -n+"${UC_CERTS}" "${INSTALL_FILE}" | head -"${UC_CERTS_SIZE}" | tr -d '\r' | tr -s '\n' >"${UPDATE_CACHE_CERT}"
else
    MIDDLEBIT_SIZE=$((ARCHIVE - MIDDLEBIT - 1))
fi

CREDENTIALS_FILE_PATH=${SOPHOS_TEMP_DIRECTORY}/credentials.txt
tail -n+"${MIDDLEBIT}" "${INSTALL_FILE}" | head -"${MIDDLEBIT_SIZE}" >"${CREDENTIALS_FILE_PATH}"

if [[ $(uname -m) = "x86_64" ]]; then
  sed -n -e '/^__ARCHIVE_BELOW__$/,/^__ARM64_ARCHIVE_BELOW__$/{ /^__ARCHIVE_BELOW__$/d; /^__ARM64_ARCHIVE_BELOW__$/d; p; }' "${INSTALL_FILE}" > ${SOPHOS_TEMP_DIRECTORY}/installer.tar.gz
  truncate -s -1 ${SOPHOS_TEMP_DIRECTORY}/installer.tar.gz
elif [[ $(uname -m) = "aarch64" ]]; then
  sed -n -e '/^__ARM64_ARCHIVE_BELOW__$/,/^__ARM64_ARCHIVE_ABOVE__$/{ /^__ARM64_ARCHIVE_BELOW__$/d; /^__ARM64_ARCHIVE_ABOVE__$/d; p; }' "${INSTALL_FILE}" > ${SOPHOS_TEMP_DIRECTORY}/installer.tar.gz
  truncate -s -1 ${SOPHOS_TEMP_DIRECTORY}/installer.tar.gz
fi

# Adjust Message Relays and Update Caches in credentials file based on override arguments
if [[ "${MESSAGE_RELAYS_OVERRIDE}" == "none" ]]; then
    echo "Message relay manually set to none, installation will not be performed via a message relay"
    modifiedCreds="$(grep -v "MESSAGE_RELAYS=" "${CREDENTIALS_FILE_PATH}")"
    echo "${modifiedCreds}" >"${CREDENTIALS_FILE_PATH}"
elif [[ -n "${MESSAGE_RELAYS_OVERRIDE}" ]]; then
    MESSAGE_RELAYS_OVERRIDE="${MESSAGE_RELAYS_OVERRIDE//,/,0,overridden-message-relay;},0,overridden-message-relay"
    modifiedCreds="$(grep -v "MESSAGE_RELAYS=" "${CREDENTIALS_FILE_PATH}")"
    echo "${modifiedCreds}" >"${CREDENTIALS_FILE_PATH}"
    echo -n "MESSAGE_RELAYS=${MESSAGE_RELAYS_OVERRIDE}" >>"${CREDENTIALS_FILE_PATH}"
fi
if [[ "${UPDATE_CACHES_OVERRIDE}" == "none" ]]; then
    echo "Update cache manually set to none, updates will not be performed via an update cache"
    modifiedCreds="$(grep -v "UPDATE_CACHES=" "${CREDENTIALS_FILE_PATH}")"
    echo "${modifiedCreds}" >"${CREDENTIALS_FILE_PATH}"
elif [[ -n "${UPDATE_CACHES_OVERRIDE}" ]]; then
    UPDATE_CACHES_OVERRIDE="${UPDATE_CACHES_OVERRIDE//,/,0,overridden-update-cache;},0,overridden-update-cache"
    modifiedCreds="$(grep -v "UPDATE_CACHES=" "${CREDENTIALS_FILE_PATH}")"
    echo "${modifiedCreds}" >"${CREDENTIALS_FILE_PATH}"
    echo -n "UPDATE_CACHES=${UPDATE_CACHES_OVERRIDE}" >>"${CREDENTIALS_FILE_PATH}"
fi

cd ${SOPHOS_TEMP_DIRECTORY}

# Read customer token from credentials file.
if [ -z "${OVERRIDE_CUSTOMER_TOKEN}" ]; then
    CUSTOMER_TOKEN=$(grep 'CUSTOMER_TOKEN=' credentials.txt | sed 's/CUSTOMER_TOKEN=//')
else
    CUSTOMER_TOKEN=${OVERRIDE_CUSTOMER_TOKEN}
fi
CUSTOMER_TOKEN_ARGUMENT="--customer-token $CUSTOMER_TOKEN"

# Read cloud token from credentials file.
if [ -z "${OVERRIDE_CLOUD_TOKEN}" ]; then
    CLOUD_TOKEN=$(grep -v 'CUSTOMER_TOKEN=' credentials.txt | grep 'TOKEN=' | sed 's/TOKEN=//')
else
    CLOUD_TOKEN=${OVERRIDE_CLOUD_TOKEN}
fi

# Read cloud URL from credentials file.
if [ -z "${OVERRIDE_CLOUD_URL}" ]; then
    CLOUD_URL=$(grep 'URL=' credentials.txt | sed 's/URL=//')
else
    CLOUD_URL=${OVERRIDE_CLOUD_URL}
fi

# Read Message Relays from credentials file.
MESSAGE_RELAYS=$(grep 'MESSAGE_RELAYS=' credentials.txt | sed 's/MESSAGE_RELAYS=//')
if [[ -n "${MESSAGE_RELAYS}" ]]; then
    [[ -n "$DEBUG_THIN_INSTALLER" ]] && echo "Message Relays: ${MESSAGE_RELAYS}"
    CMCSROUTER_MESSAGE_RELAYS="--message-relay ${MESSAGE_RELAYS}"
    MESSAGE_RELAYS="--messagerelay ${MESSAGE_RELAYS}"
fi

# Save installer arguments to file for base and plugins to use.
INSTALL_OPTIONS_FILE="${SOPHOS_TEMP_DIRECTORY}/install_options"

# File format expects the args to be either --option  or --option=value
for value in "${INSTALL_OPTIONS_ARGS[@]}"; do
    echo $value >>${INSTALL_OPTIONS_FILE}
done

UPDATE_CACHES=$(grep 'UPDATE_CACHES=' credentials.txt | sed 's/UPDATE_CACHES=//')
if [[ -n "${UPDATE_CACHES}" ]]; then
    if [[ -n "${DEBUG_THIN_INSTALLER}" ]]; then
        echo "List of update caches to install from: ${UPDATE_CACHES}"
    fi
fi

# Read Products from credentials file if the customer has not used the installer option.
INSTALLER_PRODUCTS=$(grep 'PRODUCTS=' credentials.txt | sed 's/PRODUCTS=//')
if [ -n "$INSTALLER_PRODUCTS" ] && [ -z "$PRODUCT_ARGUMENTS" ] && [ "$INSTALLER_PRODUCTS" != "all" ]; then
    PRODUCT_ARGUMENTS="$INSTALLER_PRODUCTS"
    CMCSROUTER_PRODUCT_ARGUMENTS=$(grep 'PRODUCTS=' credentials.txt | sed 's/PRODUCTS=/--products=/')
    if [ -n "$DEBUG_THIN_INSTALLER" ]; then
        echo "List of products to install: $PRODUCT_ARGUMENTS"
    fi
fi

REGISTER_CENTRAL="${SOPHOS_INSTALL}/base/bin/registerCentral"

# Check if the sophos-spl service is installed, if it is find the install path.
EXISTING_SSPL_PATH=

if [ ${FORCE_INSTALL} -eq 1 ]; then
    echo "Forcing install"

    # Stop any possibly running services - may have a broken install and be wanting a re-install.
    systemctl stop sophos-spl 2>/dev/null
elif is_sspl_installed; then
    sspl_env=$(systemctl show -p Environment sophos-spl)
    sspl_env=${sspl_env#Environment=}
    for e in ${sspl_env}; do
        if [[ ${e%%=*} == "SOPHOS_INSTALL" ]]; then
            EXISTING_SSPL_PATH=${e#*=}
            echo "Found existing installation here: $EXISTING_SSPL_PATH"
            break
        fi
    done

    # Check we have found the path for the existing installation.
    if [ ! -d "$EXISTING_SSPL_PATH" ]; then
        failure ${EXITCODE_INSTALLED_BUT_NO_PATH} "An existing installation of ${PRODUCT_NAME} was found but could not find the installed path. You could try 'SophosSetup.sh --force' to force the install"
    fi

    # If the user specified a different install dir to the existing one then they must remove the old install first.
    if [[ "${SOPHOS_INSTALL}" != "${EXISTING_SSPL_PATH}" ]]; then
        failure ${EXITCODE_ALREADY_INSTALLED} "Please uninstall ${PRODUCT_NAME} before using this installer. You can run ${EXISTING_SSPL_PATH}/bin/uninstall.sh"
    fi

    # Check the existing installation is ok, if it is then register again with the creds from this installer.
    if [ -d "${SOPHOS_INSTALL}" ]; then
        if [ -f "$REGISTER_CENTRAL" ]; then
            echo "Attempting to register existing installation with Sophos Central"
            echo "Central token is [$CLOUD_TOKEN], Central URL is [$CLOUD_URL]"

            # unset Customer Token if no product selection arguments given, so we don't potentially pass it to
            # older versions of register central
            [[ -n $PRODUCT_ARGUMENTS ]] || unset CUSTOMER_TOKEN_ARGUMENT
            ${REGISTER_CENTRAL} "${CLOUD_TOKEN}" "${CLOUD_URL}" ${MESSAGE_RELAYS} $PRODUCT_ARGUMENTS $CUSTOMER_TOKEN_ARGUMENT
            if [ $? -ne 0 ]; then
                failure ${EXITCODE_FAILED_REGISTER} "ERROR: Failed to register with Sophos Central - error $?"
            fi
            cleanup_and_exit ${EXITCODE_SUCCESS}
        else
            echo "Existing installation failed validation and will be reinstalled"
        fi
    fi
elif [ -d "${SOPHOS_INSTALL}" ]; then
    # sspl not installed
    failure ${EXITCODE_BAD_INSTALL_PATH} "The intended destination for ${PRODUCT_NAME}: ${SOPHOS_INSTALL} already exists. Please either move or delete this directory." "Don't remove install directory"
fi

tar -zxf installer.tar.gz || failure ${EXITCODE_FAILED_TO_UNPACK} "ERROR: Failed to unpack thin installer: $?"
rm -f installer.tar.gz || failure ${EXITCODE_DELETE_INSTALLER_ARCHIVE_FAILED} "ERROR: Failed to delete packed thin installer: $?"

export LD_LIBRARY_PATH=installer/bin64:installer/bin32

echo "Installation process for ${PRODUCT_NAME} started"
MCS_TOKEN="$CLOUD_TOKEN"
MCS_URL="$CLOUD_URL"

# Any created directories need to be executable so that our non-root processes can go through them
umask 066
mkdir -p "${SOPHOS_INSTALL}/base/etc/sophosspl"
CREATED_INSTALL_DIRECTORY=1
umask 077

if [[ -n "$SOPHOS_LOG_LEVEL" ]]; then
    cat <<EOF >"${SOPHOS_INSTALL}/base/etc/logger.conf"
[global]
VERBOSITY = $SOPHOS_LOG_LEVEL
EOF
else
    cat <<EOF >"${SOPHOS_INSTALL}/base/etc/logger.conf"
[global]
VERBOSITY = INFO
EOF
fi

if [[ -n "$DEBUG_THIN_INSTALLER" ]]; then
    echo "[suldownloader]" >>"${SOPHOS_INSTALL}/base/etc/logger.conf"
    echo "VERBOSITY = DEBUG" >>"${SOPHOS_INSTALL}/base/etc/logger.conf"
fi

# Avoid new-line at end of file
echo -n $(hostname -f) >${SOPHOS_TEMP_DIRECTORY}/SOPHOS_HOSTNAME_F
export SOPHOS_HOSTNAME_F=${SOPHOS_TEMP_DIRECTORY}/SOPHOS_HOSTNAME_F

FORCE_UNINSTALL_SAV=$FORCE_UNINSTALL_SAV ${BIN}/installer credentials.txt ${MCS_TOKEN} ${MCS_URL} ${CUSTOMER_TOKEN_ARGUMENT} ${CMCSROUTER_MESSAGE_RELAYS} ${CMCSROUTER_PRODUCT_ARGUMENTS} "${REGISTRATION_GROUP_ARGS}" ${SOPHOS_LOG_LEVEL}
handle_register_errorcodes $?
#setup for running suldownloader
mkdir -p "${SOPHOS_INSTALL}/base/update/rootcerts"
mkdir -p "${SOPHOS_INSTALL}/base/update/var/updatescheduler"
mkdir -p "${SOPHOS_INSTALL}/var/sophosspl"
mkdir -p "${SOPHOS_INSTALL}/base/update/cache"
mkdir -p "${SOPHOS_INSTALL}/var/lock"
mkdir -p "${SOPHOS_INSTALL}/var/lock-sophosspl"

CERT=${BIN}/../rootca.crt
[[ -n ${OVERRIDE_SOPHOS_CERTS} ]] && CERT=${OVERRIDE_SOPHOS_CERTS}/rootca.crt
[[ -f ${CERT} ]] || CERT=${BIN}/../rootca.crt
cp "$CERT" "${SOPHOS_INSTALL}/base/update/rootcerts/rootca.crt"

CERT=${BIN}/../rootca384.crt
[[ -n ${OVERRIDE_SOPHOS_CERTS} ]] && CERT=${OVERRIDE_SOPHOS_CERTS}/rootca384.crt
[[ -f ${CERT} ]] || CERT=${BIN}/../rootca384.crt
cp "$CERT" "${SOPHOS_INSTALL}/base/update/rootcerts/rootca384.crt"

[[ -n ${OVERRIDE_SOPHOS_CERTS} ]] && CERT=${OVERRIDE_SOPHOS_CERTS}/rootca384_dev.crt
[[ -n ${OVERRIDE_SOPHOS_CERTS} ]] && [ -f "${CERT}" ] && cp "${CERT}" "${SOPHOS_INSTALL}/base/update/rootcerts/rootca384_dev.crt"

[[ -n "${OVERRIDE_SUS_LOCATION}" ]] && echo "URLS = $OVERRIDE_SUS_LOCATION" >>"${SOPHOS_INSTALL}/base/update/var/sdds3_override_settings.ini"
[[ -n "${OVERRIDE_SUS_LOCATION}" ]] && echo "Overriding Sophos Update Service address with $OVERRIDE_SUS_LOCATION"
[[ -n "${OVERRIDE_CDN_LOCATION}" ]] && echo "CDN_URL = $OVERRIDE_CDN_LOCATION" >>"${SOPHOS_INSTALL}/base/update/var/sdds3_override_settings.ini"
[[ -n "${OVERRIDE_CDN_LOCATION}" ]] && echo "Overriding Sophos Update address with $OVERRIDE_CDN_LOCATION"
[[ -n "${SDDS3_USE_HTTP}" ]] && echo "USE_HTTP = true" >>"${SOPHOS_INSTALL}/base/update/var/sdds3_override_settings.ini"
[[ -n "${USE_SDDS3}" ]] && echo "USE_SDDS3 = true" >>"${SOPHOS_INSTALL}/base/update/var/sdds3_override_settings.ini"

#copy mcs files in place to base
cp mcs.config "${SOPHOS_INSTALL}/base/etc"
cp mcsPolicy.config "${SOPHOS_INSTALL}/base/etc/sophosspl/mcs.config"
cp mcs_policy.config "${SOPHOS_INSTALL}/base/etc/sophosspl/mcs_policy.config"

if [[ -n ${https_proxy} ]]; then
    echo -n "${https_proxy}" >"${SOPHOS_INSTALL}/base/etc/savedproxy.config"
elif [[ -n ${HTTPS_PROXY} ]]; then
    echo -n "${HTTPS_PROXY}" >"${SOPHOS_INSTALL}/base/etc/savedproxy.config"
elif [[ -n ${http_proxy} ]]; then
    echo -n "${http_proxy}" >"${SOPHOS_INSTALL}/base/etc/savedproxy.config"
fi

INSTALL_OPTIONS_FILE="$INSTALL_OPTIONS_FILE" ${BIN}/SulDownloader update_config.json "${SOPHOS_INSTALL}/base/update/var/updatescheduler/update_report.json"
inst_ret=$?
handle_installer_errorcodes ${inst_ret}

cleanup_and_exit ${inst_ret}
__MIDDLE_BIT__
__ARCHIVE_BELOW__
