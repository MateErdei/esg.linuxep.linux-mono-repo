#!/bin/bash

function failure()
{
    echo "$@"
    exit 1
}

function try_command_with_backoff()
{
    for i in {1..5};
    do
      "$@"
      if [[ $? != 0 ]]
      then
        EXIT_CODE=1
        sleep 15
      else
        EXIT_CODE=0
        break
      fi
    done
    return $EXIT_CODE
}

SCRIPTDIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )

python3 -m pip install --upgrade pip
PIP_ARGS="-i https://tap-artifactory1.eng.sophos/artifactory/api/pypi/pypi/simple --trusted-host tap-artifactory1.eng.sophos --cert ./sophos_certs.pem"
try_command_with_backoff  python3 -m pip install wheel ${PIP_ARGS}
try_command_with_backoff  python3 -m pip install --upgrade build_scripts ${PIP_ARGS}  || failure "Unable to install build_scripts"
try_command_with_backoff  python3 -m pip install --upgrade keyrings.alt ${PIP_ARGS}  || failure "Unable to install dependency"

#Update the hardcoded paths to filer 5 and filer 6 in the build scripts to work on dev machines
BUILD_SCRIPT_INSTALL_DIR="$(python3 -m pip show build_scripts | grep "Location" | sed s/Location:\ //)/build_scripts"

[[ -d "${BUILD_SCRIPT_INSTALL_DIR}" ]] || failure "Unable to find build_scripts install location"

echo "Patch before:"
grep "mnt/filer" "${BUILD_SCRIPT_INSTALL_DIR}/build_context.py"
sed -i "s_'/mnt/filer6'_'/mnt/filer6/bfr'_g" "${BUILD_SCRIPT_INSTALL_DIR}/build_context.py"
sed -i 's/\/mnt\/filer\/bir/\/uk-filer5\/prodro\/bir/g' "${BUILD_SCRIPT_INSTALL_DIR}/build_context.py"
echo "Patch after:"
grep "mnt/filer" "${BUILD_SCRIPT_INSTALL_DIR}/build_context.py"

#Create temporary location used by scripts
sudo mkdir -p /SophosPackages
sudo chown jenkins:jenkins /SophosPackages
chmod 777 /SophosPackages

#Create or clear out the build folder used for unpacking gcc to
sudo mkdir -p /build
sudo chown jenkins:jenkins /build
chmod 777 /build
rm -rf /build/*

