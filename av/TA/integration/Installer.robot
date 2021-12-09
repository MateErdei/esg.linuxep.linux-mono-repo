*** Settings ***
Documentation    Integration tests of Installer
Force Tags       INTEGRATION  INSTALLER

Resource    ../shared/ComponentSetup.robot
Resource    ../shared/AVAndBaseResources.robot
Resource    ../shared/AVResources.robot
Resource    ../shared/BaseResources.robot
Resource    ../shared/ErrorMarkers.robot

Library         Collections
Library         Process
Library         ../Libs/LogUtils.py
Library         ../Libs/OnFail.py
Library         ../Libs/OSUtils.py
Library         ../Libs/ProcessUtils.py
Library         ../Libs/LockFile.py
Library         ../Libs/Telemetry.py

Suite Setup     Installer Suite Setup
Suite Teardown  Installer Suite TearDown

Test Setup      Installer Test Setup
Test Teardown   Installer Test TearDown

*** Test Cases ***

IDE update doesnt restart av processes
    ${AVPLUGIN_PID} =  Record AV Plugin PID
    ${SOPHOS_THREAT_DETECTOR_PID} =  Record Sophos Threat Detector PID
    Install IDE without SUSI loaded  ${IDE_NAME}
    Check AV Plugin Has Same PID  ${AVPLUGIN_PID}
    Check Sophos Threat Detector Has Same PID  ${SOPHOS_THREAT_DETECTOR_PID}

    # Check we can detect EICAR following update
    Check avscanner can detect eicar

    # Check we can detect PEEND following update
    # This test also proves that SUSI is configured to scan executables
    Check Threat Detected  peend.exe  PE/ENDTEST

IDE update only copies updated ide
    [Tags]  DISABLED
    # /proc/diskstats sda
    ${AVPLUGIN_PID} =  Record AV Plugin PID
    ${SOPHOS_THREAT_DETECTOR_PID} =  Record Sophos Threat Detector PID
    ${WRITTEN_TO_DISK_BEFORE} =  Get Amount Written To Disk
    Install IDE  ${IDE_NAME}
    Check AV Plugin Has Same PID  ${AVPLUGIN_PID}
    Check Sophos Threat Detector Has Same PID  ${SOPHOS_THREAT_DETECTOR_PID}
    ${WRITTEN_TO_DISK_AFTER} =  Get Amount Written To Disk
    ${WRITTEN_TO_DISK_DURING} =  Evaluate  ${WRITTEN_TO_DISK_AFTER} - ${WRITTEN_TO_DISK_BEFORE}
    Should Be True  ${WRITTEN_TO_DISK_DURING} < 10000


Restart then Update Sophos Threat Detector
    Mark Sophos Threat Detector Log
    Restart sophos_threat_detector
    Check Plugin Installed and Running
    Wait Until Sophos Threat Detector Log Contains With Offset
    ...   UnixSocket <> Starting listening on socket
    ...   timeout=60
    ${SOPHOS_THREAT_DETECTOR_PID} =  Wait For Pid  ${SOPHOS_THREAT_DETECTOR_BINARY}
    Install IDE without reload check  ${IDE_NAME}
    Check Sophos Threat Detector Has Same PID  ${SOPHOS_THREAT_DETECTOR_PID}

    # Check we can detect PEEND following update
    Wait Until Keyword Succeeds
        ...    60 secs
        ...    1 secs
        ...    Check Threat Detected  peend.exe  PE/ENDTEST

    Check Sophos Threat Detector Has Same PID  ${SOPHOS_THREAT_DETECTOR_PID}

    # Extra log dump to check we have the right events happening
    dump log  ${THREAT_DETECTOR_LOG_PATH}
    Mark SPPLAV Processes Are Killed With SIGKILL

Update then Restart Sophos Threat Detector
    ${SOPHOS_THREAT_DETECTOR_PID} =  Wait For Pid  ${SOPHOS_THREAT_DETECTOR_BINARY}
    Mark Sophos Threat Detector Log
    Install IDE without reload check  ${IDE_NAME}
    Check Sophos Threat Detector Has Same PID  ${SOPHOS_THREAT_DETECTOR_PID}
    Restart sophos_threat_detector
    Check Plugin Installed and Running
    Wait Until Sophos Threat Detector Log Contains With Offset
    ...   UnixSocket <> Starting listening on socket
    ...   timeout=60

    # Check we can detect PEEND following update
    Wait Until Keyword Succeeds
        ...    60 secs
        ...    1 secs
        ...    Check Threat Detected  peend.exe  PE/ENDTEST

    # Extra log dump to check we have the right events happening
    dump log  ${THREAT_DETECTOR_LOG_PATH}
    Mark SPPLAV Processes Are Killed With SIGKILL

