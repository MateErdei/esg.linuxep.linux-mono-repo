#!/bin/sh

umask 077

args="$*"
echo "Installing Sophos Anti-Virus for Linux with arguments: [$args]"

cleanupAndExit()
{
    [ -z "$OVERRIDE_INSTALLER_CLEANUP" ] && rm -rf "$SOPHOS_TEMP_DIRECTORY"
    exit $1
}

failure()
{
    code=$1
    shift
    echo "$@" >&2
    cleanupAndExit $code
}

createSymlinks()
{
    for baselib in `ls`
    do
        shortlib=$baselib
        while extn=$(echo $shortlib | sed -n '/\.[0-9][0-9]*$/s/.*\(\.[0-9][0-9]*\)$/\1/p')
              [ -n "$extn" ]
        do
            shortlib=$(basename $shortlib $extn)
            ln -s "$baselib" "$shortlib" || failure 14 "Failed to create library symlinks"
        done
    done
}

uname -a | grep -i Linux >/dev/null
if [ $? -eq 1 ] ; then
    echo "This installer only runs on Linux." >&2
    cleanupAndExit 1
fi

if [ $(id -u) -ne 0 ]; then
    echo "Please run this installer as root." >&2
    cleanupAndExit 2
fi

INSTALL_LOCATION=/opt/sophos-av
PROXY_CREDENTIALS=
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
    echo "Overriding sophos credentials with $OVERRIDE_SOPHOS_CREDS"
}

handleInstallerErrorcodes()
{
    errcode=$1
    if [ $errcode -eq 44 ]
    then
        echo "Cannot connect to Sophos Central." >&2
        cleanupAndExit 3
    elif [ $errcode -eq 0 ]
    then
        echo "Finished downloading the medium installer."
    else
        echo "Failed to download the medium installer! (Error code = $errcode)" >&2
        cleanupAndExit 10
    fi
}

