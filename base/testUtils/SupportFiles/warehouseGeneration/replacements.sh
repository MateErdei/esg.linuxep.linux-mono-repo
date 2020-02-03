#!/bin/bash

#Functions that replace existing system utilities.
#
#These are here to provide more cross platform support
# or add extra features.

## Execute command on remote file server
function execOnRemoteFileServer()
{
    bash $TEST_ROOT/supportFiles/remoteFileServer/sshRemoteFileServer.sh "$@"
}

## Copy files to remote file server
## @param src - directory to copy from
## @param dest - directory to copy to /share/<squash option>/<hostname>
function copyToRemoteFileServer()
{
    announceFunction $FUNCNAME "$@"

    local src="$1"
    local dest="$2"

    ## Randomly passing in TESTTMP
    [[ $dest == $TESTTMP ]] && dest=

    [[ -n "$REMOTEFS_IP" ]] || setupFailure "Attempting to copy to unknown server"

    ## Ping remote server

    print_header "ping $REMOTEFS_IP"
    sophos_ping1 "$REMOTEFS_IP"
    print_divider

    if [[ -z "$dest" ]]
    then
        local MACHINEID=$(cat $INST/engine/machine_ID.txt)
        local UNIQUE="${HOSTNAME}-${TEST}-${CODELINE}-${MACHINEID}"
        if [[ -n "$SQUASH_OPT" ]]
        then
            dest="/share/$SQUASH_OPT/${UNIQUE}"
        elif [[ -n "$NFS_OPTS" ]]
        then
            dest="/share/$NFS_OPTS/${UNIQUE}"
        else
            dest="/share/no_root_squash/${UNIQUE}"
        fi
    fi
    XX_REMOTE_SHARE=$dest

    _with_onaccess_disabled "" bash \
        $TEST_ROOT/supportFiles/remoteFileServer/scpRemoteFileServer.sh "$src" "$dest" \
        || setupFailure "Failed to copy files to remote server"
}

# Replace sed -i
function ised()
{
    ## Get the last argument
    local FILE=${!#}
    [[ -f "$FILE" ]] || failure2 "ised: $FILE doesn't exist"
    local TEMP_FILE="$TMPDIR/ised.temp"
    mkdir -p $(dirname $TEMP_FILE)
    rm -f "$TEMP_FILE"
    ## "$@" expands to all the arguments separately quoted
    sed "$@" >"$TEMP_FILE" || failure2 "Error returned from sed $@"
    ## cat to the file rather than moving/copying, to preserve symlinks
    cat "$TEMP_FILE" > "$FILE"
    rm -f "$TEMP_FILE"
}

# Exports an NFS filesystem.
# Attemps to find the system's exportfs utility, but emulates the behaviour
# if it can't be found.
# Exports will be automatically deleted at the end of test case.
# @param where   path to be exported
# @param squash  squash option - or 'no_root_squash' by default
function export_nfs()
{
    announceFunction $FUNCNAME "$@"

    local where="$1"
    local squash="$2"
    [[ -n "$squash" ]] || squash=no_root_squash
    XX_NFS_SQUASH_OPTION="$squash"

    ## check whether $NFSSERVERinitscript is defined (may be blank)
    [[ ${NFSSERVERinitscript+defined} == defined ]] \
        || setupFailure "$FUNCNAME called without calling require_nfsserver()"

    [[ -n "$XX_NFS_EXPORT" ]] \
        && failure2 "ERROR: Cannot export more than 1 NFS share from 1 test case. Change export_nfs if you need it"

    case $PLATFORM in
        "linux")
            ## No need to export remote
            [[ $REMOTEFS ]] && return

            if hash exportfs 2>/dev/null
            then
                local share="${where}"
                # NFS4 on ubuntu (at least) has issues where $HOSTNAME and localhost resolve
                # to different IPs. Exporting and mounting $HOSTNAME will not work for this reason.
                # NFS4 has issues on other platforms with mounting to localhost, even when it
                # resolves to an ip it is happy to mount to. So we're left with exporting to world.
                if ! exportfs -o rw,no_subtree_check,$squash "*:${share}"
                then
                    failure2 "Unable to export filesystem '${share}'"
                fi
                if ! exportfs -o rw,no_subtree_check,$squash,fsid=0 "*:/"
                then
                    failure2 "Unable to export filesystem / (for NFS4 pseudo-filesystem)"
                fi

                # show the exports
                exportfs
            else
                declare MARKER="### The following entry was added by Peanut integration tests"

                if [[ -e /etc/exports ]]
                then
                    if [ ! -e /etc/exports.peanut_backup ]
                    then
                        mv /etc/exports /etc/exports.peanut_backup
                    fi
                    sed -e "/^$MARKER$/q" /etc/exports.peanut_backup > /etc/exports
                fi
                {
                    echo $MARKER
                    echo "/ localhost(rw,no_subtree_check,$squash,fsid=0)"
                    echo "${where##*:} localhost(rw,no_subtree_check,$squash)"
                } >> /etc/exports
                reload_nfs_server >/dev/null
                # make sure that the nfs server is running
                showmount localhost >/dev/null 2>&1 || $NFSSERVERinitscript start >/dev/null
                cat /etc/exports
            fi
            ;;
        "sunos")
            if ! share -F nfs -o rw,anon=0 "$where"
            then
                failure2 "Unable to export filesystem '$where'"
            fi
            if ! showmount -e $HOSTNAME | grep "$where" >/dev/null
            then
                failure2 "Failed to export filesystem '$where'"
            fi
            ;;
        "hpux"|"aix")
            local localhost="localhost:${HOSTNAME%%.*}:${HOSTNAME}"
            if ! /usr/sbin/exportfs -i -o rw=$localhost,root=$localhost "$where"
            then
                failure2 "Unable to export filesystem '$where'"
            fi
            ;;
        *)
            setupFailure "WARNING: Cannot export NFS on this system"
            ;;
    esac

    XX_NFS_EXPORT="$where"

    registerCleanupFunction unexport_nfs
}

