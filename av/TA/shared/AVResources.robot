*** Settings ***
Library         Process
Library         OperatingSystem
Library         String
Library         ../Libs/AVScanner.py
Library         ../Libs/ExclusionHelper.py
Library         ../Libs/FileUtils.py
Library         ../Libs/LogUtils.py
Library         ../Libs/FakeManagement.py
Library         ../Libs/FakeManagementLog.py
Library         ../Libs/BaseUtils.py
Library         ../Libs/PluginUtils.py
Library         ../Libs/ProcessUtils.py
Library         ../Libs/SophosThreatDetector.py
Library         ../Libs/serialisationtools/CapnpHelper.py

Resource    GlobalSetup.robot
Resource    ComponentSetup.robot
Resource    RunShellProcess.robot

*** Variables ***
${AV_PLUGIN_PATH}                               ${COMPONENT_ROOT_PATH}
${AV_PLUGIN_BIN}                                ${COMPONENT_BIN_PATH}
${CLI_SCANNER_PATH}                             ${COMPONENT_ROOT_PATH}/bin/avscanner
${AV_LOG_PATH}                                  ${AV_PLUGIN_PATH}/log/${COMPONENT}.log
${SOPHOS_THREAT_DETECTOR_SHUTDOWN_FILE_PATH}    ${AV_PLUGIN_PATH}/chroot/var/threat_detector_expected_shutdown
${SOPHOS_THREAT_DETECTOR_PID_FILE_PATH}         ${AV_PLUGIN_PATH}/chroot/var/threat_detector.pid
${THREAT_DETECTOR_LOG_PATH}                     ${AV_PLUGIN_PATH}/chroot/log/sophos_threat_detector.log
${THREAT_DETECTOR_INFO_LOG_PATH}                ${AV_PLUGIN_PATH}/chroot/log/sophos_threat_detector.info.log
${SUSI_DEBUG_LOG_PATH}                          ${AV_PLUGIN_PATH}/chroot/log/susi_debug.log
${SCANNOW_LOG_PATH}                             ${AV_PLUGIN_PATH}/log/Scan Now.log
${CLOUDSCAN_LOG_PATH}                           ${AV_PLUGIN_PATH}/log/Sophos Cloud Scheduled Scan.log
${TELEMETRY_LOG_PATH}                           ${SOPHOS_INSTALL}/logs/base/sophosspl/telemetry.log
${WATCHDOG_BINARY}                              ${SOPHOS_INSTALL}/base/bin/sophos_watchdog
${WATCHDOG_LOG}                                 ${SOPHOS_INSTALL}/logs/base/watchdog.log
${SULDOWNLOADER_LOG}                            ${SOPHOS_INSTALL}/logs/base/suldownloader.log
${UPDATE_SCHEDULER}                             ${SOPHOS_INSTALL}/logs/base/sophosspl/updatescheduler.log
${CHROOT_LOGGING_SYMLINK}                       ${COMPONENT_ROOT_PATH}/chroot/${COMPONENT_ROOT_PATH}/log/sophos_threat_detector
${SUSI_STARTUP_SETTINGS_FILE}                   ${AV_PLUGIN_PATH}/var/susi_startup_settings.json
${SUSI_STARTUP_SETTINGS_FILE_CHROOT}            ${COMPONENT_ROOT_PATH}/chroot/${AV_PLUGIN_PATH}/var/susi_startup_settings.json
${AV_SDDS}                                      ${COMPONENT_SDDS}
${PLUGIN_SDDS}                                  ${COMPONENT_SDDS}
${PLUGIN_BINARY}                                ${SOPHOS_INSTALL}/plugins/${COMPONENT}/sbin/${COMPONENT}
${SOPHOS_THREAT_DETECTOR_BINARY}                ${SOPHOS_INSTALL}/plugins/${COMPONENT}/sbin/sophos_threat_detector
${SOPHOS_THREAT_DETECTOR_LAUNCHER}              ${SOPHOS_INSTALL}/plugins/${COMPONENT}/sbin/sophos_threat_detector_launcher
${EXPORT_FILE}                                  /etc/exports
${AV_INSTALL_LOG}                               /tmp/avplugin_install.log
${AV_RESTORED_LOGS_DIRECTORY}                   ${AV_PLUGIN_PATH}/log/downgrade-backup/
${NORMAL_DIRECTORY}                             /home/vagrant/this/is/a/directory/for/scanning
${MCS_DIR}                                      ${SOPHOS_INSTALL}/base/mcs

