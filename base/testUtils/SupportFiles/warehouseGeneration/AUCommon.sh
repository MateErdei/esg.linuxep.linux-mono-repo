#!/bin/bash

set -e

## chmod that refuses to change /tmp
function safechmod()
{
    announceFunction $FUNCNAME "$@"

    local arg
    for arg in "$@"
    do
        if [[ $arg == "/tmp" ]]
        then
            failure2 "Attempting to chmod /tmp: $@"
        fi
        if [[ $arg == "/" ]]
        then
            failure2 "Attempting to chmod /: $@"
        fi
        if [[ -z "$arg" ]]
        then
            failure2 "Attempting to chmod empty string: $@"
        fi
    done
    chmod "$@"
}

## Get the name of the CIDDIR for a given Tarfile name.
function getCIDNameFromTarFile()
{
    local tarfile=$1
    echo "$TMPDIR/AU/$(basename $tarfile)"/sophos-av
}

## Create a shared CID for a given tarfile - assumes the tarfile timestamp changes if the tarfile changes.
## CID is created in directory with the name provided by getCIDNameFromTarFile function.
## It is also returned in CreateSharedCID_RESULT variable.
## It does NOT remove created CID automatically (does not register cleanup function)
## @param tarfile   Tar archive with installer.
## @return   CreateSharedCID_RESULT variable contains name od the directory with CID
function CreateSharedCID()
{
    announceFunction $FUNCNAME "$@"

    local CIDDIR=$1
    safechmod -R a+r ${CIDDIR}
    createCID ${CIDDIR}
}

## Create a CID with local keys from a released CID
## Regenerate if required
function CreateSharedCID_From_Released_CID()
{
    announceFunction $FUNCNAME "$@"

    local baseCID="$1"

    if [[ -z "$baseCID" ]]
    then
        getLatestReleasedCID
        baseCID=$XX_LatestReleasedCID
    fi

    local CIDDIR="$TMPDIR/AU/$(basename $baseCID)"
    XX_SharedCID=$CIDDIR

    if _isDirectoryUpToDate "$baseCID" "$CIDDIR" "$baseCID"
    then
        echo "Skipping $CIDDIR for $baseCID as it doesn't need updating"
        return
    fi

    # Take a local copy of the released CID and correct the certificates
    __createCIDcopy_no_delete "$CIDDIR" "$baseCID"
    __createCID "$CIDDIR"
    _testing_with_old_CID "$CIDDIR"
    # reset permissions as the CID downloader may have flattened them
    safechmod -R a+rx $CIDDIR
    stampDirectory "$CIDDIR" "$baseCID"
}

# Output the SSL passphrase required to unlock the regression suite private key.
function _keyPassword()
{
    sed -ne 's/^output_password.*= //p' ./openssl.cnf
}