## Cleanup exported nfs filesystems
function unexport_nfs()
{
    announceFunction $FUNCNAME "$@"

    [[ "$XX_NFS_EXPORT" ]] || return

    case $PLATFORM in
        "linux")
            ## No need to unexport remote
            [[ $REMOTEFS ]] && return

            local share="${XX_NFS_EXPORT}"
            if hash exportfs 2>/dev/null
            then
                if ! exportfs -u *:$share
                then
                    failure2 "Unable to unexport filesystem '$share'"
                fi
                if ! exportfs -u *:/
                then
                    warning "Failed to unexport ${HOSTNAME}/"
                fi
            else
                rm -f /etc/exports
                if [ -e /etc/exports.peanut_backup ]
                then
                    mv /etc/exports.peanut_backup /etc/exports
                fi
                reload_nfs_server >/dev/null
            fi
            ;;
        "sunos")
            if ! unshare "$XX_NFS_EXPORT"
            then
                failure2 "Unable to unexport filesystem '$XX_NFS_EXPORT'"
            fi
            ;;
        "hpux"|"aix")
            if ! /usr/sbin/exportfs -i -u "$XX_NFS_EXPORT"
            then
                failure2 "Unable to unexport filesystem '$XX_NFS_EXPORT'"
            fi
            ;;
        *)
            setupFailure "WARNING: Cannot unexport NFS on this system"
            ;;
    esac

    unset XX_NFS_EXPORT
}

# @param what directory to mount
# @param where where to mount it
# @param which version (default=3, 4)
function mount_nfs()
{
    announceFunction $FUNCNAME "$@"

    local what="$1"
    local where="$2"
    local version=$3
    local nfsopts=$4

    [[ -n $version ]] || version=3

    [[ -n "$XX_NFS_MOUNT" ]] \
        && failure2 "ERROR: Cannot mount more than 1 NFS share from 1 test case. Change mount_nfs if you need it"

    if [[ $PLATFORM == "linux" ]]
    then
        local share
        if [[ -n $REMOTEFS ]]
        then
            local MACHINEID=$(cat $INST/engine/machine_ID.txt)
            local UNIQUE="${HOSTNAME}-${TEST}-${CODELINE}-${MACHINEID}"
            if (( $version == 4 ))
            then
                share="${REMOTEFS_IP}:${XX_NFS_SQUASH_OPTION}/${UNIQUE}${what}"
            else
                share="${REMOTEFS_IP}:/share/${XX_NFS_SQUASH_OPTION}/${UNIQUE}${what}"
            fi
        else
            share="${HOSTNAME}:${what}"
        fi

        nfsopts=async,noatime,dev,exec,noac,soft,intr${nfsopts:+,${nfsopts}}

        local mount_type="nfs"
        if (( $version == 4 ))
        then
            mount_type="nfs4"
        else
            nfsopts="${nfsopts:+${nfsopts},}nfsvers=$version"
        fi
        if ! mount -t $mount_type ${nfsopts:+-o ${nfsopts}} "$share" "$where"
        then
            ## DLCL: repeat mount attempt as I've had some failures here
            ## That resolved them selves after I redid the test
            warning "Mount failed, retrying in 3s..."
            sleep 3
            echo "Mount command=mount -t $mount_type ${nfsopts:+-o ${nfsopts}} $share $where" >&2
            mount -t $mount_type ${nfsopts:+-o ${nfsopts}} "$share" "$where"
        fi
    elif [[  $PLATFORM == "aix" ]]
    then
        nfsopts="${nfsopts:+${nfsopts},}vers=$version,soft,intr"

        mount -v nfs -o $nfsopts "localhost:$what" "$where" \
            || mount -v nfs -o $nfsopts "$HOSTNAME:$what" "$where"
    else
        nfsopts="${nfsopts:+${nfsopts},}vers=$version,soft,intr"

        mount -F nfs -o $nfsopts "localhost:$what" "$where" \
            || mount -F nfs -o $nfsopts "$HOSTNAME:$what" "$where"
    fi

    local ret=$?

    if (( $ret != 0 ))
    then
        echo "which mount.nfs"
        which mount.nfs
        echo "/sbin/mount.nfs"
        ls -l /sbin/mount.nfs
        echo "dmesg | tail"
        dmesg | tail
        [[ -n $REMOTEFS_IP ]] && ping -c 2 $REMOTEFS_IP
        failure2 "Cannot mount '$what' as '$where' ($ret)"
    else
        XX_NFS_MOUNT="$where"
        XX_NFS_MOUNT_SRC="$what"
        registerCleanupFunction umount_nfs

        if ! mount | grep "$where"
        then
            warning "cannot find mount point ($where) in 'mount' output:"
            mount
        fi
    fi
}

function __umount_nfs_helper()
{
    local opt
    for opt in "" -i -f -fl ; do
        umount $opt "$XX_NFS_MOUNT" && return
        umount $opt "$XX_NFS_MOUNT_SRC" && return
    done
    sleep 1
}

## Cleanup mounted nfs
function umount_nfs()
{
    announceFunction $FUNCNAME "$@"

    if [[ -n "$XX_NFS_MOUNT" ]]
    then
        if have lsof
        then
            announce lsof "$XX_NFS_MOUNT"
            lsof "$XX_NFS_MOUNT" 2>&1
        fi
        __umount_nfs_helper || __umount_nfs_helper || {
            failure2 "Cannot umount '$XX_NFS_MOUNT' from '$XX_NFS_MOUNT_SRC'"
        }
        unset XX_NFS_MOUNT
    fi
}

