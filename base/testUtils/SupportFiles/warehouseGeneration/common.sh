#! /bin/bash

exec 3>&1 4>&2

function __common_sh_earlyinit()
{
    # remember the test timeout.
    XX_TEST_TIMEOUT=$(( ${TIMEOUT:-5} * 60 ))

    WARNINGS=$TESTTMP/.warnings
    registerCleanupFunction _dump_warnings
}

function __common_sh_init()
{
    XX_EICAR='H*'${XX_EICAR}
    XX_EICAR='$H+'${XX_EICAR}
    XX_EICAR='ST-FILE!'${XX_EICAR}
    XX_EICAR='RUS-TE'${XX_EICAR}
    XX_EICAR='IVI'${XX_EICAR}
    XX_EICAR='DARD-ANT'${XX_EICAR}
    XX_EICAR='TAN'${XX_EICAR}
    XX_EICAR='CAR-S'${XX_EICAR}
    XX_EICAR='CC)7}$EI'${XX_EICAR}
    XX_EICAR='X54(P^)7'${XX_EICAR}
    XX_EICAR='@AP[4\PZ'${XX_EICAR}
    XX_EICAR='!P%'${XX_EICAR}
    XX_EICAR='5O'${XX_EICAR}
    XX_EICAR='X'${XX_EICAR}

    [[ -f "${EICAR}" ]] || createEicar "${EICAR}"
}

function _create_test_path_limit_set()
{
    # don't recreate the limit set if we already have it
    [[ -n $TEST_PATH_LIMIT_SET ]] && return

    local TEST_PATH_LIMIT_VARIABLE=PROD_${TEST_PRODUCT}_PATH_INCLUDE
    TEST_PATH_LIMIT_SET=(
        $(cd $TEST_ROOT &&
            for X in $(eval echo \${$TEST_PATH_LIMIT_VARIABLE[@]})
            do
                _findMaxdepth1 . -name "$X"
            done
        ))
}

# repeatedy runs "$1" "$X" "${args 2 onwards}"
# where "$X" is replaced each time with a different path from the test path limit set
function for_each_test_path_limit_set()
{
    local SUITE_ROOT=$(dirname $0)
    local COMMAND="$1"
    shift

    local -a COMMAND_ARGS=( "${@}" )

    _create_test_path_limit_set

    local LIMIT_TEST_PATH
    for LIMIT_TEST_PATH in "${TEST_PATH_LIMIT_SET[@]}"
    do
        "$COMMAND" "${SUITE_ROOT}/$LIMIT_TEST_PATH" "${COMMAND_ARGS[@]}"
    done
}

function list_tests_recursively()
{
    local TEST_TREE_ROOT="$1"
    shift

    local SETUPDIRS=$(find ${TEST_TREE_ROOT} -name setup.sh -print | sed -e "s:/setup.sh$::")
    local SETUPDIR
    for SETUPDIR in $SETUPDIRS
    do
        _findMaxdepth1 $SETUPDIR -type f ! -name '*.expect' ! -name '*~' ! -name 'setup.sh' "$@"
    done
}

function targetSystem()
{
    # run script from overlay
    runPython $TEST_ROOT/supportFiles/targetsystem.py
}

function _timestamp()
{
    date -u "+%H:%M:%S.%3N"
}

