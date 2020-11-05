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

*** Variables ***
${AV_PLUGIN_PATH}  ${COMPONENT_ROOT_PATH}
${AV_PLUGIN_BIN}   ${COMPONENT_BIN_PATH}
${CLI_SCANNER_PATH}      ${COMPONENT_ROOT_PATH}/bin/avscanner
${AV_LOG_PATH}     ${AV_PLUGIN_PATH}/log/${COMPONENT}.log
${THREAT_DETECTOR_LOG_PATH}     ${AV_PLUGIN_PATH}/chroot/log/sophos_threat_detector.log
${SCANNOW_LOG_PATH}     ${AV_PLUGIN_PATH}/log/Scan Now.log
${TELEMETRY_LOG_PATH}   ${SOPHOS_INSTALL}/logs/base/sophosspl/telemetry.log
${AV_SDDS}         ${COMPONENT_SDDS}
${PLUGIN_SDDS}     ${COMPONENT_SDDS}
${PLUGIN_BINARY}   ${SOPHOS_INSTALL}/plugins/${COMPONENT}/sbin/${COMPONENT}
${SOPHOS_THREAT_DETECTOR_BINARY}  ${SOPHOS_INSTALL}/plugins/${COMPONENT}/sbin/sophos_threat_detector
${EXPORT_FILE}     /etc/exports
${SAMBA_CONFIG}    /etc/samba/smb.conf
${EICAR_STRING}  X5O!P%@AP[4\\PZX54(P^)7CC)7}$EICAR-STANDARD-ANTIVIRUS-TEST-FILE!$H+H*
${EICAR_PUA_STRING}  X5]+)D:)D<5N*PZ5[/EICAR-POTENTIALLY-UNWANTED-OBJECT-TEST!$*M*L

${POLICY_7DAYS}     <daySet><day>monday</day><day>tuesday</day><day>wednesday</day><day>thursday</day><day>friday</day><day>saturday</day><day>sunday</day></daySet>
${STATUS_XML}       ${MCS_PATH}/status/SAV_status.xml

*** Keywords ***
Run Shell Process
    [Arguments]  ${Command}   ${OnError}   ${timeout}=20s
    ${result} =   Run Process  ${Command}   shell=True   timeout=${timeout}
    Should Be Equal As Integers  ${result.rc}  ${0}   "${OnError}.\n${SPACE}stdout: \n${result.stdout} \n${SPACE}stderr: \n${result.stderr}"

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
    ${count} =  Count File Log Lines  ${THREAT_DETECTOR_LOG_PATH}
    Set Test Variable   ${SOPHOS_THREAT_DETECTOR_LOG_MARK}  ${count}
    Log  "SOPHOS_THREAT_DETECTOR LOG MARK = ${SOPHOS_THREAT_DETECTOR_LOG_MARK}"

Get File Contents From Offset
    [Arguments]  ${path}  ${offset}=0
    ${content} =  Get File   ${path}  encoding_errors=replace
    Log   "Skipping ${offset} lines"
    @{lines} =  Split To Lines   ${content}  ${offset}
    [Return]  Catenate  SEPARATOR=\n  @{lines}

File Log Contains With Offset
    [Arguments]  ${path}  ${input}  ${offset}=0
    ${content} =  Get File Contents From Offset  ${path}  ${offset}
    Should Contain  ${content}  ${input}

File Log Should Not Contain
    [Arguments]  ${path}  ${input}
    ${content} =  Get File   ${path}  encoding_errors=replace
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
    ...  Keyword '${logCheck}' failed after retrying for .* seconds.*does not contain '${input}'
    ...  Wait Until Keyword Succeeds
    ...    15 secs
    ...    1 secs
    ...    ${logCheck}  ${input}

AV Plugin Log Contains With Offset
    [Arguments]  ${input}
    ${offset} =  Get Variable Value  ${AV_LOG_MARK}  0
    File Log Contains With Offset  ${AV_LOG_PATH}   ${input}   offset=${offset}

AV Plugin Log Contains
    [Arguments]  ${input}
    LogUtils.File Log Contains  ${AV_LOG_PATH}   ${input}

Threat Detector Log Contains
    [Arguments]  ${input}
    File Log Contains  ${THREAT_DETECTOR_LOG_PATH}   ${input}

Threat Detector Does Not Log Contain
    [Arguments]  ${input}
    File Log Should Not Contain  ${THREAT_DETECTOR_LOG_PATH}  ${input}

Check Threat Detector Copied Files To Chroot
    ${file} =  Get File    ${THREAT_DETECTOR_LOG_PATH}
    Log to Console  ${file}

    Threat Detector Log Contains  Copying "/opt/sophos-spl/base/etc/logger.conf" to: "/opt/sophos-spl/plugins/av/chroot/opt/sophos-spl/base/etc/logger.conf"
    Threat Detector Log Contains  Copying "/opt/sophos-spl/base/etc/machine_id.txt" to: "/opt/sophos-spl/plugins/av/chroot/opt/sophos-spl/base/etc/machine_id.txt"
    Threat Detector Log Contains  Copying "/opt/sophos-spl/base/update/var/update_config.json" to: "/opt/sophos-spl/plugins/av/chroot/opt/sophos-spl/base/update/var/update_config.json"
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
    increase_threat_detector_log_to_max_size_by_path  ${THREAT_DETECTOR_LOG_PATH}

Wait Until AV Plugin Log Contains With Offset
    [Arguments]  ${input}  ${timeout}=15
    Wait Until File Log Contains  AV Plugin Log Contains With Offset  ${input}   timeout=${timeout}

Wait Until AV Plugin Log Contains
    [Arguments]  ${input}  ${timeout}=15  ${interval}=1
    Wait Until File Log Contains  AV Plugin Log Contains   ${input}   timeout=${timeout}  interval=${interval}

AV Plugin Log Does Not Contain
    [Arguments]  ${input}
    LogUtils.Over next 15 seconds ensure log does not contain   ${AV_LOG_PATH}  ${input}

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
    ...  3 secs
    ...  Check Plugin Running
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  3 secs
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
    ${result} =   Run Process   ${BASE_SDDS}install.sh  timeout=600s    stderr=STDOUT
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
    Run Keyword If Test Failed   Display All SSPL Files Installed
    Run Shell Process  ${SOPHOS_INSTALL}/bin/wdctl stop av   OnError=failed to stop plugin
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  2 secs
    ...  Check AV Plugin Not Running
    Run Keyword If Test Failed   Run Keyword And Ignore Error  Log File   ${SOPHOS_INSTALL}/logs/base/watchdog.log  encoding_errors=replace
    Run Keyword If Test Failed   Run Keyword And Ignore Error  Log File   ${SOPHOS_INSTALL}/logs/base/sophosspl/sophos_managementagent.log  encoding_errors=replace
    Run Keyword If Test Failed   Run Keyword And Ignore Error  Log File   ${THREAT_DETECTOR_LOG_PATH}  encoding_errors=replace
    Run Keyword If Test Failed   Run Keyword And Ignore Error  Log File   ${AV_LOG_PATH}  encoding_errors=replace
    Run Keyword If Test Failed   Run Keyword And Ignore Error  Log File   ${TELEMETRY_LOG_PATH}  encoding_errors=replace
    Remove File    ${AV_LOG_PATH}
    Remove File    ${THREAT_DETECTOR_LOG_PATH}
    Empty Directory  /opt/sophos-spl/base/mcs/event/
    Run Shell Process  ${SOPHOS_INSTALL}/bin/wdctl start av   OnError=failed to start plugin

Create Install Options File With Content
    [Arguments]  ${installFlags}
    Create File  ${SOPHOS_INSTALL}/base/etc/install_options  ${installFlags}
    #TODO set permissions

Check ScanNow Log Exists
    File Should Exist  ${SCANNOW_LOG_PATH}

Wait until scheduled scan updated
    Wait Until AV Plugin Log Contains  Configured number of Scheduled Scans  timeout=240

Configure Scan Exclusions Everything Else
    [Arguments]  ${inclusion}
    ${exclusions} =  exclusions_for_everything_else  ${inclusion}
    Log  "Excluding all directories except: ${inclusion}"
    Log  "Excluding the following directories: ${exclusions}"
    [return]  ${exclusions}

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

Create Local SMB Share
    [Arguments]  ${source}  ${destination}
    Copy File  ${SAMBA_CONFIG}  ${SAMBA_CONFIG}_bkp
    ${share_config}=  catenate   SEPARATOR=
    ...   [testSamba]\n
    ...   comment = SMB Share\n
    ...   path = ${source}\n
    ...   browseable = yes\n
    ...   read only = yes\n
    ...   guest ok = yes\n
    Append To File  ${SAMBA_CONFIG}   ${share_config}

    Restart Samba
    Run Shell Process   mount -t cifs \\\\\\\\localhost\\\\testSamba ${destination} -o guest   OnError=Failed to mount local SMB share

Remove Local SMB Share
    [Arguments]  ${source}  ${destination}
    Run Shell Process   umount ${destination}   OnError=Failed to unmount local SMB server
    Move File  ${SAMBA_CONFIG}_bkp  ${SAMBA_CONFIG}
    Restart Samba
    Remove Directory    ${source}  recursive=True
    
Check Scan Now Configuration File is Correct
    ${configFilename} =  Set Variable  /tmp/config-files-test/Scan_Now.config
    Wait Until Keyword Succeeds
        ...    15 secs
        ...    1 secs
        ...    File Should Exist  ${configFilename}
    @{exclusions} =  ExclusionHelper.get exclusions to scan tmp
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
    # TODO LINUXDAR-1482 Update this to check all the configuration is correct - run the test and see what's outputted first
    # TODO LINUXDAR-1482 Make the check more complicated so we check the list attributes
    @{exclusions} =  ExclusionHelper.get exclusions to scan tmp
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
    [Arguments]  ${CDDVDDrives}=false  ${hardDrives}=false  ${networkDrives}=false  ${removableDrives}=false
    [return]    <scanObjectSet><CDDVDDrives>${CDDVDDrives}</CDDVDDrives><hardDrives>${hardDrives}</hardDrives><networkDrives>${networkDrives}</networkDrives><removableDrives>${removableDrives}</removableDrives></scanObjectSet>

Create EICAR files
    [Arguments]  ${eicar_files_to_create}  ${dir_name}
     : FOR    ${INDEX}    IN RANGE    1    ${eicar_files_to_create}
        \    ${eicar_file}=    create file  ${dir_name}/eicar-${INDEX}  ${EICAR_STRING}

Restart Samba
    ${result} =  Run Process  which  yum
    Run Keyword If  "${result.rc}" == "0"
        ...   Run Shell Process   systemctl restart smb  OnError=Failed to restart SMB server
        ...   ELSE
        ...   Run Shell Process   systemctl restart smbd   OnError=Failed to restart SMB server