check_free()
{
    local path=$1
    local space=$2

    local install_path=${INSTALL_LOCATION%/*}
    local free=$(df -kP $install_path | sed -e "1d" | awk '{print $4}')
    local mountpoint=$(df -kP $install_path | sed -e "1d" | awk '{print $6}')

    local free_mb
    free_mb=$(( free / 1024 ))

    if [ $free_mb -gt $space ]
    then
        return 0
    fi
    echo "Not enough space in $mountpoint to install Sophos Anti-Virus for Linux. You can install elsewhere by re-running this installer with the --instdir argument."
    cleanupAndExit 5
}


#
# Use mktemp if it is available, or try to create unique
# directory name and use mkdir instead.
#
# Pass directory name prefix as a parameter.
#
sophos_mktempdir()
{
    # On HP-UX mktemps just outputs a suitable file name
    if [ "$PLATFORM" != "hpux" ]; then
        _mktemp=`which mktemp 2>/dev/null`
    fi

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
        exit 1
    fi

    echo ${_tmpdir}
}

# For Message Relays: Min SAV version 9.14.2, Min SLS version 10.3.2
support_message_relays()
{
    if [ -f "${INSTDIR}/engine/savShortVersion" ] ; then
        INSTALLED_VERSION=$(cat ${INSTDIR}/engine/savShortVersion)
        if is_version_same_or_after $INSTALLED_VERSION "9.14.2" "10.3.2"; then
            return 0
        fi
    fi
    echo "Current install does not support Message Relays"
    return 1
}

is_version_same_or_after()
{
    # This function assumes that SLS version is always higher than SAV version.
    INSTALLED_VERSION=$1
    COMPARE_SAV_VERSION=$2
    COMPARE_SLS_VERSION=$3

    IFS='.' read -r INSTALLED_VERSION_1 INSTALLED_VERSION_2 INSTALLED_VERSION_3 << EOM
$INSTALLED_VERSION
EOM

    IFS='.' read -r COMPARE_SAV_VERSION_1 COMPARE_SAV_VERSION_2 COMPARE_SAV_VERSION_3 << EOM
$COMPARE_SAV_VERSION
EOM

    IFS='.' read -r COMPARE_SLS_VERSION_1 COMPARE_SLS_VERSION_2 COMPARE_SLS_VERSION_3 << EOM
$COMPARE_SLS_VERSION
EOM

    if [ $INSTALLED_VERSION_1 -gt $COMPARE_SLS_VERSION_1 ] ; then
        return 0
    elif [ $INSTALLED_VERSION_1 -lt $COMPARE_SAV_VERSION_1 ] ; then
        return 1
    fi

    # Check SAV version
    if [ $INSTALLED_VERSION_1 -eq $COMPARE_SAV_VERSION_1  ] ; then
        if [ $INSTALLED_VERSION_2 -lt $COMPARE_SAV_VERSION_2  ] ; then
            return 1
        elif [ $INSTALLED_VERSION_2 -gt $COMPARE_SAV_VERSION_2  ] ; then
            return 0
        else
            if [ $INSTALLED_VERSION_3 -ge $COMPARE_SAV_VERSION_3  ] ; then
                return 0
            else
                return 1
            fi
        fi
    fi

    # Check SLS version
    if [ $INSTALLED_VERSION_1 -eq $COMPARE_SLS_VERSION_1 ] ; then
        if [ $INSTALLED_VERSION_2 -lt $COMPARE_SLS_VERSION_2 ] ; then
            return 1
        elif [ $INSTALLED_VERSION_2 -gt $COMPARE_SLS_VERSION_2 ] ; then
            return 0
        else
            if [ $INSTALLED_VERSION_3 -ge $COMPARE_SLS_VERSION_3 ] ; then
                return 0
            else
                return 1
            fi
        fi
    fi
    return 1
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
    MIDDLEBIT_SIZE=`expr $UC_CERTS - $MIDDLEBIT - 1`
    UC_CERTS_SIZE=`expr $ARCHIVE - $UC_CERTS - 1`
    tail -n+$UC_CERTS $0 | head -$UC_CERTS_SIZE  | tr -d '\r' | tr -s '\n' > $UPDATE_CACHE_CERT
else
    MIDDLEBIT_SIZE=`expr $ARCHIVE - $MIDDLEBIT - 1`
fi

tail -n+$MIDDLEBIT $0 | head -$MIDDLEBIT_SIZE > $SOPHOS_TEMP_DIRECTORY/credentials.txt
tail -n+$ARCHIVE $0 > $SOPHOS_TEMP_DIRECTORY/installer.tar.gz

cd $SOPHOS_TEMP_DIRECTORY

INSTDIR=`readlink /usr/local/bin/sweep | sed 's/bin\/savscan//g'`
if [ "$INSTDIR" != "" ] && [ -d $INSTDIR ]
then
    echo "Found an existing installation of SAV in $INSTDIR"
else
    INSTDIR="/opt/sophos-av"
fi

if [ -z "$OVERRIDE_CLOUD_TOKEN" ]
then
    CLOUD_TOKEN=$(grep 'TOKEN=' credentials.txt | sed 's/TOKEN=//')
else
    CLOUD_TOKEN=$OVERRIDE_CLOUD_TOKEN
fi

if [ -z "$OVERRIDE_CLOUD_URL" ]
then
    CLOUD_URL=$(grep 'URL=' credentials.txt | sed 's/URL=//')
else
    CLOUD_URL=$OVERRIDE_CLOUD_URL
fi

MESSAGE_RELAYS=$(grep 'MESSAGE_RELAYS=' credentials.txt | sed 's/MESSAGE_RELAYS=//')
if [ -n "$MESSAGE_RELAYS" ]
then
    echo "Message Relays: $MESSAGE_RELAYS"
    MESSAGE_RELAYS="--messagerelay $MESSAGE_RELAYS"
fi

UPDATE_CACHES=$(grep 'UPDATE_CACHES=' credentials.txt | sed 's/UPDATE_CACHES=//')
if [ -n "$UPDATE_CACHES" ]
then
    echo "Update Caches: $UPDATE_CACHES"
fi

# Check if there is already an installation.
if [ -d $INSTDIR ]
then
    if [ -f "${INSTDIR}/engine/registerMCS" ]
    then
        echo "Attempting to register existing installation with Sophos Central"

        if ! support_message_relays ; then
            MESSAGE_RELAYS=""
        fi

        echo "Cloud token is [$CLOUD_TOKEN], Cloud URL is [$CLOUD_URL]"
        ${INSTDIR}/engine/registerMCS $CLOUD_TOKEN $CLOUD_URL $MESSAGE_RELAYS

        if [ $? -ne 0 ]; then
            echo "ERROR: Failed to register with Sophos Central - error $?" >&2
            cleanupAndExit 6
        fi

        cleanupAndExit 0
    elif ! echo "$args" | grep -q ".*--ignore-existing-installation.*"
    then
        echo "Please uninstall SAV before using this installer" >&2
        cleanupAndExit 7
    fi
fi

check_free $INSTALL_LOCATION 1024

tar -zxf installer.tar.gz || failure 11 "ERROR: Failed to unpack thin installer: $?"
rm -f installer.tar.gz || failure 12 "ERROR: Failed to delete packed thin installer: $?"

export LD_LIBRARY_PATH=installer/bin64:installer/bin32

mkdir distribute
mkdir cache
mkdir warehouse
mkdir warehouse/catalogue

MACHINE_TYPE=`uname -m`
if [ $MACHINE_TYPE = "x86_64" ]; then
    BIN=installer/bin64
else
    BIN=installer/bin32
fi

echo "Downloading medium installer"
$BIN/installer credentials.txt
handleInstallerErrorcodes $?

## Verify manifest.dat
CERT=installer/rootca.crt
[ -n $OVERRIDE_SOPHOS_CERTS ] && CERT=$OVERRIDE_SOPHOS_CERTS/rootca.crt
[ -f $CERT ] || CERT=installer/rootca.crt

$BIN/versig -c$CERT -fdistribute/manifest.dat -ddistribute --check-install-sh \
    || failure 8 "ERROR: Failed to verify medium installer: $?"

[ -z "$OVERRIDE_PROD_SOPHOS_CERTS" ] || cp $OVERRIDE_PROD_SOPHOS_CERTS/* distribute/update/certificates/

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

chmod u+x install.sh || failure 13 "Failed to chmod medium installer: $?"

if [ -f "installOptions" ]
then
    # Remove excess whitespace from the arguments with tr
    echo -n " $credentials $args $update_caches $sslca --PrimaryUpdateUseHttps=True --UpdateHttpsAllowDowngradeToHttp=True" | tr -s " " >> installOptions
else
    failure 9 "INTERNAL ERROR: No 'installOptions' file in medium installer."
fi

if [ $MACHINE_TYPE = "x86_64" ]
then
    cd lib64
else
    cd lib32
fi
createSymlinks
cd ..

echo "Running medium installer (this may take some time)"
./install.sh --minipkg $installer_args
inst_ret=$?
if [ $inst_ret -ne 0 ] && [ $inst_ret -ne 4 ]
then
    failure 10 "ERROR: Installer returned $inst_ret (see above messages)"
fi

if ! support_message_relays ; then
    MESSAGE_RELAYS=""
fi

$INSTALL_LOCATION/engine/registerMCS $CLOUD_TOKEN $CLOUD_URL $MESSAGE_RELAYS
ret=$?
if [ $ret -ne 0 ]
then
    failure 11 "ERROR: Failed to register with Sophos Central - error $ret"
fi

cleanupAndExit $inst_ret
__MIDDLE_BIT__
