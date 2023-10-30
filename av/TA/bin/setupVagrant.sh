#! /bin/bash
set -e
[[ -d /vagrant ]]
(( UID == 0 )) || exec sudo -H "$0" "$@"

LMONO_PATH="/vagrant/esg.linuxep.linux-mono-repo"
GITHUB_PATH="${LMONO_PATH}/av"
COMMON_PATH="${LMONO_PATH}/common"

function failure()
{
    local EXIT=$1
    shift
    echo "$@"

}

if [ -d "$GITHUB_PATH" ]
then
  AV_PATH="$GITHUB_PATH"
else
  failure 1 "Failed to find AV path"
fi

apt -y install unzip

TEST_INPUTS=/opt/test/inputs

mkdir -p ${TEST_INPUTS}/av/
ln -snf "${AV_PATH}/TA/" ${TEST_INPUTS}/test_scripts

if [[ -d "$LMONO_PATH/.output/av" ]]
then
    AV_BUILD_PATH="$LMONO_PATH/.output/av/linux_x64_dbg"
    [[ -d "${AV_BUILD_PATH}" ]] || AV_BUILD_PATH="$LMONO_PATH/.output/av/linux_x64_rel"
    BASE_PATH="$LMONO_PATH/.output/base/linux_x64_dbg"
    [[ -d "${BASE_PATH}" ]] || BASE_PATH="$LMONO_PATH/.output/base/linux_x64_dbg"

    if [[ -d "$AV_BUILD_PATH/installer" ]]
    then
        ln -snf "$AV_BUILD_PATH/installer" ${TEST_INPUTS}/av/SDDS-COMPONENT
    elif [[ -f "$AV_BUILD_PATH/installer.zip" ]]
    then
        rm -rf "${TEST_INPUTS}/av/SDDS-COMPONENT"
        mkdir -p "${TEST_INPUTS}/av/SDDS-COMPONENT"
        pushd "${TEST_INPUTS}/av/SDDS-COMPONENT"
        unzip "$AV_BUILD_PATH/installer.zip"
        popd
    else
        failure 3 "Can't find AV installer"
    fi

    if [[ -d "$BASE_PATH/installer" ]]
    then
        ln -snf "$BASE_PATH/installer" "${TEST_INPUTS}/av/base-sdds"
    elif [[ -f "$BASE_PATH/installer.zip" ]]
    then
        rm -rf "${TEST_INPUTS}/av/base-sdds"
        mkdir -p "${TEST_INPUTS}/av/base-sdds"
        pushd "${TEST_INPUTS}/av/base-sdds"
        unzip "$BASE_PATH/installer.zip"
        popd
    else
        failure 4 "Can't find Base installer"
    fi

    mkdir -p ${TEST_INPUTS}/tap_test_output
    pushd ${TEST_INPUTS}/tap_test_output
    unzip -o "$AV_BUILD_PATH/tap_test_output.zip"
    popd
elif [[ -d "${AV_PATH}/output/SDDS-COMPONENT" ]]
then
    ln -snf "${AV_PATH}/output/SDDS-COMPONENT/" /opt/test/inputs/av/SDDS-COMPONENT
    ln -snf "${AV_PATH}/output/test-resources/" /opt/test/inputs/av/test-resources
    ln -snf "${AV_PATH}/output/base-sdds/" /opt/test/inputs/av/base-sdds
    ln -snf "${AV_PATH}/output/" /opt/test/inputs/tap_test_output_from_build
    ln -snf "${COMMON_PATH}/TA/libs" /opt/test/inputs/common_test_libs
    ln -snf "${COMMON_PATH}/TA/robot" /opt/test/inputs/common_test_robot
    ln -snf "${BASE_PATH}/testUtils/SupportFiles" /opt/test/inputs/SupportFiles
else
    failure 2 "Can't find SPL-AV build!"
fi

apt-get -y install python3 python3-pip python3-pkgconfig
apt-get -y install nfs-kernel-server samba p7zip-full gdb
cd ${AV_PATH}/TA
python3 -m pip install -r requirements.txt

# Stop samba related services as they both start up on install
systemctl stop nmbd.service
systemctl stop smbd.service

# Remove markers from other runs because at the moment they all share "/opt/test/inputs/test_scripts"
rm -f /tmp/ran_*_setup_vagrant_script
touch /tmp/ran_av_setup_vagrant_script
