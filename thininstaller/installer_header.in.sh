#!/bin/bash

umask 077

echo "This software is governed by the terms and conditions of a licence agreement with Sophos Limited"

args="$*"
VERSION=@PRODUCT_VERSION_REPLACEMENT_STRING@
PRODUCT_NAME="Sophos Linux Protection"
INSTALL_FILE=$0
# Display help
if [ "x$args" = "x--help" ] || [ "x$args" = "x-h" ]
then
    echo "${PRODUCT_NAME} Installer, help:"
    echo "Usage: [options]"
    echo "Valid options are:"
    echo -e "--help [-h]\t\t\tDisplay this summary"
    echo -e "--version [-v]\t\t\tDisplay version of installer"
    echo -e "--force\t\t\t\tForce re-install"
    echo -e "--group=<group>\t\t\tAdd this endpoint into the Sophos Central group specified"
    echo -e "--group=<path to sub group>\tAdd this endpoint into the Sophos Central nested\n\t\t\t\tgroup specified where path to the nested group\n\t\t\t\tis each group separated by a backslash\n\t\t\t\ti.e. --group=<top-level group>\\\\\<sub-group>\\\\\<bottom-level-group>\n\t\t\t\tor --group='<top-level group>\\\<sub-group>\\\<bottom-level-group>'"
    exit 0
fi

if [ "x$args" = "x--version" ] || [ "x$args" = "x-v" ]
then
    echo "${PRODUCT_NAME} Installer, version: $VERSION"
    exit 0
fi

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
EXITCODE_GROUP_NAME_EXCEEDES_MAX_SIZE=25

SOPHOS_INSTALL="/opt/sophos-spl"
PROXY_CREDENTIALS=

# TODO: Determine a sensible group name size
MAX_GROUP_NAME_SIZE=1000

BUILD_LIBC_VERSION=@BUILD_SYSTEM_LIBC_VERSION@
system_libc_version=$(ldd --version | grep 'ldd (.*)' | rev | cut -d ' ' -f 1 | rev)

function cleanup_and_exit()
{
    [ -z "$OVERRIDE_INSTALLER_CLEANUP" ] && rm -rf "$SOPHOS_TEMP_DIRECTORY"
    exit $1
}

function failure()
{
    code=$1
    shift
    echo "$@" >&2
    cleanup_and_exit ${code}
}

function build_version_less_than_system_version()
{
    lowest_version=$(printf '%s\n' ${BUILD_LIBC_VERSION} ${system_libc_version} | sort -V | head -n 1) || failure ${EXIT_FAIL_COULD_NOT_FIND_LIBC_VERSION}  "Couldn't determine libc version"
    test "${lowest_version}" != ${BUILD_LIBC_VERSION}
}

if build_version_less_than_system_version
then
    failure ${EXIT_FAIL_WRONG_LIBC_VERSION} "Failed to install on unsupported system. Detected GLIBC version ${system_libc_version} < required ${BUILD_LIBC_VERSION}"
fi

function handle_installer_errorcodes()
{
    errcode=$1
    if [ ${errcode} -eq 44 ]
    then
        echo "Cannot connect to Sophos Central - please check your network connections" >&2
        cleanup_and_exit ${EXITCODE_NO_CENTRAL}
    elif [ ${errcode} -eq 0 ]
    then
        echo "Finished downloading base installer"
    else
        echo "Failed to download the base installer! (Error code = $errcode)" >&2
        cleanup_and_exit ${EXITCODE_DOWNLOAD_FAILED}
    fi
}

