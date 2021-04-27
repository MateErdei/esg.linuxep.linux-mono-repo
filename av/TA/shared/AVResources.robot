*** Settings ***
Library         Process
Library         OperatingSystem
Library         String
Library         ../Libs/AVScanner.py
Library         ../Libs/ExclusionHelper.py
Library         ../Libs/LogUtils.py
Library         ../Libs/FakeManagement.py
Library         ../Libs/FakeManagementLog.py
Library         ../Libs/BaseUtils.py
Library         ../Libs/serialisationtools/CapnpHelper.py

Resource    GlobalSetup.robot
Resource    ComponentSetup.robot
Resource    RunShellProcess.robot

*** Variables ***
${AV_PLUGIN_PATH}  ${COMPONENT_ROOT_PATH}
${AV_PLUGIN_BIN}   ${COMPONENT_BIN_PATH}
${CLI_SCANNER_PATH}      ${COMPONENT_ROOT_PATH}/bin/avscanner
${AV_LOG_PATH}     ${AV_PLUGIN_PATH}/log/${COMPONENT}.log
${THREAT_DETECTOR_LOG_PATH}     ${AV_PLUGIN_PATH}/chroot/log/sophos_threat_detector.log
${THREAT_DETECTOR_LOG_SYMLINK}  ${AV_PLUGIN_PATH}/log/sophos_threat_detector.log
${SUSI_DEBUG_LOG_PATH}          ${AV_PLUGIN_PATH}/chroot/log/susi_debug.log
${SCANNOW_LOG_PATH}     ${AV_PLUGIN_PATH}/log/Scan Now.log
${TELEMETRY_LOG_PATH}   ${SOPHOS_INSTALL}/logs/base/sophosspl/telemetry.log
${CHROOT_LOGGING_SYMLINK}   ${COMPONENT_ROOT_PATH}/chroot/${COMPONENT_ROOT_PATH}/log/sophos_threat_detector
${SUSI_STARTUP_SETTINGS_FILE}    ${AV_PLUGIN_PATH}/var/susi_startup_settings.json
${SUSI_STARTUP_SETTINGS_FILE_CHROOT}    ${COMPONENT_ROOT_PATH}/chroot/${AV_PLUGIN_PATH}/var/susi_startup_settings.json
${AV_SDDS}         ${COMPONENT_SDDS}
${PLUGIN_SDDS}     ${COMPONENT_SDDS}
${PLUGIN_BINARY}   ${SOPHOS_INSTALL}/plugins/${COMPONENT}/sbin/${COMPONENT}
${SOPHOS_THREAT_DETECTOR_BINARY}  ${SOPHOS_INSTALL}/plugins/${COMPONENT}/sbin/sophos_threat_detector
${EXPORT_FILE}     /etc/exports

