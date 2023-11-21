*** Settings ***
Documentation    Integration tests of Installer
Force Tags       INTEGRATION  INSTALLER  TAP_PARALLEL3

Resource    ../shared/ComponentSetup.robot
Resource    ../shared/AVAndBaseResources.robot
Resource    ../shared/AVResources.robot
Resource    ../shared/BaseResources.robot
Resource    ../shared/ErrorMarkers.robot
Resource    ../shared/OnAccessResources.robot
Resource    ../shared/SafeStoreResources.robot

Library         Collections
Library         Process
Library         ../Libs/CoreDumps.py
Library         ../Libs/FileUtils.py
Library         ../Libs/FullInstallerUtils.py
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

*** Variables ***

${MOCKED_INSTALL_SET}    /tmp/mocked_install_set

*** Test Cases ***
AV Plugin Installs With Version Ini File
    Wait Until AV Plugin running
    File Should Exist   ${SOPHOS_INSTALL}/plugins/av/VERSION.ini
    VERSION Ini File Contains Proper Format   ${SOPHOS_INSTALL}/plugins/av/VERSION.ini

IDE update doesnt restart av processes
    ${AVPLUGIN_PID} =  Record AV Plugin PID
    ${SOPHOS_THREAT_DETECTOR_PID} =  Record Sophos Threat Detector PID
    ${SOAPD_PID} =  Record Soapd Plugin PID
    ${oa_mark} =  Get On Access Log Mark
    ${threat_detector_mark} =  Get Sophos Threat Detector Log Mark
    Replace Virus Data With Test Dataset A And Run IDE update without SUSI loaded
    Check AV Plugin Has Same PID  ${AVPLUGIN_PID}
    Check Sophos Threat Detector Has Same PID  ${SOPHOS_THREAT_DETECTOR_PID}
    Check Soapd Plugin has same PID  ${SOAPD_PID}

    Wait For Sophos Threat Detector Log Contains After Mark  Notify clients that the update has completed  ${threat_detector_mark}
    Wait For On Access Log Contains After Mark  Clearing on-access cache  ${oa_mark}

    # Check we can detect EICAR following update
    Check avscanner can detect eicar

    # Check we can detect PEEND following update
    # This test also proves that SUSI is configured to scan executables
    Check Threat Detected  peend.exe  PE/ENDTEST

Installer Does Not Confuse Sophos Threat Detector For A Similar Named Process
    [Tags]  Fault Injection

    ${handle}=     Start Process   while :; do echo "I am pretending to be sophos_threat_d"; sleep 1000;done  shell=True  alias=sophos_threat_d

    #If the installer confuses the process above for threat_detector it will never send a signal to the real process and the keyword will fail
    Force SUSI to be initialized
    Replace Virus Data With Test Dataset A And Run IDE update with SUSI loaded  setOldTimestamps=${True}

    Process should Be Running   handle=${handle}
    Terminate Process   ${handle}


IDE update copies updated ide
    Force SUSI to be initialized
    ${AVPLUGIN_PID} =  Record AV Plugin PID
    ${SOPHOS_THREAT_DETECTOR_PID} =  Record Sophos Threat Detector PID
    Run Shell Process  touch ${SOPHOS_INSTALL}/mark  OnError=failed to mark
    Replace Virus Data With Test Dataset A And Run IDE update with SUSI loaded  setOldTimestamps=${True}
    Check AV Plugin Has Same PID  ${AVPLUGIN_PID}
    Check Sophos Threat Detector Has Same PID  ${SOPHOS_THREAT_DETECTOR_PID}
    ${result} =  Run Shell Process  find ${SOPHOS_INSTALL} -type f -newer ${SOPHOS_INSTALL}/mark | xargs ls -ilhd  OnError=find failed   timeout=60s
    Log  ${result.stdout}
    ${result} =  Run Shell Process  find ${SOPHOS_INSTALL} -type f -newer ${SOPHOS_INSTALL}/mark | xargs du -cb  OnError=find failed   timeout=60s
    Log  ${result.stdout}
    ${lastline} =  Fetch From Right    ${result.stdout}  \n
    ${WRITTEN_TO_DISK_DURING} =  Fetch From Left    ${lastline}  \t
    # SUSI copies libraries every time, so expect up to 200MB
    Should Be True  ${WRITTEN_TO_DISK_DURING} < ${ 200 * 1024 ** 2 }

Restart then Update Sophos Threat Detector
    Check file clean   peend.exe

    Restart Sophos Threat Detector

    ${SOPHOS_THREAT_DETECTOR_PID} =  Wait For Pid  ${SOPHOS_THREAT_DETECTOR_BINARY}
    Replace Virus Data With Test Dataset A And Run IDE update without SUSI loaded
    Check Sophos Threat Detector Has Same PID  ${SOPHOS_THREAT_DETECTOR_PID}

    # Check we can detect PEEND following update
    Check Threat Detected  peend.exe  PE/ENDTEST

    Check Sophos Threat Detector Has Same PID  ${SOPHOS_THREAT_DETECTOR_PID}

    # Extra log dump to check we have the right events happening
    dump log  ${THREAT_DETECTOR_LOG_PATH}

