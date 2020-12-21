*** Settings ***
Documentation    Integration tests of Installer
Force Tags       INTEGRATION  INSTALLER

Resource    ../shared/ComponentSetup.robot
Resource    ../shared/AVResources.robot
Resource    ../shared/BaseResources.robot

Library         Collections
Library         Process
Library         ../Libs/LogUtils.py
Library         ../Libs/OnFail.py
Library         ../Libs/OSUtils.py

Suite Setup     Installer Suite Setup
Suite Teardown  Installer Suite TearDown

Test Setup      Installer Test Setup
Test Teardown   Installer Test TearDown

*** Test Cases ***

IDE update doesnt restart av processes
    ${AVPLUGIN_PID} =  Record AV Plugin PID
    ${SOPHOS_THREAT_DETECTOR_PID} =  Record Sophos Threat Detector PID
    Install IDE  ${IDE_NAME}
    Check AV Plugin Has Same PID  ${AVPLUGIN_PID}
    Check Sophos Threat Detector Has Same PID  ${SOPHOS_THREAT_DETECTOR_PID}

    # Check we can detect EICAR following update
    Create File     ${SCAN_DIRECTORY}/eicar.com    ${EICAR_STRING}
    ${rc}   ${output} =    Run And Return Rc And Output   ${AVSCANNER} ${SCAN_DIRECTORY}/eicar.com
    Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}
    Should Contain   ${output}    Detected "${SCAN_DIRECTORY}/eicar.com" is infected with EICAR-AV-Test

    # Check we can detect PEEND following update
    # This test also proves that SUSI is configured to scan executables
    Check Threat Detected  peend.exe  PE/ENDTEST


Scanner works after upgrade
    Mark Sophos Threat Detector Log

    # modify the manifest to force the installer to perform a full product update
    Modify manifest
    Run Installer From Install Set

    # Existing robot functions don't check marked logs, so we do our own log check instead
    # Check Plugin Installed and Running
    Wait Until Sophos Threat Detector Log Contains With Offset
    ...   UnixSocket <> Starting listening on socket
    ...   timeout=40

    Mark AV Log
    Mark Sophos Threat Detector Log

    # Check we can detect EICAR following upgrade
    Create File     ${SCAN_DIRECTORY}/eicar.com    ${EICAR_STRING}
    ${rc}   ${output} =    Run And Return Rc And Output   ${AVSCANNER} ${SCAN_DIRECTORY}/eicar.com
    Log   ${output}
    Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}
    Should Contain   ${output}    Detected "${SCAN_DIRECTORY}/eicar.com" is infected with EICAR-AV-Test

    # check that the logs are still working (LINUXDAR-2535)
    Sophos Threat Detector Log Contains With Offset   EICAR-AV-Test
    AV Plugin Log Contains With Offset   EICAR-AV-Test


AV Plugin gets customer id after upgrade
    ${customerIdFile1} =   Set Variable   ${AV_PLUGIN_PATH}/var/customer_id.txt
    ${customerIdFile2} =   Set Variable   ${AV_PLUGIN_PATH}/chroot${customerIdFile1}
    Remove Files   ${customerIdFile1}   ${customerIdFile2}

    Mark Sophos Threat Detector Log

    Send Alc Policy

    ${expectedId} =   Set Variable   a1c0f318e58aad6bf90d07cabda54b7d
    Wait Until Created   ${customerIdFile1}   timeout=5sec
    ${customerId1} =   Get File   ${customerIdFile1}
    Should Be Equal   ${customerId1}   ${expectedId}

    Wait Until Created   ${customerIdFile2}   timeout=5sec
    ${customerId2} =   Get File   ${customerIdFile2}
    Should Be Equal   ${customerId2}   ${expectedId}

    Wait Until Sophos Threat Detector Log Contains With Offset   UnixSocket <> Starting listening on socket

    # force an upgrade, check that customer id is set
    Mark Sophos Threat Detector Log

    Remove Files   ${customerIdFile1}   ${customerIdFile2}

    # modify the manifest to force the installer to perform a full product update
    Modify manifest
    Run Installer From Install Set

    Wait Until Created   ${customerIdFile1}   timeout=5sec
    ${customerId1} =   Get File   ${customerIdFile1}
    Should Be Equal   ${customerId1}   ${expectedId}

    Wait Until Created   ${customerIdFile2}   timeout=5sec
    ${customerId2} =   Get File   ${customerIdFile2}
    Should Be Equal   ${customerId2}   ${expectedId}

    Wait Until Sophos Threat Detector Log Contains With Offset   UnixSocket <> Starting listening on socket