# Show the user what the test scripts are doing.
# Displays all arguments to stderr, prefixed with '> '.
function announce()
{
    local spaces=${#FUNCNAME[@]}

    [[ ${FUNCNAME[1]} == announce* ]] && (( spaces-- ))

    printf "${XX_CYAN}> ${XX_NORMAL}%s%*s${XX_CYAN}%s${XX_NORMAL}\n" \
        "$(_timestamp)" $spaces '' "$*" >&4
}

# Show the user what the value of a variable is
# @param varname which variable to display
function announceVar()
{
    eval announceCallpointPrefix "\$1=\${$1}"
}

# Display a warning, prefixed with 'WARNING' and an index indicating how many
# warnings have been displayed by this test.
# Warnings are also recorded and will be displayed again at the end of the test.
function warning()
{
    printf '%q\n' "$*" >>"$WARNINGS"
    printf '%s\n' "$*" >>"${LOGDIR}/$(basename "$TEST" .sh).warn"
    local count=$(get_warning_count)

    printf "${XX_CYAN}> ${XX_NORMAL}%s ${XX_YELLOW}WARNING[%s]: %s${XX_NORMAL}\n" \
        "$(_timestamp)" $count "$*" >&4
}

function all_warnings()
{
    if [[ -s $WARNINGS ]]
    then
        local msg
        local -i i=1
        while read msg
        do
            echo >&4 "WARNING[$i]: $msg"
            (( i++ ))
        done <"$WARNINGS"
    fi
}

function _dump_warnings()
{
    local warn_count=$(get_warning_count)

    if (( warn_count > 0 ))
    then
        DIV_CHAR="-" print_header "WARNINGS [$warn_count]"
        all_warnings
        DIV_CHAR="-" print_divider
    fi
}

function get_warning_count()
{
    [[ -e $WARNINGS ]] && wc -l <"$WARNINGS" || echo 0
}

# Show the user what the test scripts are doing.
# Displays the function name prefixed with '> ' and all the arguments within quotes
function announceFunction()
{
    local fn_name=$1
    shift
    if (( $# > 0 ))
    then
        announce "$fn_name() $(printf '%q ' "$@")"
    else
        announce "$fn_name()"
    fi
}

function announceCallpointPrefix()
{
    local STUB_LINENO STUB_FNNAME STUB_FILE
    local CALL_LINENO CALL_FNNAME CALL_FILE

    IFS=" " read STUB_LINENO STUB_FNNAME STUB_FILE < <(caller 0)
    IFS=" " read CALL_LINENO CALL_FNNAME CALL_FILE < <(caller 1)
    local CALL_FILE_REL="${CALL_FILE/#${TEST_ROOT%/}\//./}"
    announce "$CALL_FILE_REL:$CALL_LINENO:INFO: $1"
}

function announceCallpoint()
{
    local STUB_LINENO STUB_FNNAME STUB_FILE
    local CALL_LINENO CALL_FNNAME CALL_FILE

    IFS=" " read STUB_LINENO STUB_FNNAME STUB_FILE < <(caller 0)
    IFS=" " read CALL_LINENO CALL_FNNAME CALL_FILE < <(caller 1)
    local CALL_FILE_REL="${CALL_FILE/#${TEST_ROOT%/}\//./}"
    announce $CALL_FILE_REL:$CALL_LINENO:INFO: "$(_bash_lines $CALL_LINENO $CALL_FILE)"
    if [[ $# == 0 ]]
    then
        announce " expanded to -> $STUB_FNNAME"
    else
        local args
        printf -v args '"%s" ' "$@"
        announce " expanded to -> $STUB_FNNAME ${args:0:255}"
    fi
}

# Returns 0 if the program was found, and 1 otherwise.
# @param PROGRAM    name of the program to check for.
function checkForProgram()
{
    local PROGRAM=$1
    isRunning $PROGRAM >/dev/null 2>&1
    return $?
}

# Check if we have rpm
# @return 0 if we have rpm, 1 if we don't
function _has_rpm()
{
    RPM=$(which rpm 2>/dev/null)
    [ -x "$RPM" ] && return 0
    return 1
}

# Generate a testable eicar file, which is written to a directory which is
# protected by the on-access scanner.  Displays this path to stdout.
function testableEicar()
{
    if [[ -z "$EICARTESTED" ]]
    then
        [[ -n "$SCANDIR" ]] || local SCANDIR="$TMPDIR/"


        if [[ ${TMPDIR%/}/ == ${SCANDIR%/}/* ]]
        then
            # Create the testable EICAR in $TMPDIR if that is protected by on-access
            EICARTESTED="${TMPDIR}/eicar/eicar.com"
        elif [[ ${TESTTMP%/}/ == ${SCANDIR%/}/* ]]
        then
            # Create the testable EICAR in $TESTTMP if that is protected by on-access
            EICARTESTED="${TESTTMP}/eicar.com"
        else
            EICARTESTED="${SCANDIR}/${TEST}_eicar.com"
        fi

        if [[ ! -r "$EICARTESTED" ]]
        then
            createEicar "$EICARTESTED" 1>&2
        fi
    fi

    echo "$EICARTESTED"
}

# Creates a clean file which is not expected to be identified as a virus.
function createCleanfile()
{
    announceFunction $FUNCNAME "$@"

    local location=$1
    echo "Hello world!" > $location
}

# Creates a clean (sparse) file with size specified in bytes
# @param1 file_size
# @param2 file_name
function createCleanFileWithSize()
{
    announceFunction $FUNCNAME "$@"

    local -i file_size=$1
    local file_name="$2"

    if (( $file_size == 0 ))
    then
        touch "$file_name"
    else
        dd if=/dev/zero of="$file_name" bs=1 count=1 seek=$(( file_size - 1 ))
    fi

    local result=$?
    if (( $result != 0 ))
    then
        failure1 "dd could not create $file_name with size $file_size error ($result)"
    fi
}

# Ensure that a specific file is present.  If the file path is relative, and
# therefore part of the regression suite, then the regression suite will be
# terminated if the file does not exist.  Otherwise, the test will be skipped
# if the file does not exist.
# @param FILENAME    path to the file to check for.
function require_file()
{
    announceFunction $FUNCNAME "$@"

    local FILENAME=$1
    [ -n "$FILENAME" ] || skipOnSystemSetup "$FUNCNAME: requires a file argument"

    if [[ $FILENAME == /* ]]
    then
        [ -f "$FILENAME" ] && {
            echo SHA1 "$FILENAME" $(_getSha1sum "$FILENAME")
            return 0
        }
        skipOnSystemSetup "$FILENAME is missing"
    else
        [ -f "$TEST_ROOT/$FILENAME" ] && {
            echo SHA1 "$TEST_ROOT/$FILENAME" $(_getSha1sum "$TEST_ROOT/$FILENAME")
            return 0
        }
        failure2 "${FILENAME#${TEST_ROOT}} is missing from the regression suite"
    fi
}

# Makes a subtle change to a file, by corrupting the last four bytes.
# @param target    file which is to be corrupted
function corruptBinaryFile()
{
    local target=$1

    local tmpcopy=${target}.copy
    cp $target $tmpcopy
    local size=$(fileSize $tmpcopy)
    local count=$(( $size - 4 ))
    dd if=$tmpcopy of=$target bs=1 count=$count
    echo deadbeef | xxd -r -p >>$target
}

# Create a copy of EICAR.
# @param location    location for the EICAR file.
function _createEicar()
{
    local location="${1}"

    mkdir -p "${location%/*}"
    echo "$XX_EICAR" > "${location}"
}

# Create a copy of EICAR in an archive.
# @param location    location for the EICAR file.
function _createArchivedEicar()
{
    local location="${1}"

    mkdir -p "${location%/*}"
    _createEicar "${location}.com" # Make a temporary eicar with it different than the archive
    tar cf "${location}" "${location}.com" # Package it up in a TAR archive
    local RET=$?
    rm -f "${location}.com" # Get rid of temporary eicar
    return ${RET}
}

# Create a copy of EICAR_PUA.
# @param location    location for the EICAR_PUA file.
function _createEicar_PUA()
{
    local location="${1}"
    local PUA=

    PUA='*L'${PUA}
    PUA='*M'${PUA}
    PUA='CT-TEST!$'${PUA}
    PUA='TED-OBJE'${PUA}
    PUA='NWAN'${PUA}
    PUA='TENTIALLY-U'${PUA}
    PUA='CAR-PO'${PUA}
    PUA='Z5[/EI'${PUA}
    PUA='<5N*P'${PUA}
    PUA='D:)D'${PUA}
    PUA='X5]+)'${PUA}

    mkdir -p ${location%/*}
    echo "$PUA" > "${location}"
}

# Creates big file with Eicar at the end, starting at an ofset specified in bytes.
# @param1 file offset to eicar
# @param2 file_name
function _createBigFileWithOffsetEicar()
{
    createCleanFileWithSize "${1}" "${2}"

    # Append the clean file with eicar at the end of the file
    echo "$XX_EICAR" >> "${2}"
}

# Creates a sparse file
# @param location    location of the sparse file to create
# @param size    size of the sparse file to create, in megabytes.
function createSparseFile()
{
    announceFunction $FUNCNAME "$@"

    local location=$1
    local size=$2

    $TEST_ROOT/bin/createSparseFile $location $size
}

# Ensure that the JFS filesystem is available, otherwise the test will be skipped.
function require_jfs()
{
    announceFunction $FUNCNAME "$@"

    if ! have mkfs.jfs
    then
        skipTest "test requires jfs utilities"
    fi
    if ! grep jfs /proc/modules
    then
        if ! modprobe jfs
        then
            skipTest "unable to load the jfs module"
        fi
    fi
}

# Create a loopback device on a file.
# @param file    file to access using a loopback device
function createLoopbackDevice()
{
    announceFunction $FUNCNAME "$@"

    local file=$1
    XX_LOOPBACKDEVICE=$(losetup -f)
    if (( $? != 0 ))
    then
        skipTest "no available loopback devices"
    fi
    losetup $XX_LOOPBACKDEVICE $file

    echo "DEBUG: Creating loopback device '$XX_LOOPBACKDEVICE'" >&2
    export XX_LOOPBACKDEVICE
    registerCleanupFunction _releaseLoopbackDevice

    echo $XX_LOOPBACKDEVICE
}

# Clean-up function which will release the loopback device claimed by the createLoopbackDevice function.
function _releaseLoopbackDevice()
{
    announceFunction $FUNCNAME "($XX_LOOPBACKDEVICE)"

    losetup -d $XX_LOOPBACKDEVICE
}

# Call the 32bit version of 'mount' provided with the regression suite
# @param "$@"    arguments to mount
function mount32bit()
{
    $TEST_ROOT/bin/mount32bit "$@"
}

# Ensure a 64bit OS is running.  This test will be skipped on a 32bit OS
function require_64bit()
{
    announceFunction $FUNCNAME "$@"

    if [ $(uname -m) != "x86_64" ]
    then
        skipTest "test requires a 64bit OS" >&2
    fi
}

# modify a file to make it directly executable
# @param "$@"    files to set executable
function makeExecutable()
{
    local file
    for file in "$@"
    do
        safechmod +x $file || failure2 "Failed to set executable permissions on $file"
    done
}

# Returns POSIX exitcode based on a [[ -e $1 ]] test expression.
# Allows glob expressions to be expanded before the -f test is done.
# @PARAM*    files
function file_exists()
{
    [[ -e $1 ]]
}

#Waits for the command given to be true.
#Returns 0 as soon as the command returns true, or 1 otherwise.
#@PARAM     timeout
#@PARAM*    The command to run
function _waitForFunction()
{
    local -i TIMEOUT=$1; shift

    "$@" && return 0

    (( $TIMEOUT >= $XX_TEST_TIMEOUT )) \
        && warning "Waiting for ${TIMEOUT}s, but test timeout is ${XX_TEST_TIMEOUT}s"

    # choose a suitable sleep interval
    if [[ $SLEEPINTERVAL ]]
    then
        :
    elif (( $TIMEOUT <= 10 ))
    then
        local SLEEPINTERVAL=0.1
    elif (( $TIMEOUT <= 60 ))
    then
        local SLEEPINTERVAL=0.2
    elif (( $TIMEOUT <= 300 ))
    then
        local SLEEPINTERVAL=1
    else
        local SLEEPINTERVAL=5
    fi

    local DEADLINE=$(( $SECONDS + $TIMEOUT ))
    while (( $SECONDS < $DEADLINE ))
    do
        safesleep "$SLEEPINTERVAL"
        "$@" && return 0
    done
    return 1
}

#Waits for the command given to be true.
#Returns 0 as soon as the command returns true, or 1 otherwise.
#@PARAM     timeout
#@PARAM*    The command to run
function _waitFor()
{
    local -i TIMEOUT=$1; shift
    _waitForFunction $TIMEOUT eval "$@"
}

#Waits for the command given to be true.
#Returns 0 as soon as the command returns true, or 1 otherwise.
#@PARAM     timeout
#@PARAM*    The command to run
function waitFor()
{
    announceFunction $FUNCNAME "$@"

    _waitFor "$@"
}

# Waits for the file expression to exist.
# Returns 0 as soon as the file is present, or 1 otherwise.
# @PARAM     timeout
# @PARAM*    the file expression (static or glob)
function waitForFile()
{
    local TIMEOUT=$1 ; shift

    waitFor $TIMEOUT file_exists $1
}

function waitForPolicy()
{
    local POLICY=$1 ; shift
    local FILE="$INST/rms/${POLICY}Policy.xml"
    local TIMEOUT=$1 ; shift
    [[ -n $TIMEOUT ]] || TIMEOUT=299

    waitFor "$TIMEOUT" file_exists "$FILE"
}

# Drop systemwide filesystem caches.
# Returns 0 if function is implemented and was successful
function dropCaches()
{
    if [ "$PLATFORM" == "linux" ]
    then
        if [ -f /proc/sys/vm/drop_caches ]
        then
            if echo 3 >/proc/sys/vm/drop_caches
            then
                sleep 3
                return 0
            fi
        fi
    fi

    return 1
}

# Finds block device from which / is mounted
function get_root_block_device()
{
    case $PLATFORM in
        linux)
            </etc/mtab tr '\t' ' ' | grep " / " | cut -d' ' -f1
            ;;
        sunos)
            </etc/mnttab tr '\t' ' ' | grep " / " | cut -d' ' -f1
            ;;
        freebsd)
            mount -p | tr '\t' ' ' | grep " / " | cut -d' ' -f1
            ;;
        *)
            failure2 "Unsupported platform $PLATFORM!"
            ;;
    esac
}

## Extract SAV4 to $TMPDIR/sav-install
# EXTRAS_SAV4 variable controls location of directory with tar file
function __extract_sav4_tarfile()
{
    local tarname
    if [ "$PLATFORM" = "linux" ]
    then
        tarname="linux.intel.libc6.glibc.2.2.tar"
    elif [ "$PLATFORM" = "sunos" ]
    then
        local cpu=$(uname -p)
        if [ "$cpu" = "i386" ]
        then
            tarname="solaris.intel.tar"
        elif [ "$cpu" = "sparc" ]
        then
            tarname="solaris.sparc.tar"
        else
            failure1 "unsupported cpu"
        fi
    elif [ "$PLATFORM" = "hpux" ]
    then
        local cpu=$(uname -m)
        if [ "$cpu" = "ia64" ]
        then
            tarname="hpux.ia64.tar"
        else
            failure1 "unsupported cpu"
        fi
    elif [ "$PLATFORM" = "freebsd" ]
    then
        local cpu=$(uname -m)
        if [ "$cpu" = "i386" ]
        then
            tarname="freebsd.elf.6+.tar"
        elif [ "$cpu" = "amd64" ]
        then
            tarname="freebsd.amd64.6+.tar"
        else
            failure1 "unsupported cpu"
        fi
    elif [ "$PLATFORM" = "aix" ]
    then
        tarname="aix.tar"
    else
        failure1 "unsupported platform"
    fi

    safermrf $TMPDIR/sav-install

    # EXTRAS_SAV4 is defined in config.sh[.default]
    if [[ ! -f "$EXTRAS_SAV4/$tarname" ]]
    then
        skipTest "SAV4 tarfile not found - please copy to $EXTRAS_SAV4/$tarname"
    fi
    unpack "$EXTRAS_SAV4/$tarname" $TMPDIR

    if [ ! -f $TMPDIR/sav-install/vdl01.vdb ]
    then
        ## Need to add the virus data
        cp_a $DIST/savi/sav/* $TMPDIR/sav-install
    fi
}

# Install SAV4 if it is not already installed
function require_sav4()
{
    announceFunction $FUNCNAME "$@"

    if [[ -f /usr/local/bin/sweep && ! -h /usr/local/bin/sweep ]]
    then
        # Already installed
        return 0
    fi

    __extract_sav4_tarfile

    export PATH=/usr/local/bin:$PATH
    export MANPATH=/usr/local/man:$MANPATH
    $TMPDIR/sav-install/install.sh || failure1 "failed to install sav4"
}

## Uninstall SAV4
function uninstall_sav4()
{
    rm /usr/local/bin/sweep
    rm /usr/local/lib/libsavi*
    rm -r /usr/local/sav
}

# Require Upstart based system (where we use Upstart)
function require_upstart()
{
    (( $USE_UPSTART == 1 )) \
        || skipTest "Not an Upstart based system or product does not use Upstart here."
}

# Require Systemd based system (where we use Systemd)
function require_systemd()
{
    (( $USE_SYSTEMD == 1 )) \
        || skipTest "Not a Systemd based system or product does not use Systemd here."
}



function get_group_id()
{
    grep "^$1:" /etc/group | tail -1 | cut -d':' -f3
}

function get_group_name()
{
     cut -d':' -f1,3 /etc/group | grep ":$1\$" | tail -1 | cut -d':' -f1
}

function get_user_id()
{
    grep "^$1:" /etc/passwd | tail -1 | cut -d':' -f3
}

function get_user_name()
{
     cut -d':' -f1,3 /etc/passwd | grep ":$1\$" | tail -1 | cut -d':' -f1
}

function get_user_gid()
{
    grep "^$1:" /etc/passwd | tail -1 | cut -d':' -f4
}

function get_user_group()
{
    get_group_name $(get_user_gid $1)
}

function get_file_uid()
{
    #~ ls -ln "$1" | tr -s ' ' | cut -d' ' -f3
    set -- $(ls -ln "$1")
    echo $3
}

function get_file_gid()
{
    #~ ls -ln "$1" | tr -s ' ' | cut -d' ' -f4
    set -- $(ls -ln "$1")
    echo $4
}

function get_dir_uid()
{
    #~ ls -ldn "$1" | tr -s ' ' | cut -d' ' -f3
    set -- $(/bin/ls -ldn "$1")
    echo $3
}

function get_dir_gid()
{
    #~ ls -ldn "$1" | tr -s ' ' | cut -d' ' -f4
    set -- $(/bin/ls -ldn "$1")
    echo $4
}

function verify_dir_ownership()
{
    #announceFunction $FUNCNAME "$@"

    local LUID=${1:-*}
    local LGID=${2:-*}
    local FILE="$3"
    [[ $FILE = /* ]] || FILE="$INST/$FILE"

    local FUID=$(get_dir_uid $FILE)
    [[ -n "$FUID" ]] || testFailure "Failed to get $FILE owner UID"

    local FGID=$(get_dir_gid $FILE)
    [[ -n "$FGID" ]] || testFailure "Failed to get $FILE group GID"

    (( $FUID >= 0 && $FGID >= 0 )) \
        || testFailure "Failed to get $FILE owner ($FUID) and group ($FGID)"
    [[ $FUID == $LUID ]] \
        || testFailure "$FILE: Owner ($FUID) does not match, should be $LUID"
    [[ $FGID == $LGID ]] \
        || testFailure "$FILE: Group ($FGID) does not match, should be $LGID"
}

function verify_file_ownership()
{
    verify_dir_ownership "$@"
}

function verify_file_not_world_writable()
{
    #announceFunction $FUNCNAME "$@"

    local FILE=$1
    [[ -z $(find "$FILE" -type f -perm -0002) ]] \
        || testFailure "World-writable file $FILE"
}

function verify_dir_not_world_writable()
{
    #announceFunction $FUNCNAME "$@"

    local DIR=$1
    [[ -z $(find "$DIR" -type d -prune -perm -0002) ]] \
        || testFailure "World-writable directory $DIR"
}

function verify_tar_file_contents_permissions()
{
    local TARFILE=$1
    local COMPRESS_OPT=""

    case $TARFILE in
        *.tar.gz|*.tgz)
            COMPRESS_OPT="z"
            ;;
        *.tar.bz2)
            COMPRESS_OPT="j"
            ;;
        *.tar)
            COMPRESS_OPT=""
            ;;
        *)
            setupFailure "Trying to verify unknown archive $TARFILE"
            ;;
    esac

    tar tv${COMPRESS_OPT}f $TARFILE | grep "^[^l]...\(.w....\|....w.\)" \
        && testFailure "World-writable files in $TARFILE"
}

function verify_rpm_contents_permissions()
{
    local RPM=$1

    rpm -qlvp $RPM | grep "^[^l]...\(.w....\|....w.\)" \
        && testFailure "World-writable files in $RPM"
}

# Get number of CPU cores
function get_cpu_cores()
{
    if (( $CPU_CORES <= 0 ))
    then
        testFailure "CPU count not known on this machine!"
    fi

    echo $CPU_CORES
}

# Enter and announce a subtest
function announceSubtest()
{
    SUBTEST="$*"
    XX_SUBTESTSTART=$(unixTime)
    print_divider '-'
    announce "SUBTEST: $SUBTEST"
}

# Check kernel version greater than params
# @param CHECK_VERSION - Kernel version to check for (e.g. 2.6.32)
function skipIfKernelVersionLessThanOrEqualTo()
{
    local CHECK_VERSION=$1

    local KERNEL_VERSION=$(uname -r)
    KERNEL_VERSION=${KERNEL_VERSION%%-*}

    versionLessThanOrEqual "$KERNEL_VERSION" "$CHECK_VERSION" \
        && skipTest "Kernel version ${KERNEL_VERSION} is less than or equal to ${CHECK_VERSION}"
}

# print a dividing line
# @param len(optional) length of the line (default: 80 chars)
# @param character to print (default: '=')
function print_divider()
{
    local char=${1:-${DIV_CHAR:-=}}
    local chars=${2:-80}
    local RULE="$(printf "%*s" $chars '')"
    echo "${RULE// /${char}}"
}

function print_header()
{
    local msg="[ $* ]"

    if (( ${#msg} <= 70 ))
    then
        local len=$(( ( 80 - ${#msg} ) / 2 ))
        local len2=$(( 80 - ${#msg} - $len ))
    else
        local len=5
        local len2=5
    fi

    echo "$(print_divider "" $len)$msg$(print_divider "" $len2)"
}

function with_header()
{
    print_header "$@"
    "$@"
    print_divider
}

# A wrapper function to hash, but without any of the output.
function have()
{
    hash 1>/dev/null 2>&1 "$@"
}

# returns non-zero if the symlink is an absolute path or contains at
# least one '..' element
function isSafeLink()
{
    local link=$(readlink $1)
    if [[ "$link" != "${link#/}" ]]
    then
        announce "link $link is an absolute path"
        return 1
    fi

    if [[ "$link" != "${link/../}" ]]
    then
        announce "link $link contains at least one '..' element"
        return 1
    fi

    return 0
}

# For interactive development/debugging only.
function pauseTest()
{
    announce "*** TEST PAUSED; press ctrl-d to continue ***"
    cat
}