IDE update during command line scan
    # Assumes that /usr/share/ takes long enough to scan, and that all files take well under one second to be scanned.
    # If this proves to be false on any of our test systems, we'll need to create a dummy fileset to scan instead.
    ${threat_detector_mark} =  Get Sophos Threat Detector Log Mark

    ${scan_log} =   Set Variable  /tmp/cli.log
    ${cls_handle} =   Start Process  ${CLI_SCANNER_PATH}  /usr/share/  stdout=${scan_log}  stderr=STDOUT
    Register Cleanup  Remove File  ${scan_log}
    Register Cleanup  Dump Log  ${scan_log}
    Register Cleanup  Terminate Process  ${cls_handle}

    # ensure scan is running and that we have had a result from SUSI to prove it is initialised
    Wait For Sophos Threat Detector Log Contains After Mark  Scanning result details  ${threat_detector_mark}

    ${start_time} =   Get Current Date   time_zone=UTC   exclude_millis=True
    Sleep  1 second   Allow some scans to occur before the update

    # do virus data update, wait for log mesages
    Replace Virus Data With Test Dataset A And Run IDE update with SUSI loaded

    # ensure scan is still running
    ${threat_detector_mark2} =  Get Sophos Threat Detector Log Mark
    Wait For Sophos Threat Detector Log Contains After Mark  ScanningServerConnectionThread scan requested of  ${threat_detector_mark2}

    Sleep  1 second   Allow some scans to occur after the update
    ${end_time} =   Get Current Date   time_zone=UTC   exclude_millis=True

    Process should Be Running   handle=${cls_handle}
    ${result} =   Terminate Process   handle=${cls_handle}

    Dump Log  ${scan_log}

    # do some magic to check that we were scanning without interruption (at least 1 file scanned every second)
    ${time_diff} =   Subtract Date From Date   ${end_time}   ${start_time}   exclude_millis=True
    FOR   ${offset}   IN RANGE   ${time_diff}
        ${timestamp} =   Add Time To Date   ${start_time}   ${offset}   result_format=%H:%M:%S
        ${line_count} =  Count Lines In Log  ${scan_log}  [${timestamp}] Scanning \
        Should Be True   ${1} <= ${line_count}
    END

On access gets IDE update
    Send Policies to enable on-access

    On-access Scan Eicar Close
    On-access Scan Peend no detect

    Replace Virus Data With Test Dataset A And Run IDE update with SUSI loaded

    On-access Scan Eicar Close
    On-access Scan Peend

On access gets IDE update to all scanners
    Set number of scanning threads in integration test  10

    Send Policies to enable on-access
    On-access Scan Eicar Close

    On-access Scan Multiple Peend no detect

    Replace Virus Data With Test Dataset A And Run IDE update with SUSI loaded

    On-access Scan Multiple Peend

On access gets IDE update to new scanners
    Set number of scanning threads in integration test  10

    # Ensure on-access is disabled
    #    restart on-access to ensure scanners are not initialized
    #    Send Policies to disable on-access
    #    Sleep   0.5s   Wait for on-access to stop
    # exclude most directories so we don't use all the scanners until after the update
    Send Policies to enable on-access with exclusions
    On-access Scan Eicar Close

    On-access Scan Peend no detect

    Replace Virus Data With Test Dataset A And Run IDE update with SUSI loaded

    ${td_mark} =   get_sophos_threat_detector_log_mark
    On-access Scan Multiple Peend
    # ensure that we used at least one new scanner (for LINUXDAR-6018)
    check_sophos_threat_detector_log_contains_after_mark   Creating SUSI scanner   mark=${td_mark}
    check_sophos_threat_detector_log_contains_after_mark   Created SUSI scanner   mark=${td_mark}


On access continues during update
    Send Policies to enable on-access

    ${test_file} =   Set Variable   /tmp/testfile
    Register Cleanup   Remove File   ${test_file}

    # start a background process to cause ~10 on-access events per second
    ${handle} =   Start Process   while :; do echo foo >${test_file}; sleep 0.1; done   shell=True

    ${threat_detector_mark} =  Get Sophos Threat Detector Log Mark
    Wait For Sophos Threat Detector Log Contains After Mark     ScanningServerConnectionThread scan requested of ${test_file}  ${threat_detector_mark}

    ${start_time} =   Get Current Date   time_zone=UTC   exclude_millis=True
    Sleep   1s   let on-access do its thing

    Replace Virus Data With Test Dataset A And Run IDE update with SUSI loaded

    ${threat_detector_mark2} =  Get Sophos Threat Detector Log Mark
    Wait For Sophos Threat Detector Log Contains After Mark     ScanningServerConnectionThread scan requested of ${test_file}  ${threat_detector_mark2}

    Sleep   1s   let on-access do its thing
    ${end_time} =   Get Current Date   time_zone=UTC   exclude_millis=True

    # check the background process didn't stop prematurely, then terminate it.
    Process should Be Running   handle=${handle}
    ${result} =   Terminate Process   handle=${handle}

    # check how many scans per second we did during an IDE update
    # we write to a test file every 0.1 seconds so we should expect at least 1 scan
    ${time_diff} =   Subtract Date From Date   ${end_time}   ${start_time}   exclude_millis=True
    FOR   ${offset}   IN RANGE   ${time_diff}
        ${timestamp} =   Add Time To Date   ${start_time}   ${offset}   result_format=%H:%M:%S
        ${lines} =   Grep File   ${THREAT_DETECTOR_LOG_PATH}   T${timestamp}.* ScanningServerConnectionThread scan requested of
        ${line_count} =   Get Line Count   ${lines}
        Should Be True   ${1} <= ${line_count}
    END

Concurrent scans get pending update
    Check file clean   peend.exe

    # prepare the pending update
    Restart Sophos Threat Detector
    Replace Virus Data With Test Dataset A And Run IDE update without SUSI loaded

    # Check we can detect PEEND following update
    ${threat_file} =   Set Variable   ${RESOURCES_PATH}/file_samples/peend.exe
    ${threat_name} =   Set Variable   PE/ENDTEST

    ${cls_handle1} =     Start Process  ${AVSCANNER}  ${threat_file}   stderr=STDOUT
    ${cls_handle2} =     Start Process  ${AVSCANNER}  ${threat_file}   stderr=STDOUT

    ${result1} =   Wait for process  ${cls_handle1}
    ${result2} =   Wait for process  ${cls_handle2}

    Should Be Equal As Integers  ${result1.rc}  ${VIRUS_DETECTED_RESULT}
    Should Be Equal As Integers  ${result2.rc}  ${VIRUS_DETECTED_RESULT}

    Should Contain   ${result1.stdout}    Detected "${threat_file}" is infected with ${threat_name}
    Should Contain   ${result2.stdout}    Detected "${threat_file}" is infected with ${threat_name}

    # Extra log dump to check we have the right events happening
    dump log  ${THREAT_DETECTOR_LOG_PATH}