## Cleanup mounted server
function cleanup_remote_share()
{
    announceFunction $FUNCNAME "$@"
    execOnRemoteFileServer "rm -rf ${XX_REMOTE_SHARE}"
}


#Provide an in-place, inverted, extended regexp grep
#
# $1 = The pattern to match
# $2 = The path to the file to process
function ivegrep()
{
    local ivegrepTEMP=$(sophos_mktemp /tmp/ivegrepTEMP.XXXXXX)
    egrep -v "$1" "$2" > $ivegrepTEMP
    #Use cat to preserve all special properties/permissions of the target.
    cat $ivegrepTEMP > "$2"
    rm -f $ivegrepTEMP
}

# find best safesleep function alternative
if sleep 0.0 2>/dev/null
then
    ## Sleep that won't report an error on sub-second sleeps
    function safesleep()
    {
        sleep "$1"
    }
else
    ## Sleep that won't report an error on sub-second sleeps
    function safesleep()
    {
        # decide if it's a sub-second sleep or not, so we only need to spawn 'sleep' once
        local delay=$(( 10#${1%.*} ))

        if (( delay == 0 ))
        then
            sleep 1
        else
            sleep $delay
        fi
    }
fi

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

## chown that refuses to change /tmp
function safechown()
{
    announceFunction $FUNCNAME "$@"

    local arg
    for arg in "$@"
    do
        if [[ $arg == "/tmp" ]]
        then
            failure2 "Attempting to chown /tmp: $@"
        fi
        if [[ $arg == "/" ]]
        then
            failure2 "Attempting to chown /: $@"
        fi
        if [[ -z "$arg" ]]
        then
            failure2 "Attempting to chown empty string: $@"
        fi
    done
    chown "$@"
}

## rm -rf that refuses to delete /tmp or /
function safermrf()
{
    announce "safermrf $@"

    local arg
    for arg in "$@"
    do
        if [ -z "$arg" ]
        then
            warning "Attempting to delete empty string"
        elif [ "$arg" = "/" ]
        then
            failure2 "Attempting to rm -rf /: $@"
        elif [ "$arg" = "//" ]
        then
            failure2 "Attempting to rm -rf //: $@"
        elif [ "$arg" = "/tmp" ]
        then
            failure2 "Attempting to rm -rf /tmp: $@"
        elif [ "$arg" = "/dev/null" ]
        then
            failure2 "Attempting to rm -rf /dev/null: $@"
        ## Simple test for alternative forms of /
        elif [ -x "$arg/bin/sh" ]
        then
            failure2 "Refusing to rm -rf $arg due to $arg/bin/sh: $@"
        else
            rm -rf "$arg"
        fi
    done
}

## Replacement for stat -c'%Y'
function modificationTime()
{
    LD_LIBRARY_PATH=$LD_LIBRARY_PATH:./lib bin/ministat "%Y" "$@"
}

## Replacement for stat -c"%s"
function fileSize()
{
    LD_LIBRARY_PATH=$LD_LIBRARY_PATH:./lib bin/ministat "%s" "$@"
}

## Get the of last access to file
## Replacement for stat -c"%x"
function accessTime()
{
    LD_LIBRARY_PATH=$LD_LIBRARY_PATH:./lib bin/ministat "%x" "$@"
}

# get the owner name of a file/dir
## Replacement for stat -c"%U" $1
function stat_U()
{
    set -- $(ls -ld "$1")
    echo $3
}



## Simulate --maxdepth 1 parameter from GNU find - remove all paths deeper than 1
## Note: Can specify only 1 path at a time, unlike original find!!
## Usage:
##   findMaxdepth1 <path> <filter expression>
function _findMaxdepth1()
{
    local dir=$1
    [[ -d $dir ]] || failure2 "Passed an invalid path to _findMaxdepth1"
    shift

    (
        # subshell to rollback any set shopts
        shopt -s dotglob
        shopt -s nullglob

        find "${dir%/}" "${dir%/}"/* -prune "$@"
    )
}

function unixTime()
{
    local DATE=$TEST_ROOT/bin/minidate
    [ -x "$DATE" ] || DATE=$(which date 2>/dev/null)
    [ -x "$DATE" ] || failure2 "neither distro date nor minidate are available"

    local x
    for x in 1 2 3 4 5
    do
       LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$TEST_ROOT/lib "$DATE" +%s
       local RET=$?
       (( $RET == 0 )) && return 0
       warning "$FUNCNAME: Failed to get epoch time ($RET) - $x"
    done
    failure2 "Unable to retrieve date in +%s format"
}

# Simulates ps -C procname
# So returns something if the process name is running.
function isRunning()
{
    ## We can only pass one argument to grep, so we assume the caller wanted all
    ## of their arguments to be treated as one string.
    if [ $PLATFORM = "freebsd" ]
    then
        ps -Ac | LC_ALL=C grep -w "${*}[.0-9]*"
    else
        ps -e  | LC_ALL=C grep -w "${*}[.0-9]*"
    fi
}


## Gets just the sha1 sum for a given file.
## NB: This is not the verbatim output of sha1sum, just the hex digits
function _getSha1sum()
{
    local SHA1SUM=$(which sha1sum 2>/dev/null)
    if [ -x "$SHA1SUM" ]
    then
        "$SHA1SUM" "$@" | sed -e 's/ .*$//'
    else
        runOpenssl dgst -sha1 "$@" | sed -ne 's/^SHA1(.*)= //p'
    fi
}

if hash md5sum &>/dev/null
then
    ## A replacement for md5sum, except that the white space between the hash and
    ## the filename is reduced to a single space (Suitable for manifest files)
    function md5sum_rep()
    {
        md5sum "$@" | sed -e"s/^\([0-9a-f]\+\)[[:space:]]\+/\1 /"
    }

    ## Get the MD5 sum of a file.
    ## NB: just the sum, not the md5sum output
    function _getMd5sum()
    {
        set -- $(md5sum "$@")
        echo $1
    }
else
    ## A replacement for md5sum, except that the white space between the hash and
    ## the filename is reduced to a single space (Suitable for manifest files)
    function md5sum_rep()
    {
        runOpenssl dgst -md5 "$@" | sed -ne"s/^MD5(\(..*\))= \([a-f0-9][a-f0-9]*\)/\2 \1/p"
    }

    ## Get the MD5 sum of a file.
    ## NB: just the sum, not the md5sum output
    function _getMd5sum()
    {
        set -- $(runOpenssl dgst -md5 "$@")
        echo $_
    }
fi

## Replicated cp -a as closely as possible
function cp_a()
{
    if [ $PLATFORM = "linux" ]
    then
        cp -a "$@"
    elif [ $PLATFORM = "sunos" ]
    then
        if [ $(uname -r) = "5.9" ]
        then
            cp -R -p -@ "$@"
        else
            cp -R -P -p -@ "$@"
        fi
    elif [ $PLATFORM = "freebsd" ]
    then
        cp -R -p "$@"
    elif [ $PLATFORM = "aix" ]
    then
        cp -hPpR "$@"
    elif [ $PLATFORM = "hpux" ]
    then
        # replicates cp -a too closely? often noisy as we cannot change
        # ownership to nobody/nogroup on HP-UX
        if [ -x /usr/local/coreutils/bin/cp ]
        then
            local TEMPPATH=/usr/local/coreutils/bin:$PATH
            PATH=$TEMPPATH cp -a "$@"
        else
            cp -R -P -p "$@"
        fi
    else
        warning "New platform - cp might work differently"
        cp -a "$@"
    fi
}

function cp_al()
{

    if [ $PLATFORM = "linux" ]
    then
        cp -al "$@"
    else
        {
            cd "$1"
            find . -depth -type f | cpio -pdl "$2"
        }
    fi
}

## Replicate ln -snf as closely as possible
function ln_snf()
{
    if [ $PLATFORM = "linux" ]
    then
        ln -snf "$@"
    elif [ $PLATFORM = "sunos" ]
    then
        rm -f "$2"
        ln -s "$@"
    elif [ $PLATFORM = "freebsd" ]
    then
        ln -snf "$@"
    elif [ $PLATFORM = "aix" ]
    then
        ln -sf "$@"
    else
        warning "New platform - ln might work differently"
        ln -snf "$@"
    fi
}

## Unpack ZIP file (created by --diagnose)
function sophos_unzip()
{
    local zipfile=$1
    local dest=$2

    runPython $TEST_ROOT/supportFiles/unzip.py -v -i $zipfile -o $dest

    (( $? != 0 )) && setupFailure "Error unzipping $zipfile"
}

## Unpack a tarfile, ensuring that we avoid TGZ and -C on solaris
## $1 - tarfile to unpack
## $2 - destination dir
## Other args, passed to tar
function unpack()
{
    announceFunction $FUNCNAME "$@"

    local tarfile="$1"
    shift
    local dest="$1"
    shift
    mkdir -p "$dest"
    local RET=0
    local TAR=$(which tar 2>/dev/null)

    [[ $tarfile == /* ]] || tarfile=$PWD/$tarfile

    [ -x "$TAR" ] || setupFailure "Can't find working tar"
    local TEMPPATH=$PATH

    ## Despite manpage and help text says -C is supported on AIX it
    ## doesn't appear to work in all cases
    local SUPPORTS_C=0
    if [ "$PLATFORM" = "linux" ]
    then
        SUPPORTS_C=1
    elif [ "$PLATFORM" = "hpux" ]
    then
        local TEMPPATH=/usr/contrib/bin:$PATH

        ## HPUX /usr/bin/tar doesn't support -C
        [ -x /usr/local/bin/tar ] && TAR=/usr/local/bin/tar

        if [ "$TAR" != /usr/bin/tar ]
        then
            SUPPORTS_C=1
            TEMPPATH=/usr/local/bin:$TEMPPATH
        fi
    fi

    if [[ $tarfile == *.tar ]]
    then
        if (( $SUPPORTS_C == 1 ))
        then
            PATH="$TEMPPATH" "$TAR" xf "$tarfile" -C "$dest" "$@"
            RET=$?
        elif [ "$PLATFORM" = "sunos" -o "$PLATFORM" = "freebsd" -o "$PLATFORM" = "aix"  -o "$PLATFORM" = "hpux" ]
        then
            pushd "$dest"
            PATH="$TEMPPATH" "$TAR" xf "$tarfile" "$@"
            RET=$?
            popd
        else
            warning "extracting tar on new platform"
            pushd "$dest"
            PATH="$TEMPPATH" "$TAR" xf "$tarfile" "$@"
            RET=$?
            popd
        fi
        return
    elif [[ $tarfile == *.tar.gz || $tarfile == *.tgz ]]
    then
        if (( $SUPPORTS_C == 1 ))
        then
            PATH="$TEMPPATH" "$TAR" xzf "$tarfile" -C "$dest" "$@"
            RET=$?
        elif [ "$PLATFORM" = "sunos" ]
        then
            pushd "$dest"
            PATH="$TEMPPATH" gunzip -c "$tarfile" | "$TAR" xf - "$@"
            RET=$?
            popd
            (( $RET == 0 )) || setupFailure "Failed to extract tgz on Solaris"
        elif [ "$PLATFORM" = "freebsd" ]
        then
            pushd "$dest"
            PATH="$TEMPPATH" "$TAR" xzf "$tarfile" "$@"
            RET=$?
            popd
        elif [ "$PLATFORM" = "aix" ]
        then
            pushd "$dest"
            PATH="$TEMPPATH" gunzip -c "$tarfile" | "$TAR" xf - "$@"
            RET=$?
            popd
        elif [ "$PLATFORM" = "hpux" ]
        then
            pushd "$dest"
            PATH="$TEMPPATH" gunzip -c "$tarfile" | "$TAR" xf - "$@"
            RET=$?
            popd
        else
            warning "extracting tgz on non-linux"
            PATH="$TEMPPATH" "$TAR" xzf "$tarfile" -C "$dest" "$@"
            RET=$?
        fi
    elif [[ $tarfile == *.tar.bz2 ]]
    then
        if (( $SUPPORTS_C == 1 ))
        then
            PATH="$TEMPPATH" "$TAR" xjf "$tarfile" -C "$dest" "$@"
            RET=$?
        else
            pushd "$dest"
            PATH="$TEMPPATH" "$TAR" xjf "$tarfile" "$@"
            RET=$?
            popd
        fi
    else
        setupFailure "Trying to unpack unknown archive $tarfile"
    fi
    return $RET
}

## Determine if a given directory is mounted
function isMounted()
{
    local MOUNTPT="$1"
    mount | grep "$MOUNTPT" >/dev/null 2>&1
}

## Create temp directory (mktemp -d)
## @param  Path to temp dir.
## @return  Actual name of created dir
function sophos_mktempdir()
{
    local _mktemp=$(which mktemp 2>/dev/null)
    if (( $IS_HPUX == 1 )) && [[ "${_mktemp}" == "/usr/bin/mktemp" ]]
    then
        ## Use a better mktemp if it is available
        [[ -x /usr/local/coreutils/bin/mktemp ]] && _mktemp=/usr/local/coreutils/bin/mktemp
    fi

    local _tempdir
    if (( $IS_HPUX == 1 )) && [[ "${_mktemp}" = "/usr/bin/mktemp" ]]
    then
        ## Work-around for rather broken mktemp on HPUX
        _tmpdir=$("${_mktemp}" -d "${1%/*}" -p ${1##*/})
    elif [[ -x "${_mktemp}" ]] ; then
        # mktemp exists - use it
        _tmpdir=$(${_mktemp} -d "$1_XXXXXXX")
    else
        # mktemp not available - use mkdir
        [[ "${_tmpdir}" ]] || _tmpdir=${1}_${RANDOM-0}.${RANDOM-0}.${RANDOM-0}.$$
        (umask 077 && mkdir ${_tmpdir})
    fi

    if [[ ! -d "${_tmpdir}" ]] ; then
        warning "Could not create temporary directory"
        exit 1
    fi

    echo ${_tmpdir}
}

## Create temp file (mktemp)
## @param  Path to temp file.
## @return  Actual name of created file
function sophos_mktemp()
{
    local _mktemp=$(which mktemp 2>/dev/null)
    if (( $IS_HPUX == 1 )) && [[ "${_mktemp}" == "/usr/bin/mktemp" ]]
    then
        ## Use a better mktemp if it is available
        [[ -x /usr/local/coreutils/bin/mktemp ]] && _mktemp=/usr/local/coreutils/bin/mktemp
    fi

    local _tmp
    if (( $IS_HPUX == 1 )) && [[ "${_mktemp}" = "/usr/bin/mktemp" ]]
    then
        ## Work-around for rather broken mktemp on HPUX
        _tmp=$("${_mktemp}" -c -d "${1%/*}" -p ${1##*/})
    elif [[ -x "${_mktemp}" ]]
    then
        # mktemp exists - use it
        _tmp=$("${_mktemp}" "$1_XXXXXXX")
    else
        # mktemp not available - use mkdir
        _tmp=${1}_${RANDOM-0}.${RANDOM-0}.${RANDOM-0}.$$
        (umask 077 && touch ${_tmp})
    fi

    if [[ ! -f "${_tmp}" ]]
    then
        warning "Could not create temporary file"
        exit 1
    fi

    echo ${_tmp}
}

## Replace id -u
function id_u()
{
    #$TEST_ROOT/bin/miniid -u || setupFailure "Unable to use miniid"
    echo $EUID
}

## Replace id -g
function id_g()
{
    #$TEST_ROOT/bin/miniid -g || setupFailure "Unable to use miniid"
    echo $GROUPS
}

## Replace id -G
function id_G()
{
    local ID
    if [[ -x /usr/xpg4/bin/id ]]
    then
        ID=/usr/xpg4/bin/id
    else
        ID=$(which id)
    fi

    $ID -G "$@"
}

## Arguments are the command to run as nobody
function su_nobody()
{
    announceFunction $FUNCNAME "$@"

    # For hpux, create and use a user qnobody, as using
    # nobody doesn't work, even though it exists
    if [ "$PLATFORM" = "hpux" ]
    then
        useradd qnobody
        su_user "qnobody" "$@"
        local ret=$?
        userdel qnobody
        return $ret
    else
        _su_user "nobody" "$@"
    fi
}

## su_user <user> <command>+
function su_user()
{
    announceFunction $FUNCNAME "$@"

    _su_user "$@"
}

function _su_user()
{
    local USER=$1
    shift

    [[ $USER ]] || failure2 "su_user: needs a username as first arg"
    [[ $@ ]] || failure2 "su_user: needs a command to run"

    case $PLATFORM in
    linux)
        #~ su -s "/bin/sh" "$USER" -c "$@"
        #~ su -s "/bin/sh" "$USER" -c "$(printf '%q ' "$@")"
        su -p -s "/bin/sh" "$USER" -c "$*"
        ;;
    hpux)
        su "$USER" -c "$*"
        ;;
    sunos)
        su "$USER" -c "$*"
        ;;
    freebsd)
        su -m "$USER" -c "$*"
        ;;
    *)
        su "$USER" -c "$*"
        ;;
    esac
}