${EICAR_STRING}  X5O!P%@AP[4\\PZX54(P^)7CC)7}$EICAR-STANDARD-ANTIVIRUS-TEST-FILE!$H+H*
${EICAR_PUA_STRING}  X5]+)D:)D<5N*PZ5[/EICAR-POTENTIALLY-UNWANTED-OBJECT-TEST!$*M*L

${POLICY_7DAYS}     <daySet><day>monday</day><day>tuesday</day><day>wednesday</day><day>thursday</day><day>friday</day><day>saturday</day><day>sunday</day></daySet>
${STATUS_XML}       ${MCS_PATH}/status/SAV_status.xml

${IDE_DIR}          ${COMPONENT_INSTALL_SET}/files/plugins/av/chroot/susi/update_source/vdl
${INSTALL_IDE_DIR}  ${COMPONENT_ROOT_PATH}/chroot/susi/update_source/vdl
${SCAN_DIRECTORY}   /home/vagrant/this/is/a/directory/for/scanning
${AVSCANNER}        /usr/local/bin/avscanner


*** Keywords ***
Check Plugin Running
    Run Shell Process  pidof ${PLUGIN_BINARY}   OnError=AV not running

Check AV Plugin Running
    Check Plugin Running

Check Sophos Threat Detector Running
    Run Shell Process  pidof ${SOPHOS_THREAT_DETECTOR_BINARY}   OnError=sophos_threat_detector not running

Check AV Plugin Not Running
    ${result} =   Run Process  pidof  ${PLUGIN_BINARY}  timeout=3
    Should Not Be Equal As Integers  ${result.rc}  ${0}

Count File Log Lines
    [Arguments]  ${path}
    ${content} =  Get File   ${path}  encoding_errors=replace
    ${count} =  Get Line Count   ${content}
    [Return]   ${count}

Count AV Log Lines
    ${count} =  Count File Log Lines   ${AV_LOG_PATH}
    [Return]   ${count}

Mark AV Log
    ${count} =  Count AV Log Lines
    Set Test Variable   ${AV_LOG_MARK}  ${count}
    Log  "AV LOG MARK = ${AV_LOG_MARK}"

Mark Sophos Threat Detector Log
    [Arguments]  ${mark}=""
    ${count} =  Count File Log Lines  ${THREAT_DETECTOR_LOG_PATH}
    Set Test Variable   ${SOPHOS_THREAT_DETECTOR_LOG_MARK}  ${count}
    Log  "SOPHOS_THREAT_DETECTOR LOG MARK = ${SOPHOS_THREAT_DETECTOR_LOG_MARK}"

Mark Susi Debug Log
    ${count} =  Count File Log Lines  ${SUSI_DEBUG_LOG_PATH}
    Set Test Variable   ${SUSI_DEBUG_LOG_MARK}  ${count}
    Log  "SUSI_DEBUG LOG MARK = ${SUSI_DEBUG_LOG_MARK}"

SUSI Debug Log Contains With Offset
    [Arguments]  ${input}
    ${offset} =  Get Variable Value  ${SUSI_DEBUG_LOG_MARK}  0
    File Log Contains With Offset  ${SUSI_DEBUG_LOG_PATH}   ${input}   offset=${offset}

SUSI Debug Log Does Not Contain With Offset
    [Arguments]  ${input}
    ${offset} =  Get Variable Value  ${SUSI_DEBUG_LOG_MARK}  0
    File Log Should Not Contain With Offset  ${SUSI_DEBUG_LOG_PATH}   ${input}   offset=${offset}

Mark Scan Now Log
    ${count} =  Count File Log Lines  ${SCANNOW_LOG_PATH}
    Set Test Variable   ${SCAN_NOW_LOG_MARK}  ${count}
    Log  "SCAN NOW LOG MARK = ${SCAN_NOW_LOG_MARK}"

Get File Contents From Offset
    [Arguments]  ${path}  ${offset}=0
    ${content} =  Get File   ${path}  encoding_errors=replace
    Log   "Skipping ${offset} lines"
    @{lines} =  Split To Lines   ${content}  ${offset}
    ${collectedLines} =  Catenate  SEPARATOR=\n  @{lines}
    [Return]  ${collectedLines}

File Log Contains With Offset
    [Arguments]  ${path}  ${input}  ${offset}=0
    ${content} =  Get File Contents From Offset  ${path}  ${offset}
    Should Contain  ${content}  ${input}

File Log Should Not Contain
    [Arguments]  ${path}  ${input}
    ${content} =  Get File   ${path}  encoding_errors=replace
    Should Not Contain  ${content}  ${input}

File Log Should Not Contain With Offset
    [Arguments]  ${path}  ${input}  ${offset}=0
    ${content} =  Get File Contents From Offset  ${path}  ${offset}
    Should Not Contain  ${content}  ${input}

Wait Until File Log Contains
    [Arguments]  ${logCheck}  ${input}  ${timeout}=15  ${interval}=1
    Wait Until Keyword Succeeds
    ...  ${timeout} secs
    ...  ${interval} secs
    ...  ${logCheck}  ${input}

File Log Does Not Contain
    [Arguments]  ${logCheck}  ${input}
    Run Keyword And Expect Error
    ...  Keyword '${logCheck}' failed after retrying for 15 seconds.*
    ...  Wait Until Keyword Succeeds
    ...    15 secs
    ...    1 secs
    ...    ${logCheck}  ${input}

AV Plugin Log Contains With Offset
    [Arguments]  ${input}
    ${offset} =  Get Variable Value  ${AV_LOG_MARK}  0
    File Log Contains With Offset  ${AV_LOG_PATH}   ${input}   offset=${offset}

AV Plugin Log Should Not Contain With Offset
    [Arguments]  ${input}
    ${offset} =  Get Variable Value  ${AV_LOG_MARK}  0
    File Log Should Not Contain With Offset  ${AV_LOG_PATH}   ${input}   offset=${offset}

AV Plugin Log Contains
    [Arguments]  ${input}
    LogUtils.File Log Contains  ${AV_LOG_PATH}   ${input}

Threat Detector Log Contains
    [Arguments]  ${input}
    File Log Contains  ${THREAT_DETECTOR_LOG_PATH}   ${input}

Threat Detector Does Not Log Contain
    [Arguments]  ${input}
    File Log Should Not Contain  ${THREAT_DETECTOR_LOG_PATH}  ${input}

Susi Debug Log Contains
    [Arguments]  ${input}
    File Log Contains  ${SUSI_DEBUG_LOG_PATH}   ${input}

SUSI Debug Log Does Not Contain
    [Arguments]  ${input}
    check log does not contain  ${input}  ${SUSI_DEBUG_LOG_PATH}   susi_debug_log

Wait Until Sophos Threat Detector Log Contains
    [Arguments]  ${input}  ${timeout}=15
    Wait Until File Log Contains  Threat Detector Log Contains   ${input}   timeout=${timeout}

Sophos Threat Detector Log Contains With Offset
    [Arguments]  ${input}
    ${offset} =  Get Variable Value  ${SOPHOS_THREAT_DETECTOR_LOG_MARK}  0
    File Log Contains With Offset  ${THREAT_DETECTOR_LOG_PATH}   ${input}   offset=${offset}

Threat Detector Log Should Not Contain With Offset
    [Arguments]  ${input}
    ${offset} =  Get Variable Value  ${SOPHOS_THREAT_DETECTOR_LOG_MARK}  0
    File Log Should Not Contain With Offset  ${THREAT_DETECTOR_LOG_PATH}   ${input}   offset=${offset}

Wait Until Sophos Threat Detector Log Contains With Offset
    [Arguments]  ${input}  ${timeout}=15
    Wait Until File Log Contains  Sophos Threat Detector Log Contains With Offset  ${input}   timeout=${timeout}

Count Lines In Log
    [Arguments]  ${log_file}  ${line_to_count}
    ${contents} =  Get File  ${log_file}
    ${lines} =  Get Lines Containing String  ${contents}  ${line_to_count}
    ${lines_count} =  Get Line Count  ${lines}
    [Return]  ${lines_count}

Count Lines In Log With Offset
    [Arguments]  ${log_file}  ${line_to_count}  ${offset}
    ${content} =  Get File Contents From Offset  ${log_file}  ${offset}
    ${lines} =  Get Lines Containing String  ${content}  ${line_to_count}
    ${lines_count} =  Get Line Count  ${lines}

    [Return]  ${lines_count}

Check Threat Detector Copied Files To Chroot
    Threat Detector Log Contains  Copying "/opt/sophos-spl/base/etc/logger.conf" to: "/opt/sophos-spl/plugins/av/chroot/opt/sophos-spl/base/etc/logger.conf"
    Threat Detector Log Contains  Copying "/opt/sophos-spl/base/etc/machine_id.txt" to: "/opt/sophos-spl/plugins/av/chroot/opt/sophos-spl/base/etc/machine_id.txt"
    Threat Detector Log Contains  Copying "/opt/sophos-spl/plugins/av/VERSION.ini" to: "/opt/sophos-spl/plugins/av/chroot/opt/sophos-spl/plugins/av/VERSION.ini"
    #dns query files
    Threat Detector Log Contains  Copying "/etc/nsswitch.conf" to: "/opt/sophos-spl/plugins/av/chroot/etc/nsswitch.conf"
    Threat Detector Log Contains  Copying "/etc/resolv.conf" to: "/opt/sophos-spl/plugins/av/chroot/etc/resolv.conf"
    Threat Detector Log Contains  Copying "/etc/ld.so.cache" to: "/opt/sophos-spl/plugins/av/chroot/etc/ld.so.cache"
    Threat Detector Log Contains  Copying "/etc/host.conf" to: "/opt/sophos-spl/plugins/av/chroot/etc/host.conf"
    Threat Detector Log Contains  Copying "/etc/hosts" to: "/opt/sophos-spl/plugins/av/chroot/etc/hosts"

    Threat Detector Does Not Log Contain  Failed to copy: /opt/sophos-spl/base/etc/logger.conf
    Threat Detector Does Not Log Contain  Failed to copy: /opt/sophos-spl/base/etc/machine_id.txt
    Threat Detector Does Not Log Contain  Failed to copy: /opt/sophos-spl/plugins/av/VERSION.ini

    File Should Exist  /opt/sophos-spl/plugins/av/chroot/opt/sophos-spl/base/etc/logger.conf
    File Should Exist  /opt/sophos-spl/plugins/av/chroot/opt/sophos-spl/base/etc/machine_id.txt
    File Should Exist  /opt/sophos-spl/plugins/av/chroot/opt/sophos-spl/plugins/av/VERSION.ini
    File Should Exist  /opt/sophos-spl/plugins/av/chroot/etc/nsswitch.conf
    File Should Exist  /opt/sophos-spl/plugins/av/chroot/etc/resolv.conf
    File Should Exist  /opt/sophos-spl/plugins/av/chroot/etc/ld.so.cache
    File Should Exist  /opt/sophos-spl/plugins/av/chroot/etc/host.conf
    File Should Exist  /opt/sophos-spl/plugins/av/chroot/etc/hosts

Increase Threat Detector Log To Max Size
    [Arguments]  ${remaining}=1
    increase_threat_detector_log_to_max_size_by_path  ${THREAT_DETECTOR_LOG_PATH}  ${remaining}

Wait Until AV Plugin Log Contains With Offset
    [Arguments]  ${input}  ${timeout}=15    ${interval}=2
    Wait Until File Log Contains  AV Plugin Log Contains With Offset  ${input}   timeout=${timeout}  interval=${interval}

Wait Until AV Plugin Log Contains
    [Arguments]  ${input}  ${timeout}=15  ${interval}=0
    ${interval} =   Set Variable If
    ...   ${interval} > 0   ${interval}
    ...   ${timeout} >= 120   10
    ...   ${timeout} >= 60   5
    ...   ${timeout} >= 15   3
    ...   1
    Wait Until File Log Contains  AV Plugin Log Contains   ${input}   timeout=${timeout}  interval=${interval}

AV Plugin Log Does Not Contain
    [Arguments]  ${input}
    LogUtils.Over next 15 seconds ensure log does not contain   ${AV_LOG_PATH}  ${input}

AV Plugin Log Does Not Contain With Offset
    [Arguments]  ${input}
    ${offset} =  Get Variable Value  ${AV_LOG_MARK}  0
    File Log Should Not Contain With Offset  ${AV_LOG_PATH}   ${input}   offset=${offset}

Plugin Log Contains
    [Arguments]  ${input}
    LogUtils.check_log_contains  ${input}  ${AV_LOG_PATH}

FakeManagement Log Contains
    [Arguments]  ${input}
    ${log_path} =   FakeManagementLog.get_fake_management_log_path
    File Log Contains  ${log_path}   ${input}

Management Log Contains
    [Arguments]  ${input}
    File Log Contains  ${MANAGEMENT_AGENT_LOG_PATH}   ${input}

Wait Until Management Log Contains
    [Arguments]  ${input}
    Wait Until File Log Contains  Management Log Contains   ${input}

Management Log Does Not Contain
    [Arguments]  ${input}
    LogUtils.Over next 15 seconds ensure log does not contain   ${MANAGEMENT_AGENT_LOG_PATH}  ${input}

SAV Status XML Contains
    [Arguments]  ${input}
    File Log Contains  ${STATUS_XML}   ${input}

Wait Until SAV Status XML Contains
    [Arguments]  ${input}  ${timeout}=15
    Wait Until File Log Contains  SAV Status XML Contains   ${input}   timeout=${timeout}

Check Plugin Installed and Running
    File Should Exist   ${PLUGIN_BINARY}
    Wait until AV Plugin running
    Wait until threat detector running

Wait until AV Plugin running
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  2 secs
    ...  Check Plugin Running
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  2 secs
    ...  Plugin Log Contains  ${COMPONENT} <> Starting the main program loop

Wait until threat detector running
    # wait for AV Plugin to initialize
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  3 secs
    ...  Check Sophos Threat Detector Running
    Wait Until Keyword Succeeds
    ...  40 secs
    ...  2 secs
    ...  Threat Detector Log Contains  UnixSocket <> Starting listening on socket

Check AV Plugin Installed
    Check Plugin Installed and Running
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  3 secs
    ...  FakeManagement Log Contains   Registered plugin: ${COMPONENT}

Install With Base SDDS
    Uninstall All
    Directory Should Not Exist  ${SOPHOS_INSTALL}
    Install Base For Component Tests
    Set Log Level  DEBUG
    Install AV Directly from SDDS

Uninstall And Revert Setup
    Uninstall All
    Setup Base And Component

Uninstall and full reinstall
    Uninstall All
    Install With Base SDDS

Install Base For Component Tests
    File Should Exist     ${BASE_SDDS}install.sh
    Run Process  chmod  +x  ${BASE_SDDS}install.sh
    ${result} =   Run Process   bash  ${BASE_SDDS}install.sh  timeout=600s    stderr=STDOUT
    Should Be Equal As Integers  ${result.rc}  0   "Failed to install base.\noutput: \n${result.stdout}"
    Run Keyword and Ignore Error   Run Shell Process    /opt/sophos-spl/bin/wdctl stop mcsrouter  OnError=Failed to stop mcsrouter

Install AV Directly from SDDS
    ${install_log} =  Set Variable   /tmp/avplugin_install.log
    ${result} =   Run Process   bash  -x  ${AV_SDDS}/install.sh   timeout=60s  stderr=STDOUT   stdout=${install_log}
    ${log_contents} =  Get File  ${install_log}
    Should Be Equal As Integers  ${result.rc}  0   "Failed to install plugin.\noutput: \n${log_contents}"

Check AV Plugin Installed With Base
    Check Plugin Installed and Running

Display All SSPL Files Installed
    ${handle}=  Start Process  find ${SOPHOS_INSTALL} | grep -v python | grep -v primarywarehouse | grep -v temp_warehouse | grep -v TestInstallFiles | grep -v lenses   shell=True
    ${result}=  Wait For Process  ${handle}  timeout=30  on_timeout=kill
    Log  ${result.stdout}
    Log  ${result.stderr}

AV And Base Teardown
    Run Teardown Functions
    Run Keyword If Test Failed   Display All SSPL Files Installed
    Run Keyword If Test Failed   Run Keyword And Ignore Error  Log File   ${SOPHOS_INSTALL}/logs/base/watchdog.log  encoding_errors=replace
    Run Keyword If Test Failed   Run Keyword And Ignore Error  Log File   ${SOPHOS_INSTALL}/logs/base/sophosspl/sophos_managementagent.log  encoding_errors=replace
    Run Keyword If Test Failed   Run Keyword And Ignore Error  Log File   ${THREAT_DETECTOR_LOG_PATH}  encoding_errors=replace
    Run Keyword If Test Failed   Run Keyword And Ignore Error  Log File   ${SUSI_DEBUG_LOG_PATH}  encoding_errors=replace
    Run Keyword If Test Failed   Run Keyword And Ignore Error  Log File   ${AV_LOG_PATH}  encoding_errors=replace
    Run Keyword If Test Failed   Run Keyword And Ignore Error  Log File   ${TELEMETRY_LOG_PATH}  encoding_errors=replace

Restart AV Plugin And Clear The Logs
    Run Shell Process  ${SOPHOS_INSTALL}/bin/wdctl stop av   OnError=failed to stop plugin
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  2 secs
    ...  Check AV Plugin Not Running

    Log  Backup logs before removing them
    Log  ${AV_LOG_PATH}
    Log  ${THREAT_DETECTOR_LOG_PATH}
    Log  ${SUSI_DEBUG_LOG_PATH}

    Remove File    ${AV_LOG_PATH}
    Remove File    ${THREAT_DETECTOR_LOG_PATH}
    Remove File    ${SUSI_DEBUG_LOG_PATH}

    Run Shell Process  ${SOPHOS_INSTALL}/bin/wdctl start av   OnError=failed to start plugin
    Wait until AV Plugin running

Create Install Options File With Content
    [Arguments]  ${installFlags}
    Create File  ${SOPHOS_INSTALL}/base/etc/install_options  ${installFlags}

Check ScanNow Log Exists
    File Should Exist  ${SCANNOW_LOG_PATH}

Check AV Plugin Log exists
    File Should Exist  ${AV_LOG_PATH}

Wait Until File exists
    [Arguments]  ${filename}  ${timeout}=15  ${interval}=0
    ${interval} =   Set Variable If
    ...   ${interval} > 0   ${interval}
    ...   ${timeout} >= 120   10
    ...   ${timeout} >= 60   5
    ...   ${timeout} >= 15   3
    ...   1
    Wait Until Keyword Succeeds
        ...    ${timeout} secs
        ...    ${interval}
        ...    File Should Exist  ${filename}

Wait Until AV Plugin Log exists
    [Arguments]  ${timeout}=15  ${interval}=0
    Wait Until File exists  ${AV_LOG_PATH}  ${timeout}  ${interval}

Wait Until Threat Detector Log exists
    [Arguments]  ${timeout}=15  ${interval}=0
    Wait Until File exists  ${THREAT_DETECTOR_LOG_PATH}  ${timeout}  ${interval}

Wait until scheduled scan updated
    Wait Until AV Plugin Log exists  timeout=30
    Wait Until AV Plugin Log Contains  Configured number of Scheduled Scans  timeout=240

Wait until scheduled scan updated With Offset
    Wait Until AV Plugin Log exists  timeout=30
    Wait Until AV Plugin Log Contains With Offset  Configured number of Scheduled Scans  timeout=240

Configure Scan Exclusions Everything Else
    [Arguments]  ${inclusion}
    ${exclusions} =  exclusions_for_everything_else  ${inclusion}
    Log  "Excluding all directories except: ${inclusion}"
    Log  "Excluding the following directories: ${exclusions}"
    [return]  ${exclusions}

Create ext2 mount
    [Arguments]  ${source}  ${destination}
    Run Shell Process   dd if=/dev/zero of=${source} bs=1024 count=102400   OnError=Failed to create image file
    Run Shell Process   mkfs -t ext2 -F ${source}                           OnError=Failed to create ext2 fs
    Run Shell Process   mount -o loop ${source} ${destination}              OnError=Failed to mount ext2 fs

Remove ext2 mount
    [Arguments]  ${source}  ${destination}
    Run Shell Process   umount ${destination}   OnError=Failed to unmount ext2 fs
    Remove file   ${source}

Create Local NFS Share
    [Arguments]  ${source}  ${destination}
    Copy File  ${EXPORT_FILE}  ${EXPORT_FILE}_bkp
    Append To File  ${EXPORT_FILE}  ${source} localhost(rw,sync,no_subtree_check)\n
    Run Shell Process   systemctl restart nfs-server            OnError=Failed to restart NFS server
    Run Shell Process   mount -t nfs localhost:${source} ${destination}   OnError=Failed to mount local NFS share

Remove Local NFS Share
    [Arguments]  ${source}  ${destination}
    Run Shell Process   umount ${destination}   OnError=Failed to unmount local NFS share
    Move File  ${EXPORT_FILE}_bkp  ${EXPORT_FILE}
    Run Shell Process   systemctl restart nfs-server   OnError=Failed to restart NFS server
    Remove Directory    ${source}  recursive=True

Check Scan Now Configuration File is Correct
    ${configFilename} =  Set Variable  /tmp/config-files-test/Scan_Now.config
    Wait Until Keyword Succeeds
        ...    15 secs
        ...    1 secs
        ...    File Should Exist  ${configFilename}
    @{exclusions} =  ExclusionHelper.get exclusions to scan tmp test
    CapnpHelper.check named scan object   ${configFilename}
        ...     name=Scan Now
        ...     exclude_paths=@{exclusions}
        ...     sophos_extension_exclusions=["exclusion1", "exclusion2", "exclusion3"]
        ...     user_defined_extension_inclusions=["exclusion1", "exclusion2", "exclusion3", "exclusion4"]
        ...     scan_archives=False
        ...     scan_all_files=False
        ...     scan_files_with_no_extensions=True
        ...     scan_hard_drives=True
        ...     scan_cd_dvd_drives=True
        ...     scan_network_drives=False
        ...     scan_removable_drives=True

Check Scheduled Scan Configuration File is Correct
    ${configFilename} =  Set Variable  /tmp/config-files-test/Sophos_Cloud_Scheduled_Scan.config
    Wait Until Keyword Succeeds
        ...    120 secs
        ...    1 secs
        ...    File Should Exist  ${configFilename}
    @{exclusions} =  ExclusionHelper.get exclusions to scan tmp test
    CapnpHelper.check named scan object   ${configFilename}
        ...     name=Sophos Cloud Scheduled Scan
        ...     exclude_paths=@{exclusions}
        ...     sophos_extension_exclusions=[]
        ...     user_defined_extension_inclusions=[]
        ...     scan_archives=False
        ...     scan_all_files=False
        ...     scan_files_with_no_extensions=True
        ...     scan_hard_drives=True
        ...     scan_cd_dvd_drives=False
        ...     scan_network_drives=False
        ...     scan_removable_drives=False

Policy Fragment FS Types
    [Arguments]  ${CDDVDDrives}=false  ${hardDrives}=true  ${networkDrives}=false  ${removableDrives}=false
    [return]    <scanObjectSet><CDDVDDrives>${CDDVDDrives}</CDDVDDrives><hardDrives>${hardDrives}</hardDrives><networkDrives>${networkDrives}</networkDrives><removableDrives>${removableDrives}</removableDrives></scanObjectSet>

Create EICAR files
    [Arguments]  ${eicar_files_to_create}  ${dir_name}
     FOR    ${INDEX}    IN RANGE    1    ${eicar_files_to_create}
         ${eicar_file}=    create file  ${dir_name}/eicar-${INDEX}  ${EICAR_STRING}
     END

Add IDE to install set
    [Arguments]  ${ide_name}
    # COMPONENT_INSTALL_SET
    ${IDE} =  Set Variable  ${RESOURCES_PATH}/ides/${ide_name}
    File Should Exist   ${IDE}
    File Should Not Exist   ${IDE_DIR}/${ide_name}
    Copy file  ${IDE}  ${IDE_DIR}/${ide_name}

Debug install set
    ${result} =  run process  find  ${COMPONENT_INSTALL_SET}/files/plugins/av/chroot/susi/distribution_version  -type  f  stdout=/tmp/proc.out   stderr=STDOUT
    Log  INSTALL_SET= ${result.stdout}
    ${result} =  run process  find  ${SOPHOS_INSTALL}/plugins/av/chroot/susi/distribution_version   stdout=/tmp/proc.out    stderr=STDOUT
    Log  INSTALLATION= ${result.stdout}

Remove IDE from install set
    [Arguments]  ${ide_name}
    File Should Exist   ${IDE_DIR}/${ide_name}
    Remove File  ${IDE_DIR}/${ide_name}

Run installer from install set
    ${result} =  run process    bash  -x  ${COMPONENT_INSTALL_SET}/install.sh  stdout=/tmp/proc.out  stderr=STDOUT
    Log  ${result.stdout}
    Should Be Equal As Integers  ${result.rc}  ${0}

Check IDE present in installation
    [Arguments]  ${ide_name}
    File should exist  ${INSTALL_IDE_DIR}/${ide_name}

Check IDE absent from installation
    [Arguments]  ${ide_name}
    file should not exist  ${INSTALL_IDE_DIR}/${ide_name}

Run IDE update
    # TODO Improve "Mark Sophos Threat Detector Log" (& related functions) to enable multiple marks in one file so it doesn't clobber any marks used for testing LINUXDAR-2677
    Mark Sophos Threat Detector Log
    ${threat_detector_pid} =  Record Sophos Threat Detector PID
    Run installer from install set
    Wait Until Sophos Threat Detector Log Contains With Offset  Reload triggered by USR1
    Wait Until Sophos Threat Detector Log Contains With Offset  Threat scanner successfully updated  timeout=120
    Threat Detector Log Should Not Contain With Offset    Current version matches that of the update source. Nothing to do.
    Check Sophos Threat Detector Has Same PID  ${threat_detector_pid}


Install IDE
    [Arguments]  ${ide_name}
    Register cleanup  Uninstall IDE  ${ide_name}
    Add IDE to install set  ${ide_name}
    Run IDE update
    Check IDE present in installation  ${ide_name}

Install IDE without reload check
    [Arguments]  ${ide_name}
    Register cleanup  Uninstall IDE  ${ide_name}
    Add IDE to install set  ${ide_name}
    Run installer from install set
    Check IDE present in installation  ${ide_name}

Uninstall IDE
    [Arguments]  ${ide_name}
    Deregister Cleanup   Uninstall IDE  ${ide_name}
    Remove IDE From Install Set  ${ide_name}
    Run IDE update
    Check IDE Absent From Installation  ${ide_name}


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

Check threat detected
     [Arguments]  ${THREAT_FILE}  ${THREAT_NAME}  ${INFECTED_CONTENTS}=${EMPTY}
     ${rc}   ${output} =    Run And Return Rc And Output   ${AVSCANNER} ${RESOURCES_PATH}/file_samples/${THREAT_FILE}
     Log  ${output}
     Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}
     Should Contain   ${output}    Detected "${RESOURCES_PATH}/file_samples/${THREAT_FILE}${INFECTED_CONTENTS}" is infected with ${THREAT_NAME}

