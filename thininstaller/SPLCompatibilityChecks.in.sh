#!/bin/bash
PRODUCT_NAME="SPL Compatibility Tool"
VERSION=1.0.0

EXIT_SUCCESS=0
EXITCODE_BAD_INSTALL_PATH=1
EXITCODE_NOT_LINUX=2
EXITCODE_NOT_64_BIT=3
EXITCODE_NO_SYSTEMD=4
EXIT_FAIL_COULD_NOT_FIND_LIBC_VERSION=5
EXIT_FAIL_WRONG_LIBC_VERSION=6
EXITCODE_NOEXEC_TMP=7
EXITCODE_MISSING_PACKAGE=8
EXITCODE_NOT_ENOUGH_SPACE=9
EXITCODE_NOT_ENOUGH_MEM=10
EXITCODE_INVALID_CA_PATHS=11

@COMMON_SPL_COMPATIBILITY_FUNCTIONS@

function failure() {
  local code=$1
  shift
  echo "$@" >&2
  exit "${code}"
}

# Copied from installer_header - decided not to make common since the ThinInstaller has the option to uninstall SAV
function check_SAV_installed()
{
    local path=$1
    local sav_instdir=`readlink ${path} | sed 's/bin\/savscan//g'`
    if [[ "$sav_instdir" == "" ]] || [[ ! -d ${sav_instdir} ]]
    then
        # Sophos Anti-Virus not found at this location
        return
    fi
    echo "WARN: Found an existing installation of Sophos Anti-Virus in $sav_instdir."
    echo "WARN: This product cannot be run alongside Sophos Anti-Virus. Please uninstall Sophos Anti-Virus or run the SPL installer with --uninstall-sav"
}

function print_help_text() {
  echo "${PRODUCT_NAME}, help:"
  echo "Version: ${VERSION}"
  echo "Usage: [options]"
  echo "Valid options are:"
  echo -e "--help [-h]\t\t\tDisplay this summary"
  echo -e "--version [-v]\t\t\tDisplay version of this tool"
  echo -e "--debug [-d]\t\t\tLog debug information"
  echo -e "--installer-path <path>\t\t\tPath to SophosSetup.sh"
  echo -e "--install-dir <path>\t\t\tPath to desired SPL install location, default is /opt/sophos-spl"

  echo -e "\nExit codes:"
  echo -e "\t- SUCCESS=0"
  echo -e "\t- BAD_INSTALL_PATH=1"
  echo -e "\t- NOT_LINUX=2"
  echo -e "\t- NOT_64_BIT=3"
  echo -e "\t- NO_SYSTEMD=4"
  echo -e "\t- COULD_NOT_FIND_LIBC_VERSION=5"
  echo -e "\t- WRONG_LIBC_VERSION=6"
  echo -e "\t- EXITCODE_NOEXEC_TMP=7"
  echo -e "\t- MISSING_PACKAGE=8"
  echo -e "\t- NOT_ENOUGH_SPACE=9"
  echo -e "\t- NOT_ENOUGH_MEM=10"
  echo -e "\t- INVALID_CA_PATHS=11"
}

echo "This software is governed by the terms and conditions of a licence agreement with Sophos Limited"
echo "This script performs pre-installation checks for Sophos Protection for Linus (SPL)"
echo -e "To view the minimum requirements for SPL, please refer to the release notes: https://docs.sophos.com/releasenotes/output/en-us/esg/linux_protection_rn.html\n"
while [[ $# -ge 1 ]]; do
  case $1 in
  --help | -h)
    shift
    print_help_text
    exit ${EXIT_SUCCESS}
    ;;
  --version | -v)
    shift
    echo "${PRODUCT_NAME}, version: ${VERSION}"
    exit ${EXIT_SUCCESS}
    ;;
  --debug | -d)
    shift
    DEBUG_THIN_INSTALLER=1
    ;;
  --installer-path)
    shift
    THIN_INSTALLER_PATH=$1
    CENTRAL_URL=$(sed -n -e "/^URL=/ s/.*\= *//p" "${THIN_INSTALLER_PATH}")
    ;;
  --install-dir)
    shift
    SOPHOS_INSTALL="$1"
    verify_install_directory
    ;;
  *)
    echo "Unrecognised argument: $1"
    print_help_text
    exit 1
    ;;
  esac
  shift
done

verify_system_requirements

# Check if SAV is installed (copied from installer_header - decided not to make common since the ThinInstaller has the option to uninstall SAV)
## We check everything on $PATH, and always /usr/local/bin and /usr/bin
## This should catch everywhere SAV might have installed the sweep symlink
SWEEP=$(which sweep 2>/dev/null)
[ -x "$SWEEP" ] && check_SAV_installed "$SWEEP"
check_SAV_installed '/usr/local/bin/sweep'
check_SAV_installed '/usr/bin/sweep'

# Set default central URL when ThinInstaller path is not provided
if [[ -z "${CENTRAL_URL}" ]]; then
  CENTRAL_URL="https://mcs2-cloudstation-eu-west-1.prod.hydra.sophos.com/sophos/management/ep"
  echo "INFO: Failed to get Central URL from the SPL installer, using default: ${CENTRAL_URL}"
  echo "WARN: This Central URL may be incorrect depending on the region of the endpoint"
fi

# Extract Update Caches from ThinInstaller
declare -a MESSAGE_RELAYS
declare -a UPDATE_CACHES

if [[ -n "${THIN_INSTALLER_PATH}" ]]; then
  IFS=';' read -ra message_relay_array <<<"$(sed -n -e "/^MESSAGE_RELAYS=[^$]/ s/.*\= *//p" "${THIN_INSTALLER_PATH}")"
  for message_relay in "${message_relay_array[@]}"; do
    MESSAGE_RELAYS+=("${message_relay%%,*}")
  done

  IFS=';' read -ra update_cache_array <<<"$(sed -n -e "/^UPDATE_CACHES=[^$]/ s/.*\= *//p" "${THIN_INSTALLER_PATH}")"
  for update_cache in "${update_cache_array[@]}"; do
    UPDATE_CACHES+=("${update_cache%%,*}")
  done
else
  echo "WARN: Path to SophosSetup.sh not provided so network checks will not be done using message relays or update caches"
fi

verify_network_connections "${MESSAGE_RELAYS[*]}" "${UPDATE_CACHES[*]}"

exit "${EXIT_SUCCESS}"