${CLEAN_STRING}         not an eicar
${EICAR_STRING}         X5O!P%@AP[4\\PZX54(P^)7CC)7}$EICAR-STANDARD-ANTIVIRUS-TEST-FILE!$H+H*
${EICAR_PUA_STRING}     X5]+)D:)D<5N*PZ5[/EICAR-POTENTIALLY-UNWANTED-OBJECT-TEST!$*M*L

${POLICY_7DAYS}     <daySet><day>monday</day><day>tuesday</day><day>wednesday</day><day>thursday</day><day>friday</day><day>saturday</day><day>sunday</day></daySet>
${STATUS_XML}       ${MCS_PATH}/status/SAV_status.xml

${IDE_DIR}          ${COMPONENT_INSTALL_SET}/files/plugins/av/chroot/susi/update_source/vdl
${INSTALL_IDE_DIR}  ${COMPONENT_ROOT_PATH}/chroot/susi/update_source/vdl
${SCAN_DIRECTORY}   /home/vagrant/this/is/a/directory/for/scanning
${AVSCANNER}        /usr/local/bin/avscanner

${AVSCANNER_TOTAL_CONNECTION_TIMEOUT_WAIT_PERIOD}  ${350}


*** Keywords ***
Check Plugin Running
    Run Shell Process  pidof ${PLUGIN_BINARY}   OnError=AV not running

Check AV Plugin Running
    Check Plugin Running

Check Sophos Threat Detector Running
    Run Shell Process  pidof ${SOPHOS_THREAT_DETECTOR_BINARY}   OnError=sophos_threat_detector not running

Run Sophos Threat Detector Directly
    ${threat_detector_handle} =  Start Process  ${SOPHOS_THREAT_DETECTOR_LAUNCHER}
    Set Suite Variable  ${THREAT_DETECTOR_PLUGIN_HANDLE}  ${threat_detector_handle}
    Register Cleanup   Terminate Process  ${THREAT_DETECTOR_PLUGIN_HANDLE}
    Wait until threat detector running

Restart threat detector once it stops
    Wait For Process  ${THREAT_DETECTOR_PLUGIN_HANDLE}
    Run Sophos Threat Detector Directly

Require Sophos Threat Detector Running
    ${result} =   ProcessUtils.pidof  ${SOPHOS_THREAT_DETECTOR_BINARY}
    Run Keyword If  ${result} == ${-1}  Run Sophos Threat Detector Directly

Check AV Plugin Not Running
    ${result} =   ProcessUtils.pidof  ${PLUGIN_BINARY}
    Should Be Equal As Integers  ${result}  ${-1}

Check Threat Detector Not Running
    ${result} =   ProcessUtils.pidof  ${SOPHOS_THREAT_DETECTOR_BINARY}
    Should Be Equal As Integers  ${result}  ${-1}

Count File Log Lines
    [Arguments]  ${path}
    ${status}  ${content} =  Run Keyword And Ignore Error
    ...      Get File   ${path}  encoding_errors=replace
    Return From Keyword If   '${status}' == 'FAIL'   ${0}
    ${count} =  Get Line Count   ${content}
    [Return]   ${count}

Count AV Log Lines
    ${count} =  Count File Log Lines   ${AV_LOG_PATH}
    [Return]   ${count}

Mark AV Log
    # Marks characters in the log, used for log checking, returns count of lines
    ${count} =  LogUtils.Mark AV Log
    # Marks the lines in the log, used for log splitting
    Set Suite Variable   ${AV_LOG_MARK}  ${count}
    Log  "AV LOG MARK = ${AV_LOG_MARK}"

Mark Sophos Threat Detector Log
    [Arguments]  ${mark}=""
    ${count} =  Count Optional File Log Lines  ${THREAT_DETECTOR_LOG_PATH}
    Set Suite Variable   ${SOPHOS_THREAT_DETECTOR_LOG_MARK}  ${count}
    Log  "SOPHOS_THREAT_DETECTOR LOG MARK = ${SOPHOS_THREAT_DETECTOR_LOG_MARK}"
    [Return]  ${count}

Mark Susi Debug Log
    ${count} =  Count File Log Lines  ${SUSI_DEBUG_LOG_PATH}
    Set Suite Variable   ${SUSI_DEBUG_LOG_MARK}  ${count}
    Log  "SUSI_DEBUG LOG MARK = ${SUSI_DEBUG_LOG_MARK}"

Mark Log
    [Arguments]  ${path}
    ${count} =  Count File Log Lines  ${path}
    Set Test Variable   ${LOG_MARK}  ${count}
    Log  "LOG MARK = ${LOG_MARK}"

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

File Log Contains One of
    [Arguments]  ${path}  ${offset}  @{inputs}
    ${content} =  Get File Contents From Offset  ${path}  ${offset}
    FOR   ${input}  IN  @{inputs}
        ${status} =  Run Keyword And Return Status  Should Contain  ${content}  ${input}
        Return From Keyword If  ${status}  ${status}
    END
    Fail  "None of inputs found in content"

File Log Should Not Contain
    [Arguments]  ${path}  ${input}
    ${content} =  Get File   ${path}  encoding_errors=replace
    Should Not Contain  ${content}  ${input}

File Log Should Not Contain With Offset
    [Arguments]  ${path}  ${input}  ${offset}=0
    ${content} =  Get File Contents From Offset  ${path}  ${offset}
    Should Not Contain  ${content}  ${input}

Wait Until File Log Contains One Of
    [Arguments]  ${logCheck}  ${timeout}  @{inputs}
    Wait Until Keyword Succeeds
    ...  ${timeout} secs
    ...  3 secs
    ...  ${logCheck}  @{inputs}

Wait Until File Log Contains
    [Arguments]  ${logCheck}  ${input}  ${timeout}=15  ${interval}=3
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
    LogUtils.Check Marked AV Log Contains   ${input}    ${offset}

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

Wait Until SUSI DEBUG Log Contains With Offset
    [Arguments]  ${input}  ${timeout}=15
    ${offset} =  Get Variable Value  ${SUSI_DEBUG_LOG_MARK}  0
    # TODO - doesn't actually wait!!
    File Log Contains With Offset  ${SUSI_DEBUG_LOG_PATH}   ${input}   offset=${offset}

Wait Until Sophos Threat Detector Log Contains
    [Arguments]  ${input}  ${timeout}=15
    Wait Until File Log Contains  Threat Detector Log Contains   ${input}   timeout=${timeout}

Sophos Threat Detector Log Contains With Offset
    [Arguments]  ${input}
    ${offset} =  Get Variable Value  ${SOPHOS_THREAT_DETECTOR_LOG_MARK}  0
    File Log Contains With Offset  ${THREAT_DETECTOR_LOG_PATH}   ${input}   offset=${offset}

Sophos Threat Detector Log Contains One of
    [Arguments]  @{inputs}
    ${offset} =  Get Variable Value  ${SOPHOS_THREAT_DETECTOR_LOG_MARK}  0
    File Log Contains One of  ${THREAT_DETECTOR_LOG_PATH}   ${offset}   @{inputs}

Threat Detector Log Should Not Contain With Offset
    [Arguments]  ${input}
    ${offset} =  Get Variable Value  ${SOPHOS_THREAT_DETECTOR_LOG_MARK}  0
    File Log Should Not Contain With Offset  ${THREAT_DETECTOR_LOG_PATH}   ${input}   offset=${offset}

Wait Until Sophos Threat Detector Log Contains With Offset
    [Arguments]  ${input}  ${timeout}=15
    Wait Until File Log Contains  Sophos Threat Detector Log Contains With Offset  ${input}   timeout=${timeout}

Wait Until Sophos Threat Detector Log Contains One of
    [Arguments]  ${timeout}  @{inputs}
    Wait Until File Log Contains One Of  Sophos Threat Detector Log Contains One Of  ${timeout}  @{inputs}

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
    Wait Until Sophos Threat Detector Log Contains  Copying "/opt/sophos-spl/base/etc/logger.conf" to: "/opt/sophos-spl/plugins/av/chroot/opt/sophos-spl/base/etc/logger.conf"
    Wait Until Sophos Threat Detector Log Contains  Copying "/opt/sophos-spl/base/etc/machine_id.txt" to: "/opt/sophos-spl/plugins/av/chroot/opt/sophos-spl/base/etc/machine_id.txt"
    Wait Until Sophos Threat Detector Log Contains  Copying "/opt/sophos-spl/plugins/av/VERSION.ini" to: "/opt/sophos-spl/plugins/av/chroot/opt/sophos-spl/plugins/av/VERSION.ini"
    #dns query files
    Wait Until Sophos Threat Detector Log Contains  Copying "/etc/nsswitch.conf" to: "/opt/sophos-spl/plugins/av/chroot/etc/nsswitch.conf"
    Wait Until Sophos Threat Detector Log Contains  Copying "/etc/resolv.conf" to: "/opt/sophos-spl/plugins/av/chroot/etc/resolv.conf"
    Wait Until Sophos Threat Detector Log Contains  Copying "/etc/ld.so.cache" to: "/opt/sophos-spl/plugins/av/chroot/etc/ld.so.cache"
    Wait Until Sophos Threat Detector Log Contains  Copying "/etc/host.conf" to: "/opt/sophos-spl/plugins/av/chroot/etc/host.conf"
    Wait Until Sophos Threat Detector Log Contains  Copying "/etc/hosts" to: "/opt/sophos-spl/plugins/av/chroot/etc/hosts"

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

Increase AV Log To Max Size
    [Arguments]  ${remaining}=1
    increase_threat_detector_log_to_max_size_by_path  ${AV_LOG_PATH}  ${remaining}

Wait Until AV Plugin Log Contains With Offset
    [Arguments]  ${input}  ${timeout}=15    ${interval}=2
    Wait Until File Log Contains  AV Plugin Log Contains With Offset  ${input}   timeout=${timeout}  interval=${interval}

Wait Until Sophos Threat Detector Shutdown File Exists
    [Arguments]  ${timeout}=15    ${interval}=2
    Wait Until File Exists  ${SOPHOS_THREAT_DETECTOR_SHUTDOWN_FILE_PATH}  timeout=${timeout}  interval=${interval}

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

Check Plugin Installed and Running With Offset
    File Should Exist   ${PLUGIN_BINARY}
    Wait until AV Plugin running with offset
    Wait until threat detector running with offset

Wait until AV Plugin running
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  2 secs
    ...  Check Plugin Running
    Wait Until Keyword Succeeds
    ...  40 secs
    ...  2 secs
    ...  Plugin Log Contains  ${COMPONENT} <> Starting scanScheduler
    # Only output in debug mode:
    # ...  Plugin Log Contains  ${COMPONENT} <> Starting the main program loop

Wait until AV Plugin running with offset
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  2 secs
    ...  Check Plugin Running
    Wait Until AV Plugin Log Contains With Offset  ${COMPONENT} <> Starting scanScheduler  timeout=40

Wait until threat detector running
    # wait for sophos_threat_detector to initialize
    ProcessUtils.wait_for_pid  ${SOPHOS_THREAT_DETECTOR_BINARY}  ${30}
    Wait Until Keyword Succeeds
    ...  60 secs
    ...  2 secs
    ...  Threat Detector Log Contains  SophosThreatDetectorImpl <> Starting USR1 monitor
    # Only output in debug mode:
    # ...  Threat Detector Log Contains  UnixSocket <> Starting listening on socket: /var/process_control_socket

Wait until threat detector running with offset
    [Arguments]  ${timeout}=${60}
    ProcessUtils.wait_for_pid  ${SOPHOS_THREAT_DETECTOR_BINARY}  ${timeout}
    Wait Until Sophos Threat Detector Log Contains With Offset
    ...  SophosThreatDetectorImpl <> Starting USR1 monitor
    ...  timeout=${timeout}

Wait until threat detector not running
    [Arguments]  ${timeout}=30
    Wait Until Keyword Succeeds
    ...  ${timeout} secs
    ...  3 secs
    ...  Check Threat Detector Not Running

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
    # restart the service to apply the new log level to watchdog
    Run Shell Process  systemctl restart sophos-spl  OnError=failed to restart sophos-spl
    # Stop MCS router since we haven't configured Central
    Run Keyword and Ignore Error   Run Shell Process    /opt/sophos-spl/bin/wdctl stop mcsrouter  OnError=Failed to stop mcsrouter

    Install AV Directly from SDDS
    Wait Until AV Plugin Log Contains  Starting sophos_threat_detector monitor
    Wait Until Sophos Threat Detector Log Contains  Starting listening on socket: /var/process_control_socket  timeout=120

Uninstall And Revert Setup
    Uninstall All
    Setup Base And Component

Uninstall and full reinstall
    Uninstall All
    Install With Base SDDS

Install Base For Component Tests
    File Should Exist     ${BASE_SDDS}/install.sh
    Run Process  chmod  +x  ${BASE_SDDS}/install.sh
    Run Process  chmod  +x  ${BASE_SDDS}/files/base/bin/*
    ${result} =   Run Process   bash  ${BASE_SDDS}/install.sh  timeout=600s    stderr=STDOUT
    Should Be Equal As Integers  ${result.rc}  ${0}   "Failed to install base.\noutput: \n${result.stdout}"
    # Check watchdog running
    ProcessUtils.wait_for_pid  ${WATCHDOG_BINARY}  ${5}
    # Stop MCS router since we haven't configured Central
    Run Keyword and Ignore Error   Run Shell Process    /opt/sophos-spl/bin/wdctl stop mcsrouter  OnError=Failed to stop mcsrouter

Install AV Directly from SDDS
    Mark AV Log
    Mark Sophos Threat Detector Log

    ${install_log} =  Set Variable   ${AV_INSTALL_LOG}
    ${result} =   Run Process   bash  -x  ${AV_SDDS}/install.sh   timeout=60s  stderr=STDOUT   stdout=${install_log}
    ${log_contents} =  Get File  ${install_log}
    File Log Should Not Contain  ${AV_INSTALL_LOG}  chown: cannot access
    Should Be Equal As Integers  ${result.rc}  ${0}   "Failed to install plugin.\noutput: \n${log_contents}"

    Wait until AV Plugin running with offset
    Wait until threat detector running with offset

Require Plugin Installed and Running
    [Arguments]  ${LogLevel}=DEBUG
    Install Base if not installed

    Set Log Level  ${LogLevel}
    Install AV if not installed
    Start AV Plugin if not running
    ${pid} =  Start Sophos Threat Detector if not running
    Log  sophos_threat_detector PID = ${pid}

Display All SSPL Files Installed
    ${handle}=  Start Process  find ${SOPHOS_INSTALL} | grep -v python | grep -v primarywarehouse | grep -v temp_warehouse | grep -v TestInstallFiles | grep -v lenses   shell=True
    ${result}=  Wait For Process  ${handle}  timeout=30  on_timeout=kill
    Log  ${result.stdout}
    Log  ${result.stderr}

AV And Base Teardown
    Run Keyword If Test Failed   Display All SSPL Files Installed
    Register On Fail  dump log  ${SOPHOS_INSTALL}/logs/base/watchdog.log
    Register On Fail  dump log  ${SOPHOS_INSTALL}/logs/base/sophosspl/sophos_managementagent.log
    Register On Fail  dump log  ${THREAT_DETECTOR_LOG_PATH}
    Register On Fail  dump log  ${SUSI_DEBUG_LOG_PATH}
    Register On Fail  dump log  ${AV_LOG_PATH}
    Register On Fail  dump log  ${TELEMETRY_LOG_PATH}
    Register On Fail  dump log  ${AV_INSTALL_LOG}

    #Just in case something restarted it, stop mcsrouter
    Run Shell Process  ${SOPHOS_INSTALL}/bin/wdctl stop mcsrouter   OnError=failed to stop plugin

    Run Cleanup Functions

    #mark errors related to scheduled scans being forcefully terminated at the end of a test
    Exclude Failed To Scan Multiple Files Cloud
    Exclude UnixSocket Interrupted System Call Error Cloud Scan
    Exclude SPPLAV Processes Are Killed With SIGKILL
    Exclude Watchdog Log Unable To Open File Error
    Exclude Expected Sweep Errors
    #mark generic errors caused by the lack of a central connection/warehouse/subscription
    Exclude Invalid Settings No Primary Product
    Exclude Configuration Data Invalid
    Exclude CustomerID Failed To Read Error
    Exclude Failed To connect To Warehouse Error
    Exclude Invalid Day From Policy
    Exclude Core Not In Policy Features
    Exclude SPL Base Not In Subscription Of The Policy
    Exclude UpdateScheduler Fails
    Exclude MCS Router is dead
    Check All Product Logs Do Not Contain Error

    Run Failure Functions If Failed

    Run Keyword If Test Failed   Restart AV Plugin And Clear The Logs For Integration Tests

Restart AV Plugin And Clear The Logs For Integration Tests
    Log  Logs have to be rotated
    Run Shell Process  ${SOPHOS_INSTALL}/bin/wdctl stop av   OnError=failed to stop plugin
    Run Shell Process  ${SOPHOS_INSTALL}/bin/wdctl stop threat_detector   OnError=failed to stop sophos_threat_detector
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  2 secs
    ...  Check AV Plugin Not Running
    Run Shell Process  systemctl stop sophos-spl  OnError=failed to stop plugin

    Log  Backup logs before removing them
    Dump Log  ${AV_LOG_PATH}
    Dump Log  ${THREAT_DETECTOR_LOG_PATH}
    Dump Log  ${SUSI_DEBUG_LOG_PATH}
    Dump Log  ${SCANNOW_LOG_PATH}
    Dump Log  ${CLOUDSCAN_LOG_PATH}
    Dump Log  ${WATCHDOG_LOG}

    Remove File    ${AV_LOG_PATH}
    Remove File    ${THREAT_DETECTOR_LOG_PATH}
    Remove File    ${SUSI_DEBUG_LOG_PATH}
    Remove File    ${THREAT_DETECTOR_INFO_LOG_PATH}
    Remove File    ${WATCHDOG_LOG}
    Remove File    ${CLOUDSCAN_LOG_PATH}
    Remove File    ${UPDATE_SCHEDULER}
    Remove File    ${SULDOWNLOADER_LOG}
    Remove File    ${SCANNOW_LOG_PATH}

    Empty Directory  /opt/sophos-spl/base/mcs/event/

    Run Shell Process  systemctl start sophos-spl  OnError=failed to stop plugin
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


Get File Size or Zero
    [Arguments]  ${filename}
    ${status}  ${size} =   Run Keyword And Ignore Error
    ...          Get File Size  ${filename}
    Return From Keyword If   '${status}' == 'FAIL'   ${0}
    [Return]   ${size}

File size is different
    [Arguments]  ${filename}  ${old_size}
    ${new_size} =   Get File Size or Zero   ${filename}
    Should Be True   ${new_size} != ${old_size}

Wait until file size changes
    [Arguments]  ${filename}  ${timeout}=15
    ${orig_size} =   Get File Size or Zero  ${filename}
    Wait Until Keyword Succeeds
        ...    ${timeout} secs
        ...    1 secs
        ...    File size is different  ${filename}  ${orig_size}

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
    Wait Until AV Plugin Log Contains With Offset  Configured number of Scheduled Scans  timeout=180

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
    Copy File If Destination Missing  ${EXPORT_FILE}  ${EXPORT_FILE}_bkp
    Ensure List appears once   ${EXPORT_FILE}  ${source} localhost(rw,sync,no_subtree_check)\n
    Register On Fail  Run Process  systemctl  status  nfs-server
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
        ...    5 secs
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

Run installer from install set and wait for reload trigger
    [Arguments]  ${threat_detector_pid}  ${mark}
    Run installer from install set
    Wait Until Sophos Threat Detector Logs Or Restarts  ${threat_detector_pid}  ${mark}  Reload triggered by USR1  timeout=60

Run IDE update with expected texts
    [Arguments]  ${timeout}  @{expected_update_texts}
    # TODO Improve "Mark Sophos Threat Detector Log" (& related functions) to enable multiple marks in one file so it doesn't clobber any marks used for testing LINUXDAR-2677
    ${mark} =  Mark Sophos Threat Detector Log
    ${threat_detector_pid} =  Record Sophos Threat Detector PID
    Run installer from install set and wait for reload trigger  ${threat_detector_pid}  ${mark}
    Wait Until Sophos Threat Detector Log Contains One Of  ${timeout}  @{expected_update_texts}
    Threat Detector Log Should Not Contain With Offset    Current version matches that of the update source. Nothing to do.
    Check Sophos Threat Detector Has Same PID  ${threat_detector_pid}

Run IDE update with expected text
    [Arguments]  ${expected_update_text}  ${timeout}=120
    # TODO Improve "Mark Sophos Threat Detector Log" (& related functions) to enable multiple marks in one file so it doesn't clobber any marks used for testing LINUXDAR-2677
    ${mark} =  Mark Sophos Threat Detector Log
    ${threat_detector_pid} =  Record Sophos Threat Detector PID
    Run installer from install set and wait for reload trigger  ${threat_detector_pid}  ${mark}
    Wait Until Sophos Threat Detector Logs Or Restarts  ${threat_detector_pid}  ${mark}  ${expected_update_text}  timeout=${timeout}
    # Wait Until Sophos Threat Detector Log Contains With Offset  ${expected_update_text}  timeout=${timeout}
    Threat Detector Log Should Not Contain With Offset    Current version matches that of the update source. Nothing to do.
    Check Sophos Threat Detector Has Same PID  ${threat_detector_pid}

Run IDE update with SUSI loaded
    # Require that SUSI has been initialized
    Run IDE update with expected text  Threat scanner successfully updated  timeout=120

Run IDE update
    Run IDE update with expected texts  120  Threat scanner successfully updated  Threat scanner update is pending

Run IDE update without SUSI loaded
    # Require SUSI hasn't been initialized, so won't actually update
    Run IDE update with expected text  Threat scanner update is pending  timeout=10

Install IDE with install func
    [Arguments]  ${ide_name}  ${ide_update_func}
    SophosThreatDetector.register ide for uninstall  ${ide_name}
    Register cleanup if unique  cleanup ides

    Add IDE to install set  ${ide_name}
    Run Keyword  ${ide_update_func}
    Check IDE present in installation  ${ide_name}

Install IDE with SUSI loaded
    [Arguments]  ${ide_name}
    Install IDE with install func  ${ide_name}  Run IDE update with SUSI loaded

Install IDE without SUSI loaded
    [Arguments]  ${ide_name}
    Install IDE with install func  ${ide_name}  Run IDE update without SUSI loaded

Install IDE without reload check
    [Arguments]  ${ide_name}
    Install IDE with install func  ${ide_name}  Run installer from install set

Install IDE
    [Arguments]  ${ide_name}
    Install IDE with SUSI loaded  ${ide_name}

Uninstall IDE
    [Arguments]  ${ide_name}  ${ide_update_func}=Run IDE update
    # We don't know how the cleanup was registered
    Deregister Optional Cleanup   Uninstall IDE  ${ide_name}  ${ide_update_func}
    Deregister Optional Cleanup   Uninstall IDE  ${ide_name}
    SophosThreatDetector.deregister ide for uninstall  ${ide_name}
    Remove IDE From Install Set  ${ide_name}
    Run Keyword  ${ide_update_func}
    Check IDE Absent From Installation  ${ide_name}


Get Pid
    [Arguments]  ${EXEC}
    ${PID} =  ProcessUtils.pidof or fail  ${EXEC}
    [Return]   ${PID}

Record AV Plugin PID
    ${PID} =  ProcessUtils.wait for pid  ${PLUGIN_BINARY}  ${5}
    [Return]   ${PID}

Record Sophos Threat Detector PID
    # We need to wait long enough for sophos_threat_detector to be restarted
    # avplugin triggers restart of sophos_threat_detector + 10 seconds for watchdog to restart it
    # It took 10 seconds for watchdog to detect sophos_threat_detector had exited - so allow 30 seconds here
    ${PID} =  ProcessUtils.wait for pid  ${SOPHOS_THREAT_DETECTOR_BINARY}  ${30}
    [Return]   ${PID}

Get Sophos Threat Detector PID From File
    ${PID} =  Get File    ${SOPHOS_THREAT_DETECTOR_PID_FILE_PATH}
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

Wait For Sophos Threat Detector to restart
    [Arguments]  ${PID}
    ProcessUtils.wait for different pid  ${SOPHOS_THREAT_DETECTOR_BINARY}  ${PID}  30

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
    [Arguments]     ${expectedContent}  ${filePath}  ${timeout}=20
    Wait Until Keyword Succeeds
    ...  ${timeout} secs
    ...  5 secs
    ...  Check Specific File Content  ${expectedContent}  ${filePath}
    Log File  ${filePath}

Check Specific File Content
    [Arguments]     ${expectedContent}  ${filePath}
    ${FileContents} =  Get File  ${filePath}
    Should Contain    ${FileContents}   ${expectedContent}

Get ALC Policy
    [Arguments]  ${revid}=${EMPTY}  ${algorithm}=Clear  ${username}=B  ${userpassword}=A
    ${policyContent} =  Catenate   SEPARATOR=${\n}
    ...   <?xml version="1.0"?>
    ...   <AUConfigurations xmlns:csc="com.sophos\\msys\\csc" xmlns="http://www.sophos.com/EE/AUConfig">
    ...     <csc:Comp RevID="${revid}" policyType="1"/>
    ...     <AUConfig>
    ...       <primary_location>
    ...         <server Algorithm="${algorithm}" UserPassword="${userpassword}" UserName="${username}"/>
    ...       </primary_location>
    ...     </AUConfig>
    ...   </AUConfigurations>
    ${policyContent} =   Replace Variables   ${policyContent}
    [Return]   ${policyContent}

Get SAV Policy
    [Arguments]  ${revid}=${EMPTY}  ${sxlLookupEnabled}=A
    ${policyContent} =  Catenate   SEPARATOR=${\n}
    ...   <?xml version="1.0"?>
    ...   <config>
    ...       <csc:Comp RevID="${revid}" policyType="2"/>
    ...       <detectionFeedback>
    ...           <sendData>${sxlLookupEnabled}</sendData>
    ...       </detectionFeedback>
    ...   </config>
    ${policyContent} =   Replace Variables   ${policyContent}
    [Return]   ${policyContent}

Check If The Logs Are Close To Rotating
    ${AV_LOG_SIZE}=  Get File Size in MB   ${AV_LOG_PATH}
    ${THREAT_DETECTOR_LOG_SIZE}=  Get File Size in MB   ${THREAT_DETECTOR_LOG_PATH}
    ${SUSI_DEBUG_LOG_SIZE}=  Get File Size in MB   ${SUSI_DEBUG_LOG_PATH}

    ${av_evaluation}=  Evaluate  ${AV_LOG_SIZE} > ${0.5}
    ${susi_evaluation}=  Evaluate  ${SUSI_DEBUG_LOG_SIZE} > ${9}
    ${threat_detector_evaluation}=  Evaluate  ${THREAT_DETECTOR_LOG_SIZE} > ${9}

    [return]  ${av_evaluation} or ${susi_evaluation} or ${threat_detector_evaluation}

Clear AV Plugin Logs If They Are Close To Rotating For Integration Tests
    ${result} =     Check If The Logs Are Close To Rotating
    run keyword if  ${result}  Restart AV Plugin And Clear The Logs For Integration Tests


Check avscanner can detect eicar in
    [Arguments]  ${EICAR_PATH}  ${LOCAL_AVSCANNER}=${AVSCANNER}
    ${rc}   ${output} =    Run And Return Rc And Output   ${LOCAL_AVSCANNER} ${EICAR_PATH}
    Log   ${output}
    Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}
    Should Contain   ${output}    Detected "${EICAR_PATH}" is infected with EICAR-AV-Test


Check avscanner can detect eicar
    [Arguments]  ${LOCAL_AVSCANNER}=${AVSCANNER}
    Create File     ${SCAN_DIRECTORY}/eicar.com    ${EICAR_STRING}
    Check avscanner can detect eicar in  ${SCAN_DIRECTORY}/eicar.com   ${LOCAL_AVSCANNER}

Force SUSI to be initialized
    Check avscanner can detect eicar  ${CLI_SCANNER_PATH}