Update then Restart Sophos Threat Detector
    # ensure SUSI is initialized
    check avscanner can detect eicar

    ${SOPHOS_THREAT_DETECTOR_PID} =  Wait For Pid  ${SOPHOS_THREAT_DETECTOR_BINARY}
    Replace Virus Data With Test Dataset A And Run IDE update with SUSI loaded

    Check Sophos Threat Detector Has Same PID  ${SOPHOS_THREAT_DETECTOR_PID}
    ${threat_detector_mark} =  Get Sophos Threat Detector Log Mark
    Restart sophos_threat_detector
    Check Plugin Installed and Running
    Wait For Sophos Threat Detector Log Contains After Mark
    ...   UnixSocket <> ProcessControlServer starting listening on socket:
    ...   ${threat_detector_mark}
    ...   timeout=60

    # Check we can detect PEEND following update
    Wait Until Keyword Succeeds
        ...    60 secs
        ...    1 secs
        ...    Check Threat Detected  peend.exe  PE/ENDTEST

    # Extra log dump to check we have the right events happening
    dump log  ${THREAT_DETECTOR_LOG_PATH}

Update before Init then Restart Threat Detector
    Register Cleanup  Exclude Aborted Scan Errors

    # run a quick scan to make sure SUSI is loaded and up-to-date
    check avscanner can detect eicar

    # restart STD and create a pending update
    Restart Sophos Threat Detector
    ${threat_detector_mark} =  Get Sophos Threat Detector Log Mark
    Replace Virus Data With Test Dataset A And Run IDE update without SUSI loaded
    Wait For Sophos Threat Detector Log Contains After Mark   Threat scanner update is pending  ${threat_detector_mark}

    # start a scan in the background, to trigger SUSI init & update
    ${LOG_FILE} =       Set Variable   /tmp/scan.log
    Remove File  ${LOG_FILE}
    Register Cleanup    Remove File  ${LOG_FILE}
    Register Cleanup    Dump Log     ${LOG_FILE}
    ${cls_handle} =     Start Process  ${CLI_SCANNER_PATH}  /usr/
    ...   stdout=${LOG_FILE}   stderr=STDOUT

    Register Cleanup    Terminate Process  ${cls_handle}

    Wait For Sophos Threat Detector Log Contains After Mark   ScanningServerConnectionThread scan requested  ${threat_detector_mark}
    Wait For Sophos Threat Detector Log Contains After Mark   Initializing SUSI  ${threat_detector_mark}   timeout=30

    # try to restart as soon as SUSI starts to load
    Restart Sophos Threat Detector

    # check the state of our scan (it *should* work, eventually)
    Process Should Be Running   ${cls_handle}
    ${cls_mark} =  Mark Log Size   ${LOG_FILE}
    Wait For Log Contains From Mark  ${cls_mark}  Scanning  timeout=30

    # Stop CLS
    ${result} =   Terminate Process  ${cls_handle}
    deregister cleanup  Terminate Process  ${cls_handle}

    Should Contain  ${{ [ ${EXECUTION_INTERRUPTED}, ${SCAN_ABORTED_WITH_THREAT} ] }}   ${result.rc}

    # force a log dump, for investigation / debug purposes
    dump log  ${THREAT_DETECTOR_LOG_PATH}

Installer doesnt try to create an existing user
    Modify manifest
    Install AV Directly from SDDS

    File Log Should Not Contain  ${AV_INSTALL_LOG}  useradd: user 'sophos-spl-av' already exists


Scanner works after upgrade
    ${av_mark} =  Get AV Log Mark
    ${threat_detector_mark} =  Get Sophos Threat Detector Log Mark

    # modify the manifest to force the installer to perform a full product update
    Modify manifest
    Run Installer From Install Set

    # Existing robot functions don't check marked logs, so we do our own log check instead
    Check Plugin Installed and Running After Marks  ${av_mark}  ${threat_detector_mark}
    Wait For Sophos Threat Detector Log Contains After Mark
    ...   UnixSocket <> ProcessControlServer starting listening on socket:
    ...   ${threat_detector_mark}
    ...   timeout=60
    Wait For AV Log Contains After Mark
    ...   Starting threatReporter
    ...   ${av_mark}
    ...   timeout=60

    ${av_mark2} =  Get AV Log Mark
    ${threat_detector_mark2} =  Get Sophos Threat Detector Log Mark

    # Check we can detect EICAR following upgrade
    Check avscanner can detect eicar

    # check that the logs are still working (LINUXDAR-2535)
    Wait For Sophos Threat Detector Log Contains After Mark  EICAR-AV-Test  ${threat_detector_mark2}
    Wait For AV Log Contains After Mark  EICAR-AV-Test  ${av_mark2}

Services restarted after upgrade
    ${soapd_log_mark} =  Mark Log Size  ${ON_ACCESS_LOG_PATH}
    ${safestore_log_mark} =  Mark Log Size  ${SAFESTORE_LOG_PATH}

    ${old_soapd_pid} =  ProcessUtils.wait for pid  ${ON_ACCESS_BIN}  timeout=${5}
    ${old_safestore_pid} =  ProcessUtils.wait for pid  ${SAFESTORE_BIN}  timeout=${5}

    # modify the manifest to force the installer to perform a full product update
    Modify manifest
    Run Installer From Install Set

    ## Wait for soapd to be running
    LogUtils.wait for log contains after mark  ${ON_ACCESS_LOG_PATH}  configured for level:  ${soapd_log_mark}  timeout=${5}
    wait for log contains after mark  ${SAFESTORE_LOG_PATH}  configured for level:  ${safestore_log_mark}  timeout=${5}

    ${new_soapd_pid} =  ProcessUtils.wait for pid  ${ON_ACCESS_BIN}  timeout=${5}
    ${new_safestore_pid} =  ProcessUtils.wait for pid  ${SAFESTORE_BIN}  timeout=${5}

    Should Not Be Equal As Integers  ${old_soapd_pid}  ${new_soapd_pid}
    Should Not Be Equal As Integers  ${old_safestore_pid}  ${new_safestore_pid}