Installer doesnt try to create an existing user
    Modify manifest
    Install AV Directly from SDDS

    File Log Should Not Contain  ${AV_INSTALL_LOG}  useradd: user 'sophos-spl-av' already exists


Scanner works after upgrade
    Mark AV Log
    Mark Sophos Threat Detector Log

    # modify the manifest to force the installer to perform a full product update
    Modify manifest
    Run Installer From Install Set

    # Existing robot functions don't check marked logs, so we do our own log check instead
    Check Plugin Installed and Running With Offset
    Wait Until Sophos Threat Detector Log Contains With Offset
    ...   UnixSocket <> Starting listening on socket
    ...   timeout=60
    Wait Until AV Plugin Log Contains With Offset
    ...   Starting threatReporter
    ...   timeout=60

    Mark AV Log
    Mark Sophos Threat Detector Log

    # Check we can detect EICAR following upgrade
    Check avscanner can detect eicar

    # check that the logs are still working (LINUXDAR-2535)
    Sophos Threat Detector Log Contains With Offset   EICAR-AV-Test
    AV Plugin Log Contains With Offset   EICAR-AV-Test
    Mark SPPLAV Processes Are Killed With SIGKILL

AV Plugin gets customer id after upgrade
    ${customerIdFile1} =   Set Variable   ${AV_PLUGIN_PATH}/var/customer_id.txt
    ${customerIdFile2} =   Set Variable   ${AV_PLUGIN_PATH}/chroot${customerIdFile1}
    Remove Files   ${customerIdFile1}   ${customerIdFile2}

    Send Alc Policy

    ${expectedId} =   Set Variable   a1c0f318e58aad6bf90d07cabda54b7d
    #A max of 10 seconds might pass before the threatDetector starts
    Wait Until Created   ${customerIdFile1}   timeout=12sec
    ${customerId1} =   Get File   ${customerIdFile1}
    Should Be Equal   ${customerId1}   ${expectedId}

    #A max of 10 seconds might pass before the threatDetector starts
    Wait Until Created   ${customerIdFile2}   timeout=12sec
    ${customerId2} =   Get File   ${customerIdFile2}
    Should Be Equal   ${customerId2}   ${expectedId}

    Wait Until Sophos Threat Detector Log Contains With Offset   UnixSocket <> Starting listening on socket  timeout=30

    # force an upgrade, check that customer id is set
    Mark Sophos Threat Detector Log

    Remove Files   ${customerIdFile1}   ${customerIdFile2}

    # modify the manifest to force the installer to perform a full product update
    Modify manifest
    Run Installer From Install Set

    #A max of 10 seconds might pass before the threatDetector starts
    Wait Until Created   ${customerIdFile1}   timeout=12sec
    ${customerId1} =   Get File   ${customerIdFile1}
    Should Be Equal   ${customerId1}   ${expectedId}

    #A max of 10 seconds might pass before the threatDetector starts
    Wait Until Created   ${customerIdFile2}   timeout=12sec
    ${customerId2} =   Get File   ${customerIdFile2}
    Should Be Equal   ${customerId2}   ${expectedId}

    Wait Until Sophos Threat Detector Log Contains With Offset   UnixSocket <> Starting listening on socket  timeout=30
    Mark Failed To connect To Warehouse Error
    Mark UpdateScheduler Fails