## Arguments are the command to run as nobody when accessing the REMOTEFS
function su_nobody_remotefs()
{
    announceFunction $FUNCNAME "$@"

    if [[ $(grep '65534:65534' /etc/passwd | cut -d: -f1) == "nfsnobody" ]]
    then
        _su_user "nfsnobody" "$@"
    else
        # For hpux, create and use a user qnobody, as using
        # nobody doesn't work, even though it exists
        if [ "$PLATFORM" = "hpux" ]
        then
            useradd qnobody
            su_user "qnobody" "$@"
            local ret=$?
            userdel qnobody
            return $ret
        else
            _su_user "nobody" "$@"
        fi
    fi
}

## $1 is a file.
## $2 is a command to run as the user/group of the file
function su_file()
{
    announceFunction $FUNCNAME "$@"
    if [[ -x $TEST_ROOT/bin/su_file ]]
    then
        LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$TEST_ROOT/lib $TEST_ROOT/bin/su_file "$@"
    else
        runPython $TEST_ROOT/supportFiles/su_file.py "$@"
    fi
}

## readlink does not exist on Solaris
function sophos_readlink()
{
    local _readlink="$(which readlink 2>/dev/null)"

    if [ -x "${_readlink}" ]
    then
        # readlink exists - use it
        local _tmp="$(${_readlink} $1)"
    else
        # mktemp not available - use ls and awk
        local _tmp="$(ls -l $1 | awk '{print $NF}')"
    fi

    echo ${_tmp}
}

