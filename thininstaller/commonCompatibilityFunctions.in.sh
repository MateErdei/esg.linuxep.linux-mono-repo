BUILD_LIBC_VERSION=@BUILD_SYSTEM_LIBC_VERSION@
system_libc_version=$(ldd --version | grep 'ldd (.*)' | rev | cut -d ' ' -f 1 | rev)

SUS_URL="https://sus.sophosupd.com"
SDDS3_COM_URL="https://sdds3.sophosupd.com:443"
SDDS3_NET_URL="https://sdds3.sophosupd.net:443"
DAT_FILES=("supplement/sdds3.ScheduledQueryPack.dat"
  "supplement/sdds3.ML_MODEL3_LINUX_X86_64.dat"
  "supplement/sdds3.DataSetA.dat"
  "supplement/sdds3.LocalRepData.dat"
  "supplement/sdds3.RuntimeDetectionRules.dat"
  "supplement/sdds3.SSPLFLAGS.dat")

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
      failure ${EXITCODE_BAD_INSTALL_PATH} "ERROR: Can not install to '${SOPHOS_INSTALL}' because it contains spaces."
    else
      failure ${EXITCODE_BAD_INSTALL_PATH} "ERROR: Can not install to '${SOPHOS_INSTALL}' because it contains non-alphanumeric characters: $LEFT_OVER"
    fi
  fi

  realPath=$(realpath -m "${SOPHOS_INSTALL}")
  [[ "${SOPHOS_INSTALL}" = /* ]] || failure ${EXITCODE_BAD_INSTALL_PATH} "ERROR: Can not install to '${SOPHOS_INSTALL}' because it is a relative path. To install under this directory, please re-run with --install-dir=${realPath}"
  [[ "${realPath}" != "${SOPHOS_INSTALL}" ]] && failure ${EXITCODE_BAD_INSTALL_PATH} "ERROR: Can not install to '${SOPHOS_INSTALL}'. To install under this directory, please re-run with --install-dir=${realPath}"
  SOPHOS_INSTALL="${SOPHOS_INSTALL}/sophos-spl"
}

function verify_compatible_glibc_version() {
  lowest_version=$(printf '%s\n' ${BUILD_LIBC_VERSION} "${system_libc_version}" | sort -V | head -n 1) || failure ${EXIT_FAIL_COULD_NOT_FIND_LIBC_VERSION} "Couldn't determine libc version"
  if [[ "${lowest_version}" != "${BUILD_LIBC_VERSION}" ]]; then
    failure ${EXIT_FAIL_WRONG_LIBC_VERSION} "ERROR: Can not install on unsupported system. Detected GLIBC version ${system_libc_version} < required ${BUILD_LIBC_VERSION}"
  fi
}

function verify_installed_packages() {
  required_packages=("bash" "grep" "getent" "groupadd" "useradd" "usermod")
  for package in "${required_packages[@]}"; do
    [[ -z $(which "${package}") ]] && failure ${EXITCODE_MISSING_PACKAGE} "ERROR: SPL requires ${package} to be installed on this endpoint"
  done

  av_packages=("setcap")
  for package in "${av_packages[@]}"; do
    [[ -z $(which "${package}") ]] && echo "WARN: AV Plugin requires ${package} to be installed on this endpoint. SPL can be installed but AV will not work"
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
  failure ${EXITCODE_NOT_ENOUGH_SPACE} "ERROR: Not enough space in ${mountpoint} to install SPL. You need at least ${space}mB to install SPL"
}

function check_total_mem() {
  local neededMemKiloBytes=$1
  local totalMemKiloBytes=$(grep MemTotal /proc/meminfo | awk '{print $2}')

  if [ "${totalMemKiloBytes}" -gt "${neededMemKiloBytes}" ]; then
    return 0
  fi
  failure ${EXITCODE_NOT_ENOUGH_MEM} "ERROR: This machine does not meet product requirements (total RAM: ${totalMemKiloBytes}kB). The product requires at least ${neededMemKiloBytes}kB of RAM"
}

function check_ca_certs() {
  if [[ -f "/etc/ssl/certs/ca-certificates.crt" ]]; then
    [[ -n "${DEBUG_THIN_INSTALLER}" ]] && echo "DEBUG: Installation will use system CA path '/etc/ssl/certs/ca-certificates.crt'"
  elif [[ -f "/etc/pki/tls/certs/ca-bundle.crt" ]]; then
    [[ -n "${DEBUG_THIN_INSTALLER}" ]] && echo "DEBUG: Installation will use system CA path '/etc/pki/tls/certs/ca-bundle.crt'"
  elif [[ -f "/etc/ssl/ca-bundle.pem" ]]; then
    [[ -n "${DEBUG_THIN_INSTALLER}" ]] && echo "DEBUG: Installation will use system CA path '/etc/ssl/ca-bundle.pem'"
  else
    failure ${EXITCODE_INVALID_CA_PATHS} "ERROR: Could not find system CA path. SPL uses either '/etc/ssl/certs/ca-certificates.crt', '/etc/pki/tls/certs/ca-bundle.crt' or '/etc/ssl/ca-bundle.pem'"
  fi
}

function verify_system_requirements() {
  # Check that the OS is Linux
  [[ $(uname -a | grep -i Linux >/dev/null) -eq 1 ]] && failure ${EXITCODE_NOT_LINUX} "ERROR: SPL can only be installed on Linux"

  # Check machine architecture
  if [[ $(uname -m) = "x86_64" ]]; then
    BIN="installer/bin"
  else
    failure ${EXITCODE_NOT_64_BIT} "ERROR: SPL can only be installed on a x86_64 system"
  fi

  # Check for systemd
  [[ $(ps -p 1 -o comm=) == "systemd" ]] || failure ${EXITCODE_NO_SYSTEMD} "ERROR: SPL requires systemd to run"

  # Check kernel supports GLIBC version 2.17 or later
  verify_compatible_glibc_version

  # Check /tmp is not mounted to a partition with noexec
  [[ $(mount | grep "$(df -P /tmp | tail -1 | cut -d' ' -f 1)") =~ "noexec" ]] && failure ${EXITCODE_NOEXEC_TMP} "ERROR: SPL cannot be installed if /tmp is mounted to a partition with noexec"

  # Check required packages are installed
  verify_installed_packages
  [[ (-z $(which getcap)) || (-z $(which setcap)) ]] && echo "WARN: SPL requires capabilities for the AV and RTD plugins to be installed. The base installation will succeed but these plugins will not be operational"

  # Check Fanotify is enabled in kernel
  fanotify_config=$(cat /boot/config-"$(uname -r)" 2>/dev/null)
  [[ ("${fanotify_config}" =~ "CONFIG_FANOTIFY=y") && ("${fanotify_config}" =~ "CONFIG_FANOTIFY_ACCESS_PERMISSIONS=y") ]] || echo "WARN: Fanotify is not enabled by this kernel so the AV plugin will not be able to scan files on access and, if necessary, block access to threats. SPL can be installed but OnAccess scanning will not be operational, see https://support.sophos.com/support/s/article/KB-000034610 for more details"

  # Check there is enough disk space
  check_free_storage 2048

  # Check there is enough RAM (930000kB)
  check_total_mem 930000

  # Check System CA certificate locations
  check_ca_certs
}

function debug_curl_output() {
  local url=$1
  local proxy=$2

  echo "DEBUG: See curl output for more detail:"
  if [[ -z ${proxy} ]]; then
    echo "$(curl -k -v ${url})"
  else
    echo "$(curl --proxy ${proxy} -k -v ${url})"
  fi
}

function verify_connection_to_central() {
  local central_url=$1
  local proxy=$2

  if [[ -z ${proxy} ]]; then
    if [[ $(curl -k -is "${CENTRAL_URL}" | head -n 1) =~ "200" ]]; then
      echo "INFO: Endpoint can connect to Central directly"
    else
      echo "WARN: Endpoint cannot connect to Central directly"
      [[ ${MESSAGE_RELAYS_CONFIGURED} == 0 ]] && echo "ERROR: SPL installation will fail as the endpoint cannot connect to Central and no proxies are configured"
      [[ -n "${DEBUG_THIN_INSTALLER}" ]] && debug_curl_output "${central_url}"
    fi
  else
    if [[ $(curl --proxy "${proxy}" -k -is "${CENTRAL_URL}" | head -n 1) =~ "200" ]]; then
      echo "INFO: Endpoint can connect to Central via ${proxy}"
    else
      echo "WARN: SPL installation will not be performed via ${proxy}. Endpoint cannot connect to Central via ${proxy}"
      [[ -n "${DEBUG_THIN_INSTALLER}" ]] && debug_curl_output "${central_url}" "${proxy}"
    fi
  fi
}

function verify_connection_to_sus() {
  local sus_url=$1
  local proxy=$2

  if [[ -z ${proxy} ]]; then
    if [[ $(curl -is "${sus_url}" | head -n 1) =~ "200" ]]; then
      echo "INFO: Endpoint can connect to the SUS server (${sus_url}) directly"
    else
      echo "WARN: Endpoint cannot connect to the SUS server (${sus_url}) directly"
      [[ ${MESSAGE_RELAYS_CONFIGURED} == 0 ]] && echo "ERROR: SPL installation will fail as the endpoint cannot connect to the SUS server (${sus_url}) directly and no proxies are configured"
      [[ -n "${DEBUG_THIN_INSTALLER}" ]] && debug_curl_output "${sus_url}"
    fi
  else
    if [[ $(curl --proxy "${proxy}" -is "${sus_url}" | head -n 1) =~ "200" ]]; then
      echo "INFO: Endpoint can connect to the SUS server (${sus_url}) via ${proxy}"
    else
      echo "WARN: SPL installation will not be performed via ${proxy}. Endpoint cannot connect to the SUS server (${sus_url}) via ${proxy}"
      [[ -n "${DEBUG_THIN_INSTALLER}" ]] && debug_curl_output "${sus_url}" "${proxy}"
    fi
  fi
}

function verify_connection_to_cdn() {
  local cdn_url=$1
  local proxy=$2

  for supplement in "${DAT_FILES[@]}"; do
    if [[ -z ${proxy} ]]; then
      if wget -q --delete-after "${cdn_url}/${supplement}"; then
        echo "INFO: Connection verified to CDN server, endpoint is able to download ${supplement#*/} supplement from ${cdn_url} directly"
      else
        echo "WARN: Failed to get supplement from CDN server (${cdn_url}) directly"
        [[ ${MESSAGE_RELAYS_CONFIGURED} == 0 ]] && echo "ERROR: SPL installation will fail as the endpoint is not able to download packages from the CDN server (${cdn_url}) directly and no proxies are configured"
      fi
    else
      if wget -q --delete-after -e use_proxy=yes -e https_proxy="${proxy}" "${cdn_url}/${supplement}"; then
        echo "INFO: Connection verified to CDN server via ${proxy}. Endpoint is able to download ${supplement#*/} supplement from ${cdn_url} via ${proxy}"
      else
        echo "WARN: SPL installation will not be performed via ${proxy}. Failed to get supplement from CDN server (${cdn_url}) via ${proxy}"
      fi
    fi
  done
}

