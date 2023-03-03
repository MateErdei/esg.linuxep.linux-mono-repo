*** Settings ***
Library     ../libs/Cleanup.py
Library     ../libs/HostsUtils.py
Library     ../libs/OSUtils.py
Library     ../libs/LogUtils.py
Library     ../libs/ThinInstallerUtils.py
Library     ../libs/UpdateServer.py
Library     Process
Library     OperatingSystem

Resource  FakeCloud.robot

*** Variables ***
${CUSTOM_DIR_BASE} =  /CustomPath
${INPUT_DIRECTORY} =  /opt/test/inputs
${CUSTOMER_DIRECTORY} =  ${INPUT_DIRECTORY}/customer
${THIN_INSTALLER_DIRECTORY} =  ${INPUT_DIRECTORY}/thin_installer
${UPDATE_CREDENTIALS} =  7ca577258eb34a6b2a1b04b3e817b76a
${SOPHOS_INSTALL} =   /opt/sophos-spl

*** Keywords ***
Base Test Setup
    Uninstall SSPL if installed


Teardown
    Run Teardown Functions

Create Thin Installer
    [Arguments]  ${MCS_URL}=https://dzr-mcs-amzn-eu-west-1-f9b7.upe.d.hmr.sophos.com/sophos/management/ep
    extract thin installer  ${THIN_INSTALLER_DIRECTORY}  /tmp/SophosInstallBase.sh
    create credentials file  /tmp/credentials.txt
    ...   b370c75f6dd86503c8cca4edbbd29b5b06162fa9b4e67f992066120ee22612d6
    ...   ${MCS_URL}
    ...   ${UPDATE_CREDENTIALS}

    build thininstaller from sections  /tmp/credentials.txt  /tmp/SophosInstallBase.sh  /tmp/SophosInstallCombined.sh


Check Base Installed
    Directory Should Exist  ${SOPHOS_INSTALL}


SDDS3 server setup
    ${handle}=  Start Process  python3 ${LIB_FILES}/SDDS3server.py --launchdarkly ${INPUT_DIRECTORY}/launchdarkly --sdds3 ${INPUT_DIRECTORY}/repo  shell=true
    Set Suite Variable    ${GL_handle}    ${handle}

SDDS3 Server Teardown
     dump log    /tmp/sdds3_server.log
     terminate process  ${GL_handle}  True
     terminate all processes  True