AV Plugin gets customer id after upgrade
    Register Cleanup    Exclude Failed To connect To Warehouse Error
    Register Cleanup    Exclude UpdateScheduler Fails
    ${customerIdFile} =   Set Variable   ${AV_PLUGIN_PATH}/var/customer_id.txt
    ${customerIdFile_chroot} =   Set Variable   ${AV_PLUGIN_PATH}/chroot${customerIdFile}
    Remove Files   ${customerIdFile}   ${customerIdFile_chroot}

    Send Alc Policy

    ${expectedId} =   Set Variable   a1c0f318e58aad6bf90d07cabda54b7d
    #A max of 10 seconds might pass before the threatDetector starts
    Wait Until File Contains  ${customerIdFile}  ${expectedId}  timeout=12sec

    #A max of 10 seconds might pass before the threatDetector starts
    Wait Until File Contains  ${customerIdFile_chroot}  ${expectedId}  timeout=12sec

    # force an upgrade, check that customer id is set
    Remove Files   ${customerIdFile}   ${customerIdFile_chroot}

    # modify the manifest to force the installer to perform a full product update
    Modify manifest
    Run Installer From Install Set

    #A max of 10 seconds might pass before the threatDetector starts
    Wait Until File Contains  ${customerIdFile}  ${expectedId}  timeout=12sec

    #A max of 10 seconds might pass before the threatDetector starts
    Wait Until File Contains  ${customerIdFile_chroot}  ${expectedId}  timeout=12sec

IDE can be removed
    ${threat_detector_mark} =  Get Sophos Threat Detector Log Mark
    Restart sophos_threat_detector
    Check Plugin Installed and Running
    Wait For Sophos Threat Detector Log Contains After Mark
    ...   UnixSocket <> ProcessControlServer starting listening on socket: /var/process_control_socket
    ...   ${threat_detector_mark}
    ...   timeout=60
    File should not exist  ${INSTALL_IDE_DIR}/${ide_name}
    ${SOPHOS_THREAT_DETECTOR_PID} =  Record Sophos Threat Detector PID

    Replace Virus Data With Test Dataset A
    Register Cleanup  Revert Virus Data and Run Update if Required
    Run IDE update without SUSI loaded
    File should exist  ${INSTALL_IDE_DIR}/${ide_name}

    Revert Virus Data To Live Dataset A
    Run IDE update without SUSI loaded

    File should not exist  ${INSTALL_IDE_DIR}/${ide_name}
    File should not exist  ${INSTALL_IDE_DIR}/${ide_name}.0
    File Should Not Exist   ${COMPONENT_ROOT_PATH}/chroot/susi/distribution_version/libsusi.so

    Check Sophos Threat Detector has same PID  ${SOPHOS_THREAT_DETECTOR_PID}

sophos_threat_detector can start after multiple IDE updates
    [Timeout]  10 minutes
    ${SOPHOS_THREAT_DETECTOR_PID} =  Record Sophos Threat Detector PID

    Force SUSI to be initialized
    # can't check for SUSI in logs, as it may have already been initialized

    ${threat_detector_mark} =  Get Sophos Threat Detector Log Mark

    Replace Virus Data With Test Dataset A
    Register Cleanup  Revert Virus Data and Run Update if Required
    Run IDE update with SUSI loaded
    Check Sophos Threat Detector Log Contains After Mark  Threat scanner successfully updated  ${threat_detector_mark}
    Check Sophos Threat Detector Log Contains After Mark  SUSI Libraries loaded:  ${threat_detector_mark}
    ${threat_detector_mark} =  Get Sophos Threat Detector Log Mark
    Revert Virus Data To Live Dataset A
    Run IDE update with SUSI loaded
    Replace Virus Data With Test Dataset A
    Run IDE update with SUSI loaded
    # We need the updates to have actually updated SUSI
    File Should Not Exist   ${COMPONENT_ROOT_PATH}/chroot/susi/distribution_version/libsusi.so
    Check Sophos Threat Detector has same PID  ${SOPHOS_THREAT_DETECTOR_PID}
    Restart sophos_threat_detector
    Check Plugin Installed and Running
    Wait For Sophos Threat Detector Log Contains After Mark
    ...   UnixSocket <> ProcessControlServer starting listening on socket:
    ...   ${threat_detector_mark}
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


Check group writable install permissions
    [Documentation]   Find files or directories owned by sophos-spl-user or writable by sophos-spl-group.
    ...               Check results against an allowed list.
    ${rc}   ${output} =    Run And Return Rc And Output
    ...     find ${COMPONENT_ROOT_PATH} -! -type l -\\( -user sophos-spl-av -o -group sophos-spl-group -perm -0020 -\\) -prune
    Should Be Equal As Integers  ${rc}  ${0}
    ${output} =   Replace String   ${output}   ${COMPONENT_ROOT_PATH}/   ${EMPTY}
    @{items} =    Split To Lines   ${output}
    Sort List   ${items}
    Log List   ${items}
    Should Not Be Empty   ${items}
    Remove Values From List    ${items}   @{GROUP_WRITE_ALLOWED_TO_WRITE}
    Log List   ${items}
    Should Be Empty   ${items}


Check world writable permissions
    ${rc}   ${output} =    Run And Return Rc And Output
    ...     find ${COMPONENT_ROOT_PATH} -! -type l -perm -0002
    Should Be Equal As Integers  ${rc}  ${0}
    ${output} =   Replace String Using Regexp   ${output}   /[^/]*\\.cov   ${EMPTY}
    @{items} =    Split To Lines   ${output}
    Log List   ${items}
    Should Not Be Empty   ${items}
    Remove Values From List    ${items}   @{WORLD_WRITE_ALLOWED_TO_WRITE}
    Log List   ${items}
    Should Be Empty   ${items}