IDE can be removed
    Mark Sophos Threat Detector Log
    Restart sophos_threat_detector
    Check Plugin Installed and Running
    Wait Until Sophos Threat Detector Log Contains With Offset
    ...   UnixSocket <> Starting listening on socket: /var/process_control_socket
    ...   timeout=60
    File should not exist  ${INSTALL_IDE_DIR}/${ide_name}
    ${SOPHOS_THREAT_DETECTOR_PID} =  Record Sophos Threat Detector PID

    Install IDE without SUSI loaded  ${IDE_NAME}
    File should exist  ${INSTALL_IDE_DIR}/${ide_name}

    Uninstall IDE  ${IDE_NAME}
    File should not exist  ${INSTALL_IDE_DIR}/${ide_name}
    File should not exist  ${INSTALL_IDE_DIR}/${ide_name}.0
    File Should Not Exist   ${COMPONENT_ROOT_PATH}/chroot/susi/distribution_version/libsusi.so

    Check Sophos Threat Detector has same PID  ${SOPHOS_THREAT_DETECTOR_PID}

sophos_threat_detector can start after multiple IDE updates
    [Timeout]  10 minutes
    ${SOPHOS_THREAT_DETECTOR_PID} =  Record Sophos Threat Detector PID
    Register Cleanup  Installer Suite Setup

    Force SUSI to be initialized
    Sophos Threat Detector Log Contains With Offset  Initializing SUSI
    Sophos Threat Detector Log Contains With Offset  SUSI Libraries loaded:
    mark sophos threat detector log

    Install IDE with SUSI loaded  ${IDE_NAME}
    Sophos Threat Detector Log Contains With Offset  Threat scanner successfully updated
    Sophos Threat Detector Log Contains With Offset  SUSI Libraries loaded:
    mark sophos threat detector log
    Install IDE with SUSI loaded  ${IDE2_NAME}
    Install IDE with SUSI loaded  ${IDE3_NAME}
    # We need the updates to have actually updated SUSI
    File Should Not Exist   ${COMPONENT_ROOT_PATH}/chroot/susi/distribution_version/libsusi.so
    Check Sophos Threat Detector has same PID  ${SOPHOS_THREAT_DETECTOR_PID}
    Restart sophos_threat_detector
    Check Plugin Installed and Running
    Wait Until Sophos Threat Detector Log Contains With Offset
    ...   UnixSocket <> Starting listening on socket
    ...   timeout=60
    dump log  ${THREAT_DETECTOR_LOG_PATH}

IDE update waits if lock already taken
    ${lockfile} =  Set Variable  ${COMPONENT_ROOT_PATH}/chroot/var/susi_update.lock
    ${timeout} =  Set Variable  1
    Open And Acquire Lock   ${lockfile}
    Register Cleanup  Release Lock
    Register On Fail  dump log  /tmp/proc.out
    ${result} =  run process    bash  -x  ${COMPONENT_INSTALL_SET}/install.sh  stdout=/tmp/proc.out  stderr=STDOUT  env:LOCKFILE_TIMEOUT=${timeout}
    Log  ${result.stdout}
    Should Contain  ${result.stdout}  Failed to obtain a lock on ${lockfile} within ${timeout} seconds
    Should Be Equal As Integers  ${result.rc}  ${25}


Check install permissions
    [Documentation]   Find files or directories owned by sophos-spl-user or writable by sophos-spl-group.
    ...               Check results against an allowed list.
    ${rc}   ${output} =    Run And Return Rc And Output
    ...     find ${COMPONENT_ROOT_PATH} -! -type l -\\( -user sophos-spl-av -o -group sophos-spl-group -perm -0020 -\\) -prune
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
    ...     find ${COMPONENT_ROOT_PATH} -name susi -prune -o -type d -\\( -user sophos-spl-av -perm -0200 -o -group sophos-spl-group -perm -0020 -\\) -print
    Should Be Equal As Integers  ${rc}  0
    @{items} =    Split To Lines   ${output}
    Sort List   ${items}
    Log List   ${items}

    # create writable files
    @{files} =   Create List
    FOR   ${item}   IN   @{items}
       ${file} =   Set Variable   ${item}/test_file
       Create File   ${file}   test data
       Change Owner   ${file}   sophos-spl-av   sophos-spl-group
       Append To List   ${files}   ${file}
    END
    Log List   ${files}
    ${files_as_args} =   Catenate   @{files}

    Change Owner   ${COMPONENT_ROOT_PATH}/chroot/log/test_file  sophos-spl-threat-detector   sophos-spl-group
    Change Owner   ${COMPONENT_ROOT_PATH}/chroot/etc/test_file  sophos-spl-threat-detector   sophos-spl-group
    Change Owner   ${COMPONENT_ROOT_PATH}/chroot/${SOPHOS_INSTALL}/base/etc/test_file  sophos-spl-threat-detector   sophos-spl-group
    Change Owner   ${COMPONENT_ROOT_PATH}/chroot/${SOPHOS_INSTALL}/base/update/var/test_file  sophos-spl-threat-detector   sophos-spl-group
    Change Owner   ${COMPONENT_ROOT_PATH}/chroot/${COMPONENT_ROOT_PATH}/log/test_file  sophos-spl-threat-detector   sophos-spl-group
    Change Owner   ${COMPONENT_ROOT_PATH}/chroot/${COMPONENT_ROOT_PATH}/test_file  sophos-spl-threat-detector   sophos-spl-group


    # store current permissions for our files
    ${rc}   ${output} =    Run And Return Rc And Output
    ...     ls -l ${files_as_args}
    Log   ${output}
    Should Be Equal As Integers  ${rc}  0
    ${before} =   Set Variable   ${output}

    # modify the manifest to force the installer to perform a full product update
    Modify manifest
    Run Installer From Install Set

    # check our files are still writable
    ${rc}   ${output} =    Run And Return Rc And Output
    ...     ls -l ${files_as_args}
    Log   ${output}
    Should Be Equal As Integers  ${rc}  0
    ${after} =   Set Variable   ${output}
    Should Be Equal   ${before}   ${after}

    Remove Files   @{files}

