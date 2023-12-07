#! /bin/bash
set -e
[[ -d /vagrant ]]
(( UID == 0 )) || exec sudo -H "$0" "$@"

REPO_PATH="/vagrant/esg.linuxep.linux-mono-repo"
EDR_PATH="$REPO_PATH/edr"
BASE_PATH="$REPO_PATH/base"
COMMON_PATH="$REPO_PATH/common"

if [ ! -d "$EDR_PATH" ]
then
  echo "Failed to find EDR path: $EDR_PATH"
  exit 1
fi

if [ ! -d "$BASE_PATH" ]
then
  echo "Failed to find Base path: $BASE_PATH"
  exit 1
fi

LMONO_PATH="/vagrant/esg.linuxep.linux-mono-repo"
if [[ -d "$LMONO_PATH/.output/edr" ]]
then
    EDR_BUILD_PATH="$LMONO_PATH/.output/edr/linux_x64_dbg"
    [[ -d "${EDR_BUILD_PATH}" ]] || EDR_BUILD_PATH="$LMONO_PATH/.output/edr/linux_x64_rel"
    BASE_PATH="$LMONO_PATH/.output/base/linux_x64_dbg"
    [[ -d "${BASE_PATH}" ]] || BASE_PATH="$LMONO_PATH/.output/base/linux_x64_dbg"
fi

TEST_INPUTS=/opt/test/inputs
EDR_INPUTS=${TEST_INPUTS}/edr
mkdir -p ${EDR_INPUTS}

rm -rf "${EDR_INPUTS}/SDDS-COMPONENT"
mkdir -p "${EDR_INPUTS}/SDDS-COMPONENT"
pushd "${EDR_INPUTS}/SDDS-COMPONENT"
unzip "${EDR_BUILD_PATH}/installer.zip"
popd
ln -snf "${EDR_INPUTS}/SDDS-COMPONENT" "${TEST_INPUTS}/edr_sdds"


rm -rf "${EDR_INPUTS}/base-sdds"
mkdir -p "${EDR_INPUTS}/base-sdds"
pushd "${EDR_INPUTS}/base-sdds"
unzip "$BASE_PATH/installer.zip"
popd
ln -snf "${EDR_INPUTS}/base-sdds" "${TEST_INPUTS}/base_sdds"

ln -snf "$EDR_PATH/TA"  ${TEST_INPUTS}/test_scripts
ln -snf "$EDR_PATH/local_test_input/qp"  ${TEST_INPUTS}/qp
ln -snf "$EDR_PATH/local_test_input/lp"  ${TEST_INPUTS}/lp
ln -snf "$COMMON_PATH/TA/libs" ${TEST_INPUTS}/common_test_libs
ln -snf "$COMMON_PATH/TA/robot" ${TEST_INPUTS}/common_test_robot
ln -snf "$BASE_PATH/testUtils/SupportFiles" ${TEST_INPUTS}/SupportFiles
apt-get -y install python3 python3-pip python3-pkgconfig
python3 -m pip install -r "$EDR_PATH/TA/requirements.txt"

# Remove markers from other runs because at the moment they all share "/opt/test/inputs/test_scripts"
rm -f /tmp/ran_*_setup_vagrant_script
touch /tmp/ran_edr_setup_vagrant_script