## readlink does not exist on Solaris
function sophos_readlink_f()
{
    local result="$1"

    while [[ -L $result ]]
    do
        local _tmp="$(sophos_readlink "$result")"
        if [[ $_tmp == /* ]]
        then
            result="$_tmp"
        else
            result="${result%/*}/$_tmp"
        fi
    done

    echo "$result"
}

function sophos_pstack()
{
    announceFunction $FUNCNAME "$@"

    # is arg non-numeric?
    if [[ $1 == *[!0-9]* ]]
    then
            local PID
            for PID in $(sophos_pgrep $1)
            do
                sophos_pstack $PID
            done
            return
    fi

    local PID=$1

    case $PLATFORM in
    linux)
        if ! command -v gdb >/dev/null
        then
            echo "$FUNCNAME: cannot find 'gdb'"
            return 2
        fi

        print_header "pstack $PID"
        unset PYTHON_HOME
        unset PYTHON_PATH

        if [[ -d /proc/$PID ]]
        then
            # show the WCHAN of all the threads (if we can).
            ps -p $PID -L -o user,tid,pid,ppid,%cpu,stime,tty,time,wchan:24,comm 2>/dev/null \
                || ps -f -p $PID
            echo
            gdb "/proc/$PID/exe" "$PID" -batch -x $SUP/pstack.gdb </dev/null
        else
            echo "$FUNCNAME: Can't find process: $PID"
            return 1
        fi
        print_divider
        ;;
    sunos|hpux)
        # try /usr/ccs/bin too, for HP-UX
        local PSTACK=$(PATH=$PATH:/usr/ccs/bin which pstack 2>/dev/null)
        if [[ ! -x $PSTACK ]]
        then
            echo "$FUNCNAME: Cannot find pstack" >&2
            return 2
        fi

        # pstack always succeeds, check the process is alive first
        kill -0 $PID 2>/dev/null || return 1

        print_header "pstack $PID"
        $PSTACK $PID || return 1
        print_divider
        ;;
    aix)
        if [[ -x /usr/bin/procstack ]]
        then
            print_header "procstack $PID"
            procstack $PID || return 1
            print_divider
            return 0
        else
            warning "Can't find procstack"
            return 1
        fi
        ;;
    *)
        warning "No pstack defined for $PLATFORM"
        return 2
        ;;
    esac
}

if [ "$PLATFORM" = "hpux" ]
then
    MOUNT=/sbin/mount
    UMOUNT=/sbin/umount
else
    MOUNT=mount
    UMOUNT=umount
fi

## Echo a usable custom install directory
## Directory has to be big enough. Different systems have it configured in different way.
function getCustomInstallationDirectory()
{
    if [ -d /export/home ]
    then
        ## Different install path, as /tmp on Solaris 10 is rather tight
        echo "/export/home/savinstall"
    elif [ -d /storage ]
    then
        ## FreeBSD has large disk at /storage
        echo "/storage/savinstall"
    else
        echo "/opt/custom"
    fi
}

## Output the hostname of the system - but remove the domain
function sophos_hostbasename()
{
    echo ${HOSTNAME%%.*}
}

## Output the full hostname of the system, with the domain
function sophos_hostname_full()
{
    echo ${HOSTNAME}
}

## Place-holder for wc --chars, that is a bit more cross platform
## (AIX wc can't handle wc --chars)
function wc_chars()
{
    wc -c "$@"
}

# ## Convert input parameters to upper case
# function toupper()
# {
#     echo "$@" | LC_ALL=C tr '[:lower:]' '[:upper:]'
# }

## Convert input parameters to lower case
function tolower()
{
    echo "$@" | LC_ALL=C tr '[:upper:]' '[:lower:]'
}

## netcat
function netcat()
{
    local NETCATBIN=

    if [ $PLATFORM != "linux" ]
    then
        warning "New platform - netcat may not be available?"
    fi

    NETCATBIN=$(which netcat 2>/dev/null)
    if [ "$NETCATBIN" = "" ]
    then
        NETCATBIN=$(which nc 2>/dev/null)
    fi

    if [[ "$NETCATBIN" != "" && -x "$NETCATBIN" ]]
    then
        $NETCATBIN "$@"
    else
        testFailure "No netcat command found!"
    fi
}

## do an ls which displays the best time accuracy available
function lsfulltime()
{
    ## Add per platform controls as we discover them...
    ls --full-time "$@" 2>/dev/null \
        || ls -l --time-style=full-iso "$@" 2>/dev/null \
        || ls -l "$@"
}

## Copy a file, while ensuring that on-access doesn't detect it
## and setupFailure if we can't copy it.
# @param src    source file to copy
# @param dst    destination for the copy
# @params remainder Included in failure messages
function copyThreat()
{
    announceFunction $FUNCNAME "$@"
    _copyThreat "$@"
}

## Copy a file, while ensuring that on-access doesn't detect it
## and setupFailure if we can't copy it.
# @param src    source file to copy
# @param dst    destination for the copy
# @params remainder Included in failure messages
function _copyThreat()
{
    local SRC="$1"
    shift
    [[ -f "$SRC" ]] || setupFailure "Attempting to copy $SRC, which doesn't exist"

    local DEST="$1"
    mkdir -p "${DEST%/*}"
    shift

    local disabled=
    if ! checkForOnAccess
    then
        disabled="on_access_disabled"
    fi

    [ -n "$disabled" ] || onaccess_disable
    if ! cp -f "$SRC" "$DEST"
    then
        ## Need to re-enable onaccess if the copy failed for other reasons
        local RET=$?
        echo "$disabled"
        ls -l "$INST/var/run/onaccess.status"
        cat "$INST/var/run/onaccess.status"
        [ -n "$disabled" ] || onaccess_enable
        setupFailure "Unable to copy $SRC to $DEST: $RET : $@"
    fi
    local MODE="$1"
    if [ -n "$MODE" ]
    then
        safechmod $MODE $DEST || setupFailure "Unable to chmod $MODE $DEST"
    fi
    [ -n "$disabled" ] || onaccess_enable
}

# Consistent version of 'groups' command. Returns a list of groups that a user
# is a member of.
# Some platforms just return a list of groups, but others will prepend it with
# the username, so remove that if it is present.
function sophos_groups()
{
    groups "$@" | sed "s/.* : //"
}

# Find a POSIX df implementation.
function sophos_df()
{
    local DF
    if [[ $PLATFORM == "sunos" ]]
    then
        [[ -x /usr/xpg4/bin/df ]] && DF=/usr/xpg4/bin/df
    else
        DF=$(which df)
    fi
    [[ -x $DF ]] || setupFailure "cannot find 'df' command"

    $DF -P "$@"
}

# Find a POSIX du implementation.
function sophos_du()
{
    local DU
    DU=$(which du)
    case $PLATFORM in
    sunos)
        [[ -x /usr/xpg4/bin/du ]] && DU=/usr/xpg4/bin/du
        ;;
    esac

    [[ -x $DU ]] || setupFailure "cannot find 'du' command"

    # for Solaris 10. (and HP-UX?) du will give the wrong answer if we don't do this!
    sync

    $DU "$@"
}

function sophos_du_sm()
{
    set -- $(sophos_du -sk "$@")
    # take size in KB and divide, but add 1023 first, so we round up.
    echo $(( ( $1 + 1023 ) / 1024 ))
}

function sophos_groupadd()
{
    local group=$1

    if [[ $PLATFORM == "aix" ]]
    then
        mkgroup $group
    else
        groupadd $group
    fi
}

function sophos_groupdel()
{
    local group=$1

    if [[ $PLATFORM == "aix" ]]
    then
        rmgroup $group
    else
        groupdel $group
    fi
}

function sophos_ping1()
{
    local PING=$(which ping 2>/dev/null)
    [[ -x "$PING" ]] || return

    case $PLATFORM in
        hpux)
            $PING "$1" -n 1
            ;;
        solaris|sunos)
            $PING -s "$1" 64 1
            ;;
        *)
            $PING -c 1 "$1"
            ;;
    esac
}

function sophos_ping_is_alive()
{

    local PING=$(which ping 2>/dev/null)
    [[ -x "$PING" ]] || return

    case $PLATFORM in
        hpux)
            $PING "$1" -n 1
            ;;
        solaris|sunos)
            $PING "$1"
            ;;
        *)
            $PING -c 1 "$1"
            ;;
    esac
}

# Ensure that an NFS server is running.  If one can not be started, the test
# will be skipped.
function require_nfsserver()
{
    announceFunction $FUNCNAME "$@"

    export NFSSERVERinitscript=""
    if [ -f /etc/init.d/nfsserver ]
    then
        NFSSERVERinitscript="/etc/init.d/nfsserver"
        if ! isRunning nfsd >/dev/null
        then
            $NFSSERVERinitscript status >/dev/null || $NFSSERVERinitscript start &>/dev/null
            $NFSSERVERinitscript status >/dev/null || skipOnMissingDependancy "nfsserver not running"
        fi
    elif [ -x /etc/init.d/nfs ]
    then
        NFSSERVERinitscript="/etc/init.d/nfs"
        if ! isRunning nfsd >/dev/null
        then
            $NFSSERVERinitscript restart &>/dev/null
            waitFor 5 isRunning nfsd >/dev/null \
                || skipOnMissingDependancy "nfsserver not running"
        fi

    elif [ -x /etc/init.d/nfs-user-server ]
    then
        NFSSERVERinitscript="/etc/init.d/nfs-user-server"
        if ! isRunning nfsd >/dev/null
        then
            $NFSSERVERinitscript restart &>/dev/null
            waitFor 5 isRunning nfsd >/dev/null \
                || skipOnMissingDependancy "nfsserver not running"
        fi
    elif [ -x /etc/init.d/nfs-kernel-server ]
    then
        # Like Solaris below, on some distros the kernel NFS server won't start if there's nothing exported.
        # So, we check if /etc/exports has any non-comment lines in it; if not, we add a dummy export.
        if ! egrep -v -e '\#' -e '^\W*$' /etc/exports &>/dev/null
        then
            cp /etc/exports "$TMPDIR/exports.backup"
            registerCleanupFunction cp -f "$TMPDIR/exports.backup" /etc/exports
            mkdir -p "$TMPDIR/nfs"
            echo "$TMPDIR/nfs  127.0.0.1(ro,sync,no_subtree_check)" >>/etc/exports
        fi

        NFSSERVERinitscript="/etc/init.d/nfs-kernel-server"
        if ! isRunning nfsd >/dev/null
        then
            $NFSSERVERinitscript restart &>/dev/null
            waitFor 5 isRunning nfsd >/dev/null \
                || skipOnMissingDependancy "nfsserver not running"
        fi
    elif [ -x /bin/systemctl ]
    then
        NFSSERVERinitscript="/bin/systemctl start nfs.service"
        NFSSERVERinitscriptTwo="/bin/systemctl start nfsserver.service"
        if ! isRunning nfsd >/dev/null
        then
            $NFSSERVERinitscript &>/dev/null
            $NFSSERVERinitscriptTwo &>/dev/null
            waitFor 5 isRunning nfsd >/dev/null \
                || skipOnMissingDependancy "nfsserver not running"
        fi
    elif (( $USE_SVCADM == 1 )) && svcs "svc:/network/nfs/server" >/dev/null 2>&1
    then
        # Solaris 10 & 11
        if [[ $( svcs -H -o state "svc:/network/nfs/server") != online ]]
        then
            warning "nfs server was not running, new mounts may suffer a 90s delay while waiting for old locks"

            # nfs won't start unless something is shared
            mkdir -p "$TMPDIR/nfs"
            share -F nfs "$TMPDIR/nfs"
            registerCleanupFunction unshare -F nfs "$TMPDIR/nfs"

            svcadm clear "svc:/network/nfs/server"
            svcadm -v enable -s -t "svc:/network/nfs/server"

            local DEADLINE=$(( $SECONDS + 10 ))
            while (( $SECONDS < $DEADLINE ))
            do
                [[ $(svcs -H -o state "svc:/network/nfs/server") == online ]] \
                    && break

                safesleep 0.1
            done

            [[ $(svcs -H -o state "svc:/network/nfs/server") == online ]] \
                || setupFailure "Failed to start nfs server"
        fi
    elif [ -x /etc/init.d/nfs.server ]
    then
        # Solaris 9
        NFSSERVERinitscript="/etc/init.d/nfs.server"
        if ! isRunning nfsd >/dev/null
        then
            $NFSSERVERinitscript stop
            $NFSSERVERinitscript start
            waitFor 5 isRunning nfsd >/dev/null \
                || skipOnMissingDependancy "nfsserver not running"
        fi
    elif [ -x /sbin/init.d/nfs.server ]
    then
        # HPUX
        RPCBINDinitscript="/sbin/init.d/nfs.core"
        if ! isRunning rpcbind >/dev/null
        then
            $RPCBINDinitscript stop
            $RPCBINDinitscript start
            waitFor 5 isRunning rpcbind >/dev/null \
                || skipOnMissingDependancy "nfsserver not running"
        fi
        NFSSERVERinitscript="/sbin/init.d/nfs.server"
        if ! isRunning nfsd >/dev/null
        then
            $NFSSERVERinitscript stop
            $NFSSERVERinitscript start
            waitFor 5 isRunning nfsd >/dev/null \
                || skipOnMissingDependancy "nfsserver not running"
        fi
    elif [ -x /etc/rc.nfs ]
    then
        # AIX
        NFSSERVERinitscript="/etc/rc.nfs"
        if [ ! -f /etc/exports ]
        then
            skipOnMissingDependancy "missing /etc/exports, nfsd will not start"
        elif ! isRunning nfsd > /dev/null
        then
            # rc.nfs doesn't like to be re-run, so just skip
            skipOnMissingDependancy "nfsserver not running"
        fi
    else
        skipOnMissingDependancy "Can't find nfs server initd script"
    fi

    if [ -f /etc/init.d/nfslock ]
    then
        NFSLOCKinitscript="/etc/init.d/nfslock"
        if ! isRunning rpc.statd >/dev/null
        then
            $NFSLOCKinitscript status >/dev/null || $NFSLOCKinitscript start &>/dev/null
            $NFSLOCKinitscript status >/dev/null || skipOnMissingDependancy "nfslock / rpc.statd not running"
        fi
    fi
}

function __nfs_server_command()
{
    if [[ -f $NFSLOCKinitscript ]]
    then
        $NFSLOCKinitscript "$@"
    elif [[ -x /bin/systemctl ]]
    then
        /bin/systemctl "$@" nfs.service
    else
        ## will do a 'start' rather than to normal action?
        $NFSLOCKinitscript
    fi
}

function restart_nfs_server()
{
    announceFunction $FUNCNAME "$@"

    __nfs_server_command restart
}

function reload_nfs_server()
{
    announceFunction $FUNCNAME "$@"

    __nfs_server_command reload
}

function start_nfs_server()
{
    announceFunction $FUNCNAME "$@"

    __nfs_server_command start
}

function stop_nfs_server()
{
    announceFunction $FUNCNAME "$@"

    __nfs_server_command stop
}

function nfs_server_status()
{
    announceFunction $FUNCNAME "$@"

    __nfs_server_command status
}