IDE can be removed
    File should not exist  ${INSTALL_IDE_DIR}/${ide_name}
    ${SOPHOS_THREAT_DETECTOR_PID} =  Record Sophos Threat Detector PID

    Install IDE  ${IDE_NAME}
    File should exist  ${INSTALL_IDE_DIR}/${ide_name}
    File should exist  ${INSTALL_IDE_DIR}/${ide_name}.0

    Uninstall IDE  ${IDE_NAME}
    File should not exist  ${INSTALL_IDE_DIR}/${ide_name}
    File should not exist  ${INSTALL_IDE_DIR}/${ide_name}.0
    File Should Not Exist   ${COMPONENT_ROOT_PATH}/chroot/susi/distribution_version/libsusi.so

    Check Sophos Threat Detector has same PID  ${SOPHOS_THREAT_DETECTOR_PID}

sophos_threat_detector can start after multiple IDE updates
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

Check permissions after upgrade
    # find writable directories
    ${rc}   ${output} =    Run And Return Rc And Output
    ...     find ${COMPONENT_ROOT_PATH} -name susi -prune -o -type d -\\( -user sophos-spl-user -perm -0200 -o -group sophos-spl-group -perm -0020 -\\) -print
    Should Be Equal As Integers  ${rc}  0
    @{items} =    Split To Lines   ${output}
    Sort List   ${items}
    Log List   ${items}

    # create writable files
    @{files} =   Create List
    FOR   ${item}   IN   @{items}
       ${file} =   Set Variable   ${item}/test_file
       Create File   ${file}   test data
       Change Owner   ${file}   sophos-spl-user   sophos-spl-group
       Append To List   ${files}   ${file}
    END
    Log List   ${files}
    ${files_as_args} =   Catenate   @{files}

    # store current permissions for our files
    ${rc}   ${output} =    Run And Return Rc And Output
    ...     ls -l ${files_as_args}
    Should Be Equal As Integers  ${rc}  0
    Log   ${output}
    ${before} =   Set Variable   ${output}

    # modify the manifest to force the installer to perform a full product update
    Modify manifest
    Run Installer From Install Set

    # check our files are still writable
    ${rc}   ${output} =    Run And Return Rc And Output
    ...     ls -l ${files_as_args}
    Should Be Equal As Integers  ${rc}  0
    Log   ${output}
    ${after} =   Set Variable   ${output}
    Should Be Equal   ${before}   ${after}

    Remove Files   @{files}

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
    Remove Files
    ...   ${IDE_DIR}/${IDE_NAME}
    ...   ${IDE_DIR}/${IDE2_NAME}
    ...   ${IDE_DIR}/${IDE3_NAME}
    Install With Base SDDS

Installer Suite TearDown
    No Operation

Installer Test Setup
    Register On Fail  Debug install set
    Register On Fail  dump log  ${THREAT_DETECTOR_LOG_PATH}
    Register On Fail  dump log  ${AV_LOG_PATH}
    Check AV Plugin Installed With Base
    Mark AV Log

Installer Test TearDown
    Run Teardown Functions
    Run Keyword If Test Failed   Installer Suite Setup

Debug install set
    ${result} =  run process  find  ${COMPONENT_INSTALL_SET}/files/plugins/av/chroot/susi/update_source  -type  f  stdout=/tmp/proc.out   stderr=STDOUT
    Log  INSTALL_SET= ${result.stdout}
    ${result} =  run process  find  ${SOPHOS_INSTALL}/plugins/av/chroot/susi/update_source  -type  f   stdout=/tmp/proc.out    stderr=STDOUT
    Log  INSTALLATION= ${result.stdout}
    ${result} =  run process  find  ${SOPHOS_INSTALL}/plugins/av/chroot/susi/distribution_version   stdout=/tmp/proc.out    stderr=STDOUT
    Log  INSTALLATION= ${result.stdout}

Force Sophos Threat Detector to restart
    Restart sophos_threat_detector

Kill sophos_threat_detector
   ${rc}   ${output} =    Run And Return Rc And Output    pgrep sophos_threat
   Run Process   /bin/kill   -9   ${output}

Restart sophos_threat_detector
    Mark AV Log
    Kill sophos_threat_detector

    # Existing robot functions don't check marked logs, so we do our own log check instead
    # Check Plugin Installed and Running
    Wait Until Sophos Threat Detector Log Contains With Offset
    ...   UnixSocket <> Starting listening on socket
    ...   timeout=40

Modify manifest
    Append To File   ${COMPONENT_ROOT_PATH}/var/manifest.dat   "junk"