Check permissions after upgrade
    # find writable directories
    ${rc}   ${output} =    Run And Return Rc And Output
    ...     find ${COMPONENT_ROOT_PATH} -name susi -prune -o -type d -\\( -user sophos-spl-av -perm -0200 -o -group sophos-spl-group -perm -0020 -\\) -print
    Should Be Equal As Integers  ${rc}  ${0}
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
    Should Be Equal As Integers  ${rc}  ${0}
    ${before} =   Set Variable   ${output}

    # modify the manifest to force the installer to perform a full product update
    Modify manifest
    Run Installer From Install Set

    # check our files are still writable
    ${rc}   ${output} =    Run And Return Rc And Output
    ...     ls -l ${files_as_args}
    Log   ${output}
    Should Be Equal As Integers  ${rc}  ${0}
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
    ${threat_detector_mark} =  Get Sophos Threat Detector Log Mark
    Restart sophos_threat_detector
    Check Plugin Installed and Running
    Wait For Sophos Threat Detector Log Contains After Mark
    ...   UnixSocket <> ProcessControlServer starting listening on socket: /var/process_control_socket
    ...   ${threat_detector_mark}
    ...   timeout=60
    ${rc}   ${susiHash} =    Run And Return Rc And Output   head -n 1 ${COMPONENT_ROOT_PATH}/chroot/susi/update_source/package_manifest.txt
    Should Be Equal As Integers  ${rc}  ${0}
    ${vdlInUse} =   Set Variable  ${COMPONENT_ROOT_PATH}/chroot/susi/distribution_version/${susiHash}/vdb
    ${vdlUpdate} =  Set Variable  ${COMPONENT_ROOT_PATH}/chroot/susi/update_source/vdl

    #force a bootstrap
    Check avscanner can detect eicar

    # Check there are no symlinks from a versioned copy
    Check no symlinks in directory  ${vdlInUse}
    Check no symlinks in directory  ${vdlUpdate}

    # Check there are no .0 duplicate files
    Check no duplicate files in directory  ${vdlInUse}
    Check no duplicate files in directory  ${vdlUpdate}

    Replace Virus Data With Test Dataset A And Run IDE update with SUSI loaded

    # Check there are still no symlinks or .0 duplicate files
    Check no symlinks in directory  ${vdlUpdate}
    Check no duplicate files in directory  ${vdlUpdate}

Check installer corrects permissions of logs directory on upgrade
    Register Cleanup    Exclude Failed To connect To Warehouse Error
    Register Cleanup    Exclude UpdateScheduler Fails

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
    Should Be Equal As Integers  ${rc}  ${0}
    Should Be Empty  ${output}

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
    ${threat_detector_mark} =  Get Sophos Threat Detector Log Mark
    stop sophos_threat_detector
    wait until threat detector not running
    Wait For Sophos Threat Detector Log Contains After Mark  Closing lock file  ${threat_detector_mark}
    #Product will log the chroot path, so only /var/threat_detector.pid will show up in the threat detector log
    ${tdPidFile} =  Set Variable  ${COMPONENT_ROOT_PATH}/chroot/var/threat_detector.pid
    Create file   ${tdPidFile}
    Change Owner  ${tdPidFile}  sophos-spl-user  sophos-spl-group
    Modify manifest
    ${threat_detector_mark2} =  Get Sophos Threat Detector Log Mark

    Install AV Directly from SDDS
    ${rc}  ${output} =  Run And Return Rc And Output  ls -l ${COMPONENT_ROOT_PATH}/chroot/opt/sophos-spl/plugins/av/var/
    Log  ${output}

    ${rc}   ${output} =    Run And Return Rc And Output   find ${AV_PLUGIN_PATH} -user sophos-spl-user -print
    Should Be Equal As Integers  ${rc}  ${0}
    Should Be Empty  ${output}

    # Request a scan in order to load SUSI
    Create File     ${NORMAL_DIRECTORY}/eicar.com    ${EICAR_STRING}
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}/
    Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}
    Check Sophos Threat Detector Log Does Not Contain After Mark  Failed to read customerID - using default value  ${threat_detector_mark2}

    #Product logs the chroot path
    Wait For Sophos Threat Detector Log Contains After Mark  Lock taken on: /var/threat_detector.pid  ${threat_detector_mark2}

Check installer can handle versioned copied Virus Data from 1-0-0
    # Simulate the versioned copied Virus Data that exists in a 1.0.0 install
    Empty Directory  ${AV_PLUGIN_PATH}/chroot/susi/update_source/vdl

    ${cwd} =  get cwd then change directory  ${AV_SDDS}
    Register Cleanup  get cwd then change directory  ${cwd}
    ${rc}   ${output} =    Run And Return Rc And Output
    ...     find "files/plugins/av/chroot/susi/update_source/vdl" -type f -print0 | xargs -0 ${SOPHOS_INSTALL}/base/bin/versionedcopy

    Modify manifest
    Install AV Directly from SDDS

    ${number_of_VDL_files}   Count Files In Directory   ${AV_PLUGIN_PATH}/chroot/susi/update_source/vdl
    Should Be True   ${number_of_VDL_files} > ${1}

