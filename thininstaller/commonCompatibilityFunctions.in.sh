BUILD_LIBC_VERSION=@BUILD_SYSTEM_LIBC_VERSION@

SUS_URL="https://sus.sophosupd.com"
SDDS3_COM_URL="https://sdds3.sophosupd.com:443"
SDDS3_NET_URL="https://sdds3.sophosupd.net:443"
DAT_FILES=("supplement/sdds3.ScheduledQueryPack.dat"
    "supplement/sdds3.ML_MODEL3_LINUX_X86_64.dat"
    "supplement/sdds3.DataSetA.dat"
    "supplement/sdds3.LocalRepData.dat"
    "supplement/sdds3.RuntimeDetectionRules.dat"
    "supplement/sdds3.SSPLFLAGS.dat")

VALID_CENTRAL_CONNECTION=0
VALID_SUS_CONNECTION=0
VALID_CDN_CONNECTION=0

# If TMPDIR has not been set then default it to /tmp
if [[ -z "${TMPDIR}" ]]; then
    TMPDIR=/tmp
    export TMPDIR
fi

function log_info() {
    echo "INFO: ${1}"
}

function log_debug() {
    [[ -n "${DEBUG_THIN_INSTALLER}" ]] && echo "DEBUG: ${1}"
}

function log_warn() {
    COMPATIBILITY_WARNING_FOUND=1
    echo "WARN: ${1}"
}

function log_error() {
    COMPATIBILITY_ERROR_FOUND=1
    echo "ERROR: ${1}"
}

function debug_curl_output() {
    local url=$1
    local proxy=$2

    if [[ -n "${DEBUG_THIN_INSTALLER}" ]]; then
        echo "DEBUG: See curl output for more detail:"
        if [[ -z ${proxy} ]]; then
            echo "$(curl -k -v ${url} -m 60)"
        else
            echo "$(curl --proxy ${proxy} -k -v ${url} -m 60)"
        fi
    fi
}

function check_install_path_has_correct_permissions() {
    # loop through given install path from right to left to find the first directory that exists.
    local install_path="${SOPHOS_INSTALL%/*}"

    # Make sure that the install_path string is not empty, in the case of "/foo"
    if [[ -z "$install_path" ]]; then
        install_path="/"
    fi

    while [[ ! -d "${install_path}" ]]; do
        install_path="${install_path%/*}"

        # Make sure that the install_path string is not empty.
        if [[ -z "$install_path" ]]; then
            install_path="/"
        fi
    done

    # continue looping through all the existing directories ensuring that the current permissions will allow sophos-spl to execute

    while [[ "${install_path}" != "/" ]]; do
        permissions=$(stat -c '%A' "${install_path}")
        if [[ ${permissions: -1} != "x" ]]; then
            failure ${EXITCODE_BAD_INSTALL_PATH} "ERROR: SPL installation will fail, can not install to ${SOPHOS_INSTALL} because ${install_path} does not have correct execute permissions. Requires execute rights for all users"
        fi

        install_path="${install_path%/*}"

        if [[ -z "$install_path" ]]; then
            install_path="/"
        fi
    done
}