Check file clean
     [Arguments]  ${THREAT_FILE}
     ${rc}   ${output} =    Run And Return Rc And Output   ${AVSCANNER} ${RESOURCES_PATH}/file_samples/${THREAT_FILE}
     Log  ${output}
     Should Be Equal As Integers  ${rc}  ${CLEAN_RESULT}
     Should Contain   ${output}    0 files out of 1 were infected.

Wait For File With Particular Contents
    [Arguments]     ${expectedContent}  ${filePath}
    Wait Until Keyword Succeeds
    ...  20 secs
    ...  5 secs
    ...  Check Specific File Content  ${expectedContent}  ${filePath}
    Log File  ${filePath}

Check Specific File Content
    [Arguments]     ${expectedContent}  ${filePath}
    ${FileContents} =  Get File  ${filePath}
    Should Contain    ${FileContents}   ${expectedContent}


Clear AV Plugin Logs If They Are Close To Rotating
    ${AV_LOG_SIZE}=  Get File Size   ${AV_LOG_PATH}
    ${THREAT_DETECTOR_LOG_SIZE}=  Get File Size   ${THREAT_DETECTOR_LOG_PATH}
    ${SUSI_DEBUG_LOG_SIZE}=  Get File Size   ${SUSI_DEBUG_LOG_PATH}
    ${av_evaluation}=  Evaluate  ${AV_LOG_SIZE} / ${1000000} > ${9}
    ${susi_evaluation}=  Evaluate  ${SUSI_DEBUG_LOG_SIZE} / ${1000000} > ${9}
    ${threat_detector_evaluation}=  Evaluate  ${THREAT_DETECTOR_LOG_SIZE} / ${1000000} > ${9}

    run keyword if  ${av_evaluation} or ${susi_evaluation} or ${threat_detector_evaluation}  Restart AV Plugin And Clear The Logs