function check_free_storage()
{
    local space=$1

    local install_path=${SOPHOS_INSTALL%/*}

    # Make sure that the install_path string is not empty, in the case of "/foo"
    if [ -z $install_path ]
    then
        install_path="/"
    fi

    if ! echo "$install_path" | grep -q ^/
    then
        echo "Please specify an absolute path, starting with /"
        cleanup_and_exit ${EXITCODE_BAD_INSTALL_PATH}
    fi

    # Loop through directory path from right to left, finding the first part of the path that exists.
    # Then we will use the df command on that path.  df command will fail if used on a path that does not exist.
    while [ ! -d ${install_path} ]
    do
        install_path=${install_path%/*}

        # Make sure that the install_path string is not empty.
        if [ -z $install_path ]
        then
            install_path="/"
        fi
    done

    local free=$(df -kP ${install_path} | sed -e "1d" | awk '{print $4}')
    local mountpoint=$(df -kP ${install_path} | sed -e "1d" | awk '{print $6}')

    local free_mb
    free_mb=$(( free / 1024 ))

    if [ ${free_mb} -gt ${space} ]
    then
        return 0
    fi
    echo "Not enough space in $mountpoint to install ${PRODUCT_NAME}. You can install elsewhere by re-running this installer with the --instdir argument"
    cleanup_and_exit ${EXITCODE_NOT_ENOUGH_SPACE}
}

function check_install_path_has_correct_permissions()
{
    # loop through given install path from right to left to find the first directory that exists.
    local install_path=${SOPHOS_INSTALL%/*}

    # Make sure that the install_path string is not empty, in the case of "/foo"
    if [ -z $install_path ]
    then
        install_path="/"
    fi

    while [ ! -d ${install_path} ]
    do
        install_path=${install_path%/*}

        # Make sure that the install_path string is not empty.
        if [ -z $install_path ]
        then
            install_path="/"
        fi
    done

    # continue looping through all the existing directories ensuring that the current permissions will allow sophos-spl to execute

    while [ ${install_path} != "/" ]
    do
        permissions=$(stat -c '%A' ${install_path})
        if [[ ${permissions: -1} != "x" ]]
        then
            echo "Can not install to ${SOPHOS_INSTALL} because ${install_path} does not have correct execute permissions. Requires execute rights for all users"
            cleanup_and_exit ${EXITCODE_BAD_INSTALL_PATH}
        fi

        install_path=${install_path%/*}

        if [ -z $install_path ]
        then
            install_path="/"
        fi
     done
}

function check_total_mem()
{
    local neededMemKiloBytes=$1
    local totalMemKiloBytes=$(grep MemTotal /proc/meminfo | awk '{print $2}')

    if [ ${totalMemKiloBytes} -gt ${neededMemKiloBytes} ]
    then
        return 0
    fi
    echo "This machine does not meet product requirements. The product requires at least 1GB of RAM"
    cleanup_and_exit ${EXITCODE_NOT_ENOUGH_MEM}
}

function check_SAV_installed()
{
    local path=$1
    local sav_instdir=`readlink ${path} | sed 's/bin\/savscan//g'`
    if [ "$sav_instdir" != "" ] && [ -d ${sav_instdir} ]
    then
        echo "Found an existing installation of SAV in $sav_instdir. This product cannot be run alongside Sophos Anti-Virus"
        cleanup_and_exit ${EXITCODE_SAV_INSTALLED}
    fi
}

#
# Use mktemp if it is available, or try to create unique
# directory name and use mkdir instead.
#
# Pass directory name prefix as a parameter.
#
function sophos_mktempdir()
{
    _mktemp=`which mktemp 2>/dev/null`
    if [ -x "${_mktemp}" ] ; then
        # mktemp exists - use it
        _tmpdirTemplate="$TMPDIR/$1_XXXXXXX"
        _tmpdir=`${_mktemp} -d ${_tmpdirTemplate}`
        [ $? = 0 ] || { echo "Could not create temporary directory" 1>&2 ; exit 1 ; }
    else
        _od=`which od 2>/dev/null`
        if [ -x "${_od}" ] ; then
            # mktemp not available - use /dev/urandom, od and mkdir
            _random=/dev/urandom
            [ -f "${_random}" ] || _random=/dev/random
            _tmpdir=$TMPDIR/$1_`${_od} -An -N16 -tu2 ${_random} | tr -d " \t\r\n"`.$$
        else
            # mktemp not available - /dev/urandom not available - use $RANDOM and mkdir
            _tmpdir=$TMPDIR/$1_${RANDOM-0}.${RANDOM-0}.${RANDOM-0}.$$
        fi
        [ -d ${_tmpdir} ] && { echo "Temporary directory already exists" 1>&2 ; exit 1 ; }
        (umask 077 && mkdir ${_tmpdir}) || { echo "Could not create temporary directory" 1>&2 ; exit 1 ; }
    fi

    if [ ! -d "${_tmpdir}" ] ; then
        echo "Could not create temporary directory" 1>&2
        exit ${EXITCODE_CANNOT_MAKE_TEMP}
    fi

    echo ${_tmpdir}
}

function is_sspl_installed()
{
    systemctl list-unit-files | grep -q sophos-spl
}

function force_argument()
{
    echo "$args" | grep -q ".*--force"
}

function check_for_duplicate_arguments()
{
    declare -a checked_arguments
    for argument in "$@"; do
        argument_name="${argument%=*}"
        [[ "${checked_arguments[@]}" =~ "$argument_name" ]] && failure ${EXITCODE_DUPLICATE_ARGUMENTS_GIVEN}
        checked_arguments+=("$argument_name")
    done
}

function validate_group_name()
{
    [ -z "$1" ] && failure ${EXITCODE_BAD_GROUP_NAME}
    [[ ( ${#1} > MAX_GROUP_NAME_SIZE ) ]] && failure ${EXITCODE_GROUP_NAME_EXCEEDES_MAX_SIZE}
}

# Check that the OS is Linux
uname -a | grep -i Linux >/dev/null
if [ $? -eq 1 ] ; then
    echo "This installer only runs on Linux" >&2
    cleanup_and_exit ${EXITCODE_NOT_LINUX}
fi

# Check running as root
if [ $(id -u) -ne 0 ]; then
    echo "Please run this installer as root" >&2
    cleanup_and_exit ${EXITCODE_NOT_ROOT}
fi

# Check machine architecture (only support 64 bit)
MACHINE_TYPE=`uname -m`
if [ ${MACHINE_TYPE} = "x86_64" ]; then
    BIN="installer/bin"
else
    echo "This product can only be installed on a 64bit system"
    cleanup_and_exit ${EXITCODE_NOT_64_BIT}
fi

# Check if SAV is installed.
## We check everything on $PATH, and always /usr/local/bin and /usr/bin
## This should catch everywhere SAV might have installed the sweep symlink
SWEEP=$(which sweep 2>/dev/null)
[ -x "$SWEEP" ] && check_SAV_installed "$SWEEP"
check_SAV_installed '/usr/local/bin/sweep'
check_SAV_installed '/usr/bin/sweep'
declare -a INSTALL_OPTIONS_ARGS
# Handle arguments
check_for_duplicate_arguments "$@"
for i in "$@"
do
    case $i in
        --update-source-creds=*)
            export OVERRIDE_SOPHOS_CREDS="${i#*=}"
            shift
        ;;
        --instdir=*)
            SOPHOS_INSTALL="${i#*=}"
            if ((${#SOPHOS_INSTALL} > 50))
            then
                echo "The --instdir path provided is too long and needs to be 50 characters or less. ${SOPHOS_INSTALL} is ${#SOPHOS_INSTALL} characters long"
                cleanup_and_exit ${EXITCODE_BAD_INSTALL_PATH}
            fi
            if [[ ${SOPHOS_INSTALL} == /tmp* ]]
            then
                echo "The --instdir path provided is in the non-persistent /tmp folder. Please choose a location that is persistent"
                cleanup_and_exit ${EXITCODE_BAD_INSTALL_PATH}
            fi
            export SOPHOS_INSTALL="${SOPHOS_INSTALL}/sophos-spl"
            shift
        ;;
        --proxy-credentials=*)
            export PROXY_CREDENTIALS="${i#*=}"
        ;;
        --allow-override-mcs-ca)
            ALLOW_OVERRIDE_MCS_CA=--allow-override-mcs-ca
            shift
        ;;
        --group=*)
            validate_group_name "${i#*=}"
            INSTALL_OPTIONS_ARGS+=("$i")
            shift
        ;;
        *)
            failure ${EXITCODE_UNEXPECTED_ARGUMENT}
        ;;
    esac
done

# Verify that instdir does not contain special characters that may cause problems.
if ! echo "$SOPHOS_INSTALL" | grep -q '^[-a-zA-Z0-9\/\_\.]*$'
then
    echo "The --instdir path provided contains invalid characters. Only alphanumeric and '/' '-' '_' '.' characters are accepted"
    cleanup_and_exit ${EXITCODE_BAD_INSTALL_PATH}
fi

# Check to see if the Sophos credentials override is being used.
[ -n "$OVERRIDE_SOPHOS_CREDS" ] && {
    echo "Overriding Sophos credentials with $OVERRIDE_SOPHOS_CREDS"
}

# If TMPDIR has not been set then default it to /tmp
if [ -z "$TMPDIR" ] ; then
    TMPDIR=/tmp
    export TMPDIR
fi

# Create Sophos temp directory (unless overridden with an existing dir)
if [ -z "$SOPHOS_TEMP_DIRECTORY" ]; then
    SOPHOS_TEMP_DIRECTORY=`sophos_mktempdir SophosCentralInstall`
fi

mkdir -p "$SOPHOS_TEMP_DIRECTORY"

# Check that the tmp directory we're using allows execution
echo "exit 0" > "$SOPHOS_TEMP_DIRECTORY/exectest" && chmod +x "$SOPHOS_TEMP_DIRECTORY/exectest"
$SOPHOS_TEMP_DIRECTORY/exectest || failure ${EXITCODE_NOEXEC_TMP} "Cannot execute files within $TMPDIR directory. Please see KBA 131783 http://www.sophos.com/kb/131783"

# Get line numbers for the each of the sections
MIDDLEBIT=`awk '/^__MIDDLE_BIT__/ {print NR + 1; exit 0; }' "${INSTALL_FILE}"`
UC_CERTS=`awk '/^__UPDATE_CACHE_CERTS__/ {print NR + 1; exit 0; }' "${INSTALL_FILE}"`
ARCHIVE=`awk '/^__ARCHIVE_BELOW__/ {print NR + 1; exit 0; }' "${INSTALL_FILE}"`

# If we have __UPDATE_CACHE_CERTS__ section then the middle section ends there, else it ends at the ARCHIVE marker.
if [ -n "$UC_CERTS" ]
then
    mkdir -p "${SOPHOS_TEMP_DIRECTORY}/installer"
    UPDATE_CACHE_CERT="${SOPHOS_TEMP_DIRECTORY}/installer/uc_certs.crt"
    MIDDLEBIT_SIZE=`expr ${UC_CERTS} - ${MIDDLEBIT} - 1`
    UC_CERTS_SIZE=`expr ${ARCHIVE} - ${UC_CERTS} - 1`
    tail -n+${UC_CERTS} "${INSTALL_FILE}" | head -${UC_CERTS_SIZE}  | tr -d '\r' | tr -s '\n' > ${UPDATE_CACHE_CERT}
else
    MIDDLEBIT_SIZE=`expr $ARCHIVE - $MIDDLEBIT - 1`
fi

tail -n+${MIDDLEBIT} "${INSTALL_FILE}" | head -${MIDDLEBIT_SIZE} > ${SOPHOS_TEMP_DIRECTORY}/credentials.txt
tail -n+${ARCHIVE} "${INSTALL_FILE}" > ${SOPHOS_TEMP_DIRECTORY}/installer.tar.gz

cd ${SOPHOS_TEMP_DIRECTORY}

# Read cloud token from credentials file.
if [ -z "${OVERRIDE_CLOUD_TOKEN}" ]
then
    CLOUD_TOKEN=$(grep 'TOKEN=' credentials.txt | sed 's/TOKEN=//')
else
    CLOUD_TOKEN=${OVERRIDE_CLOUD_TOKEN}
fi

# Read cloud URL from credentials file.
if [ -z "${OVERRIDE_CLOUD_URL}" ]
then
    CLOUD_URL=$(grep 'URL=' credentials.txt | sed 's/URL=//')
else
    CLOUD_URL=${OVERRIDE_CLOUD_URL}
fi

# Read possible Message Relays from credentials file.
MESSAGE_RELAYS=$(grep 'MESSAGE_RELAYS=' credentials.txt | sed 's/MESSAGE_RELAYS=//')
if [ -n "$MESSAGE_RELAYS" ]
then
    if [ -n "$DEBUG_THIN_INSTALLER" ]
    then
      echo "Message Relays: $MESSAGE_RELAYS"
    fi
    MESSAGE_RELAYS="--messagerelay $MESSAGE_RELAYS"
fi

# Save installer arguments to file for base and plugins to use.
INSTALL_OPTIONS_FILE="${SOPHOS_TEMP_DIRECTORY}/install_options"

# File format expects the args to be either --option  or --option=value
for value in "${INSTALL_OPTIONS_ARGS[@]}"
do
     echo $value >> ${INSTALL_OPTIONS_FILE}
done
# Read possible Update Caches from credentials file.
UPDATE_CACHES=$(grep 'UPDATE_CACHES=' credentials.txt | sed 's/UPDATE_CACHES=//')
if [ -n "$UPDATE_CACHES" ]
then
    if [ -n "$DEBUG_THIN_INSTALLER" ]
    then
        echo "List of update caches to install from: $UPDATE_CACHES"
    fi
fi

REGISTER_CENTRAL="${SOPHOS_INSTALL}/base/bin/registerCentral"

# Check if the sophos-spl service is installed, if it is find the install path.
EXISTING_SSPL_PATH=

if force_argument
then
    echo "Forcing install"

    # Base installer will install sophos unit files even if they are already installed.
    FORCE_INSTALL=1

    # Stop any possibly running services - may have a broken install and be wanting a re-install.
    systemctl stop sophos-spl 2>/dev/null
elif is_sspl_installed
then
    sspl_env=$(systemctl show -p Environment sophos-spl)
    sspl_env=${sspl_env#Environment=}
    for e in ${sspl_env}
    do
        if [[ ${e%%=*} == "SOPHOS_INSTALL" ]]
        then
            EXISTING_SSPL_PATH=${e#*=}
            echo "Found existing installation here: $EXISTING_SSPL_PATH"
            break
        fi
    done

    # Check we have found the path for the existing installation.
    if [ ! -d "$EXISTING_SSPL_PATH" ]
    then
        echo "An existing installation of ${PRODUCT_NAME} was found but could not find the installed path. You could try 'SophosSetup.sh --force' to force the install"
        cleanup_and_exit ${EXITCODE_INSTALLED_BUT_NO_PATH}
    fi

    # If the user specified a different install dir to the existing one then they must remove the old install first.
    if [[ ${SOPHOS_INSTALL} != ${EXISTING_SSPL_PATH} ]]
    then
        echo "Please uninstall ${PRODUCT_NAME} before using this installer. You can run ${EXISTING_SSPL_PATH}/bin/uninstall.sh" >&2
        cleanup_and_exit ${EXITCODE_ALREADY_INSTALLED}
    fi

    # Check the existing installation is ok, if it is then register again with the creds from this installer.
    if [ -d "$SOPHOS_INSTALL" ]
    then
        if [ -f "$REGISTER_CENTRAL" ]
        then
            echo "Attempting to register existing installation with Sophos Central"
            echo "Central token is [$CLOUD_TOKEN], Central URL is [$CLOUD_URL]"
            ${REGISTER_CENTRAL} ${CLOUD_TOKEN} ${CLOUD_URL} ${MESSAGE_RELAYS}
            if [ $? -ne 0 ]; then
                echo "ERROR: Failed to register with Sophos Central - error $?" >&2
                cleanup_and_exit ${EXITCODE_FAILED_REGISTER}
            fi
            cleanup_and_exit ${EXITCODE_SUCCESS}
        else
            echo "Existing installation failed validation and will be reinstalled"
        fi
    fi
else  # sspl not installed
    if [ -d ${SOPHOS_INSTALL} ]
    then
        echo "The intended destination for ${PRODUCT_NAME}: ${SOPHOS_INSTALL} already exists. Please either delete this folder or choose another location" >&2
        cleanup_and_exit ${EXITCODE_BAD_INSTALL_PATH}
    fi
fi
# Check there is enough disk space
check_free_storage 1024

# Check that the install path has valid permission on the existing directories
check_install_path_has_correct_permissions

# Check there is enough RAM (~1GB in kB)
check_total_mem 1000000

tar -zxf installer.tar.gz || failure ${EXITCODE_FAILED_TO_UNPACK} "ERROR: Failed to unpack thin installer: $?"
rm -f installer.tar.gz || failure ${EXITCODE_DELETE_INSTALLER_ARCHIVE_FAILED} "ERROR: Failed to delete packed thin installer: $?"

export LD_LIBRARY_PATH=installer/bin64:installer/bin32

# Make necessary directories
mkdir distribute
mkdir cache
mkdir warehouse
mkdir warehouse/catalogue

echo "Installation process for ${PRODUCT_NAME} started"
${BIN}/installer credentials.txt
handle_installer_errorcodes $?

if [ -n "$DEBUG_THIN_INSTALLER" ]
then
    echo "Validating downloaded installer"
fi

# Verify manifest.dat
CERT=installer/rootca.crt
[ -n ${OVERRIDE_SOPHOS_CERTS} ] && CERT=${OVERRIDE_SOPHOS_CERTS}/rootca.crt
[ -f ${CERT} ] || CERT=installer/rootca.crt

DEBUG_VERSIG=
if [ -n "$DEBUG_THIN_INSTALLER" ]
then
    DEBUG_VERSIG=--silent-off
fi

${BIN}/versig -c$CERT -fdistribute/manifest.dat -ddistribute --check-install-sh $DEBUG_VERSIG \
    || failure ${EXITCODE_VERIFY_INSTALLER_FAILED} "ERROR: Failed to verify base installer: $?"

[ -z "$OVERRIDE_PROD_SOPHOS_CERTS" ] || cp ${OVERRIDE_PROD_SOPHOS_CERTS}/* distribute/update/certificates/

credentials=""
if [ -z "$OVERRIDE_SOPHOS_CREDS" ]
then
    creds=$(grep 'UPDATE_CREDENTIALS=' credentials.txt | sed 's/UPDATE_CREDENTIALS=//')
    credentials="--update-type=s --update-source-username=$creds --update-source-password=$creds"
else
    credentials="--update-type=s --update-source-username=$OVERRIDE_SOPHOS_CREDS --update-source-password=$OVERRIDE_SOPHOS_CREDS"
fi

sslca=""
if [ -n "$OVERRIDE_SSL_SOPHOS_CERTS" ]
then
    sslca="--PrimaryUpdateSSLSophosCA=$OVERRIDE_SSL_SOPHOS_CERTS"
fi

update_caches=""
if [ -n "$UPDATE_CACHES" ]
then
    update_caches="--UpdateCacheLocations=$UPDATE_CACHES --UpdateCacheSSLCA=$UPDATE_CACHE_CERT"
fi

cd distribute

chmod u+x install.sh || failure ${EXITCODE_CHMOD_FAILED} "Failed to chmod base installer: $?"

echo "Running base installer"
echo "Product will be installed to: ${SOPHOS_INSTALL}"
MCS_TOKEN="$CLOUD_TOKEN" MCS_URL="$CLOUD_URL" MCS_MESSAGE_RELAYS="$MESSAGE_RELAYS" INSTALL_OPTIONS_FILE="$INSTALL_OPTIONS_FILE" ./install.sh $ALLOW_OVERRIDE_MCS_CA
inst_ret=$?
if [ ${inst_ret} -ne 0 ] && [ ${inst_ret} -ne 4 ]
then
    failure ${EXITCODE_BASE_INSTALL_FAILED} "ERROR: Installer returned $inst_ret (see above messages)"
fi

cleanup_and_exit ${inst_ret}
__MIDDLE_BIT__
__ARCHIVE_BELOW__
