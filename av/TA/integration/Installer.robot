*** Settings ***
Documentation    Integration tests of Installer
Force Tags       INTEGRATION  INSTALLER

Resource    ../shared/ComponentSetup.robot
Resource    ../shared/AVResources.robot

Library         Process
Library         ../Libs/LogUtils.py
Library         ../Libs/OnFail.py

Suite Setup     Installer Suite Setup
Suite Teardown  Installer Suite TearDown

Test Setup      Installer Test Setup
Test Teardown   Installer Test TearDown

*** Test Cases ***

IDE update doesnt restart av plugin
    # TODO: LINUXDAR-2365 fix for hot-updating - pid won't change
    register on fail  Debug install set
    register cleanup  dump log  ${THREAT_DETECTOR_LOG_PATH}
    register cleanup  dump log  ${AV_LOG_PATH}
    ${PID} =  Record AV Plugin PID
    ${SOPHOS_THREAT_DETECTOR_PID} =  Record Sophos Threat Detector PID
    Add IDE to install set
    Mark AV Log
    Mark Sophos Threat Detector Log
    Run installer from install set
    Check IDE present in installation
    Check AV Plugin has same PID  ${PID}
    Wait Until AV Plugin Log Contains With Offset  Starting "${AV_PLUGIN_PATH}/sbin/sophos_threat_detector_launcher"
    Wait Until Sophos Threat Detector Log Contains With Offset  Reload triggered by USR1
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  1 secs
    ...  Check Sophos Threat Detector has different PID  ${SOPHOS_THREAT_DETECTOR_PID}

IDE can be removed
    register on fail  Debug install set
    Add IDE to install set
    Run installer from install set
    Check IDE present in installation
    Remove IDE from install set
    Run installer from install set
    Check IDE absent from installation

*** Variables ***
${IDE_NAME}         peend.ide
${IDE_DIR}          ${COMPONENT_INSTALL_SET}/files/plugins/av/chroot/susi/distribution_version/version1/vdb
${INSTALL_IDE_DIR}  ${COMPONENT_ROOT_PATH}/chroot/susi/distribution_version/version1/vdb

*** Keywords ***
Installer Suite Setup
    Install With Base SDDS

Installer Suite TearDown
    Log  Installer Suite TearDown

Installer Test Setup
    Check AV Plugin Installed With Base
    Mark AV Log

Installer Test TearDown
    run teardown functions

Sophos Threat Detector Log Contains With Offset
    [Arguments]  ${input}
    ${offset} =  Get Variable Value  ${SOPHOS_THREAT_DETECTOR_LOG_MARK}  0
    File Log Contains With Offset  ${THREAT_DETECTOR_LOG_PATH}   ${input}   offset=${offset}

Wait Until Sophos Threat Detector Log Contains With Offset
    [Arguments]  ${input}  ${timeout}=15
    Wait Until File Log Contains  Sophos Threat Detector Log Contains With Offset  ${input}   timeout=${timeout}

Get Pid
    [Arguments]  ${EXEC}
    ${result} =  run process  pidof  ${EXEC}  stderr=STDOUT
    Should Be Equal As Integers  ${result.rc}  ${0}
    Log  pid == ${result.stdout}
    [Return]   ${result.stdout}

Record AV Plugin PID
    ${PID} =  Get Pid  ${PLUGIN_BINARY}
    [Return]   ${PID}

Record Sophos Threat Detector PID
    ${PID} =  Get Pid  ${SOPHOS_THREAT_DETECTOR_BINARY}
    [Return]   ${PID}

Remove IDE from install set
    Remove File  ${IDE_DIR}/${IDE_NAME}

Add IDE to install set
    # COMPONENT_INSTALL_SET
    # TODO: LINUXDAR-2365 fix for hot-updating
    ${IDE} =  Set Variable  ${RESOURCES_PATH}/ides/${IDE_NAME}
    Copy file  ${IDE}  ${IDE_DIR}/${IDE_NAME}
    register cleanup  Remove IDE from install set

Run installer from install set
    ${result} =  run process    bash  -x  ${COMPONENT_INSTALL_SET}/install.sh  stdout=/tmp/proc.out  stderr=STDOUT
    Log  ${result.stdout}
    Should Be Equal As Integers  ${result.rc}  ${0}

Check IDE present in installation
    file should exist  ${INSTALL_IDE_DIR}/${IDE_NAME}

Check IDE absent from installation
    file should not exist  ${INSTALL_IDE_DIR}/${IDE_NAME}

Check AV Plugin has same PID
    [Arguments]  ${PID}
    ${currentPID} =  Record AV Plugin PID
    Should Be Equal As Integers  ${PID}  ${currentPID}

Check Sophos Threat Detector has different PID
    [Arguments]  ${PID}
    ${currentPID} =  Record Sophos Threat Detector PID
    Should Not Be Equal As Integers  ${PID}  ${currentPID}

Debug install set
    ${result} =  run process  find  ${COMPONENT_INSTALL_SET}/files/plugins/av/chroot/susi/distribution_version  -type  f  stdout=/tmp/proc.out   stderr=STDOUT
    Log  INSTALL_SET= ${result.stdout}
    ${result} =  run process  find  ${SOPHOS_INSTALL}/plugins/av/chroot/susi/distribution_version   stdout=/tmp/proc.out    stderr=STDOUT
    Log  INSTALLATION= ${result.stdout}