# Output an entry for a file in a format needed for the manifest.dat files.
# @param file    File to include in the manifest.dat file.
function _CIDsumFile()
{
    local file=$1
    local size=$(fileSize $file)
    local sum=$(_getSha1sum $file)
    local butcher=${file//\//\\} # replace "/"s with "\"s
    echo "\".\\$butcher\" $size $sum"
}


# Output the payload of a manifest.dat file.
# @param "$@"    files and directories to be included in the manifest.dat file.
function _CIDcreateSummary()
{
    local item
    for item in "$@"
    do
        if [ -d $item ]
        then
            find $item -type f -print | while read entry
            do
                _CIDsumFile $entry
            done
        elif [ -f $item ]
        then
            _CIDsumFile $item
        fi
    done
}

# Creates a manifest.dat file.
# @param xxkey    private key used to create the manifest.dat file
# @param xxcert    public certificate which is added to the manifest.dat file
# @param xxpassphrase    passphrase which is required to access the private key
# @param "$@"    remaining arguments are files and directories to include in CID
function _CIDcreateManifest()
{
    local xxkey=$1
    shift
    local xxcert=$1
    shift
    local xxpassphrase=$1
    shift

    TMPFILE=$(sophos_mktemp "$TMPDIR/CID")
    _CIDcreateSummary "$@" > $TMPFILE
    cat $TMPFILE
    echo -----BEGIN SIGNATURE-----
    echo $xxpassphrase | runOpenssl sha1 -passin stdin -sign "$xxkey" $TMPFILE | runOpenssl base64
    rm $TMPFILE
    echo -----END SIGNATURE-----
    cat $xxcert
}

# Correct an existing CID.  To be honest, this function simply re-generates the CID.
function correctCID()
{
# alias for createCID
    createCID $*
}

# Correct the certificates in a CID.
# Regenerate the manifest.spec
# @param CIDLOCATION the cid location
function _createCID_correctCertificatesInCID()
{

    announceFunction $FUNCNAME "$@"

    local CIDLOCATION=$1
    local targetCertDir="$CIDLOCATION/sdds/certs"
    local sourceCertDir="./certs"

    mkdir -p "$targetCertDir"
    local SSLCERTIFICATE="$sourceCertDir/rootca.crt"
    local SSLCRL="$sourceCertDir/combined.crl"

    cat $SSLCERTIFICATE >"$targetCertDir/rootca.crt"
    cat $SSLCRL >"$targetCertDir/root.crl"


}

## Regenerate the manifest.dat (also root_manifest.dat and master.dat) manifest files
function createCID_regenerateManifestDats_fast()
{

    announceFunction $FUNCNAME "$@"

    local cidLocation=$1
    shift
    componentroot='sdds'
    SSLPRIVATEKEY=./certs/testkey.pem
    COMBINED_CERTIFICATE=./certs/combined.crt

    ## Private key for bottom level certificate
    local key=${SSLPRIVATEKEY}
    ## Combined bottom and intermediate level certificates
    local cert=${COMBINED_CERTIFICATE}

    local passphrase=$(_keyPassword)
    (
        export OPENSSL_LD_LIBRARY_PATH=./lib:$LD_LIBRARY_PATH
        export OPENSSL_PATH=./bin/openssl
        export OPENSSL_CONF=./openssl.cnf
        runPython ./generateManifests.py "$cidLocation" "$key" "$passphrase" "$cert" "$componentroot" "$@"
    ) || failure2 "Unable to generate certificates"
}

## Regenerate the manifest.dat (also root_manifest.dat and master.dat) manifest files
function createCID_regenerateManifestDats()
{
    announceFunction $FUNCNAME "$@"

    createCID_regenerateManifestDats_fast "$@"
}

## Correct the certificates in both the CID and installation.
## Regenerate manifest.spec file for certificates in CID
function _createCID_correctCertificates()
{

    announceFunction $FUNCNAME "$@"

    local CIDLOCATION=$1

    _createCID_correctCertificatesInCID "${CIDLOCATION}"
}

## Correct the certificates and correct the manifest.spec and manifest.dat files
## To create a CID from an unpacked tarfile.
## NB: This must be done in the correct case
function _createCID_correctCertificatesAndCorrectManifests()
{
    announceFunction $FUNCNAME "$@"

    local CIDLOCATION=$1

    _createCID_correctCertificates "${CIDLOCATION}"
    createCID_regenerateManifestDats "${CIDLOCATION}"
}

## Create the *.upd files for a CID.
## NB: For an emlibrary CID this must be done after lower-casing the CID.
function createCID_createUPDfiles_slow()
{
    local CIDLOCATION=$1

    (
        cd $CIDLOCATION

        export LD_LIBRARY_PATH=$DIST/sav-$PLATFORM/$ARCHITECTURE/lib

        ## I've discovered that all CIDs will and have always had both the top-level cidsync.upd
        ## And the root.upd master.upd

        ## Need to delete old indexes
        rm -f cidsync.upd master.upd root.upd

        local item
        for item in sav-$PLATFORM doc talpa savi uncdownload
        do
            if [[ -d $item ]]
            then
                rm -f $item/cidsync.upd
            fi
        done
        for item in config talpa-custom
        do
            if [[ -d $item ]]
            then
                rm -f $item/index.spec
            fi
        done

        echo "createCID_createUPDfiles: cidsync.upd"
        $TEST_ROOT/bin/updCreate -o cidsync.upd .

        for item in sav-$PLATFORM doc talpa savi uncdownload
        do
            if [[ -d $item ]]
            then
                echo "createCID_createUPDfiles: $item/cidsync.upd"
                $TEST_ROOT/bin/updCreate -r $item -o $item/cidsync.upd .
            fi
        done

        ## root upd file
        # Note: removed unsafe use of -print0 and -0 arguments in find and xargs because they are not supported
        # on Solaris.
        echo "createCID_createUPDfiles: root.upd"
        _findMaxdepth1 . \( -name \*.sh -o -name \*.dat -o -name \*.txt -o -name version -o -name fullVersion \) -type f \
                | sed -e "s/\(.\)/\\\\\1/g" | xargs -- $TEST_ROOT/bin/updCreate -o root.upd

        echo "createCID_createUPDfiles: master.upd"
        $TEST_ROOT/bin/updCreate -m -o master.upd root.upd */cidsync.upd

        for item in config talpa-custom
        do
            if [[ -d $item ]]
            then
                ## DLCL: Force index.spec to be removed from sum list as seemed to include it sometimes
                cd $item

                rm -f index.spec 2>/dev/null

                echo "createCID_createUPDfiles: $item/index.spec"
                md5sum_rep * | grep -v index.spec > index.spec

                cd $CIDLOCATION

                if grep -I -s index.spec $item/index.spec >/dev/null
                then
                    cat $item/index.spec
                    failure2 "index.spec included in index.spec for ${item}!"
                fi
            fi
        done

    )
    local RET=$?

    (( $RET == 0 )) || exit $RET
}

function createCID_createUPDfiles_fast()
{

    announceFunction $FUNCNAME "$@"

    local CIDLOCATION=$1


    local CID_VERSION="0.5.0"

    local TOPONLY=

    TOPONLY=--top

    DIST=$DIST \
        SAV_DASH_DIR=$SAV_DASH_DIR \
        runPython $SUP/generateUpd.py $TOPONLY "$@" || failure2 "Unable to generate upd files"

    pushd $CIDLOCATION

    local item
    for item in config talpa-custom
    do
        if [[ -d $item ]]
        then
            ## DLCL: Force index.spec to be removed from sum list as seemed to include it sometimes
            pushd $item

            rm -f index.spec 2>/dev/null

            echo "createCID_createUPDfiles: $item/index.spec"
            md5sum_rep * | grep -v index.spec > index.spec

            popd

            if grep -I -s index.spec $item/index.spec >/dev/null
            then
                cat $item/index.spec
                failure2 "index.spec included in index.spec for ${item}!"
            fi
        fi
    done
    popd
}

function createCID_createUPDfiles()
{
    announceFunction $FUNCNAME "$@"

    createCID_createUPDfiles_fast "$@"
    local RET=$?

    return $RET
}

function __createCID()
{

    announceFunction $FUNCNAME "$@"

    local CIDLOCATION=${1%/}
    local EMLIBRARY=$2
    local CUSTOMER_ID_FILE=$3

    _createCID_correctCertificatesAndCorrectManifests "$CIDLOCATION"

    echo "------ EMLIBRARY = $EMLIBRARY"

    ## Create the customer id file
    if [[ -z "$EMLIBRARY" ]]
    then
        echo "createCID: createCID_createUPDfiles sum"

        [ -f "$CUSTOMER_ID_FILE" ] || CUSTOMER_ID_FILE="./RegressionSuiteCustomerID.txt"
        rm -f "$CIDLOCATION/customer_ID.txt"
        cp "$CUSTOMER_ID_FILE" "$CIDLOCATION/customer_ID.txt" \
            || setupFailure "Unable to copy $CUSTOMER_ID_FILE"
        safechmod 644 "$CIDLOCATION/customer_ID.txt" \
            || setupFailure "Unable to chmod $CUSTOMER_ID_FILE"

        #createCID_createUPDfiles "$CIDLOCATION" --sum
    else
        echo "createCID: createCID_createUPDfiles emlibrary"
        ## No customer_ID.txt in emlibrary CIDs
        rm "$CIDLOCATION/customer_ID.txt"
        [[ -f $CIDLOCATION/savVersion ]] && {
            rm -f "$CIDLOCATION/suiteVersion"
            rm -f "$CIDLOCATION/version"
        }
        #createCID_createUPDfiles "$CIDLOCATION" --emlibrary
    fi

    safechmod -R a+rx "$CIDLOCATION"
}

# Turns an exploded installer into a full-on CID
# @param CIDLOCATION    location of installer files which are to be turned into a CID
# @param EMLLIBRARY     OPTIONAL non-empty to make an emlibrary CID
# @param customer_ID_file OPTIONAL file to copy the customer_ID.txt from (default is "$SUP/RegressionSuiteCustomerID.txt")
# By default CIDs are now SUM cids
function createCID()
{
    announceFunction $FUNCNAME "$@"

    __createCID "$@"
}

function __createCIDcopy_no_delete()
{
    local dir="$1"
    local source="${2}"
    if [[ ! $source ]]
    then
        echo "__createCIDcopy_no_delete -> CALLING require_distcid HERE"
        require_distcid
        source=$DISTCID
    fi

    if [[ -z "$dir" ]] ; then
        [[ -z "$CIDDIR" ]] && CIDDIR="${TESTTMP}/CID"
        dir="$CIDDIR"
    fi

    safermrf "$dir" || setupFailure "Unable to delete $dir"
    mkdir -p "$dir" || setupFailure "Unable to create $dir"
    echo "Copying CID from \"$source\" to \"$dir\"..." >&2
    local BEFORE=$SECONDS
    if (( $IS_LINUX == 1 )) && [[ $source == $DISTCID ]] && [[ $HARD_LINK_CID_COPY == 1 ]]
    then
        cp -l -a $source/* $dir || setupFailure "Unable to create linked cid copy from $source to $dir"
        echo "Hard link done. Took $(( $SECONDS - $BEFORE ))s." >&2
    else
        cp -r -p $source/* $dir || setupFailure "Unable to create cid copy from $source to $dir"
        echo "Copy done. Took $(( $SECONDS - $BEFORE ))s." >&2
    fi

    XX_CID_COPY="$dir"
}


function __createCIDcopy()
{
    __createCIDcopy_no_delete "$@"
    registerCleanupFunction _removeCIDcopy
}


## Create a copy of the prepared CID
# @param dir    optional location to use for the CID
function createCIDcopy()
{
    announceFunction $FUNCNAME "$@"

    __createCIDcopy "$@"

    ## Copy certificates to installation so that updates work - should no longer be required...
    local certdir="$INST/update/certificates"
    if [[ -d "$certdir" ]] ; then
        local certsrc="$XX_CID_COPY/sav-$PLATFORM/common/update/certificates"
        cp -f "$certsrc/rootca.crt" "$certdir/rootca.crt"
        cp -f "$certsrc/root.crl"   "$certdir/root.crl"
    fi

    rm -f $XX_CID_COPY/[mM][rR][iI]nit.conf "$XX_CID_COPY/cac.pem"
    return 0
}

function copyAndRegenerateCID()
{
    announceFunction $FUNCNAME "$@"

    __createCIDcopy "$@"
    __createCID "$XX_CID_COPY"
}

# Clean-up function which removes a copy of the CID.
function _removeCIDcopy()
{
    if [[ -d "$XX_CID_COPY" ]]
    then
        echo Removing $XX_CID_COPY
        safermrf $XX_CID_COPY
    fi

    unset XX_CID_COPY
}

## Create a dummy CID with only manifest.dat and cidsync.upd
## Adds a rogue install.sh which gets included in the upd file
## @param dir location for the CID
function createDummyCID()
{
    announceFunction $FUNCNAME "$@"

    CIDDIR=$1
    mkdir -p "$CIDDIR" || setupFailure "Unable to create $CIDDIR"
    mkdir -p "$CIDDIR/sav-$PLATFORM" || setupfailure "Unable to create $CIDDIR/sav-$PLATFORM"
    safechmod -R a+r $CIDDIR
    safechmod -R a+r "$CIDDIR/sav-$PLATFORM"
    echo " " > "$CIDDIR/sav-$PLATFORM/manifest.spec"

    runPython ./generateManifestSpec.py "$CIDDIR" || failure2 "Unable to generate certificates"
    createCID_regenerateManifestDats "$CIDDIR"

    ## Now Add (a rogue) install.sh which creates a directory in root
    echo "#! /bin/bash" > "$CIDUPDATEDIR/install.sh"
    echo "mkdir $RANDOMDIR" >> "$CIDUPDATEDIR/install.sh"
    safechmod  a+rx "$CIDUPDATEDIR/install.sh"

    ## The rogue installer will be present in the upd file
    createCID_createUPDfiles "$CIDDIR" --sum
}

## Create a CID, as would appear from a emlibrary with a SEC.
## @param dir    optional location to use for the CID
function createManagedCID()
{
    announceFunction $FUNCNAME "$@"

    require_magent
    createCIDcopy "$@"
    makeCIDmanaged "$XX_CID_COPY"
}

## Do whatever is necessary to convert unmanaged CID into managed one.
## At the moment it just copies mrinit.conf and cac.pem
## @param ciddir    Directory where existing unmanaged CID is located
## @public
function makeCIDmanaged()
{
    announceFunction $FUNCNAME "$@"

    local ciddir=$1
    [ -z $ciddir ] && setupFailure "makeCIDManaged: Location of CID not specified"

    require_mrinit_conf

    if [ -f "$MR_INIT_CONF" ] && [ -f "$(dirname $MR_INIT_CONF)/cac.pem" ]
    then
        ## As the installer currently searches only for the correct case version of this
        ## file we'll copy it in the correct case.
        cp -f "$MR_INIT_CONF" "$ciddir/MRInit.conf" || setupFailure "Unable to copy MRInit.conf"
        cp -f "$(dirname $MR_INIT_CONF)/cac.pem" "$ciddir/cac.pem" || setupFailure "Unable to copy cac.pem"
        # Make sure SEC credentials are readable
        safechmod 644 "$ciddir/MRInit.conf" "$ciddir/cac.pem" || setupFailure "Unable to chmod"
    else
        failure2 "Unable to find cac.pem and MRInit.conf to create managed CID: $MR_INIT_CONF"
    fi
}

function removeDummyRMS()
{
    announceFunction $FUNCNAME "$@"

    stop_rms
    rm -f $INST/rms/MRInit.conf $INST/rms/sophos.config $INST/rms/agent.config $INST/rms/router.config
}

## Create a dummy managed CID which contains cac.pem and MRInit.conf files
## So SAV installation will enable rms.
function createDummyManagedCID()
{
    announceFunction $FUNCNAME "$@"

    local ciddir=$1

    createCIDcopy "$ciddir"
    create_cac_pem "$ciddir/cac.pem"
    createMRInit_conf "$ciddir/MRInit.conf"

    # don't leave RMS config in the cache or install
    registerCleanupFunction removeLocalCache
    registerCleanupFunction removeDummyRMS

    # don't check for RMS errors, we expect them with a bogus server
    unregisterCleanupFunction checkForErrorsInSavRMSLog
}

function makeCIDdummyManaged()
{
    announceFunction $FUNCNAME "$@"
    local ciddir=$1

    # if the CID is cached, mark the modified CID as invalid so it is not used
    # by other tests, unless it has been restored.
    if [[ -f $ciddir/stamp ]]
    then
        mv $ciddir/stamp $ciddir/stamp.modified

        XX_DUMMY_MANANGED_CID=$ciddir
        registerCleanupFunction _restoreDummyManagedCID
    fi

    create_cac_pem "$ciddir/cac.pem"
    createMRInit_conf "$ciddir/MRInit.conf"

    # don't leave RMS config in the cache or install
    registerCleanupFunction removeLocalCache
    registerCleanupFunction removeDummyRMS

    # don't check for RMS errors, we expect them with a bogus server
    if isCleanupFunctionRegistered checkForErrorsInSavRMSLog
    then
        unregisterCleanupFunction checkForErrorsInSavRMSLog
    fi
}

function _restoreDummyManagedCID()
{
    announceFunction $FUNCNAME "$@" "($XX_DUMMY_MANANGED_CID)"

    local ciddir=$XX_DUMMY_MANANGED_CID
    [[ $ciddir ]] || return

    rm $ciddir/MRInit.conf
    rm $ciddir/cac.pem

    [[ -f $ciddir/stamp.modified ]] && mv $ciddir/stamp.modified $ciddir/stamp
}

function create_cac_pem()
{
    cat > "$1" << EOF
-----BEGIN CERTIFICATE-----
MIIDFzCCAf+gAwIBAgIBATANBgkqhkiG9w0BAQQFADARMQ8wDQYDVQQDFAZFTTJf
Q0EwHhcNMTAwODMxMDkwNTAxWhcNMzAwODI3MDkwNTAxWjARMQ8wDQYDVQQDFAZF
TTJfQ0EwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQC5fPdnkG3f2TR2
1bwu8hPcpGoHh5ls42atpPVDuAZpcg9AsZvPtjAn3fNTAwFVXwDOWIktGmf0uq4E
9BD88nITfhHjq+mAOFYUHwcNxNUjnRyzv74tLuU6x035kWAs7PF2C/DmGF0Fg+Nn
wQQ8FqSYppPB9n3Hq5OEebhHJddTE7/w0C+q+B9ln6DigNv2zrre1L5btwoR+cgO
prBmgLJKH+IQK/jYjCIuRKhTZtWUO8P3SCfv3HoBbOF2lh9/2eeJXDfXu1PHKdff
mm0hPaFzhIYQekz6MjNJxikgAnY4t9mStzXtYhKQgkcMnnjzw6chhu6uX6Q1dx2L
GuTSIeKbAgMBAAGjejB4MB0GA1UdDgQWBBQVaMo0IPaA9jG7YrHDZJqovi29oTA5
BgNVHSMEMjAwgBQVaMo0IPaA9jG7YrHDZJqovi29oaEVpBMwETEPMA0GA1UEAxQG
RU0yX0NBggEBMAwGA1UdEwQFMAMBAf8wDgYDVR0PAQH/BAQDAgEGMA0GCSqGSIb3
DQEBBAUAA4IBAQCioGXCH/+SZ9Bu2EGvvpEqWUpwvE0jbHDShRicFVxNI0kC6tSq
fcDo4fL8LmXZqx4Bj+6b1wTmlHnNsdCky3Hwalm1OZO/9hx/5PQ/i3DRJwAQZ5Qe
GG1ph1/TGKu8D5BEs8mxzBBO7b2L42Fsc5AA8s52OoyyyIDIccZs7By3IspSkC7S
Uz/pRILLNBKTzk3wEMonbubO+ShemoapqgRKldwSoiX0J2Jf8zXKERiXBD29973L
nhMTsnaol6+C4qMnMDrdJCzFD/OJAMIVGqfjnxMtY2I3iVovEl7btaVT1ZC9XspM
Yy4KnLYRdlfl/KVIHHvEIVs0EPqboXv1vhkj
-----END CERTIFICATE-----
EOF
}

function createMRInit_conf()
{
    cat > "$1" << EOF
[Config]
"NotifyRouterUpdate"="EM"
"ClientIIOPPort"=dword:00002001
"ClientSSLPort"=dword:00002002
"ClientIORPort"=dword:00002000
"IORSenderPort"=dword:00002000
"DelegatedManagerCertIdentityKey"="1OIvH+I75ACSsaA3I7ORUjncT7g="
"ManagedAppCertIdentityKey"="GhAS2xKna3na0rfWPWd+O1C9Lts="
"RouterCertIdentityKey"="WfZJt+91rn7ZnwmxFwH2AywmDec="
"ServiceArgs"=""
"MRParentAddress"="bogus.eng.sophos,bogus"
"ParentRouterAddress"="bogus.eng.sophos,bogus"
EOF
}

# Converts every file in a path to have lower-case filenames.
# @param path    path to convert to lower case.
function makeFilesLowercase()
{
    announceFunction $FUNCNAME "$@"

    local path="$1"

    shopt -s nullglob # return null if no files are found
    local entries
    entries=("$path"/*)
    shopt -u nullglob

    local entry
    for entry in "${entries[@]}"
    do
        [[ -d $entry ]] && makeFilesLowercase "$entry"
        local name="${entry##*/}"
        # if the entry name contains no upper-case letters, continue
        [[ $name == *[A-Z]* ]] || continue
        local lcname="$(tolower "$name")"
        [[ "$lcname" == "$name" ]] || mv "$path/$name" "$path/$lcname"
    done
}