AV Plugin Can Send Telemetry After IDE Update
    # Restarting threat_detector when OA is enabled can lead to some file scans being aborted
    Register Cleanup  Exclude Aborted Scan Errors
    #reset telemetry values
    Run Process  ${SOPHOS_INSTALL}/bin/wdctl  stop  av
    Remove File  ${SOPHOS_INSTALL}/base/telemetry/cache/av-telemetry.json
    Remove File  ${THREAT_DATABASE_PATH}
    Run Process  ${SOPHOS_INSTALL}/bin/wdctl  start  av

    Send Policies to enable on-access
    ${threat_detector_mark} =  Get Sophos Threat Detector Log Mark
    Restart sophos_threat_detector
    Check Plugin Installed and Running
    Wait For Sophos Threat Detector Log Contains After Mark
    ...   UnixSocket <> ProcessControlServer starting listening on socket: /var/process_control_socket
    ...   ${threat_detector_mark}
    ...   timeout=60
    Force SUSI to be initialized

    Run  chmod go-rwx ${AV_PLUGIN_PATH}/chroot/susi/update_source
    ${AVPLUGIN_PID} =  Record AV Plugin PID
    ${SOPHOS_THREAT_DETECTOR_PID} =  Record Sophos Threat Detector PID
    Replace Virus Data With Test Dataset A And Run IDE update with SUSI loaded
    Check AV Plugin Has Same PID  ${AVPLUGIN_PID}
    Check Sophos Threat Detector Has Same PID  ${SOPHOS_THREAT_DETECTOR_PID}

    ${rc}   ${output} =    Run And Return Rc And Output
    ...     ls -l ${AV_PLUGIN_PATH}/chroot/susi/update_source/
    Log  ${output}

    Run Telemetry Executable With HTTPS Protocol
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Log  ${telemetryFileContents}
    Check Telemetry  ${telemetryFileContents}
    ${telemetryLogContents} =  Get File    ${TELEMETRY_EXECUTABLE_LOG}
    Should Contain   ${telemetryLogContents}    Gathered telemetry for av

AV Plugin Can Send Telemetry After Upgrade
    # Restarting threat_detector when OA is enabled can lead to some file scans being aborted
    Register Cleanup  Exclude Aborted Scan Errors
    #reset telemetry values
    Run Process  ${SOPHOS_INSTALL}/bin/wdctl  stop  av
    Remove File  ${SOPHOS_INSTALL}/base/telemetry/cache/av-telemetry.json
    ${av_mark} =  Get AV Log Mark
    Run Process  ${SOPHOS_INSTALL}/bin/wdctl  start  av
    Wait until AV Plugin running after mark  ${av_mark}
    Send Policies to enable on-access

    ${threat_detector_mark} =  Get Sophos Threat Detector Log Mark
    Restart sophos_threat_detector
    Check Plugin Installed and Running
    Wait For Sophos Threat Detector Log Contains After Mark
    ...   UnixSocket <> ProcessControlServer starting listening on socket: /var/process_control_socket
    ...   ${threat_detector_mark}
    ...   timeout=60

    # Can no longer change permissions on the update_source, since TD requires group access to read update_source

    Modify manifest
    Install AV Directly from SDDS

    ${rc}   ${output} =    Run And Return Rc And Output
    ...     ls -l ${AV_PLUGIN_PATH}/chroot/susi/update_source/
    Log  ${output}

    Run Telemetry Executable With HTTPS Protocol

    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Log  ${telemetryFileContents}
    Check Telemetry  ${telemetryFileContents}
    ${telemetryLogContents} =  Get File    ${TELEMETRY_EXECUTABLE_LOG}
    Should Contain   ${telemetryLogContents}    Gathered telemetry for av

AV Plugin Restores Downgrade Logs
    Run plugin uninstaller with downgrade flag
    Directory Should Exist  ${AV_BACKUP_DIR}
    Check AV Plugin Not Installed
    Install AV Directly from SDDS
    Directory Should Exist  ${AV_RESTORED_LOGS_DIRECTORY}
    File Should Exist  ${AV_RESTORED_LOGS_DIRECTORY}/av.log
    File Should Exist  ${AV_RESTORED_LOGS_DIRECTORY}/sophos_threat_detector.log

AV Plugin Restores Downgrade SafeStore Databases
    Restart AV Plugin
    Wait Until File exists    ${THREAT_DATABASE_PATH}

    Run plugin uninstaller with downgrade flag
    Directory Should Exist  ${SAFESTORE_BACKUP_DIR}
    Check AV Plugin Not Installed
    Install AV Directly from SDDS

    # File will be removed so test it is put back
    Wait Until File exists  ${THREAT_DATABASE_PATH}
    Verify SafeStore Database Backups Exist in Path    ${AV_RESTORED_VAR_DIRECTORY}

AV Plugin Restores Older SafeStore Database On Upgrade
    Check AV Plugin Running
    Run plugin uninstaller with downgrade flag
    Check AV Plugin Not Installed

    Remove All But One SafeStore Backup
    ${safeStoreDatabaseBackupDirs} =    List Directories In Directory    ${AV_RESTORED_VAR_DIRECTORY}
    ${safeStoreDatabaseBackup} =    Set Variable    ${AV_RESTORED_VAR_DIRECTORY}/${safeStoreDatabaseBackupDirs[0]}

    Install AV Directly from SDDS
    Wait Until Keyword Succeeds
    ...    60 secs
    ...    5 secs
    ...    File Log Contains    ${AV_INSTALL_LOG}    Successfully restored old SafeStore database (${safeStoreDatabaseBackup}) to ${SAFESTORE_DB_DIR}
    Directory Should Not Exist    ${safeStoreDatabaseBackup}
    Directory Should Not Exist    ${AV_RESTORED_VAR_DIRECTORY}
    Verify SafeStore Database Exists

    Wait Until SafeStore Log Contains    Successfully initialised SafeStore database

