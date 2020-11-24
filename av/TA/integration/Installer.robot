*** Settings ***
Documentation    Integration tests of Installer
Force Tags       INTEGRATION  INSTALLER

Resource    ../shared/ComponentSetup.robot
Resource    ../shared/AVResources.robot

Library         Collections
Library         Process
Library         ../Libs/LogUtils.py
Library         ../Libs/OnFail.py

Suite Setup     Installer Suite Setup
Suite Teardown  Installer Suite TearDown

Test Setup      Installer Test Setup
Test Teardown   Installer Test TearDown

*** Test Cases ***

IDE update doesnt restart av processes
    ${AVPLUGIN_PID} =  Record AV Plugin PID
    ${SOPHOS_THREAT_DETECTOR_PID} =  Record Sophos Threat Detector PID
    Add IDE to Install Set  ${IDE_NAME}
    Mark AV Log
    Mark Sophos Threat Detector Log
    Run installer From Install Set
    Check IDE Present In Installation  ${IDE_NAME}
    Check AV Plugin Has Same PID  ${AVPLUGIN_PID}
    Check Sophos Threat Detector Has Same PID  ${SOPHOS_THREAT_DETECTOR_PID}
    Wait Until Sophos Threat Detector Log Contains With Offset  Reload triggered by USR1
    Wait Until Sophos Threat Detector Log Contains With Offset  SUSI update finished successfully  timeout=120

    # Check we can detect EICAR following update
    Create File     ${SCAN_DIRECTORY}/eicar.com    ${EICAR_STRING}
    ${rc}   ${output} =    Run And Return Rc And Output   ${AVSCANNER} ${SCAN_DIRECTORY}/eicar.com
    Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}
    Should Contain   ${output}    Detected "${SCAN_DIRECTORY}/eicar.com" is infected with EICAR-AV-Test

    # Check we can detect PEEND following update
    # This test also proves that SUSI is configured to scan executables
    Check Threat Detected  peend.exe  PE/ENDTEST


IDE can be removed
    Add IDE To Install Set  ${IDE_NAME}
    Run Installer From Install Set
    Check IDE Present In Installation  ${IDE_NAME}
    Remove IDE From Install Set  ${IDE_NAME}
    Run Installer From Install Set
    Check IDE Absent From Installation  ${IDE_NAME}


sophos_threat_detector can start after multiple IDE updates
    [Tags]  MANUAL
    # TODO - remove MANUAL keyword once LINUXDAR-2478 is fixed.
    ${SOPHOS_THREAT_DETECTOR_PID} =  Record Sophos Threat Detector PID
    Install IDE  ${IDE_NAME}
    Install IDE  ${IDE2_NAME}
    Install IDE  ${IDE3_NAME}
    File Should Not Exist   ${COMPONENT_ROOT_PATH}/chroot/susi/distribution_version/libsusi.so
    Check Sophos Threat Detector has same PID  ${SOPHOS_THREAT_DETECTOR_PID}
    Force Sophos Threat Detector to restart


Check install permissions
    [Documentation]   Find files or directories owned by sophos-spl-user or writable by sophos-spl-group.
    ...               Check results against an allowed list.
    Run Installer From Install Set
    ${rc}   ${output} =    Run And Return Rc And Output
    ...     find ${COMPONENT_ROOT_PATH} -! -type l -\\( -user sophos-spl-user -o -group sophos-spl-group -perm -0020 -\\) -prune
    Should Be Equal As Integers  ${rc}  0
    ${output} =   Replace String   ${output}   ${COMPONENT_ROOT_PATH}/   ${EMPTY}
    @{items} =    Split To Lines   ${output}
    Sort List   ${items}
    Log List   ${items}
    Should Not Be Empty   ${items}
    Remove Values From List    ${items}   @{ALLOWED_TO_WRITE}
    Log List   ${items}
    Should Be Empty   ${items}


*** Variables ***
${IDE_NAME}         peend.ide
${IDE2_NAME}        pemid.ide
${IDE3_NAME}        Sus2Exp.ide
@{ALLOWED_TO_WRITE}
...     chroot/etc
...     chroot/log
...     chroot/opt/sophos-spl/base/etc
...     chroot/opt/sophos-spl/base/update/var
...     chroot/opt/sophos-spl/plugins/av
...     chroot/susi/distribution_version
...     chroot/susi/update_source
...     chroot/tmp
...     chroot/var
...     log
...     var

*** Keywords ***
Installer Suite Setup
    Register On Fail  Debug install set
    Register On Fail  dump log  ${THREAT_DETECTOR_LOG_PATH}
    Register On Fail  dump log  ${AV_LOG_PATH}
    Install With Base SDDS

Installer Suite TearDown
    Log  Installer Suite TearDown

Installer Test Setup
    Check AV Plugin Installed With Base
    Mark AV Log

Installer Test TearDown
    Run Teardown Functions
    #TODO: Remove this line once CORE-2095 is fixed
    #Currently loading more than 1 IDE in test, stops SUSI
    Install With Base SDDS

Sophos Threat Detector Log Contains With Offset
    [Arguments]  ${input}
    ${offset} =  Get Variable Value  ${SOPHOS_THREAT_DETECTOR_LOG_MARK}  0
    File Log Contains With Offset  ${THREAT_DETECTOR_LOG_PATH}   ${input}   offset=${offset}

Wait Until Sophos Threat Detector Log Contains With Offset
    [Arguments]  ${input}  ${timeout}=15
    Wait Until File Log Contains  Sophos Threat Detector Log Contains With Offset  ${input}   timeout=${timeout}

Install IDE
    [Arguments]  ${ide_name}
    Add IDE to install set  ${ide_name}
    Run installer from install set
    Check IDE present in installation  ${ide_name}

Check IDE absent from installation
    [Arguments]  ${ide_name}
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