Check no dangling symlinks
    ${rc}   ${output} =    Run And Return Rc And Output
    ...     find ${COMPONENT_ROOT_PATH} -xtype l
    Should Be Equal As Integers  ${rc}  ${0}
    Log    ${output}
    ${count} =   Get Line Count   ${output}
    Should Be Equal As Integers  ${count}  ${0}

Check logging symlink
    Remove File   ${AV_PLUGIN_PATH}/chroot/log/testfile
    Should not exist   ${AV_PLUGIN_PATH}/chroot/log/testfile
    Create file   ${CHROOT_LOGGING_SYMLINK}/testfile
    Should exist   ${AV_PLUGIN_PATH}/chroot/log/testfile

Check no duplicate virus data files
    Mark Sophos Threat Detector Log
    Restart sophos_threat_detector
    Check Plugin Installed and Running
    Wait Until Sophos Threat Detector Log Contains With Offset
    ...   UnixSocket <> Starting listening on socket: /var/process_control_socket
    ...   timeout=60
    ${rc}   ${susiHash} =    Run And Return Rc And Output   head -n 1 ${COMPONENT_ROOT_PATH}/chroot/susi/update_source/package_manifest.txt
    Should Be Equal As Integers  ${rc}  ${0}
    ${vdlInUse} =   Set Variable  ${COMPONENT_ROOT_PATH}/chroot/susi/distribution_version/${susiHash}/vdb
    ${vdlUpdate} =  Set Variable  ${COMPONENT_ROOT_PATH}/chroot/susi/update_source/vdl

    # Check there are no symlinks from a versioned copy
    Check no symlinks in directory  ${vdlInUse}
    Check no symlinks in directory  ${vdlUpdate}

    # Check there are no .0 duplicate files
    Check no duplicate files in directory  ${vdlInUse}
    Check no duplicate files in directory  ${vdlUpdate}

    Install IDE without SUSI loaded  ${IDE_NAME}

    # Check there are still no symlinks or .0 duplicate files
    Check no symlinks in directory  ${vdlUpdate}
    Check no duplicate files in directory  ${vdlUpdate}

Check installer corrects permissions of var directory on upgrade
    Register On Fail  dump watchdog log
    Mark Watchdog Log
    ${customerIdFile} =  Set Variable  ${COMPONENT_ROOT_PATH}/var/customer_id.txt
    Create file   ${customerIdFile}
    Change Owner  ${customerIdFile}  sophos-spl-user  sophos-spl-group
    Modify manifest
    Install AV Directly from SDDS
    Send Alc Policy

    File Log Does Not Contain  Check Marked Watchdog Log Contains  Failed to create file: '${customerIdFile}', Permission denied

    ${rc}   ${output} =    Run And Return Rc And Output   find ${AV_PLUGIN_PATH} -user sophos-spl-user -print
    Should Be Equal As Integers  ${rc}  0
    Should Be Empty  ${output}

    # Request a scan in order to load SUSI
    Create File     ${NORMAL_DIRECTORY}/eicar.com    ${EICAR_STRING}
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/
    Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}
    Threat Detector Does Not Log Contain  Failed to read customerID - using default value
    Mark Failed To connect To Warehouse Error
    Mark UpdateScheduler Fails
    Mark SPPLAV Processes Are Killed With SIGKILL

