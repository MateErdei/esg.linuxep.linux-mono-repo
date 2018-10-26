#!/bin/bash

umask 077

args="$*"

echo "This software is governed by the terms and conditions of a licence agreement with Sophos Limited."

echo "Installing Sophos Server Protection for Linux with arguments: [$args]"

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

SOPHOS_INSTALL="/opt/sophos-spl"
PROXY_CREDENTIALS=

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

function handle_installer_errorcodes()
{
    errcode=$1
    if [ ${errcode} -eq 44 ]
    then
        echo "Cannot connect to Sophos Central." >&2
        cleanup_and_exit ${EXITCODE_NO_CENTRAL}
    elif [ ${errcode} -eq 0 ]
    then
        echo "Finished downloading base installer."
    else
        echo "Failed to download the base installer! (Error code = $errcode)" >&2
        cleanup_and_exit ${EXITCODE_DOWNLOAD_FAILED}
    fi
}

function check_free_storage()
{
    local path=$1
    local space=$2

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
    echo "Not enough space in $mountpoint to install Sophos Server Protection for Linux. You can install elsewhere by re-running this installer with the --instdir argument."
    cleanup_and_exit ${EXITCODE_NOT_ENOUGH_SPACE}
}

function check_total_mem()
{
    local neededMemKiloBytes=$1
    local totalMemKiloBytes=$(grep MemTotal /proc/meminfo | awk '{print $2}')

    if [ ${totalMemKiloBytes} -gt ${neededMemKiloBytes} ]
    then
        return 0
    fi
    echo "This machine does not meet product requirements. The product requires at least 1GB of RAM."
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

# Check that the OS is Linux
uname -a | grep -i Linux >/dev/null
if [ $? -eq 1 ] ; then
    echo "This installer only runs on Linux." >&2
    cleanup_and_exit ${EXITCODE_NOT_LINUX}
fi

# Check running as root
if [ $(id -u) -ne 0 ]; then
    echo "Please run this installer as root." >&2
    cleanup_and_exit ${EXITCODE_NOT_ROOT}
fi

# Check machine architecture (only support 64 bit)
MACHINE_TYPE=`uname -m`
if [ ${MACHINE_TYPE} = "x86_64" ]; then
    BIN="installer/bin"
else
    echo "This product can only be installed on a 64bit system."
    cleanup_and_exit ${EXITCODE_NOT_64_BIT}
fi

# Check if SAV is installed.
# TODO LINUXEP-6541: we should check if it's centrally managed, potentially going to allow non-centrally managed endpoints to have SSPL installed alongside.
check_SAV_installed '/usr/local/bin/sweep'
check_SAV_installed '/usr/bin/sweep'

# Handle arguments
for i in "$@"
do
    case $i in
        --update-source-creds=*)
            export OVERRIDE_SOPHOS_CREDS="${i#*=}"
            shift
        ;;
        --instdir=*)
            export SOPHOS_INSTALL="${i#*=}"
            shift
        ;;
        --proxy-credentials=*)
            export PROXY_CREDENTIALS="${i#*=}"
        ;;
    esac
done

REGISTER_CENTRAL="${SOPHOS_INSTALL}/base/bin/registerCentral"

# Verify that instdir does not contain special characters that may cause problems.
if ! echo "$SOPHOS_INSTALL" | grep -q '^[-a-Z0-9\/\_\.]*$'
then
    echo "The --instdir path provided contains invalid characters. Only alphanumeric and '/' '-' '_' '.' characters are accepted."
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
MIDDLEBIT=`awk '/^__MIDDLE_BIT__/ {print NR + 1; exit 0; }' $0`
UC_CERTS=`awk '/^__UPDATE_CACHE_CERTS__/ {print NR + 1; exit 0; }' $0`
ARCHIVE=`awk '/^__ARCHIVE_BELOW__/ {print NR + 1; exit 0; }' $0`

# If we have __UPDATE_CACHE_CERTS__ section then the middle section ends there, else it ends at the ARCHIVE marker.
if [ -n "$UC_CERTS" ]
then
    mkdir -p "${SOPHOS_TEMP_DIRECTORY}/installer"
    UPDATE_CACHE_CERT="${SOPHOS_TEMP_DIRECTORY}/installer/uc_certs.crt"
    MIDDLEBIT_SIZE=`expr ${UC_CERTS} - ${MIDDLEBIT} - 1`
    UC_CERTS_SIZE=`expr ${ARCHIVE} - ${UC_CERTS} - 1`
    tail -n+${UC_CERTS} $0 | head -${UC_CERTS_SIZE}  | tr -d '\r' | tr -s '\n' > ${UPDATE_CACHE_CERT}
else
    MIDDLEBIT_SIZE=`expr $ARCHIVE - $MIDDLEBIT - 1`
fi

tail -n+${MIDDLEBIT} $0 | head -${MIDDLEBIT_SIZE} > ${SOPHOS_TEMP_DIRECTORY}/credentials.txt
tail -n+${ARCHIVE} $0 > ${SOPHOS_TEMP_DIRECTORY}/installer.tar.gz

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
    echo "Message Relays: $MESSAGE_RELAYS"
    MESSAGE_RELAYS="--messagerelay $MESSAGE_RELAYS"
fi

# Read possible Update Caches from credentials file.
UPDATE_CACHES=$(grep 'UPDATE_CACHES=' credentials.txt | sed 's/UPDATE_CACHES=//')
if [ -n "$UPDATE_CACHES" ]
then
    echo "Update Caches: $UPDATE_CACHES"
fi

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
        echo "An existing installation of Sophos Server Protection for Linux was found but could not find the installed path. You could try 'SophosSetup.sh --force' to force the install."
        cleanup_and_exit ${EXITCODE_INSTALLED_BUT_NO_PATH}
    fi

    # If the user specified a different install dir to the existing one then they must remove the old install first.
    if [[ ${SOPHOS_INSTALL} != ${EXISTING_SSPL_PATH} ]]
    then
        echo "Please uninstall Sophos Server Protection for Linux before using this installer. You can run ${EXISTING_SSPL_PATH}/bin/uninstall.sh" >&2
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
            echo "Existing installation failed validation and will be reinstalled."
        fi
    fi
fi

# Check there is enough disk space
check_free_storage ${SOPHOS_INSTALL} 1024

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

echo "Downloading base installer"
${BIN}/installer credentials.txt
handle_installer_errorcodes $?

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

echo "Running base installer (this may take some time)"
MCS_TOKEN="$CLOUD_TOKEN" MCS_URL="$CLOUD_URL" MCS_MESSAGE_RELAYS="$MESSAGE_RELAYS" ./install.sh
inst_ret=$?
if [ ${inst_ret} -ne 0 ] && [ ${inst_ret} -ne 4 ]
then
    failure ${EXITCODE_BASE_INSTALL_FAILED} "ERROR: Installer returned $inst_ret (see above messages)"
fi

cleanup_and_exit ${inst_ret}
__MIDDLE_BIT__
__ARCHIVE_BELOW__