# Clean-up function which will remove a fake EMLibrary CID
function _removeEMLibraryCID()
{
    announceFunction $FUNCNAME "$@"
    if [[ -d $XX_EMLIBRARY_CID ]]
    then
        echo Removing $XX_EMLIBRARY_CID
        safermrf $XX_EMLIBRARY_CID
    fi
}

# Generates an EM Library CID from a normal CID
# @param DIR    conventional CID
# @param EMLIBRARYDIR    location to be used as a fake EMLibrary CID
function convertToEmlibrary()
{
    announceFunction $FUNCNAME "$@"

    local DIR=$1
    local EMLIBRARYDIR=$2
    if [[ -z $DIR || -z $EMLIBRARYDIR ]]
    then
        failure2 "convertToEmlibrary requires more parameters"
    fi

    safermrf "$EMLIBRARYDIR"
    cp_a "$DIR" "$EMLIBRARYDIR"
    _createCID_correctCertificatesAndCorrectManifests "$EMLIBRARYDIR"

    _testing_with_old_CID $EMLIBRARYDIR

    makeFilesLowercase "$EMLIBRARYDIR"
    export UPD_SHOULD_BE_LOWERCASE=1
    createCID_createUPDfiles "$EMLIBRARYDIR" --emlibrary
    XX_EMLIBRARY_CID=$EMLIBRARYDIR
    registerCleanupFunction _removeEMLibraryCID
}