AV Plugin Restores Newest SafeStore Database Backup On Upgrade
    Register Cleanup    Remove Directory    /tmp/safestore_db_copy    recursive=True
    Check AV Plugin Running
    Copy Directory    ${SAFESTORE_DB_DIR}    /tmp/safestore_db_copy

    Run plugin uninstaller with downgrade flag
    Check AV Plugin Not Installed

    # (2003-10-19 16:24:42)
    Copy Directory    /tmp/safestore_db_copy    ${AV_RESTORED_VAR_DIRECTORY}/1066580682_SafeStore_1.0.0
    # (2006-02-02 10:01:31)
    Copy Directory    /tmp/safestore_db_copy    ${AV_RESTORED_VAR_DIRECTORY}/1138874491_SafeStore_1.0.0
    # (2019-01-13 23:23:49)
    Copy Directory    /tmp/safestore_db_copy    ${AV_RESTORED_VAR_DIRECTORY}/1376107843_SafeStore_1.0.0
    # (2013-08-10 04:10:43)
    Copy Directory    /tmp/safestore_db_copy    ${AV_RESTORED_VAR_DIRECTORY}/1066580682_SafeStore_1.0.0
    # (2056-11-17 10:57:13) - Newest Database that will be restored
    ${backupToRestore} =    Set Variable    ${AV_RESTORED_VAR_DIRECTORY}/2741684233_SafeStore_1.0.0
    Copy Directory    /tmp/safestore_db_copy    ${backupToRestore}

    Install AV Directly from SDDS

    Wait Until Keyword Succeeds
    ...    60 secs
    ...    5 secs
    ...    File Log Contains    ${AV_INSTALL_LOG}    Successfully restored old SafeStore database (${backupToRestore}) to ${SAFESTORE_DB_DIR}

    Directory Should Not Exist    ${backupToRestore}
    Directory Should Not Be Empty    ${AV_RESTORED_VAR_DIRECTORY}
    Verify SafeStore Database Exists
    Wait Until SafeStore Log Contains    Successfully initialised SafeStore database

Older SafeStore Database Is Not Restored When A SafeStore Database Is Already Present
    Register Cleanup    Remove Directory    /tmp/safestore_db_copy    recursive=True
    Check AV Plugin Running
    Copy Directory    ${SAFESTORE_DB_DIR}    /tmp/safestore_db_copy

    Run plugin uninstaller with downgrade flag
    Check AV Plugin Not Installed

    Remove All But One SafeStore Backup
    ${safeStoreDatabaseBackupDirs} =    List Directories In Directory    ${AV_RESTORED_VAR_DIRECTORY}
    ${safeStoreDatabaseBackup} =    Set Variable    ${AV_RESTORED_VAR_DIRECTORY}/${safeStoreDatabaseBackupDirs[0]}
    Move Directory    /tmp/safestore_db_copy    ${SAFESTORE_DB_DIR}

    Install AV Directly from SDDS

    Wait Until Keyword Succeeds
    ...    60 secs
    ...    5 secs
    ...    File Log Contains    ${AV_INSTALL_LOG}    SafeStore database already exists so will not attempt to restore: ${safeStoreDatabaseBackup}
    Directory Should Exist    ${safeStoreDatabaseBackup}
    Verify SafeStore Database Exists

Older SafeStore Database Is Not Restored When It Is Not Compaitible With Current AV Version
    Check AV Plugin Running
    Run plugin uninstaller with downgrade flag
    Check AV Plugin Not Installed

    Remove All But One SafeStore Backup
    ${safeStoreDatabaseBackupDirs} =    List Directories In Directory    ${AV_RESTORED_VAR_DIRECTORY}
    ${incompatibleBackup} =    Set Variable    ${AV_RESTORED_VAR_DIRECTORY}/123456789_SafeStore_9.9.9
    Move Directory    ${AV_RESTORED_VAR_DIRECTORY}/${safeStoreDatabaseBackupDirs[0]}    ${incompatibleBackup}

    Install AV Directly from SDDS
    ${version} =  Get Version Number From Ini File  ${COMPONENT_ROOT_PATH}/VERSION.ini

    Wait Until Keyword Succeeds
    ...    60 secs
    ...    5 secs
    ...    File Log Contains    ${AV_INSTALL_LOG}    SafeStore Database (${incompatibleBackup}) is not compatible with the current AV version (${version}) so will not attempt to restore
    Directory Should Exist    ${incompatibleBackup}
    Wait Until SafeStore Log Contains    Successfully initialised SafeStore database
    Verify SafeStore Database Exists


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

    ${av_mark} =  mark_log_size  ${AV_LOG_PATH}
    Send Sav Policy With No Scheduled Scans
    Wait For Log Contains From Mark  ${av_mark}  Processing SAV policy
    check_av_log_does_not_contain_after_mark  Failed to create file  mark=${av_mark}

Check installer removes sophos_threat_detector log symlink
    Run Process   ln  -snf  ${COMPONENT_ROOT_PATH}/log/sophos_threat_detector/sophos_threat_detector.log  ${COMPONENT_ROOT_PATH}/log/sophos_threat_detector.log
    File Should Exist  ${COMPONENT_ROOT_PATH}/log/sophos_threat_detector.log
    # modify the manifest to force the installer to perform a full product update
    Modify manifest
    Run Installer From Install Set

    File Should Not Exist  ${COMPONENT_ROOT_PATH}/log/sophos_threat_detector.log

Check AV installer can add AV users when /usr/sbin is not in path
    Run plugin uninstaller
    Install AV Directly from SDDS Without /usr/sbin in PATH
    User Should Exist  sophos-spl-av
    User Should Exist  sophos-spl-threat-detector

Check AV uninstaller can remove AV users when /usr/sbin is not in path

    Uninstall AV Without /usr/sbin in PATH
    User Should Not Exist   sophos-spl-av
    User Should Not Exist   sophos-spl-threat-detector

Check AV installer sets correct home directory for the users it creates
    Run plugin uninstaller
    Install AV Directly from SDDS

    User Should Exist  sophos-spl-av
    ${rc}  ${homedir} =  Run And Return Rc And Output  getent passwd sophos-spl-av | cut -d: -f6
    Should Be Equal As Strings  ${homedir}  /opt/sophos-spl

    User Should Exist  sophos-spl-threat-detector
    ${rc}  ${homedir} =  Run And Return Rc And Output  getent passwd sophos-spl-threat-detector | cut -d: -f6
    Should Be Equal As Strings  ${homedir}  /opt/sophos-spl