function verify_network_connections() {
  IFS=' ' read -ra MESSAGE_RELAYS <<<"${1}"
  IFS=' ' read -ra UPDATE_CACHES <<<"${2}"
  [[ ${#MESSAGE_RELAYS[@]} != 0 ]] && MESSAGE_RELAYS_CONFIGURED=1

  # Check connection to Central
  echo -e "\nINFO: Verifying connections to Central"
  verify_connection_to_central "${CENTRAL_URL}"
  for message_relay in "${MESSAGE_RELAYS[@]}"; do
    verify_connection_to_central "${CENTRAL_URL}" "${message_relay}"
  done

  # Check connection to SUS
  echo -e "\nINFO: Verifying connections to SUS"
  verify_connection_to_sus "${SUS_URL}"
  for message_relay in "${MESSAGE_RELAYS[@]}"; do
    verify_connection_to_sus "${SUS_URL}" "${message_relay}"
  done

  # Check connection to CDN
  echo -e "\nINFO: Verifying connections to the CDN server"
  CDN_URL=""
  if [[ $(curl -is ${SDDS3_COM_URL} | head -n 1) =~ "404" ]]; then
    echo "INFO: Endpoint can connect to .com CDN address (${SDDS3_COM_URL})"
    CDN_URL="${SDDS3_COM_URL}"
  elif [[ $(curl -is ${SDDS3_NET_URL} | head -n 1) =~ "404" ]]; then
    echo "INFO: Endpoint can connect to .net CDN address (${SDDS3_NET_URL})"
    CDN_URL="${SDDS3_NET_URL}"
  else
    echo "WARN: SPL installation will fail: Endpoint cannot connect to either the .com or .net CDN addresses directly"
    [[ -n "${DEBUG_THIN_INSTALLER}" ]] && debug_curl_output "${SDDS3_COM_URL}"
    [[ -n "${DEBUG_THIN_INSTALLER}" ]] && debug_curl_output "${SDDS3_NET_URL}"

    if [[ ${MESSAGE_RELAYS_CONFIGURED} == 0 ]]; then
      echo "ERROR: SPL installation will fail as the endpoint cannot connect to either the .com or .net CDN server directly and no proxies are configured"
    else
      for message_relay in "${MESSAGE_RELAYS[@]}"; do
        if [[ $(curl --proxy "${message_relay}" -is ${SDDS3_COM_URL} | head -n 1) =~ "404" ]]; then
          echo "DEBUG: Endpoint can connect to .com CDN address (${SDDS3_COM_URL}) via ${message_relay}"
          CDN_URL="${SDDS3_COM_URL}"
        elif [[ $(curl --proxy "${message_relay}" -is ${SDDS3_NET_URL} | head -n 1) =~ "404" ]]; then
          echo "DEBUG: Endpoint can connect to .net CDN address (${SDDS3_NET_URL}) via ${message_relay}"
          CDN_URL="${SDDS3_NET_URL}"
        else
          echo "WARN: Endpoint cannot connect to either the .com or .net CDN address via ${message_relay}"
        fi
      done
      [[ -z "${CDN_URL}" ]] && echo "ERROR: SPL installation will fail as the endpoint cannot connect to either the .com or .net CDN server directly or via the configured proxies"
    fi
  fi

  if [[ -n "${CDN_URL}" ]]; then
    verify_connection_to_cdn "${CDN_URL}"

    for message_relay in "${MESSAGE_RELAYS[@]}"; do
      verify_connection_to_cdn "${CDN_URL}" "${message_relay}"
    done
  fi

  [[ ${#UPDATE_CACHES[@]} != 0 ]] && echo -e "\nINFO: Verifying connections to configured Update Caches"
  for update_cache in "${UPDATE_CACHES[@]}"; do
    if [[ $(curl -k -is "${update_cache}" | head -n 1) =~ "200" ]]; then
      echo "INFO: Endpoint can connect to configured Update Cache (${update_cache})"
    else
      echo "WARN: Endpoint cannot connect to configured Update Cache (${update_cache})"
      [[ -n "${DEBUG_THIN_INSTALLER}" ]] && debug_curl_output "${update_cache}"
    fi
  done
}