Check installer corrects permissions of logs directory on upgrade
    Register On Fail  dump watchdog log
    Mark Watchdog Log
    Change Owner  ${AV_LOG_PATH}  sophos-spl-user  sophos-spl-group
    Change Owner  ${THREAT_DETECTOR_LOG_PATH}  sophos-spl-user  sophos-spl-group
    Modify manifest
    Install AV Directly from SDDS
    Run Process  ${SOPHOS_INSTALL}/bin/wdctl  stop  av
    Run Process  ${SOPHOS_INSTALL}/bin/wdctl  start  av

    File Log Does Not Contain  Check Marked Watchdog Log Contains  log4cplus:ERROR Unable to open file: ${AV_LOG_PATH}

    ${rc}   ${output} =    Run And Return Rc And Output   find ${AV_PLUGIN_PATH} -user sophos-spl-user -print
    Should Be Equal As Integers  ${rc}  0
    Should Be Empty  ${output}
    Mark Failed To connect To Warehouse Error
    Mark UpdateScheduler Fails
    Mark SPPLAV Processes Are Killed With SIGKILL

Check installer corrects permissions of chroot files on upgrade
    Register On Fail  dump watchdog log
    Mark Watchdog Log
    Change Owner  ${AV_LOG_PATH}  sophos-spl-user  sophos-spl-group
    Change Owner  ${THREAT_DETECTOR_LOG_PATH}  sophos-spl-user  sophos-spl-group
    ${customerIdFile} =  Set Variable  ${COMPONENT_ROOT_PATH}/chroot/opt/sophos-spl/plugins/av/var/customer_id.txt
    Create file   ${customerIdFile}  0123456789ABCDEF0123456789ABCDEF
    Change Owner  ${customerIdFile}  sophos-spl-user  sophos-spl-group
    Run  chmod go-rwx ${customerIdFile}
    Change Owner  ${COMPONENT_ROOT_PATH}/chroot/opt/sophos-spl/plugins/av/VERSION.ini  sophos-spl-user  sophos-spl-group
    ${loggerConfFile} =  Set Variable  ${COMPONENT_ROOT_PATH}/chroot/opt/sophos-spl/plugins/base/etc/logger.conf
    Create file   ${loggerConfFile}
    Change Owner  ${loggerConfFile}  sophos-spl-user  sophos-spl-group
    ${machineIDFile} =  Set Variable  ${COMPONENT_ROOT_PATH}/chroot/opt/sophos-spl/plugins/base/etc/machine_id.txt
    Create file   ${machineIDFile}
    Change Owner  ${machineIDFile}  sophos-spl-user  sophos-spl-group
    Change Owner  ${COMPONENT_ROOT_PATH}/chroot/etc/host.conf  sophos-spl-user  sophos-spl-group
    Change Owner  ${COMPONENT_ROOT_PATH}/chroot/etc/nsswitch.conf  sophos-spl-user  sophos-spl-group
    Change Owner  ${COMPONENT_ROOT_PATH}/chroot/etc/resolv.conf  sophos-spl-user  sophos-spl-group
    Change Owner  ${COMPONENT_ROOT_PATH}/chroot/etc/ld.so.cache  sophos-spl-user  sophos-spl-group
    Change Owner  ${COMPONENT_ROOT_PATH}/chroot/etc/hosts  sophos-spl-user  sophos-spl-group
    Modify manifest
    Install AV Directly from SDDS
    ${rc}  ${output} =  Run And Return Rc And Output  ls -l ${COMPONENT_ROOT_PATH}/chroot/opt/sophos-spl/plugins/av/var/
    Log  ${output}

    ${rc}   ${output} =    Run And Return Rc And Output   find ${AV_PLUGIN_PATH} -user sophos-spl-user -print
    Should Be Equal As Integers  ${rc}  0
    Should Be Empty  ${output}

    # Request a scan in order to load SUSI
    Create File     ${NORMAL_DIRECTORY}/eicar.com    ${EICAR_STRING}
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/
    Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}
    Threat Detector Does Not Log Contain  Failed to read customerID - using default value

