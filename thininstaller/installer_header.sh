#!/bin/sh

umask 077

args="$*"
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

INSTALL_LOCATION="/opt/sophos-spl"
PROXY_CREDENTIALS=

cleanup_and_exit()
{
    [ -z "$OVERRIDE_INSTALLER_CLEANUP" ] && rm -rf "$SOPHOS_TEMP_DIRECTORY"
    exit $1
}

failure()
{
    code=$1
    shift
    echo "$@" >&2
    cleanup_and_exit ${code}
}

create_symlinks()
{
    for baselib in `ls`
    do
        shortlib=${baselib}
        while extn=$(echo ${shortlib} | sed -n '/\.[0-9][0-9]*$/s/.*\(\.[0-9][0-9]*\)$/\1/p')
              [ -n "$extn" ]
        do
            shortlib=$(basename ${shortlib} ${extn})
            ln -s "$baselib" "$shortlib" || failure 14 "Failed to create library symlinks"
        done
    done
}


handle_installer_errorcodes()
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

check_free_storage()
{
    local path=$1
    local space=$2

    local install_path=${INSTALL_LOCATION%/*}
    local free=$(df -kP ${install_path} | sed -e "1d" | awk '{print $4}')
    local mountpoint=$(df -kP ${install_path} | sed -e "1d" | awk '{print $6}')

    local free_mb
    free_mb=$(( free / 1024 ))

    if [ ${free_mb} -gt ${space} ]
    then
        return 0
    fi
    echo "Not enough space in $mountpoint to install Sophos Anti-Virus for Linux. You can install elsewhere by re-running this installer with the --instdir argument."
    cleanup_and_exit ${EXITCODE_NOT_ENOUGH_SPACE}
}

check_total_mem()
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

check_SAV_installed()
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
sophos_mktempdir()
{
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

REGISTER_CENTRAL="${INSTALL_LOCATION}/base/bin/registerCentral"

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

# Handle arguments
for i in "$@"
do
    case $i in
        --update-source-creds=*)
            export OVERRIDE_SOPHOS_CREDS="${i#*=}"
            shift
        ;;
        --instdir=*)
            export INSTALL_LOCATION="${i#*=}"
            shift
        ;;
        --proxy-credentials=*)
            export PROXY_CREDENTIALS="${i#*=}"
        ;;
    esac
done

[ -n "$OVERRIDE_SOPHOS_CREDS" ] && {
    echo "Overriding Sophos credentials with $OVERRIDE_SOPHOS_CREDS"
}

if [ -z "$TMPDIR" ] ; then
    TMPDIR=/tmp
    export TMPDIR
fi

if [ -z "$SOPHOS_TEMP_DIRECTORY" ]; then
    SOPHOS_TEMP_DIRECTORY=`sophos_mktempdir SophosCentralInstall`
fi

mkdir -p "$SOPHOS_TEMP_DIRECTORY"

echo "exit 0" > "$SOPHOS_TEMP_DIRECTORY/exectest" && chmod +x "$SOPHOS_TEMP_DIRECTORY/exectest"
$SOPHOS_TEMP_DIRECTORY/exectest || failure 15 "Cannot execute files within $TMPDIR directory. Please see KBA 131783 http://www.sophos.com/kb/131783"

MIDDLEBIT=`awk '/^__MIDDLE_BIT__/ {print NR + 1; exit 0; }' $0`
UC_CERTS=`awk '/^__UPDATE_CACHE_CERTS__/ {print NR + 1; exit 0; }' $0`
ARCHIVE=`awk '/^__ARCHIVE_BELOW__/ {print NR + 1; exit 0; }' $0`

# If we have __UPDATE_CACHE_CERTS__ section then the middle section ends there.
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

# Check if SAV is installed. TODO we should check if it's centrally managed, potentially going to allow non-centrally managed endpoints to have SSPL installed alongside.
check_SAV_installed '/usr/local/bin/sweep'
check_SAV_installed '/usr/bin/sweep'

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

# Check if there is already an installation. Re-register if there is.
if [ -d ${INSTALL_LOCATION} ]
then
    if [ -f "$REGISTER_CENTRAL" ]
    then
        echo "Attempting to re-register existing installation with Sophos Central"
        echo "Cloud token is [$CLOUD_TOKEN], Cloud URL is [$CLOUD_URL]"
        ${REGISTER_CENTRAL} ${CLOUD_TOKEN} ${CLOUD_URL} ${MESSAGE_RELAYS}

        if [ $? -ne 0 ]; then
            echo "ERROR: Failed to register with Sophos Central - error $?" >&2
            cleanup_and_exit ${EXITCODE_FAILED_REGISTER}
        fi

        cleanup_and_exit ${EXITCODE_SUCCESS}
    elif ! echo "$args" | grep -q ".*--ignore-existing-installation.*"
    then
        echo "Please uninstall SSPL before using this installer." >&2
        cleanup_and_exit ${EXITCODE_ALREADY_INSTALLED}
    fi
fi

# Check there is enough disk space
check_free_storage ${INSTALL_LOCATION} 1024

# Check there is enough RAM (~1GB in kB)
check_total_mem 1000000

tar -zxf installer.tar.gz || failure ${EXITCODE_FAILED_TO_UNPACK} "ERROR: Failed to unpack thin installer: $?"
rm -f installer.tar.gz || failure 12 "ERROR: Failed to delete packed thin installer: $?"

export LD_LIBRARY_PATH=installer/bin64:installer/bin32

#todo what do these need changing to?
# Make necessary directories
mkdir distribute
mkdir cache
mkdir warehouse
mkdir warehouse/catalogue

# Check machine architecture (only support 64 bit)
MACHINE_TYPE=`uname -m`
if [ ${MACHINE_TYPE} = "x86_64" ]; then
    BIN="installer/bin"
else
    echo "This product can only be installed on a 64bit system."
    cleanup_and_exit ${EXITCODE_NOT_64_BIT}
fi

echo "Downloading base installer"
${BIN}/installer credentials.txt
handle_installer_errorcodes $?

# Verify manifest.dat
CERT=installer/rootca.crt
[ -n ${OVERRIDE_SOPHOS_CERTS} ] && CERT=${OVERRIDE_SOPHOS_CERTS}/rootca.crt
[ -f ${CERT} ] || CERT=installer/rootca.crt

${BIN}/versig -c$CERT -fdistribute/manifest.dat -ddistribute --check-install-sh \
    || failure 8 "ERROR: Failed to verify base installer: $?"

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

installer_args=""
if [ -n "$DEBUG_THIN_INSTALLER" ]
then
    installer_args="-v5 --debug"
fi

cd distribute

chmod u+x install.sh || failure 13 "Failed to chmod base installer: $?"

# todo add this back once we know what libs we need to symlink (if any)
#if [ $MACHINE_TYPE = "x86_64" ]
#then
#    cd lib64
#else
#    cd lib32
#fi
#create_symlinks
#cd ..

echo "Running base installer (this may take some time)"
./install.sh ${installer_args}
inst_ret=$?
if [ ${inst_ret} -ne 0 ] && [ $inst_ret -ne 4 ]
then
    failure 10 "ERROR: Installer returned $inst_ret (see above messages)"
fi

${REGISTER_CENTRAL} ${CLOUD_TOKEN} ${CLOUD_URL} ${MESSAGE_RELAYS}
ret=$?
if [ ${ret} -ne 0 ]
then
    failure ${EXITCODE_FAILED_REGISTER} "ERROR: Failed to register with Sophos Central - error $ret"
fi

cleanup_and_exit ${inst_ret}
__MIDDLE_BIT__
