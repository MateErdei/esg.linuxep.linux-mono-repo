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
    register on fail  Debug install set
    register cleanup  dump log  ${THREAT_DETECTOR_LOG_PATH}
    register cleanup  dump log  ${AV_LOG_PATH}
    ${AVPLUGIN_PID} =  Record AV Plugin PID
    ${SOPHOS_THREAT_DETECTOR_PID} =  Record Sophos Threat Detector PID
    Add IDE to install set  ${IDE_NAME}
    Mark AV Log
    Mark Sophos Threat Detector Log
    Run installer from install set
    Check IDE present in installation  ${IDE_NAME}
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
    Register on fail  Debug install set
    Add IDE to install set  ${IDE_NAME}
    Run installer from install set
    Check IDE present in installation  ${IDE_NAME}
    Remove IDE from install set  ${IDE_NAME}
    Run installer from install set
    Check IDE absent from installation  ${IDE_NAME}

Check install permissions
    [Documentation]   Find files or directories owned by sophos-spl-user or writable by sophos-spl-group.
    ...               Check results against an allowed list.
    Run installer from install set
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

Check IDE absent from installation
    [Arguments]  ${IDE_NAME}
    file should not exist  ${INSTALL_IDE_DIR}/${IDE_NAME}

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