Check installer can handle versioned copied Virus Data from 1.0.0
    # Simulate the versioned copied Virus Data that exists in a 1.0.0 install
    Empty Directory  ${AV_PLUGIN_PATH}/chroot/susi/update_source/vdl

    ${cwd} =  get cwd then change directory  ${AV_SDDS}
    Register Cleanup  get cwd then change directory  ${cwd}
    ${rc}   ${output} =    Run And Return Rc And Output
    ...     find "files/plugins/av/chroot/susi/update_source/vdl" -type f -print0 | xargs -0 ${SOPHOS_INSTALL}/base/bin/versionedcopy

    Modify manifest
    Install AV Directly from SDDS

    ${number_of_VDL_files}   Count Files In Directory   ${AV_PLUGIN_PATH}/chroot/susi/update_source/vdl
    Should Be True   ${number_of_VDL_files} > 1

AV Plugin Can Send Telemetry After IDE Update
    Mark Sophos Threat Detector Log
    Restart sophos_threat_detector
    Check Plugin Installed and Running
    Wait Until Sophos Threat Detector Log Contains With Offset
    ...   UnixSocket <> Starting listening on socket: /var/process_control_socket
    ...   timeout=60
    Run  chmod go-rwx ${AV_PLUGIN_PATH}/chroot/susi/update_source
    ${AVPLUGIN_PID} =  Record AV Plugin PID
    ${SOPHOS_THREAT_DETECTOR_PID} =  Record Sophos Threat Detector PID
    Install IDE without SUSI loaded  ${IDE_NAME}
    Check AV Plugin Has Same PID  ${AVPLUGIN_PID}
    Check Sophos Threat Detector Has Same PID  ${SOPHOS_THREAT_DETECTOR_PID}

    ${rc}   ${output} =    Run And Return Rc And Output
    ...     ls -l ${AV_PLUGIN_PATH}/chroot/susi/update_source/
    Log To Console  ${output}
    Prepare To Run Telemetry Executable

    Run Telemetry Executable     ${EXE_CONFIG_FILE}     0
    Wait Until Keyword Succeeds
             ...  10 secs
             ...  1 secs
             ...  File Should Exist  ${TELEMETRY_OUTPUT_JSON}

    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Log  ${telemetryFileContents}
    Check Telemetry  ${telemetryFileContents}
    ${telemetryLogContents} =  Get File    ${TELEMETRY_EXECUTABLE_LOG}
    Should Contain   ${telemetryLogContents}    Gathered telemetry for av