function verify_install_directory() {
    # No custom install location so use default or special case if SOPHOS_INSTALL is set to the default, then we just use it and don't append
    if [[ (-z "${SOPHOS_INSTALL}") || ("${SOPHOS_INSTALL}" == "/opt/sophos-spl") ]]; then
        export SOPHOS_INSTALL=/opt/sophos-spl
        return
    fi

    # Remove trailing /
    SOPHOS_INSTALL="${SOPHOS_INSTALL%/}"

    # Sophos install must be in an ASCII path - no spaces
    local LEFT_OVER=$(echo ${SOPHOS_INSTALL} | tr -d '[:alnum:]_/.-')

    if [[ -n "${LEFT_OVER}" ]]; then
        local SPACES=$(echo ${SOPHOS_INSTALL} | sed -e's/[^ ]//g')
        if [[ -n "${SPACES}" ]]; then
            failure ${EXITCODE_BAD_INSTALL_PATH} "ERROR: SPL installation will fail, can not install to '${SOPHOS_INSTALL}' because it contains spaces."
        else
            failure ${EXITCODE_BAD_INSTALL_PATH} "ERROR: SPL installation will fail, can not install to '${SOPHOS_INSTALL}' because it contains non-alphanumeric characters: ${LEFT_OVER}"
        fi
    fi

    realPath=$(realpath -m "${SOPHOS_INSTALL}")
    [[ "${SOPHOS_INSTALL}" = /* ]] || failure ${EXITCODE_BAD_INSTALL_PATH} "ERROR: SPL installation will fail, can not install to '${SOPHOS_INSTALL}' because it is a relative path. To install under this directory, please re-run with --install-dir=${realPath}"
    [[ "${realPath}" != "${SOPHOS_INSTALL}" ]] && failure ${EXITCODE_BAD_INSTALL_PATH} "ERROR: SPL installation will fail, can not install to '${SOPHOS_INSTALL}'. To install under this directory, please re-run with --install-dir=${realPath}"

    # SOPHOS_INSTALL is custom dirname to contain sophos-spl
    export SOPHOS_INSTALL="$SOPHOS_INSTALL/sophos-spl"

    # Check that the install path has valid permission on the existing directories
    check_install_path_has_correct_permissions
}

function verify_compatible_glibc_version() {
    system_libc_version=$(ldd --version | grep 'ldd (.*)' | rev | cut -d ' ' -f 1 | rev)
    lowest_version=$(printf '%s\n' ${BUILD_LIBC_VERSION} "${system_libc_version}" | sort -V | head -n 1) || failure ${EXIT_FAIL_COULD_NOT_FIND_LIBC_VERSION} "ERROR: SPL installation will fail, could not determine libc version"
    if [[ "${lowest_version}" != "${BUILD_LIBC_VERSION}" ]]; then
        failure ${EXIT_FAIL_WRONG_LIBC_VERSION} "ERROR: SPL installation will fail, can not install on unsupported system. Detected GLIBC version ${system_libc_version} < required ${BUILD_LIBC_VERSION}"
    fi
}

function verify_installed_packages() {
    required_packages=("bash" "systemctl" "grep" "getent" "groupadd" "useradd" "usermod")
    for package in "${required_packages[@]}"; do
        [[ -z $(which "${package}") ]] && failure ${EXITCODE_MISSING_PACKAGE} "ERROR: SPL installation will fail as it requires ${package} to be installed on this server"
    done

    av_packages=("setcap")
    for package in "${av_packages[@]}"; do
        [[ -z $(which "${package}") ]] && log_warn "AV Plugin requires ${package} to be installed on this server. SPL can be installed but AV will not work"
    done
}

function check_free_storage() {
    local space="$1"
    local install_path="${SOPHOS_INSTALL%/*}"

    # Loop through directory path from right to left, finding the first part of the path that exists.
    # Then we will use the df command on that path.  df command will fail if used on a path that does not exist.
    while [[ ! -d "${install_path}" ]]; do
        install_path="${install_path%/*}"

        # Make sure that the install_path string is not empty.
        if [[ -z "$install_path" ]]; then
            install_path="/"
        fi
    done

    local free=$(df -kP "${install_path}" | sed -e "1d" | awk '{print $4}')
    local mountpoint=$(df -kP "${install_path}" | sed -e "1d" | awk '{print $6}')

    local free_mb
    free_mb=$((free / 1024))

    if [[ ${free_mb} -gt ${space} ]]; then
        return 0
    fi
    failure ${EXITCODE_NOT_ENOUGH_SPACE} "ERROR: SPL installation will fail as there is not enough space in ${mountpoint} to install SPL. You need at least ${space}MiB to install SPL"
}

function check_total_mem() {
    local neededMemKiloBytes=$1
    local totalMemKiloBytes=$(grep MemTotal /proc/meminfo | awk '{print $2}')

    if [ "${totalMemKiloBytes}" -gt "${neededMemKiloBytes}" ]; then
        return 0
    fi
    failure ${EXITCODE_NOT_ENOUGH_MEM} "ERROR: SPL installation will fail as this machine does not meet product requirements (total RAM: ${totalMemKiloBytes}kB). The product requires at least ${neededMemKiloBytes}kB of RAM"
}

function check_ca_certs() {
    if [[ -f "/etc/ssl/certs/ca-certificates.crt" ]]; then
        log_debug "Installation will use system CA path '/etc/ssl/certs/ca-certificates.crt'"
    elif [[ -f "/etc/pki/tls/certs/ca-bundle.crt" ]]; then
        log_debug "Installation will use system CA path '/etc/pki/tls/certs/ca-bundle.crt'"
    elif [[ -f "/etc/ssl/ca-bundle.pem" ]]; then
        log_debug "Installation will use system CA path '/etc/ssl/ca-bundle.pem'"
    else
        failure ${EXITCODE_INVALID_CA_PATHS} "ERROR: SPL installation will fail as the system CA path could not be found. SPL uses either '/etc/ssl/certs/ca-certificates.crt', '/etc/pki/tls/certs/ca-bundle.crt' or '/etc/ssl/ca-bundle.pem'"
    fi
}

function verify_system_requirements() {
    # Check that the OS is Linux
    [[ $(uname -a) =~ "Linux" ]] || failure ${EXITCODE_NOT_LINUX} "ERROR: SPL installation will fail as it can only be installed on Linux"

    # Check machine architecture
    if [[ $(uname -m) = "x86_64" ]]; then
        BIN="bin"
    else
        failure ${EXITCODE_NOT_64_BIT} "ERROR: SPL installation will fail as it can only be installed on a x86_64 system"
    fi

    # Check for systemd
    [[ $(ps -p 1 -o comm=) == "systemd" ]] || failure ${EXITCODE_NO_SYSTEMD} "ERROR: SPL installation will fail as it requires systemd to run"

    # Check required packages are installed
    verify_installed_packages

    # Check kernel supports GLIBC version 2.17 or later
    verify_compatible_glibc_version

    # Check /tmp is not mounted to a partition with noexec
    [[ $(mount | grep "$(df -P "${TMPDIR}" | tail -1 | cut -d' ' -f 1)") =~ "noexec" ]] && failure ${EXITCODE_NOEXEC_TMP} "ERROR: SPL installation will fail as ${TMPDIR} is mounted to a partition with noexec"

    # Check server has capabilities
    [[ (-z $(which getcap)) || (-z $(which setcap)) ]] && log_warn "SPL requires capabilities for the AV and RTD plugins to be installed. The base installation will succeed but these plugins will not be operational"

    # Check Fanotify is enabled in kernel
    fanotify_config=$(cat /boot/config-"$(uname -r)" 2>/dev/null | grep FANOTIFY 2>/dev/null)
    [[ ("${fanotify_config}" =~ "CONFIG_FANOTIFY=y") && ("${fanotify_config}" =~ "CONFIG_FANOTIFY_ACCESS_PERMISSIONS=y") ]] || log_warn "Fanotify is not enabled by this kernel so the AV plugin will not be able to scan files on access and, if necessary, block access to threats. SPL can be installed but OnAccess scanning will not be operational, see https://support.sophos.com/support/s/article/KB-000034610 for more details"

    # Check there is enough disk space
    check_free_storage 2048

    # Check there is enough RAM (930000kB)
    check_total_mem 930000

    # Check System CA certificate locations
    check_ca_certs
}

function verify_connection_to_central() {
    local proxy=$1

    if [[ -z ${proxy} ]]; then
        if [[ $(curl --noproxy '*' -k -is "${CENTRAL_URL}" -m 60 | head -n 1) =~ "200" ]]; then
            VALID_CENTRAL_CONNECTION=1
            log_info "Server can connect to Sophos Central directly"
        else
            log_warn "Server cannot connect to Sophos Central directly"
            debug_curl_output "${CENTRAL_URL}"
        fi
    else
        if [[ $(curl --proxy "${proxy}" -k -is "${CENTRAL_URL}" -m 60 | head -n 1) =~ "200" ]]; then
            VALID_CENTRAL_CONNECTION=1
            log_info "Server can connect to Sophos Central via ${proxy}"
        else
            log_warn "SPL installation will not be performed via ${proxy}. Server cannot connect to Sophos Central via ${proxy}"
            debug_curl_output "${CENTRAL_URL}" "${proxy}"
        fi
    fi
}

function verify_connection_to_sus() {
    local proxy=$1

    if [[ -z ${proxy} ]]; then
        if [[ $(curl --noproxy '*' -is "${SUS_URL}" -m 60 | head -n 1) =~ "200" ]]; then
            VALID_SUS_CONNECTION=1
            log_info "Server can connect to the SUS server (${SUS_URL}) directly"
        else
            log_warn "Server cannot connect to the SUS server (${SUS_URL}) directly"
            debug_curl_output "${SUS_URL}"
        fi
    else
        if [[ $(curl --proxy "${proxy}" -is "${SUS_URL}" -m 60 | head -n 1) =~ "200" ]]; then
            VALID_SUS_CONNECTION=1
            log_info "Server can connect to the SUS server (${SUS_URL}) via ${proxy}"
        else
            log_warn "SPL installation will not be performed via ${proxy}. Server cannot connect to the SUS server (${SUS_URL}) via ${proxy}"
            debug_curl_output "${SUS_URL}" "${proxy}"
        fi
    fi
}

function verify_connection_to_cdn() {
    local cdn_url=$1
    local proxy=$2

    if [[ -z ${proxy} ]]; then
        successful_downloads=0
        for supplement in "${DAT_FILES[@]}"; do
            if wget --no-proxy -q --delete-after "${cdn_url}/${supplement}" -T 12 -t 1; then
                successful_downloads=$((successful_downloads + 1))
                log_debug "Connection verified to CDN server, server is able to download ${supplement#*/} supplement from ${cdn_url} directly"
            else
                log_warn "Failed to get supplement from CDN server (${cdn_url}) directly"
            fi
        done

        if [[ "${successful_downloads}" -eq "${#DAT_FILES[@]}" ]]; then
            VALID_CDN_CONNECTION=1
            log_info "Connection verified to CDN server, server was able to download all SPL packages directly"
        fi
    else
        successful_downloads=0
        for supplement in "${DAT_FILES[@]}"; do
            if wget -q --delete-after -e use_proxy=yes -e https_proxy="${proxy}" "${cdn_url}/${supplement}" -T 12 -t 1; then
                successful_downloads=$((successful_downloads + 1))
                log_debug "Connection verified to CDN server via proxy (${proxy}). Server is able to download ${supplement#*/} supplement from ${cdn_url} via ${proxy}"
            else
                log_warn "SPL installation will not be performed via proxy (${proxy}). Failed to download ${supplement#*/} supplement from CDN server (${cdn_url}) via ${proxy}"
            fi
        done
        if [[ "${successful_downloads}" -eq "${#DAT_FILES[@]}" ]]; then
            VALID_CDN_CONNECTION=1
            log_info "Connection verified to CDN server, server was able to download all SPL packages via ${proxy}"
        fi
    fi
}

function verify_network_connections() {
    IFS=' ' read -ra MESSAGE_RELAYS <<<"${1}"
    IFS=' ' read -ra UPDATE_CACHES <<<"${2}"

    [[ ${#UPDATE_CACHES[@]} != 0 ]] && log_info "Verifying connections to configured Update Caches"
    for update_cache in "${UPDATE_CACHES[@]}"; do
        if curl -k -is "https://${update_cache}/v3" -m 60 >/dev/null; then
            VALID_UPDATE_CACHE_CONNECTION=1
            log_info "Server can connect to configured Update Cache (${update_cache})"
        else
            log_warn "Server cannot connect to configured Update Cache (${update_cache})"
            debug_curl_output "${update_cache}"
        fi
    done

    if [[ -n "${https_proxy}" ]]; then
        log_debug "Network connections will be tested with the configured https_proxy: ${https_proxy}"
        PROXY="${https_proxy}"
    elif [[ -n "${HTTPS_PROXY}" ]]; then
        log_debug "Network connections will be tested with the configured HTTPS_PROXY: ${HTTPS_PROXY}"
        PROXY="${HTTPS_PROXY}"
    elif [[ -n ${http_proxy} ]]; then
        log_debug "Network connections will be tested with the configured http_proxy: ${http_proxy}"
        PROXY="${http_proxy}"
    fi

    # Check connection to Central
    log_info "Verifying connections to Sophos Central"
    verify_connection_to_central
    [[ -n "${PROXY}" ]] && verify_connection_to_central "${PROXY}"
    for message_relay in "${MESSAGE_RELAYS[@]}"; do
        verify_connection_to_central "${message_relay}"
    done
    [[ "${VALID_CENTRAL_CONNECTION}" == 0 ]] && log_error "SPL installation will fail as a connection to Sophos Central could not be established"

    # Check connection to SUS
    log_info "Verifying connections to Sophos Update Service (SUS) server"
    verify_connection_to_sus
    [[ -n "${PROXY}" ]] && verify_connection_to_sus "${PROXY}"
    for message_relay in "${MESSAGE_RELAYS[@]}"; do
        verify_connection_to_sus "${message_relay}"
    done
    [[ "${VALID_SUS_CONNECTION}" == 0 ]] && log_error "SPL installation will fail as a connection to the SUS server could not be established"

    # Check connection to CDN
    log_info "Verifying connections to the CDN server"
    CDN_URL=""
    if [[ $(curl --noproxy '*' -is ${SDDS3_COM_URL} -m 60 | head -n 1) =~ "404" ]]; then
        log_info "Server can connect to .com CDN address (${SDDS3_COM_URL}) directly"
        CDN_URL="${SDDS3_COM_URL}"
    elif [[ $(curl --noproxy '*' -is ${SDDS3_NET_URL} -m 60 | head -n 1) =~ "404" ]]; then
        log_info "Server can connect to .net CDN address (${SDDS3_NET_URL}) directly"
        CDN_URL="${SDDS3_NET_URL}"
    else
        if [[ -z "${PROXY}" ]]; then
            if [[ "${VALID_UPDATE_CACHE_CONNECTION}" == 1 ]]; then
                log_warn "A direct connection to a CDN server could not be established"
            else
                log_error "SPL installation will fail as a connection to a CDN server could not be established"
            fi
        else
            if [[ $(curl --proxy "${PROXY}" -is ${SDDS3_COM_URL} -m 60 | head -n 1) =~ "404" ]]; then
                log_debug "Server can connect to .com CDN address (${SDDS3_COM_URL}) via ${PROXY}"
                CDN_URL="${SDDS3_COM_URL}"
            elif [[ $(curl --proxy "${PROXY}" -is ${SDDS3_NET_URL} -m 60 | head -n 1) =~ "404" ]]; then
                log_debug "Server can connect to .net CDN address (${SDDS3_NET_URL}) via ${PROXY}"
                CDN_URL="${SDDS3_NET_URL}"
            else
                log_warn "Server cannot connect to either the .com or .net CDN address via ${PROXY}"
            fi
            [[ -z "${CDN_URL}" ]] && log_error "SPL installation will fail as the server cannot connect to either the .com or .net CDN server directly or via a proxy"
        fi
    fi

    if [[ -n "${CDN_URL}" ]]; then
        verify_connection_to_cdn "${CDN_URL}"
        [[ -n "${PROXY}" ]] && verify_connection_to_cdn "${CDN_URL}" "${PROXY}"
    fi
    [[ "${VALID_CDN_CONNECTION}" == 0 && "${VALID_UPDATE_CACHE_CONNECTION}" == 0 ]] && log_error "SPL installation will fail as the server is not able to download packages from the CDN server"
}
