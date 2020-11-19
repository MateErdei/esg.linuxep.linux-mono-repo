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

IDE update doesnt restart av processes
    register on fail  Debug install set
    register cleanup  dump log  ${THREAT_DETECTOR_LOG_PATH}
    register cleanup  dump log  ${AV_LOG_PATH}
    ${AVPLUGIN_PID} =  Record AV Plugin PID
    ${SOPHOS_THREAT_DETECTOR_PID} =  Record Sophos Threat Detector PID
    Add IDE to install set
    Mark AV Log
    Mark Sophos Threat Detector Log
    Run installer from install set
    Check IDE present in installation
    Check AV Plugin has same PID  ${AVPLUGIN_PID}
    Check Sophos Threat Detector has same PID  ${SOPHOS_THREAT_DETECTOR_PID}
    Wait Until Sophos Threat Detector Log Contains With Offset  Reload triggered by USR1
    Wait Until Sophos Threat Detector Log Contains With Offset  SUSI update finished successfully  timeout=120

    # Check we can detect EICAR following update
    Create File     ${SCAN_DIRECTORY}/eicar.com    ${EICAR_STRING}
    ${rc}   ${output} =    Run And Return Rc And Output   ${AVSCANNER} ${SCAN_DIRECTORY}/eicar.com
    Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}
    Should Contain   ${output}    Detected "${SCAN_DIRECTORY}/eicar.com" is infected with EICAR-AV-Test

    # Check we can detect PEEND following update
    # This test also proves that SUSI is configured to scan executables
    Copy File   ${RESOURCES_PATH}/file_samples/peend.exe  ${SCAN_DIRECTORY}
    ${rc}   ${output} =    Run And Return Rc And Output   ${AVSCANNER} ${SCAN_DIRECTORY}/peend.exe
    Log To Console  ${output}
    Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}
    Should Contain   ${output}    Detected "${SCAN_DIRECTORY}/peend.exe" is infected with PE/ENDTEST


IDE can be removed
    register on fail  Debug install set
    Add IDE to install set
    Run installer from install set
    Check IDE present in installation
    Remove IDE from install set
    Run installer from install set
    Check IDE absent from installation

sophos_threat_detector can start after multiple IDE updates
    register on fail  Debug install set
    register cleanup  dump log  ${THREAT_DETECTOR_LOG_PATH}
    register cleanup  dump log  ${AV_LOG_PATH}
    ${SOPHOS_THREAT_DETECTOR_PID} =  Record Sophos Threat Detector PID
    Install IDE  ${IDE_NAME}
    Install IDE  ${IDE2_NAME}
    Install IDE  ${IDE3_NAME}
    File Should Not Exist   ${COMPONENT_ROOT_PATH}/chroot/susi/distribution_version/libsusi.so
    Check Sophos Threat Detector has same PID  ${SOPHOS_THREAT_DETECTOR_PID}
    Force Sophos Threat Detector to restart


*** Variables ***
${IDE_NAME}         peend.ide
${IDE2_NAME}        pemid.ide
${IDE3_NAME}        Sus2Exp.ide
${IDE_DIR}          ${COMPONENT_INSTALL_SET}/files/plugins/av/chroot/susi/update_source/vdl
${INSTALL_IDE_DIR}  ${COMPONENT_ROOT_PATH}/chroot/susi/update_source/vdl
${SCAN_DIRECTORY}   /home/vagrant/this/is/a/directory/for/scanning
${AVSCANNER}        /usr/local/bin/avscanner

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

Install IDE
    [Arguments]  ${ide_name}=${IDE_NAME}
    Add IDE to install set  ${ide_name}
    Run installer from install set
    Check IDE present in installation  ${ide_name}

Remove IDE from install set
    [Arguments]  ${ide_name}=${IDE_NAME}
    Remove File  ${IDE_DIR}/${ide_name}

Add IDE to install set
    [Arguments]  ${ide_name}=${IDE_NAME}
    # COMPONENT_INSTALL_SET
    ${IDE} =  Set Variable  ${RESOURCES_PATH}/ides/${ide_name}
    Copy file  ${IDE}  ${IDE_DIR}/${ide_name}
    register cleanup  Remove IDE from install set  ${ide_name}

Run installer from install set
    ${result} =  run process    bash  -x  ${COMPONENT_INSTALL_SET}/install.sh  stdout=/tmp/proc.out  stderr=STDOUT
    Log  ${result.stdout}
    Should Be Equal As Integers  ${result.rc}  ${0}

Check IDE present in installation
    [Arguments]  ${ide_name}=${IDE_NAME}
    file should exist  ${INSTALL_IDE_DIR}/${ide_name}

Check IDE absent from installation
    [Arguments]  ${ide_name}=${IDE_NAME}
    file should not exist  ${INSTALL_IDE_DIR}/${ide_name}

Check AV Plugin has same PID
    [Arguments]  ${PID}
    ${currentPID} =  Record AV Plugin PID
    Should Be Equal As Integers  ${PID}  ${currentPID}

Check Sophos Threat Detector has same PID
    [Arguments]  ${PID}
    ${currentPID} =  Record Sophos Threat Detector PID
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

Force Sophos Threat Detector to restart
    Restart sophos_threat_detector

Kill sophos_threat_detector
   ${rc}   ${output} =    Run And Return Rc And Output    pgrep sophos_threat
   Run Process   /bin/kill   -9   ${output}

Restart sophos_threat_detector
    Kill sophos_threat_detector
    Mark AV Log
    Wait until threat detector running