AV Plugin Can Send Telemetry After Upgrade
    Mark Sophos Threat Detector Log
    Restart sophos_threat_detector
    Check Plugin Installed and Running
    Wait Until Sophos Threat Detector Log Contains With Offset
    ...   UnixSocket <> Starting listening on socket: /var/process_control_socket
    ...   timeout=60
    Run  chmod go-rwx ${AV_PLUGIN_PATH}/chroot/susi/update_source/*

    Modify manifest
    Install AV Directly from SDDS

    ${rc}   ${output} =    Run And Return Rc And Output
    ...     ls -l ${AV_PLUGIN_PATH}/chroot/susi/update_source/
    Log To Console  ${output}
    Prepare To Run Telemetry Executable

    Run Telemetry Executable     ${EXE_CONFIG_FILE}     0
    Wait Until Keyword Succeeds
             ...  10 secs
             ...  1 secs
             ...  File Should Exist  ${TELEMETRY_OUTPUT_JSON}

    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Log  ${telemetryFileContents}
    Check Telemetry  ${telemetryFileContents}
    ${telemetryLogContents} =  Get File    ${TELEMETRY_EXECUTABLE_LOG}
    Should Contain   ${telemetryLogContents}    Gathered telemetry for av

AV Plugin Restores Downgrade Logs
    Run plugin uninstaller with downgrade flag
    Check AV Plugin Not Installed
    Install AV Directly from SDDS
    Directory Should Exist  ${AV_RESTORED_LOGS_DIRECTORY}
    File Should Exist  ${AV_RESTORED_LOGS_DIRECTORY}/av.log
    File Should Exist  ${AV_RESTORED_LOGS_DIRECTORY}/sophos_threat_detector.log

AV Can not install from SDDS Component
    ${result} =  Run Process  bash  ${COMPONENT_SDDS_COMPONENT}/install.sh  stderr=STDOUT  timeout=30s
    Log  ${result.stdout}
    Should Be Equal As Integers  ${result.rc}  ${26}

Check installer keeps SUSI startup settings as writable by AV Plugin
    Restart AV Plugin And Clear The Logs For Integration Tests
    Create file   ${SUSI_STARTUP_SETTINGS_FILE_CHROOT}
    Change Owner  ${SUSI_STARTUP_SETTINGS_FILE_CHROOT}  sophos-spl-av  sophos-spl-group
    Modify manifest
    Install AV Directly from SDDS
    ${rc}   ${output} =    Run And Return Rc And Output
    ...     ls -l ${SUSI_STARTUP_SETTINGS_FILE_CHROOT}
    Log   ${output}

    Mark AV Log
    Send Sav Policy To Base  SAV_Policy.xml
    Wait Until AV Plugin Log Contains With Offset  Processing SAV Policy
    AV Plugin Log Does Not Contain With Offset  Failed to create file
    Mark Invalid Day From Policy Error

Check installer removes sophos_threat_detector log symlink
    Run Process   ln  -snf  ${COMPONENT_ROOT_PATH}/log/sophos_threat_detector/sophos_threat_detector.log  ${COMPONENT_ROOT_PATH}/log/sophos_threat_detector.log
    File Should Exist  ${COMPONENT_ROOT_PATH}/log/sophos_threat_detector.log
    # modify the manifest to force the installer to perform a full product update
    Modify manifest
    Run Installer From Install Set

    File Should Not Exist  ${COMPONENT_ROOT_PATH}/log/sophos_threat_detector.log
    Mark Invalid Day From Policy Error

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
    Register On Fail  dump log  ${SUSI_DEBUG_LOG_PATH}
    Register On Fail  dump log  ${AV_LOG_PATH}
    Register On Fail  dump log  ${SOPHOS_INSTALL}/logs/base/watchdog.log
    Check Plugin Installed and Running
    Mark AV Log
    Mark Sophos Threat Detector Log

Installer Test TearDown
    Run Teardown Functions
    Mark CustomerID Failed To Read Error
    Mark MCS Router is dead
    Check All Product Logs Do Not Contain Error
    Run Keyword If Test Failed   Installer Suite Setup

Debug install set
    ${result} =  run process  find  ${COMPONENT_INSTALL_SET}/files/plugins/av/chroot/susi/update_source  -type  f  stdout=/tmp/proc.out   stderr=STDOUT
    Log  INSTALL_SET= ${result.stdout}
    ${result} =  run process  find  ${SOPHOS_INSTALL}/plugins/av/chroot/susi/update_source  -type  f   stdout=/tmp/proc.out    stderr=STDOUT
    Log  INSTALLATION= ${result.stdout}
    ${result} =  run process  find  ${SOPHOS_INSTALL}/plugins/av/chroot/susi/distribution_version   stdout=/tmp/proc.out    stderr=STDOUT
    Log  INSTALLATION= ${result.stdout}

Kill sophos_threat_detector
    [Arguments]  ${signal}=9
    ${rc}   ${output} =    Run And Return Rc And Output    pgrep sophos_threat
    Run Process   /bin/kill   -${signal}   ${output}

Modify manifest
    Append To File   ${COMPONENT_ROOT_PATH}/var/manifest.dat   "junk"

Check no symlinks in directory
    [Arguments]  ${dirToCheck}
    ${rc}   ${output} =    Run And Return Rc And Output
    ...     find ${dirToCheck} -type l
    Should Be Equal As Integers  ${rc}  ${0}
    Log    ${output}
    ${count} =   Get Line Count   ${output}
    Should Be Equal As Integers  ${count}  ${0}

Check no duplicate files in directory
    [Arguments]  ${dirToCheck}
    ${rc}   ${output} =    Run And Return Rc And Output
    ...     find ${dirToCheck} -type f -name "*.0"
    Should Be Equal As Integers  ${rc}  ${0}
    Log    ${output}
    ${count} =   Get Line Count   ${output}
    Should Be Equal As Integers  ${count}  ${0}