# Update the manifest.spec file to cater for a changed file
# @param cid    location of CID
# @param file    location of file relative to the cid.
function _updateFileInManifest()
{
    local cid=$1
    local file=$2
    local inst=$3

    local name=${file//\//\\\/} # replace "/"s with "\/"s
    local sum=$(_getMd5sum $cid/$file)

    ## Avoid altering hard-linked copy
    if (( $IS_LINUX == 1 ))
    then
        mv $cid/sav-$PLATFORM/manifest.spec $cid/sav-$PLATFORM/manifest.spec.temp
        cp -p $cid/sav-$PLATFORM/manifest.spec.temp $cid/sav-$PLATFORM/manifest.spec
        rm -f $cid/sav-$PLATFORM/manifest.spec.temp
    fi

    ised -e "s/^.*\\( $name\\)$/$sum\\1/" $cid/sav-$PLATFORM/manifest.spec
    if [[ -n "$inst" ]]
    then
        ised -e "s/^.*\\( $name\\)$/$sum\\1/" $inst/update/manifest.spec
    fi
}

function _check_disk_space()
{
    print_header "disk space"
    # try local only first, so that we don't hang if there are network issues
    df -P -l -k 2>/dev/null \
        || df -l -k 2>/dev/null \
        || df -k
    print_divider

    ## Check /opt/sophos-av and do a du if space is tight
    runPython $SUP/checkDiskSpace.py $INST
    if [[ $? != 0 ]]
    then
        print_header "du -xak /"
        du -xak /
        print_divider
    fi
}

## Update from a location, ensuring that the location is set as the update location
## before and after
function updateFrom()
{
    announceFunction $FUNCNAME "$@"
    local UPDATE_SOURCE="$1"
    shift

    runSavconfig set PrimaryUpdateSourcePath "$UPDATE_SOURCE"

    ## Check if we've set it
    if ! __checkConfig PrimaryUpdateSourcePath "$UPDATE_SOURCE"
    then
        runSavconfig deleteLayer --consoleupdate || setupFailure "Failed to delete console update layer"
        runSavconfig set PrimaryUpdateSourcePath "$UPDATE_SOURCE" || setupFailure "Failed to set update source"
    fi

    while true
    do
        declare -x SAVUPDATE_IS_INTERACTIVE=1
        $INST/bin/savupdate "$@"
        local RET=$?

        if __checkConfig PrimaryUpdateSourcePath "$UPDATE_SOURCE"
        then
            break
        fi

        runSavconfig deleteLayer --consoleupdate || setupFailure "Failed to delete console update layer"
        runSavconfig set PrimaryUpdateSourcePath "$UPDATE_SOURCE" || setupFailure "Failed to set update source"
    done

    return $RET
}

# Set how long ago savupdate last successfully ran.
# @param age    number of seconds since savupdate apparently last successfully completed.
function setLastUpdateAttemptTimestamp()
{
    announceFunction $FUNCNAME "$@"
    local age=$1

    local file=$INST/etc/update.last_update_attempt_timestamp
    echo $(($(unixTime) - $age)) > $file
    safechown sophosav:${GROUPS[0]} $file
    safechmod 0600 $file
}

# Set how long ago savupdate last updated from Sophos
# @param age    number of seconds since savupdate apparently tried to update from Sophos
function setLastSophosUpdate()
{
    announceFunction $FUNCNAME "$@"
    local age=$1
    local file=$INST/etc/update.last_sophos_sync
    echo $(($(unixTime) - $age)) > $file
    safechown sophosav:${GROUPS[0]} $file
    safechmod 0600 $file

}

## For testing AllDistros code, we need to add extra files
function addDummyCoreFiles()
{
    # add some dummy core files to the distribution
    #    These may be the only such files in the distribution
    #    These may be additional to the core files in the distribution
    #    Either way, we use these dummy core files to determine that savupdate is correctly updating
    #     core files when configured as a deployment server
    announceFunction $FUNCNAME "$@"

    local cid=$1
    [ -z "$cid" ] && cid=$CIDDIR
    if [ -z "$cid" ]
    then
        failure2 "No CID specified to addDummyCoreFiles"
    fi

    [ -z $DUMMYCOREFILE ] && DUMMYCOREFILE=${TESTTMP}/dummy.txt

    cat <<EOL > $DUMMYCOREFILE
Dummy file
EOL
    mkdir -p "$cid/$COREDIR_RH90"
    cp $DUMMYCOREFILE $cid/$COREDIR_RH90/dummy-core-rh90.txt
    cp $DUMMYCOREFILE $cid/$COREDIR_RH90/dummy-UPPER-CASE.txt
    mkdir -p "$cid/$COREDIR_STD"
    cp $DUMMYCOREFILE $cid/$COREDIR_STD/dummy-core-std.txt
    cp $DUMMYCOREFILE $cid/$COREDIR_STD/dummy-UPPER-CASE.txt
}

## Check that a file in a CID and a LCD match
function CheckFile()
{
    announceFunction $FUNCNAME "$@"

    local cid=$1
    local cache=$2
    local FNAME=$3

    [ -z "$cid" ] && cid=$CIDDIR
    if [ -z "$cid" ]
    then
        failure2 "No CID specified to CheckFile"
    fi
    [ -z "$cache" ] && cache=$LOCALCACHE
    if [ -z "$cache" ]
    then
        failure2 "No LOCALCACHE specified to CheckFile"
    fi

    if [ ! -e "$cid/$FNAME" ] && [ ! -e "$cache/$FNAME" ]
    then
        ## if both files are absent then that is OK
        return
    fi

    if ! diff "$cid/$FNAME" "$cache/$FNAME"
    then
        testFailure "Failed to update file $FNAME (diff $cid/$FNAME vs $cache/$FNAME) "
    fi
}

## Check that the CID and LCD are matching after a addDummyCoreFiles/addDummyTBPs/modifyCID
function CheckUpdatedFiles()
{
# confirm that the expected  core & TBP files have been updated
#    confirms presence and contents of each of the dummy files that we *know* are in the CID
#    we also check the cidsync.upd and manifest.dat files, because we can
#    inelegant, but it does the job
    announceFunction $FUNCNAME "$@"

    local cid=$1
    local cache=$2

    [ -z "$cid" ] && cid=$CIDDIR
    if [ -z "$cid" ]
    then
        failure2 "No CID specified to CheckUpdatedFiles"
    fi
    [ -z "$cache" ] && cache=$LOCALCACHE
    if [ -z "$cache" ]
    then
        failure2 "No LOCALCACHE specified to CheckUpdatedFiles"
    fi

    ## Now everything is using a top-level cidsync.upd so we need to check that
    CheckFile $cid $cache "cidsync.upd"
    CheckFile $cid $cache "manifest.upd"

    ## Old indexes - which we don't use any more
    #     CheckIndexes $cid $cache "sav-$PLATFORM"
    #     CheckIndexes $cid $cache "savi"
    #     CheckIndexes $cid $cache "talpa"

    CheckFile $cid $cache "$COREDIR_RH90/dummy-core-rh90.txt"
    CheckFile $cid $cache "$COREDIR_STD/dummy-core-std.txt"
    CheckFile $cid $cache "$TALPADIR_REDHAT/talpa-binpack-redhat-dummy1.tar.gz"
    CheckFile $cid $cache "$TALPADIR_REDHAT/talpa-binpack-redhat-dummy2.tar.gz"
    CheckFile $cid $cache "$TALPADIR_REDHAT/talpa-binpack-redhat-dummy3.tar.gz"
    CheckFile $cid $cache "$TALPADIR_SUSE/talpa-binpack-suse-dummy1.tar.gz"
    CheckFile $cid $cache "$TALPADIR_SUSE/talpa-binpack-suse-dummy2.tar.gz"
    CheckFile $cid $cache "$TALPADIR_TURBO/talpa-binpack-turbo-dummy1.tar.gz"
    echo "Expected cache files present and correct"
}

## Change md5 checksum of file $FILE in manifest file $MANIFEST
## @param MANIFEST manifest file
## @param FILE checksum of thi file will be changed
## @public
function changeManifest()
{
    announceFunction $FUNCNAME "$@"

    local MANIFEST="$1"
    local -a FILES=( "${@:2}" )

    runPython "./changeManifest.py" "$MANIFEST" "${FILES[@]}"
    local ret=$?

    if (( $ret != 0 ))
    then
        # see when the manifest last changed, so we can work out if another test has broken it
        lsfulltime $MANIFEST
        setupFailure "Failed to change Manifest: $ret"
    fi
}

## Pretent savd has changed in default CID (CIDDIR). Next update will cause savd to be reloaded.
## @param CID_TO_MODIFY CID which will be affected. Defaults to CIDDIR if parameter not specified
## @public
function modifyIndexChecksumForSavd()
{
    announceFunction $FUNCNAME "$@"

    local CID_TO_MODIFY="$1"
    [ -z "$CID_TO_MODIFY" ] && CID_TO_MODIFY=$CIDDIR

#    changeManifest "$CID_TO_MODIFY/sav-$PLATFORM/manifest.spec" "${COREDIR}/engine/_/savd"
    createCID_regenerateManifestDats "$CID_TO_MODIFY"
    createCID_createUPDfiles "$CID_TO_MODIFY"
}


## Pretent savscand has changed in default CID (CIDDIR). Next update will cause savscand to be reloaded.
## @param CID_TO_MODIFY CID which will be affected. Defaults to CIDDIR if parameter not specified
## @public
function modifyIndexChecksumForSavscand()
{
    announceFunction $FUNCNAME "$@"

    local CID_TO_MODIFY="$1"
    [ -z "$CID_TO_MODIFY" ] && CID_TO_MODIFY=$CIDDIR

    changeManifest "$CID_TO_MODIFY/sav-$PLATFORM/manifest.spec" "${COREDIR}/engine/_/savscand"
    createCID_regenerateManifestDats "$CID_TO_MODIFY"
    createCID_createUPDfiles "$CID_TO_MODIFY"
}

# Restore hosts file backup
# To do: Make sure this works cross platform..
function _restoreHostsFile()
{
    local host_file=/etc/hosts

    [[ -f ${host_file}.backup ]] \
        || cleanupFailure "Cannot restore hosts file, ${host_file}.backup missing"

    ## cat the backup into the original file to preserve ownership, mode, symlinks, etc.
    cat "${host_file}.backup" >"${host_file}" \
        && rm "${host_file}.backup"

    if [ -f /etc/inet/ipnodes.backup ]
    then
        cp -p /etc/inet/ipnodes.backup /etc/inet/ipnodes \
            && rm /etc/inet/ipnodes.backup
    fi

    # Flush the cached hosts so we use values which we are expecting from a test.
    nscd -i hosts 2>/dev/null
}


# Initialise run once flag for _startFakeSophosWebCID, this prevents _startFakeSophosWebCID from
# unnessecarily being run more than once per test
__XX_fake_sophos_webcid_started=0


function _fixOwnershipPermsOnUpdateStatusFile()
{
    local file=$1
    safechown sophosav:${GROUPS[0]} $file
    safechmod 0600 $file
}

function _writeUpdateStatusFile()
{
    local file="$1"
    local contents="$2"
    echo "$contents" >"$file"
    _fixOwnershipPermsOnUpdateStatusFile "$file"
}

# Set how long ago savupdate last successfully ran.
# @param age    number of seconds since savupdate apparently last successfully completed.
#               default: 0
function setLastUpdate()
{
    announceFunction $FUNCNAME "$@"

    local age=${1:-0}

    local file=${INST}/etc/update.last_update
    _writeUpdateStatusFile "$file" "$(($(unixTime) - ${age}))"

}

# Manually set the update.outcome file to simulate a successfull update
# Will change the output of savdstatus --lastupdatestatus
function setLastUpdateStatusSuccess()
{
    announceFunction $FUNCNAME "$@"

    local file=${INST}/etc/update.outcome
    _writeUpdateStatusFile "$file" ""
}

# Manually set the update.outcome file to simulate a failed update
# Will change the output of savdstatus --lastupdatestatus
function setLastUpdateStatusFailure()
{
    announceFunction $FUNCNAME "$@"

    local file=${INST}/etc/update.outcome
    _writeUpdateStatusFile "$file" "/this/path/was/set/directly/in/update/outcome/by/the/regression/suite/to/simulate/failure"
}

# Manually remove the update.outcome file to simulate an unknown update status
# Will change the output of savdstatus --lastupdatestatus
function setLastUpdateStatusUnknown()
{
    announceFunction $FUNCNAME "$@"

    rm "${INST}/etc/update.outcome"
}

# Manually set the update.failed_count file to simulate a number of failed updates
# Will change the output of savdstatus --lastupdatestatus
function setUpdateFailureCount()
{
    announceFunction $FUNCNAME "$@"

    local COUNT=$1

    local file=${INST}/etc/update.outcome
    _writeUpdateStatusFile "$file" "/this/path/was/set/directly/in/update/outcome/by/the/regression/suite/to/simulate/failure"

    file=${INST}/etc/update.failed_count
    _writeUpdateStatusFile "$file" "$COUNT"
}

# Create dummy files so that CID creation results in a large UPD file.
function createDummyFilesInCID()
{
    announceFunction $FUNCNAME "$@"

    local CIDDIR=$1
    local NFILES=${2:-20000}
    local MINSIZE=${3:-$(( 1024 ** 2 ))}

    # Create the UPD file (do the printf outside the loop, since subshells add
    # 1ms to each iteration, even on a good Linux box).
    local BEGIN=$SECONDS
    local ZEROS=$(printf '%020d' 0)

    printf "Creating dummy files for UPD file..."
    mkdir -p $CIDDIR/savi/sav
    local RND BADFILE i
    for (( i=0 ; i<NFILES ; i++ )) ; do
        (( i % 1000 == 0 )) && printf "."
        RND=$ZEROS$RANDOM
        BADFILE=$CIDDIR/savi/sav/fakeupdfile-$i-${RND: -15}.ide
        echo >$BADFILE $RANDOM || setupFailure "Failed to create: $BADFILE"
    done
    printf ' done. Took %d secs\n' $(( SECONDS - BEGIN ))

    createCID_createUPDfiles $CIDDIR

    local CIDSYNC=$CIDDIR/cidsync.upd
    [[ -f $CIDSYNC ]] || setupFailure "Didn't find $CIDSYNC"
    ls -l $CIDSYNC

    local UPDSIZE=$(fileSize $CIDSYNC)
    printf "cidsync size = %d\n" $UPDSIZE
    (( UPDSIZE > MINSIZE )) ||
        setupFailure "$CIDSYNC not large enough ($UPDSIZE < $MINSIZE)"
}

## Create the certificates needed by addextra in SAV 9.3+
## Export SIGNING_KEY, SIGNING_CERTIFICATE, SIGNING_PASSWORD to pass these
## options to addextra.
function require_customer_certificates()
{
    announceFunction $FUNCNAME "$@"

    unset \
        XX_ROOT_KEY \
        XX_ROOT_CERTIFICATE \
        XX_ROOT_KEY_PASSWORD \
        XX_SIGNING_KEY \
        XX_SIGNING_CERTIFICATE \
        XX_SIGNING_PASSWORD \
        XX_KEYDIR \
        XX_CERTDIR

    # fall back to using index.spec if we can't generate certs.
    savVersionLessThan 9.3 && return

    local priv=$SUP/addextra/private
    local pub=$SUP/addextra/public

    [[ -d $priv && -d $pub ]] \
        || setupFailure "Missing customer certificate: $rootca"

    XX_CERTDIR=$pub
    XX_KEYDIR=$priv

    XX_ROOT_KEY=$priv/extrafiles-root-ca.key
    XX_ROOT_CERTIFICATE=$pub/extrafiles-root-ca.crt
    XX_ROOT_KEY_PASSWORD=$(< $priv/extrafiles-root.passwd)

    XX_SIGNING_KEY=$priv/extrafiles-signing.key
    XX_SIGNING_CERTIFICATE=$priv/extrafiles-signing.crt
    XX_SIGNING_PASSWORD=$(< $priv/extrafiles-signing.passwd)

    # These are exported for running manifest creation within automation
    export SIGNING_KEY=$XX_SIGNING_KEY
    export SIGNING_PASSWORD=$XX_SIGNING_PASSWORD
    export SIGNING_CERTIFICATE=$XX_SIGNING_CERTIFICATE
}

function install_customer_certificates()
{
    announceFunction $FUNCNAME "$@"

    [[ $XX_CERTDIR ]] || require_customer_certificates

    $INST/update/addextra_certs --install=$XX_CERTDIR \
        || setupFailure "Unable to install customer root cert"

    registerCleanupFunction uninstall_customer_certificates
}

function uninstall_customer_certificates()
{
    announceFunction $FUNCNAME "$@"

    # fall back to using index.spec if we can't manage certs.
    savVersionLessThan 9.3 && return

    $INST/update/addextra_certs --uninstall \
        || setupFailure "Unable to uninstall customer root cert"
}

## Create the certificates needed by addextra in SAV 9.3+
## Export SIGNING_KEY, SIGNING_CERTIFICATE, SIGNING_PASSWORD to pass these
## options to addextra.
function create_customer_certificates()
{
    announceFunction $FUNCNAME "$@"

    require_openssl_signing

    XX_ROOT_KEY_PASSWORD=$1
    XX_SIGNING_PASSWORD=$2
    local dest=$3

    [[ $XX_ROOT_KEY_PASSWORD != $XX_SIGNING_PASSWORD ]] \
        || setupFailure "Attempting to create customer certs with the same root and signing keys"

    safermrf "$dest"
    mkdir "$dest"
    chmod 0700 "$dest"

    if [[ $( ls /tmp/create_certificates.sh.* 2>/dev/null) ]]
    then
        warning "Removing old temp data from create_certificates.sh"
        safermrf /tmp/create_certificates.sh.*
    fi

    sh $SUP/create_certificates.sh "$dest" <<EOF
$XX_ROOT_KEY_PASSWORD
$XX_ROOT_KEY_PASSWORD
$XX_SIGNING_PASSWORD
$XX_SIGNING_PASSWORD
EOF
    (( $? == 0 )) || setupFailure "Unable to generate the keys for addextra"

    if [[ $( ls /tmp/create_certificates.sh.* 2>/dev/null) ]]
    then
        ls -l /tmp/create_certificates.sh.*
        testFailure "create_certificates.sh left data in /tmp"
    fi

    XX_SIGNING_KEY=$dest/extrafiles-signing.key
    XX_SIGNING_CERTIFICATE=$dest/extrafiles-signing.crt

    # These are exported for running manifest creation within automation
    export SIGNING_KEY=$XX_SIGNING_KEY
    export SIGNING_PASSWORD=$XX_SIGNING_PASSWORD
    export SIGNING_CERTIFICATE=$XX_SIGNING_CERTIFICATE
}

function checkForDataSupplement()
{
    announceFunction $FUNCNAME "$@"

    if isInstalledSavVersionEqualOrGreaterThan 9.5.5
    then
        if [[ ! -f ${INST}/lib/sav/vdlmnfst.dat ]]
        then
            testFailure "vdlmnfst.dat not found in installation directory"
        fi
    fi
}

#=============================================================================
# Free SAV

## Check that the installation has free credentials
function checkFree()
{
    announceFunction $FUNCNAME "$@"

    [[ -f $INST/etc/free_version ]] || testFailure "SAV install is not free: $INST/etc/free_version doesn't exist"
    local username="$(runSavconfig query PrimaryUpdateUsername)"
    [[ "$username" != FAVL* ]] && testFailure "SAV install is not free: Update username doesn't start with FAVL"
}

## Check that the installation doesn't have free credentials
function checkNotFree()
{
    announceFunction $FUNCNAME "$@"

    [[ -f $INST/etc/free_version ]] && testFailure "SAV install is free: $INST/etc/free_version exists"
    local username="$(runSavconfig query PrimaryUpdateUsername)"
    [[ "$username" = FAVL* ]] && testFailure "SAV install is free: Update username starts with FAVL"
}

# Check that the credential server is reachable.
function pingFreeCredsServer()
{
    announceFunction $FUNCNAME "$@"

    # Warnings here indicate that test failures are probably due to
    #  infrastructure rather than problems in SAV.

    local SERVER="https://amicreds.sophosupd.com/freelinux/creds.dat"
    if https_proxy="$LIVE_PROXY" runPython $SUP/testWebServer.py -- "$SERVER"
    then
        echo "Success"
    else
        warning "Cannot contact cred server: ${SERVER}"
    fi
}