IDE Update Invalidates On Access Cache
    Send Policies to enable on-access
    Register Cleanup  Exclude On Access Scan Errors
    ${srcfile} =  Set Variable  /tmp_test/clean.txt

    ${oa_mark} =  Get On Access Log Mark
    Create File  ${srcfile}  clean
    Register Cleanup  Remove File  ${srcfile}
    wait for log contains from mark  ${oa_mark}  On-close event for ${srcfile} from
    Get File  ${srcfile}
    wait for log contains from mark  ${oa_mark}  caching ${srcfile}

    ${oa_mark} =  Get On Access Log Mark
    Replace Virus Data With Test Dataset A And Run IDE update with SUSI loaded
    wait for log contains from mark  ${oa_mark}  Clearing on-access cache

    ${oa_mark} =  Get On Access Log Mark
    Get File  ${srcfile}
    wait for log contains from mark  ${oa_mark}  On-open event for ${srcfile} from  timeout=60


SSPLAV can load old VDL
    ${threat_detector_mark} =  Get Sophos Threat Detector Log Mark
    Restart sophos_threat_detector
    Check Plugin Installed and Running
    Wait For Sophos Threat Detector Log Contains After Mark
    ...   UnixSocket <> ProcessControlServer starting listening on socket: /var/process_control_socket
    ...   ${threat_detector_mark}
    ...   timeout=60

    Replace Virus Data With Test Dataset A  dataset=20220503
    Register Cleanup  Revert Virus Data and Run Update if Required
    Run IDE update without SUSI loaded

    # Check we can detect EICAR following update
    Check avscanner can detect eicar

    Wait For Sophos Threat Detector Log Contains After Mark  SUSI Loaded old data  ${threat_detector_mark}

AV Installer Sets UID From Base Install Options File When Present
    Run plugin uninstaller
    Copy File    ${RESOURCES_PATH}/requested_user_group_ids_install_options    ${SOPHOS_INSTALL}/base/etc/install_options

    Install AV Directly from SDDS
    User Should Exist  sophos-spl-av
    User Should Exist  sophos-spl-threat-detector

    ${av_uid} =    Get UID From Username    sophos-spl-av
    ${td_uid} =    Get UID From Username    sophos-spl-threat-detector

    Should Be Equal As Strings    ${av_uid}    1997
    Should Be Equal As Strings    ${td_uid}    1998
    Remove File    ${SOPHOS_INSTALL}/base/etc/install_options

AV Installer deletes old libsusi
    # Fake old libsusi
    Create File  ${SOPHOS_INSTALL}/plugins/av/lib64/libsusi.so.2
    Create File  ${SOPHOS_INSTALL}/plugins/av/lib64/libupdater.so.2

    Modify manifest
    Install AV Directly from SDDS

    File Should Not Exist  ${SOPHOS_INSTALL}/plugins/av/lib64/libsusi.so.2
    File Should Not Exist  ${SOPHOS_INSTALL}/plugins/av/lib64/libupdater.so.2

*** Variables ***
${IDE_NAME}         peend.ide
@{GROUP_WRITE_ALLOWED_TO_WRITE}
...     chroot/etc
...     chroot/log
...     chroot/opt/sophos-spl/base/etc
...     chroot/opt/sophos-spl/base/update/var
...     chroot/opt/sophos-spl/plugins/av
...     chroot/susi/distribution_version
...     chroot/susi/update_source
...     chroot/tmp
...     chroot/var
...     chroot/susi
...     log
...     var

@{WORLD_WRITE_ALLOWED_TO_WRITE}
...     /opt/sophos-spl/plugins/av/chroot/var/scanning_socket
...     /opt/sophos-spl/plugins/av/chroot/tmp

*** Keywords ***
Installer Suite Setup
    Install With Base SDDS
    #TODO LINUXDAR-5808 remove this line when systemctl restart sophos-spl-update hang is fixed
    Run Process  ${SOPHOS_INSTALL}/bin/wdctl  removePluginRegistration  updatescheduler

Installer Suite TearDown
    No Operation

Installer Test Setup
    Register On Fail  Debug install set
    Register Cleanup  analyse Journalctl   print_always=True
    Register Cleanup  Dump All Sophos Processes
    Register Cleanup  Log Status Of Sophos Spl
    Register On Fail  dump log  ${THREAT_DETECTOR_LOG_PATH}
    Register On Fail  dump log  ${SUSI_DEBUG_LOG_PATH}
    Register On Fail  dump log  ${AV_LOG_PATH}
    Register On Fail  dump log  ${ON_ACCESS_LOG_PATH}
    Register On Fail  dump log  ${SAFESTORE_LOG_PATH}
    Register On Fail  dump log  ${AV_INSTALL_LOG}
    Register On Fail  dump log  ${SOPHOS_INSTALL}/logs/base/watchdog.log

    Require Plugin Installed and Running

    #Register Cleanup has LIFO order, so checking for errors is done last.
    Register Cleanup    Check All Product Logs Do Not Contain Error
    Register Cleanup    Exclude CustomerID Failed To Read Error
    Register Cleanup    Exclude MCS Router is dead
    Register Cleanup    Exclude Communication Between AV And Base Due To No Incoming Data
    Register Cleanup    Require No Unhandled Exception
    Register Cleanup    Check For Coredumps  ${TEST NAME}
    Register Cleanup    Check Dmesg For Segfaults

Installer Test TearDown
    #Run Teardown Functions
    run cleanup functions
    run failure functions if failed
    Run Keyword If Test Failed   Installer Suite Setup

Installer Test TearDown With Mocked Installset Cleanup
    Installer Test TearDown
    Cleanup Mocked Install Set

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

Cleanup Mocked Install Set
    Remove Directory    ${MOCKED_INSTALL_SET}    recursive=${True}

Check File Contains
    [Arguments]  ${filepath}  ${expectedContents}
    ${actualContents} =   Get File   ${filepath}
    Should Be Equal   ${actualContents}   ${expectedContents}

Wait Until File Contains
    [Arguments]  ${filepath}  ${expectedContents}  ${timeout}=12sec
    Wait Until Created   ${filepath}   timeout=${timeout}
    Wait Until Keyword Succeeds  ${2}  1 sec  Check File Contains  ${filepath}  ${expectedContents}
