*** Settings ***
Library         Process
Library         OperatingSystem
Library         String

Library   ${COMMON_TEST_LIBS}/LogUtils.py
Library   ${COMMON_TEST_LIBS}/OnFail.py
Library   ${COMMON_TEST_LIBS}/FullInstallerUtils.py

Resource  ${COMMON_TEST_ROBOT}/GeneralUtilsResources.robot

*** Variables ***
${DEVICE_ISOLATION_PLUGIN_PATH}     ${SOPHOS_INSTALL}/plugins/deviceisolation
${DEVICE_ISOLATION_LOG_PATH}     ${DEVICE_ISOLATION_PLUGIN_PATH}/log/deviceisolation.log
${BASE_SDDS}                     ${TEST_INPUT_PATH}/base_sdds/


*** Keywords ***
Install Base For Component Tests
    File Should Exist     ${BASE_SDDS}/install.sh
    Run Shell Process   bash -x ${BASE_SDDS}/install.sh --log-level DEBUG 2>/tmp/installer.log   OnError=Failed to Install Base   timeout=60s
    Run Keyword and Ignore Error   Run Shell Process    /opt/sophos-spl/bin/wdctl stop mcsrouter  OnError=Failed to stop mcsrouter

Install Device Isolation Directly from SDDS
    ${DEVICE_ISOLATION_SDDS} =   get_sspl_device_isolation_plugin_sdds
    File Should Exist   ${DEVICE_ISOLATION_SDDS}/install.sh
    File Should Exist   ${DEVICE_ISOLATION_SDDS}/files/plugins/deviceisolation/bin/deviceisolation
    File Should Exist   ${DEVICE_ISOLATION_SDDS}/files/plugins/deviceisolation/bin/nft
    ${result} =   Run Process  bash  -x  ${DEVICE_ISOLATION_SDDS}/install.sh   timeout=120s   stderr=STDOUT
    log  ${result.stdout}
    Should Be Equal As Integers  ${result.rc}  ${0}   "Failed to install deviceisolation.\noutput: \n${result.stdout}"

Uninstall Base
    ${result} =   Run Process  bash  ${SOPHOS_INSTALL}/bin/uninstall.sh  --force   timeout=30s  stderr=STDOUT
    Should Be Equal As Integers  ${result.rc}  ${0}   "Failed to uninstall base.\noutput: \n${result.stdout}"
    
Uninstall Device Isolation
    ${result} =   Run Process  bash ${SOPHOS_INSTALL}/plugins/deviceisolation/bin/uninstall.sh --force   shell=True   timeout=20s
    Should Be Equal As Integers  ${result.rc}  ${0}   "Failed to uninstall Device Isolation.\nstdout: \n${result.stdout}\n. stderr: \n${result.stderr}"

Downgrade Device Isolation
    ${result} =   Run Process  bash ${SOPHOS_INSTALL}/plugins/deviceisolation/bin/uninstall.sh --downgrade   shell=True   timeout=20s
    Should Be Equal As Integers  ${result.rc}  ${0}   "Failed to downgrade Device Isolation.\nstdout: \n${result.stdout}\n. stderr: \n${result.stderr}"

Check Device Isolation Executable Running
    ${result} =    Run Process  pgrep deviceisolation | wc -w  shell=true
    Should Be Equal As Integers    ${result.stdout}    ${1}       msg="stdout:${result.stdout}\nerr: ${result.stderr}"

Check Device Isolation Executable Not Running
    ${result} =    Run Process  pgrep  -a  deviceisolation
    Run Keyword If  ${result.rc}==0   Report On Process   ${result.stdout}
    Should Not Be Equal As Integers    ${result.rc}    ${0}     msg="stdout:${result.stdout}\nerr: ${result.stderr}"

Stop Device Isolation
    Run Shell Process  ${SOPHOS_INSTALL}/bin/wdctl stop deviceisolation   OnError=failed to stop deviceisolation  timeout=35s

Start Device Isolation
    Run Shell Process  ${SOPHOS_INSTALL}/bin/wdctl start deviceisolation   OnError=failed to start deviceisolation

Restart Device Isolation
    ${di_mark} =    Get Device Isolation Log Mark
    Stop Device Isolation
    Wait For Log Contains From Mark    ${di_mark}    deviceisolation <> Plugin Finished

    ${di_mark} =    Get Device Isolation Log Mark
    Start Device Isolation
    Wait For Log Contains From Mark    ${di_mark}    Entering the main loop


List File In Test Dir
    ${filenames} =  List Directory  /opt/test/inputs/fake_management/
    Log    ${filenames}


Get Device Isolation Log Mark
    ${mark} =  mark_log_size  ${DEVICE_ISOLATION_LOG_PATH}
    [Return]  ${mark}

Device Isolation Suite Setup
    Install Base For Component Tests
    Install Device Isolation Directly from SDDS

Device Isolation Suite Teardown
    Uninstall Base

Device Isolation Test Setup
    Register on fail  dump log  ${SOPHOS_INSTALL}/logs/base/watchdog.log
    Register on fail  dump log  ${SOPHOS_INSTALL}/logs/base/sophosspl/sophos_managementagent.log
    Register on fail  dump log  ${DEVICE_ISOLATION_LOG_PATH}
    Register on fail  List File In Test Dir

Device Isolation Test Teardown
	Run teardown functions

