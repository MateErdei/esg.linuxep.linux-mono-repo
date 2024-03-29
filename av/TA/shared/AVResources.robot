*** Settings ***
Library         Collections
Library         Process
Library         OperatingSystem
Library         String
Library         ../Libs/AVScanner.py
Library         ../Libs/BaseUtils.py
Library         ${COMMON_TEST_LIBS}/CoreDumps.py
Library         ../Libs/BaseInteractionTools/PolicyUtils.py
Library         ../Libs/ExclusionHelper.py
Library         ../Libs/FileUtils.py
Library         ${COMMON_TEST_LIBS}/LogUtils.py
Library         ../Libs/FakeManagement.py
Library         ../Libs/FakeWatchdog.py
Library         ../Libs/FakeManagementLog.py
Library         ../Libs/BaseUtils.py
Library         ../Libs/PluginUtils.py
Library         ${COMMON_TEST_LIBS}/ProcessUtils.py
Library         ../Libs/SophosThreatDetector.py
Library         ../Libs/serialisationtools/CapnpHelper.py

Resource    ComponentSetup.robot
Resource    GlobalSetup.robot
Resource    DumpLog.robot
Resource    ${COMMON_TEST_ROBOT}/SafeStoreResources.robot
Resource    ${COMMON_TEST_ROBOT}/AVResources.robot
Resource    ErrorMarkers.robot

*** Variables ***
${AV_PLUGIN_PATH}                               ${COMPONENT_ROOT_PATH}
${AV_PLUGIN_BIN}                                ${COMPONENT_BIN_PATH}
${CLI_SCANNER_PATH}                             ${COMPONENT_ROOT_PATH}/bin/avscanner
${AV_LOG_PATH}                                  ${AV_PLUGIN_PATH}/log/${COMPONENT}.log
${AV_CHROOT_DIR}                                ${AV_PLUGIN_PATH}/chroot
${AV_VAR_DIR}                                   ${AV_PLUGIN_PATH}/var
${AV_CHROOT_VAR_DIR}                            ${AV_CHROOT_DIR}/var
${AV_CHROOT_LOG_DIR}                            ${AV_CHROOT_DIR}/log
${SOPHOS_THREAT_DETECTOR_SHUTDOWN_FILE_PATH}    ${AV_CHROOT_VAR_DIR}/threat_detector_expected_shutdown
${SOPHOS_THREAT_DETECTOR_PID_FILE_PATH}         ${AV_CHROOT_VAR_DIR}/threat_detector.pid
${ON_ACCESS_LOG_PATH}                           ${AV_PLUGIN_PATH}/log/soapd.log
${THREAT_DETECTOR_LOG_PATH}                     ${AV_CHROOT_LOG_DIR}/sophos_threat_detector.log
${THREAT_DETECTOR_INFO_LOG_PATH}                ${AV_CHROOT_LOG_DIR}/sophos_threat_detector.info.log
${SUSI_DEBUG_LOG_PATH}                          ${AV_CHROOT_DIR}/log/susi_debug.log
${SCANNOW_LOG_PATH}                             ${AV_PLUGIN_PATH}/log/Scan Now.log
${CLOUDSCAN_LOG_PATH}                           ${AV_PLUGIN_PATH}/log/Sophos Cloud Scheduled Scan.log
${TELEMETRY_LOG_PATH}                           ${SOPHOS_INSTALL}/logs/base/sophosspl/telemetry.log
${WATCHDOG_BINARY}                              ${SOPHOS_INSTALL}/base/bin/sophos_watchdog
${WATCHDOG_LOG}                                 ${SOPHOS_INSTALL}/logs/base/watchdog.log
${SULDOWNLOADER_LOG}                            ${SOPHOS_INSTALL}/logs/base/suldownloader.log
${UPDATE_SCHEDULER}                             ${SOPHOS_INSTALL}/logs/base/sophosspl/updatescheduler.log
${COMPONENT_ROOT_PATH_CHROOT}                   ${COMPONENT_ROOT_PATH}/chroot/${COMPONENT_ROOT_PATH}
${CHROOT_LOGGING_SYMLINK}                       ${COMPONENT_ROOT_PATH_CHROOT}/log/sophos_threat_detector
${SUSI_STARTUP_SETTINGS_FILE}                   ${AV_PLUGIN_PATH}/var/susi_startup_settings.json
${SUSI_STARTUP_SETTINGS_FILE_CHROOT}            ${COMPONENT_ROOT_PATH_CHROOT}/var/susi_startup_settings.json
${AV_SDDS}                                      ${COMPONENT_INSTALL_SET}
${PLUGIN_BINARY}                                ${COMPONENT_ROOT_PATH}/sbin/${COMPONENT}
${SCHEDULED_FILE_WALKER_LAUNCHER}               ${COMPONENT_ROOT_PATH}/sbin/scheduled_file_walker_launcher
${ON_ACCESS_BIN}                                ${COMPONENT_ROOT_PATH}/sbin/soapd
${SAFESTORE_BIN}                                ${COMPONENT_ROOT_PATH}/sbin/safestore
${SOPHOS_THREAT_DETECTOR_BINARY}                ${COMPONENT_ROOT_PATH}/sbin/sophos_threat_detector
${SOPHOS_THREAT_DETECTOR_LAUNCHER}              ${COMPONENT_ROOT_PATH}/sbin/sophos_threat_detector_launcher
${EXPORT_FILE}                                  /etc/exports
${AV_INSTALL_LOG}                               /tmp/avplugin_install.log
${AV_UNINSTALL_LOG}                             /tmp/avplugin_uninstall.log
${AV_BACKUP_DIR}                                ${SOPHOS_INSTALL}/tmp/av_downgrade/
${SAFESTORE_BACKUP_DIR}                         ${SOPHOS_INSTALL}/tmp/safestore_downgrade
${AV_RESTORED_LOGS_DIRECTORY}                   ${AV_PLUGIN_PATH}/log/downgrade-backup/
${AV_RESTORED_VAR_DIRECTORY}                    ${AV_PLUGIN_PATH}/var/downgrade-backup
${NORMAL_DIRECTORY}                             /home/vagrant/this/is/a/directory/for/scanning
${MCS_DIR}                                      ${SOPHOS_INSTALL}/base/mcs
${TESTTMP}                                      /tmp_test/SSPLAVTests
${METADATA_RESCAN_SOCKET}                       ${AV_CHROOT_VAR_DIR}/metadata_rescan_socket

${CLEAN_STRING}         not an eicar
${EICAR_STRING}         X5O!P%@AP[4\\PZX54(P^)7CC)7}$EICAR-STANDARD-ANTIVIRUS-TEST-FILE!$H+H*
${EICAR_PUA_STRING}     X5]+)D:)D<5N*PZ5[/EICAR-POTENTIALLY-UNWANTED-OBJECT-TEST!$*M*L
${DSA_BY_NAME_STRING}   UNIQUE-NONPE-TESTFILE-STUB_1ze492x5f_239c42qe_2342eee_249f3dea_DSA_DET_NO_FORCE_LOOKUP__SUPP_BY_NAME

${POLICY_7DAYS}     <daySet><day>monday</day><day>tuesday</day><day>wednesday</day><day>thursday</day><day>friday</day><day>saturday</day><day>sunday</day></daySet>
${STATUS_XML}       ${MCS_PATH}/status/SAV_status.xml

${LONG_DIRECTORY}   0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
${IDE_DIR}          ${COMPONENT_INSTALL_SET}/files/plugins/av/chroot/susi/update_source/vdl
${INSTALL_IDE_DIR}  ${COMPONENT_ROOT_PATH}/chroot/susi/update_source/vdl
${SCAN_DIRECTORY}   /home/vagrant/this/is/a/directory/for/scanning
${AVSCANNER}        /usr/local/bin/avscanner

${AVSCANNER_TOTAL_CONNECTION_TIMEOUT_WAIT_PERIOD}  ${350}


*** Keywords ***
Check Sophos Threat Detector Running
    ${result} =   Pidof Or Fail  ${SOPHOS_THREAT_DETECTOR_BINARY}  timeout=${1}

Run Sophos Threat Detector Directly
    ${threat_detector_handle} =  Start Process  ${SOPHOS_THREAT_DETECTOR_LAUNCHER}
    Set Suite Variable  ${THREAT_DETECTOR_PLUGIN_HANDLE}  ${threat_detector_handle}
    Register Cleanup   Terminate Process  ${THREAT_DETECTOR_PLUGIN_HANDLE}
    Wait until threat detector running

Require Sophos Threat Detector Running
    ${result} =   Pidof  ${SOPHOS_THREAT_DETECTOR_BINARY}
    Run Keyword If  ${result} == ${-1}  Run Sophos Threat Detector Directly

Dump Threads And Fail
    [Arguments]  ${ERROR_MESSAGE}
    Dump Threads  ${PLUGIN_BINARY}
    Dump All Processes
    Fail  ${ERROR_MESSAGE}

Check AV Plugin Not Running
    ${result} =  Run Process  ps  -ef   |    grep   sophos  stderr=STDOUT  shell=yes
    Log  output is ${result.stdout}
    ${result} =   Pidof  ${PLUGIN_BINARY}
    Run Keyword If  ${result} != ${-1}      Dump Threads And Fail    AV plugin still running: ${result}

Check OnAccess Not Running
    ${result} =   Pidof  ${ON_ACCESS_BIN}
    Should Be Equal As Integers  ${result}  ${-1}

Check Threat Detector Not Running
    ${result} =   Pidof  ${SOPHOS_THREAT_DETECTOR_BINARY}
    Should Be Equal As Integers  ${result}  ${-1}

Check Threat Detector PID File Does Not Exist
    # TODO - LINUXDAR-5780 investigate why PID file sometimes does not get cleaned up
    Run Keyword And Ignore Error  File Should Not Exist  ${AV_PLUGIN_PATH}/chroot/var/threat_detector.pid
    Remove File  ${AV_PLUGIN_PATH}/chroot/var/threat_detector.pid


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

File Log Should Not Contain
    [Arguments]  ${path}  ${input}
    ${content} =  Get File   ${path}  encoding_errors=replace
    Should Not Contain  ${content}  ${input}

Wait Until File Log Contains
    [Arguments]  ${logCheck}  ${input}  ${timeout}=${15}  ${interval}=${3}
    Wait Until Keyword Succeeds
    ...  ${timeout} secs
    ...  ${interval} secs
    ...  ${logCheck}  ${input}

Wait Until File Log Contains Times
    [Arguments]  ${logCheck}  ${input}  ${times}=1  ${timeout}=15  ${interval}=3
    Wait Until Keyword Succeeds
    ...  ${timeout} secs
    ...  ${interval} secs
    ...  ${logCheck}  ${input}  ${times}

File Log Does Not Contain
    [Arguments]  ${logCheck}  ${input}
    Run Keyword And Expect Error
    ...  REGEXP:Keyword '.*' failed after retrying for [0-9]* seconds\..*
    ...  Wait Until Keyword Succeeds
    ...    15 secs
    ...    1 secs
    ...    ${logCheck}  ${input}

On Access Log Contains
    [Arguments]  ${input}
    File Log Contains     ${ON_ACCESS_LOG_PATH}   ${input}

On Access Log Does Not Contain
    [Arguments]  ${input}
    Run Keyword And Expect Error
    ...  Wait    ${ON_ACCESS_LOG_PATH}  ${input}

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
    [Arguments]  ${input}  ${timeout}=${15}
    LogUtils.wait_until_latest_sophos_threat_detector_log_contains  ${input}  ${timeout}

Count Lines In Log
    [Arguments]  ${log_file}  ${line_to_count}
    ${contents} =  Get File  ${log_file}
    ${lines} =  Get Lines Containing String  ${contents}  ${line_to_count}
    ${lines_count} =  Get Line Count  ${lines}
    [Return]  ${lines_count}

Check Threat Detector Copied Files To Chroot
    Wait Until Sophos Threat Detector Log Contains  Copying "/opt/sophos-spl/base/etc/machine_id.txt" to: "/opt/sophos-spl/plugins/av/chroot/opt/sophos-spl/base/etc/machine_id.txt"
    Wait Until Sophos Threat Detector Log Contains  Copying "/opt/sophos-spl/plugins/av/VERSION.ini" to: "/opt/sophos-spl/plugins/av/chroot/opt/sophos-spl/plugins/av/VERSION.ini"
    #dns query files
    Wait Until Sophos Threat Detector Log Contains  Copying "/etc/nsswitch.conf" to: "/opt/sophos-spl/plugins/av/chroot/etc/nsswitch.conf"
    Wait Until Sophos Threat Detector Log Contains  Copying "/etc/resolv.conf" to: "/opt/sophos-spl/plugins/av/chroot/etc/resolv.conf"
    Wait Until Sophos Threat Detector Log Contains  Copying "/etc/ld.so.cache" to: "/opt/sophos-spl/plugins/av/chroot/etc/ld.so.cache"
    Wait Until Sophos Threat Detector Log Contains  Copying "/etc/host.conf" to: "/opt/sophos-spl/plugins/av/chroot/etc/host.conf"
    Wait Until Sophos Threat Detector Log Contains  Copying "/etc/hosts" to: "/opt/sophos-spl/plugins/av/chroot/etc/hosts"

    Threat Detector Does Not Log Contain  Failed to copy: /opt/sophos-spl/base/etc/machine_id.txt
    Threat Detector Does Not Log Contain  Failed to copy: /opt/sophos-spl/plugins/av/VERSION.ini

    File Should Exist  /opt/sophos-spl/plugins/av/chroot/opt/sophos-spl/base/etc/machine_id.txt
    File Should Exist  /opt/sophos-spl/plugins/av/chroot/opt/sophos-spl/plugins/av/VERSION.ini
    File Should Exist  /opt/sophos-spl/plugins/av/chroot/etc/nsswitch.conf
    File Should Exist  /opt/sophos-spl/plugins/av/chroot/etc/resolv.conf
    File Should Exist  /opt/sophos-spl/plugins/av/chroot/etc/ld.so.cache
    File Should Exist  /opt/sophos-spl/plugins/av/chroot/etc/host.conf
    File Should Exist  /opt/sophos-spl/plugins/av/chroot/etc/hosts

Increase On Access Log To Max Size
    [Arguments]  ${remaining}=1
    increase_threat_detector_log_to_max_size_by_path  ${ON_ACCESS_LOG_PATH}  ${remaining}

Increase Threat Detector Log To Max Size
    [Arguments]  ${remaining}=1
    increase_threat_detector_log_to_max_size_by_path  ${THREAT_DETECTOR_LOG_PATH}  ${remaining}

Increase AV Log To Max Size
    [Arguments]  ${remaining}=1
    increase_threat_detector_log_to_max_size_by_path  ${AV_LOG_PATH}  ${remaining}

Wait Until AV Plugin Log Contains Detection Name After Mark
    [Arguments]  ${mark}  ${name}  ${timeout}=15
    wait_for_av_log_contains_after_mark  Found '${name}'  mark=${mark}  timeout=${timeout}

Wait Until AV Plugin Log Contains Detection Name And Path After Mark
    [Arguments]  ${mark}  ${name}  ${path}  ${timeout}=15
    wait_for_av_log_contains_after_mark  Found '${name}' in '${path}'  timeout=${timeout}  mark=${mark}

AV Plugin Log Should Not Contain Detection Name And Path After Mark
    [Arguments]  ${name}  ${path}  ${mark}
    check_av_log_does_not_contain_after_mark  Found '${name}' in '${path}'  ${mark}

Check String Contains Detection Event XML
    [Arguments]  ${string}  ${user_id}  ${name}  ${threat_type}  ${origin}  ${remote}  ${sha256}  ${path}
    ${regexp} =  Catenate  SEPARATOR=\n
    ...  \\s*<\\?xml version="1\\.0" encoding="utf-8"\\?>
    ...  \\s*<event type="sophos\\.core\\.detection" ts="(?P<ts>.*)">
    ...  \\s*<user userId="${user_id}"/>
    ...  \\s*<alert id="(?P<id>.*)" name="${name}" threatType="${threat_type}" origin="${origin}" remote="${remote}">
    ...  \\s*<sha256>${sha256}</sha256>
    ...  \\s*<path>${path}</path>
    ...  \\s*</alert>
    ...  \\s*</event>
    ${matches} =  Get Regexp Matches  ${string}  ${regexp}  id
    [Return]  ${matches}

Wait Until AV Plugin Log Contains Detection Event XML After Mark
    [Arguments]  ${mark}  ${user_id}  ${name}  ${threat_type}  ${origin}  ${remote}  ${sha256}  ${path}  ${timeout}=15
    wait_for_av_log_contains_after_mark  Sending threat detection notification to central  timeout=${timeout}  mark=${mark}
    ${marked_av_log} =  get_av_log_after_mark_as_unicode  ${mark}
    ${matches} =  Check String Contains Detection Event XML  ${marked_av_log}  ${user_id}  ${name}  ${threat_type}  ${origin}  ${remote}  ${sha256}  ${path}
    Should Not Be Empty  ${matches}
    [Return]  ${matches}[0]

Base CORE Event Paths
    @{paths} =  List Files In Directory  ${MCS_PATH}/event  CORE_*.xml  absolute
    [Return]  ${paths}

Base Has Number Of CORE Events
    [Arguments]  ${expected_count}
    @{paths} =  Base CORE Event Paths
    ${actual_count} =  Get Length  ${paths}
    Should Be Equal As Integers  ${expected_count}  ${actual_count}

Base Has Detection Event
    [Arguments]  ${user_id}  ${name}  ${threat_type}  ${origin}  ${remote}  ${sha256}  ${path}
    @{files} =  Base CORE Event Paths
    FOR  ${file}  IN  @{files}
        ${xml} =  Get File  ${file}
        ${matches} =  Check String Contains Detection Event XML  ${xml}  ${user_id}  ${name}  ${threat_type}  ${origin}  ${remote}  ${sha256}  ${path}
        Return From Keyword If  len($matches) == 1  ${matches}[0]
        Log  ${xml}
    END
    Fail  No matching detection event found

Wait Until Base Has Detection Event
    [Arguments]  ${user_id}  ${name}  ${threat_type}  ${origin}  ${remote}  ${sha256}  ${path}  ${timeout}=60  ${interval}=3
    ${correlation_id} =  Wait Until Keyword Succeeds
    ...  ${timeout} secs
    ...  ${interval} secs
    ...  Base Has Detection Event  ${user_id}  ${name}  ${threat_type}  ${origin}  ${remote}  ${sha256}  ${path}
    [Return]  ${correlation_id}

Check String Contains Clean Event XML
    [Arguments]  ${input}  ${alert_id}  ${succeeded}  ${origin}  ${result}  ${path}
    Should Contain  ${input}  type="sophos.core.clean" ts="
    Should Contain  ${input}  <alert id="${alert_id}" succeeded="${succeeded}" origin="${origin}">
    Should Contain  ${input}  <items totalItems="1">
    Should Contain  ${input}  <item type="file" result="${result}">
    Should Contain  ${input}  <descriptor>${path}</descriptor>
    [Return]  ${true}

Base Has Core Clean Event
    [Arguments]   ${alert_id}  ${succeeded}  ${origin}  ${result}  ${path}
    @{files} =  Base CORE Event Paths
    FOR  ${file}  IN  @{files}
        ${xml} =  Get File  ${file}
        ${was_found} =  Run Keyword And Return Status  Check String Contains Clean Event XML  ${xml}  ${alert_id}  ${succeeded}  ${origin}  ${result}  ${path}
        Log    ${xml}
        Return From Keyword If  ${was_found}
    END
    Fail  No matching CORE Clean event found

Wait Until Base Has Core Clean Event
    [Arguments]  ${alert_id}  ${succeeded}  ${origin}  ${result}  ${path}  ${timeout}=20  ${interval}=1
    Wait Until Keyword Succeeds
    ...  ${timeout} secs
    ...  ${interval} secs
    ...  Base Has Core Clean Event  ${alert_id}  ${succeeded}  ${origin}  ${result}  ${path}

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
    Wait For Log To Not Contain From Start Of File  ${AV_LOG_PATH}  ${input}  15

Plugin Log Contains
    [Arguments]  ${input}
    LogUtils.check_log_contains  ${input}  ${AV_LOG_PATH}

Wait Until On Access Log Contains
    [Arguments]  ${input}  ${timeout}=15  ${interval}=0
    ${interval} =   Set Variable If
    ...   ${interval} > 0   ${interval}
    ...   ${timeout} >= 120   10
    ...   ${timeout} >= 60   5
    ...   ${timeout} >= 15   3
    ...   1
    Wait Until File Log Contains  On Access Log Contains   ${input}   timeout=${timeout}  interval=${interval}

FakeManagement Log Contains
    [Arguments]  ${input}
    ${log_path} =   FakeManagementLog.get_fake_management_log_path
    File Log Contains  ${log_path}   ${input}

Management Log Contains
    [Arguments]  ${input}
    File Log Contains  ${MANAGEMENT_AGENT_LOG_PATH}   ${input}

Wait Until Management Log Contains
    [Arguments]  ${input}  ${timeout}=15
    Wait Until File Log Contains  Management Log Contains   ${input}   ${timeout}

Management Log Does Not Contain
    [Arguments]  ${input}
    Wait For Log To Not Contain From Start Of File   ${MANAGEMENT_AGENT_LOG_PATH}  ${input}  15

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

Check Plugin Installed and Running After Marks
    [Arguments]  ${av_mark}  ${threat_detector_mark}
    File Should Exist   ${PLUGIN_BINARY}
    Wait until AV Plugin running after mark  ${av_mark}
    Wait until threat detector running after mark  ${threat_detector_mark}

Wait until AV Plugin running
    Wait For Pid  ${PLUGIN_BINARY}  ${10}
    LogUtils.Wait For AV Log contains after last restart  Starting Scan Scheduler  timeout=${20}

Wait until AV Plugin running after mark
    [Arguments]   ${mark}
    Wait For Pid  ${PLUGIN_BINARY}  ${30}
    wait_for_av_log_contains_after_mark  Starting Scan Scheduler  timeout=${40}  mark=${mark}

Wait until AV Plugin not running
    [Arguments]  ${timeout}=${30}
    Wait For No Pid  ${PLUGIN_BINARY}  ${timeout}

Wait Until On Access Running After Mark
    [Arguments]  ${mark}
    Wait For Pid  ${ON_ACCESS_BIN}  ${30}
    LogUtils.Wait For Log Contains After Mark    ${ON_ACCESS_LOG_PATH}    ProcessPolicy    ${mark}  timeout=60

Wait until threat detector running after mark
    [Arguments]  ${mark}  ${timeout}=${60}
    # wait for sophos_threat_detector to initialize
    Wait For Pid  ${SOPHOS_THREAT_DETECTOR_BINARY}  ${30}
    Wait_For_Log_contains_after_last_restart  ${THREAT_DETECTOR_LOG_PATH}  Common <> Starting USR1 monitor  ${timeout}  mark=${mark}
    # Only output in debug mode:
    # ...  Threat Detector Log Contains  UnixSocket <> Starting listening on socket: /var/process_control_socket

Wait until threat detector not running
    [Arguments]  ${timeout}=30
    Wait Until Keyword Succeeds
    ...  ${timeout} secs
    ...  3 secs
    ...  Check Threat Detector Not Running

Check AV Plugin Installed
    File Should Exist   ${PLUGIN_BINARY}
    Wait until AV Plugin running
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  3 secs
    ...  FakeManagement Log Contains   Registered plugin: ${COMPONENT}

Check AV Plugin Installed from Marks
    [Arguments]  ${fake_management_mark}
    File Should Exist   ${PLUGIN_BINARY}
    Wait until AV Plugin running
    wait_for_log_contains_from_mark  ${fake_management_mark}  Registered plugin: ${COMPONENT}  timeout=${15}

Install With Base SDDS
    [Arguments]  ${LogLevel}=DEBUG
    Uninstall All
    Directory Should Not Exist  ${SOPHOS_INSTALL}

    Install Base For Component Tests

    Set SPL Log Level And Restart Watchdog if changed   ${LogLevel}

    Install AV Directly from SDDS
    wait_for_av_log_contains_after_last_restart  Starting sophos_threat_detector monitor
    wait_for_sophos_threat_detector_log_contains_after_last_restart
    ...  ProcessControlServer starting listening on socket: /var/process_control_socket  timeout=120


Uninstall And Revert Setup
    Uninstall All
    Setup Base And Component

Uninstall and full reinstall
    Uninstall All
    Install With Base SDDS

Install Base For Component Tests
    [Arguments]    ${installDir}=${SOPHOS_INSTALL}
    File Should Exist     ${BASE_SDDS}/install.sh
    Run Process  chmod  +x  ${BASE_SDDS}/install.sh
    Run Process  chmod  +x  ${BASE_SDDS}/files/base/bin/*
    #large stdout cause a hang in robot
    ${result} =   Run Process   bash  ${BASE_SDDS}/install.sh  timeout=600s    stderr=STDOUT     stdout=/tmp/base-install.log

    Log File    /tmp/base-install.log
    Remove File    /tmp/base-install.log
    Should Be Equal As Integers  ${result.rc}  ${0}   "Failed to install base."

    # Check watchdog running
    Wait For Pid  ${installDir}/base/bin/sophos_watchdog  ${5}
    # Stop MCS router since we haven't configured Central
    Run Process  ${installDir}/bin/wdctl  removePluginRegistration  mcsrouter

Install AV Directly from SDDS
    ${av_mark} =  Mark AV Log

    ${install_log} =  Set Variable   ${AV_INSTALL_LOG}
    ${result} =   Run Process   bash  -x  ${AV_SDDS}/install.sh   timeout=60s  stderr=STDOUT   stdout=${install_log}
    ${log_contents} =  Get File  ${install_log}
    File Log Should Not Contain  ${AV_INSTALL_LOG}  chown: cannot access
    Should Be Equal As Integers  ${result.rc}  ${0}   "Failed to install plugin.\noutput: \n${log_contents}"

    Wait until AV Plugin running after mark  ${av_mark}
    Wait until threat detector running

Uninstall AV Without /usr/sbin in PATH
    ${result} =   Run Process   bash  -x  ${AV_PLUGIN_PATH}/sbin/uninstall.sh   timeout=60s  stderr=STDOUT   stdout=${AV_UNINSTALL_LOG}  env:PATH=/usr/local/bin:/usr/bin:/bin
    ${log_contents} =  Get File  ${AV_UNINSTALL_LOG}
    File Log Should Not Contain  ${AV_UNINSTALL_LOG}  Unable to delete
    File Log Should Not Contain  ${AV_UNINSTALL_LOG}  Failed to delete
    Should Be Equal As Integers  ${result.rc}  ${0}   "Failed to uninstall plugin.\noutput: \n${log_contents}"

Install AV Directly from SDDS Without /usr/sbin in PATH
    ${install_log} =  Set Variable   ${AV_INSTALL_LOG}
    ${result} =   Run Process   bash  -x  ${AV_SDDS}/install.sh   timeout=60s  stderr=STDOUT   stdout=${install_log}  env:PATH=/usr/local/bin:/usr/bin:/bin
    ${log_contents} =  Get File  ${install_log}
    File Log Should Not Contain  ${AV_INSTALL_LOG}  chown: cannot access
    Should Be Equal As Integers  ${result.rc}  ${0}   "Failed to install plugin.\noutput: \n${log_contents}"

    Wait until AV Plugin running
    Wait until threat detector running

Require Plugin Installed and Running
    [Arguments]  ${LogLevel}=DEBUG
    Install Base if not installed

    Set SPL Log Level And Restart Watchdog if changed   ${LogLevel}

    Install AV if not installed
    Start AV Plugin if not running
    ${pid} =  Start Sophos Threat Detector if not running
    Log  sophos_threat_detector PID = ${pid}

register on fail dump logs
    register on fail  on fail dump logs

AV And Base Teardown
    Run Keyword If Test Failed   Display All SSPL Files Installed

    register on fail dump logs

    Register On Fail    dump_dmesg
    Register On Fail    dump_log     /var/log/syslog
    Register On Fail    dump_log     /var/log/messages
    Register On Fail    analyse Journalctl    print_always=True

    Run Cleanup Functions

    Run Keyword And Ignore Error   Empty Directory   ${SCAN_DIRECTORY}

    #mark errors related to scheduled scans being forcefully terminated at the end of a test
    Exclude Failed To Scan Multiple Files Cloud
    Exclude UnixSocket Interrupted System Call Error Cloud Scan
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
    Exclude Failed To Update Because JWToken Was Empty
    Check All Product Logs Do Not Contain Error

    Run Failure Functions If Failed

    Run Keyword If Test Failed   Restart AV Plugin And Clear The Logs For Integration Tests

Restart AV Plugin And Clear The Logs For Integration Tests
    Log  Logs have to be rotated
    Run Shell Process  systemctl stop sophos-spl  OnError=failed to stop plugin

    Log  Backup logs before removing them
    Dump Log  ${AV_LOG_PATH}
    Dump Log  ${THREAT_DETECTOR_LOG_PATH}
    Dump Log  ${SUSI_DEBUG_LOG_PATH}
    Dump Log  ${ON_ACCESS_LOG_PATH}
    Dump Log  ${SCANNOW_LOG_PATH}
    Dump Log  ${CLOUDSCAN_LOG_PATH}
    Dump Log  ${SAFESTORE_LOG_PATH}
    Dump Log  ${WATCHDOG_LOG}

    Remove File    ${AV_LOG_PATH}
    Remove File    ${THREAT_DETECTOR_LOG_PATH}
    Remove File    ${SUSI_DEBUG_LOG_PATH}
    Remove File    ${THREAT_DETECTOR_INFO_LOG_PATH}
    Remove File    ${ON_ACCESS_LOG_PATH}
    Remove File    ${WATCHDOG_LOG}
    Remove File    ${CLOUDSCAN_LOG_PATH}
    Remove File    ${UPDATE_SCHEDULER}
    Remove File    ${SULDOWNLOADER_LOG}
    Remove File    ${SCANNOW_LOG_PATH}

    Empty Directory  ${SOPHOS_INSTALL}/base/mcs/event/
    Empty Directory  ${SOPHOS_INSTALL}/base/mcs/action/

    Run Shell Process  systemctl start sophos-spl  OnError=failed to start plugin
    Wait until AV Plugin running
    Wait until threat detector running

Create Install Options File With Content
    [Arguments]  ${installFlags}
    Create File  ${SOPHOS_INSTALL}/base/etc/install_options  ${installFlags}

Check ScanNow Log Exists
    File Should Exist  ${SCANNOW_LOG_PATH}

Check AV Plugin Log exists
    File Should Exist  ${AV_LOG_PATH}

Wait Until File exists
    [Arguments]  ${filename}  ${timeout}=15  ${interval}=0
    Wait Until Created  ${filename}  timeout=${timeout}

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

Produce long string
    [Arguments]   ${lenght}=249
    ${long_string}=  Run Process  tr -dc A-Za-z0-9 </dev/urandom | head -c ${lenght}  shell=True
    ${s}=  Set Variable   ${long_string.stdout}
    [Return]    ${s}

Wait Until AV Plugin Log exists
    [Arguments]  ${timeout}=15  ${interval}=0
    Wait Until File exists  ${AV_LOG_PATH}  ${timeout}  ${interval}

Wait Until Threat Detector Log exists
    [Arguments]  ${timeout}=15  ${interval}=0
    Wait Until File exists  ${THREAT_DETECTOR_LOG_PATH}  ${timeout}  ${interval}

Wait until scheduled scan updated
    Wait Until AV Plugin Log exists  timeout=30
    Wait Until AV Plugin Log Contains  Configured number of Scheduled Scans  timeout=240

Wait until scheduled scan updated After Mark
    [Arguments]  ${mark}
    wait_for_av_log_contains_after_mark  Configured number of Scheduled Scans  timeout=180  mark=${mark}

Configure Scan Exclusions Everything Else
#   exclude by default bullseye files
    [Arguments]  ${inclusion}   ${user_exclusion}=<filePath>/mnt/pandorum/</filePath>
    ${exclusions} =  exclusions_for_everything_else  ${inclusion}   ${user_exclusion}
    Log  "Excluding all directories except: ${inclusion}"
    Log  "Excluding the following directories: ${exclusions}"
    [return]  ${exclusions}

Create ext2 mount
    [Arguments]  ${source}  ${destination}
    Run Shell Process   dd if=/dev/zero of=${source} bs=102400 count=1024   OnError=Failed to create image file
    Run Shell Process   mkfs -t ext2 -F ${source}                           OnError=Failed to create ext2 fs  timeout=${30}
    Run Shell Process   mount -o loop ${source} ${destination}              OnError=Failed to mount ext2 fs

Remove ext2 mount
    [Arguments]  ${source}  ${destination}
    Unmount Image Internal    ${destination}    Failed to unmount ext2 fs
    Remove file   ${source}

Debug NFS Server
    ${result} =   Run Process  systemctl  status  nfs-server
    Log   ${result.stdout}

    Analyse Journalctl   print_always=True

    Dump Log  ${EXPORT_FILE}
    Dump Log  ${EXPORT_FILE}_bkp

Create Local NFS Share
    [Arguments]  ${source}  ${destination}  ${share_opts}=no_root_squash  ${mount_opts}=defaults
    Copy File If Destination Missing  ${EXPORT_FILE}  ${EXPORT_FILE}_bkp
    Create File   ${EXPORT_FILE}  ${source} localhost(fsid=1,rw,sync,no_subtree_check,${share_opts})\n
    Register On Fail  Debug NFS Server

    Run Shell Process   systemctl start nfs-server   OnError=Failed to start NFS server   timeout=60s
    # try exportfs. If that fails, restart nfs-server
    ${status} =      Run Keyword And Return Status
    ...     Run Shell Process   exportfs -ra            OnError=Failed to force NFS server reload
    Run Keyword If   ${status} != True
    ...     Run Shell Process   systemctl restart nfs-server   OnError=Failed to restart NFS server   timeout=60s

    Run Shell Process   mount -t nfs localhost:${source} ${destination} -o ${mount_opts}
    ...   OnError=Failed to mount local NFS share
    Register Cleanup  Remove Local NFS Share   ${source}   ${destination}

Remove Local NFS Share
    [Arguments]  ${source}  ${destination}
    Unmount Test Mount  ${destination}
    Remove Directory    ${destination}

    Move File  ${EXPORT_FILE}_bkp  ${EXPORT_FILE}
    Run Shell Process   exportfs -ra            OnError=Failed to force NFS server reload
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
         Create File   ${dir_name}/eicar-${INDEX}  ${EICAR_STRING}
     END

Debug install set
    ${result} =  run process  find  ${COMPONENT_INSTALL_SET}/files/plugins/av/chroot/susi/distribution_version  -type  f  stdout=/tmp/proc.out   stderr=STDOUT
    Log  INSTALL_SET= ${result.stdout}
    ${result} =  run process  find  ${SOPHOS_INSTALL}/plugins/av/chroot/susi/distribution_version   stdout=/tmp/proc.out    stderr=STDOUT
    Log  INSTALLATION= ${result.stdout}

Run installer from install set
    ${result} =  run process    bash  -x  ${COMPONENT_INSTALL_SET}/install.sh  stdout=/tmp/proc.out  stderr=STDOUT
    Log  ${result.stdout}
    Should Be Equal As Integers  ${result.rc}  ${0}


Run installer from install set and wait for reload trigger
    [Arguments]  ${threat_detector_pid}  ${mark}
    # ${mark} can either be line-count or LogHandler.LogMark
    Run installer from install set
    Wait Until Sophos Threat Detector Logs Or Restarts  ${threat_detector_pid}  ${mark}  Reload triggered by USR1  timeout=60

Run IDE update with expected texts
    [Arguments]  ${timeout}  @{expected_update_texts}

    ${td_mark} =  Mark Sophos Threat Detector Log
    ${threat_detector_pid} =  Record Sophos Threat Detector PID
    Run installer from install set and wait for reload trigger  ${threat_detector_pid}  ${td_mark}
    Wait For Sophos Threat Detector Log Contains One Of After Mark  expected=@{expected_update_texts}  mark=${td_mark}  timeout=${timeout}
    Check Sophos Threat Detector Log Does Not Contain After Mark    Current version matches that of the update source. Nothing to do.  ${td_mark}
    Check Sophos Threat Detector Has Same PID  ${threat_detector_pid}

Run IDE update with expected text
    [Arguments]  ${expected_update_text}  ${timeout}=120
    ${td_mark} =  Mark Sophos Threat Detector Log
    ${threat_detector_pid} =  Record Sophos Threat Detector PID
    Run installer from install set and wait for reload trigger  ${threat_detector_pid}  ${td_mark}
    Wait Until Sophos Threat Detector Logs Or Restarts  ${threat_detector_pid}  ${td_mark}  ${expected_update_text}  timeout=${timeout}
    # Wait For Sophos Threat Detector Log Contains After Mark  ${expected_update_text}  ${td_mark}  timeout=${timeout}
    Check Sophos Threat Detector Log Does Not Contain After Mark    Current version matches that of the update source. Nothing to do.  ${td_mark}
    Check Sophos Threat Detector Has Same PID  ${threat_detector_pid}

Run IDE update with SUSI loaded
    # Require that SUSI has been initialized
    Run IDE update with expected text  Notify clients that the update has completed  timeout=120

Run IDE update
    Run IDE update with expected texts  120  Threat scanner successfully updated  Threat scanner update is pending

Run IDE update without SUSI loaded
    # Require SUSI hasn't been initialized, so won't actually update
    Run IDE update with expected text  Threat scanner update is pending  timeout=10


Get Pid
    [Arguments]  ${EXEC}
    ${PID} =  Pidof Or Fail  ${EXEC}
    [Return]   ${PID}

Record AV Plugin PID
    ${PID} =  Wait For Pid  ${PLUGIN_BINARY}  ${5}
    [Return]   ${PID}

Record Sophos Threat Detector PID
    # We need to wait long enough for sophos_threat_detector to be restarted
    # avplugin triggers restart of sophos_threat_detector + 10 seconds for watchdog to restart it
    # It took 10 seconds for watchdog to detect sophos_threat_detector had exited - so allow 30 seconds here
    ${PID} =  Wait For Pid  ${SOPHOS_THREAT_DETECTOR_BINARY}  ${30}
    [Return]   ${PID}

Record Soapd Plugin PID
    ${PID} =  Wait For Pid  ${ON_ACCESS_BIN}  ${5}
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

Check Soapd Plugin has same PID
    [Arguments]  ${PID}
    ${currentPID} =  Record Soapd Plugin PID
    Should Be Equal As Integers  ${PID}  ${currentPID}

Check Sophos Threat Detector has different PID
    [Arguments]  ${PID}
    ${currentPID} =  Record Sophos Threat Detector PID
    Should Not Be Equal As Integers  ${PID}  ${currentPID}

Wait For Sophos Threat Detector to restart
    [Arguments]  ${PID}
    Wait For Different Pid  ${SOPHOS_THREAT_DETECTOR_BINARY}  ${PID}  30

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
    [Arguments]  ${revid}=${EMPTY}  ${algorithm}=Clear   ${username}=B  ${userpassword}=A  ${customer_id}=a1c0f318-e58a-ad6b-f90d-07cabda54b7d
    ${policyContent} =  populate_alc_policy    ${revid}  ${algorithm}  ${username}  ${userpassword}  ${customer_id}
    LOG    ${policyContent}
    [Return]   ${policyContent}

Get SAV Policy
    [Arguments]  ${revid}=${EMPTY}  ${sxlLookupEnabled}=A    ${oaPuaDetectionsEnabled}=true
    ${policyContent} =  Catenate   SEPARATOR=${\n}
    ...   <?xml version="1.0"?>
    ...   <config>
    ...       <csc:Comp RevID="${revid}" policyType="2"/>
    ...       <detectionFeedback>
    ...           <sendData>${sxlLookupEnabled}</sendData>
    ...       </detectionFeedback>
    ...       <onAccessScan>
    ...         <enabled>true</enabled>
    ...         <scanBehaviour>
    ...           <pua>${oaPuaDetectionsEnabled}</pua>
    ...         </scanBehaviour>
    ...       </onAccessScan>
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

Create eicar on read only mount
    [Arguments]   ${dir_path}=${SCAN_DIRECTORY}
    Create File     ${dir_path}/eicar.com    ${EICAR_STRING}
    Create Directory  ${dir_path}/readOnly
    ${result} =  run process    mount  --bind  -o  ro  ${dir_path}  /${dir_path}/readOnly
    Register Cleanup   Unmount Test Mount  ${dir_path}/readOnly
    Register Cleanup   Remove File   ${dir_path}/eicar.com

Check avscanner can detect eicar on read only mount
    [Arguments]  ${LOCAL_AVSCANNER}=${AVSCANNER}
    Create eicar on read only mount
    Check avscanner can detect eicar in  ${SCAN_DIRECTORY}/readOnly/eicar.com   ${LOCAL_AVSCANNER}

Create immutable eicar and trigger scan
    [Arguments]  ${LOCAL_AVSCANNER}=${AVSCANNER}
    File should exist  ${LOCAL_AVSCANNER}
    Create File     ${SCAN_DIRECTORY}/eicar.com    ${EICAR_STRING}
    ${result} =  run process    chattr    +i    ${SCAN_DIRECTORY}/eicar.com
    Should Be Equal As Integers  ${result.rc}  ${0}
    # Note that these clean up steps are exectued from last to first.
    Register Cleanup    Remove File   ${SCAN_DIRECTORY}/eicar.com
    Register Cleanup    run process    chattr    -i    ${SCAN_DIRECTORY}/eicar.com
    Check avscanner can detect eicar in  ${SCAN_DIRECTORY}/eicar.com   ${LOCAL_AVSCANNER}

Create eicar on network mount
    [Tags]   NFS
    ${source} =       Set Variable  /tmp_test/nfsshare
    ${destination} =  Set Variable  /testmnt/nfsshare
    Create Directory  ${source}
    Create Directory  ${destination}
    Create Local NFS Share   ${source}   ${destination}

    Create File     ${source}/eicar.com    ${EICAR_STRING}
    Register Cleanup   Remove File   ${source}/eicar.com

Check avscanner can detect eicar on network mount
    [Tags]   NFS
    [Arguments]  ${LOCAL_AVSCANNER}=${AVSCANNER}

    ${destination} =  Set Variable  /testmnt/nfsshare
    Create eicar on network mount
    Check avscanner can detect eicar in  ${destination}/eicar.com   ${LOCAL_AVSCANNER}

Check avscanner can scan clean file in
    [Arguments]  ${CLEAN_PATH}  ${LOCAL_AVSCANNER}=${AVSCANNER}
    ${rc}   ${output} =    Run And Return Rc And Output   ${LOCAL_AVSCANNER} ${CLEAN_PATH}
    Log   ${output}
    Should Be Equal As Integers  ${rc}  ${CLEAN_RESULT}
    Should Not Contain   ${output}    Detected "${CLEAN_PATH}" is infected

Check avscanner can scan clean file
    [Arguments]  ${LOCAL_AVSCANNER}=${AVSCANNER}
    Create File     ${SCAN_DIRECTORY}/cleanfile.txt    ${CLEAN_STRING}
    Register Cleanup   Remove File   ${SCAN_DIRECTORY}/cleanfile.txt
    Check avscanner can scan clean file in  ${SCAN_DIRECTORY}/cleanfile.txt   ${LOCAL_AVSCANNER}

Force SUSI to be initialized
    Check avscanner can scan clean file  ${CLI_SCANNER_PATH}

Create Big Dir
    [Arguments]   ${count}=${100}   ${path}=${SCAN_DIRECTORY}/big_dir
    Register Cleanup   Remove Directory   ${path}   recursive=true
    Create Directory   ${path}
    Copy File     ${RESOURCES_PATH}/file_samples/Firefox.exe   ${path}/Firefox.exe
    FOR   ${i}   IN RANGE   ${count}
       Evaluate   os.link("${path}/Firefox.exe", "${path}/Firefox${i}.exe")   modules=os
    END
    List Directory   ${path}
    [Return]   ${path}

Replace Virus Data With Test Dataset A
    [Arguments]  ${setOldTimestamps}=${False}  ${dataset}=20221010
#    ${SUSI_UPDATE_SRC} =  Set Variable   ${COMPONENT_ROOT_PATH}/chroot/susi/update_source/

    # Copy Test Dataset A to installset IDE directory
    Copy Files  ${IDE_DIR}/*   /tmp/vdl-tmp/
    Empty Directory  ${IDE_DIR}
    Copy Files  ${RESOURCES_PATH}/testVirusData/${dataset}/*   ${IDE_DIR}/
    Run Keyword If  ${setOldTimestamps}  Set Old Timestamps  ${IDE_DIR}

    ${result} =  Run Process  ls  -l  ${IDE_DIR}/
    Log  VDL after replacement: ${result.stdout}

Revert Virus Data To Live Dataset A
    Directory Should Exist   /tmp/vdl-tmp/    No backup virus data, cannot revert
    Empty Directory  ${IDE_DIR}
    Copy Files  /tmp/vdl-tmp/*   ${IDE_DIR}/
    Remove Directory  /tmp/vdl-tmp  recursive=True

Revert Virus Data and Run Update if Required
    ${exists} =   Run Keyword And Return Status
    ...   Directory Should Exist   /tmp/vdl-tmp/
    IF   ${exists}
        Revert Virus Data To Live Dataset A
        Run IDE update
    END

Replace Virus Data With Test Dataset A And Run IDE update without SUSI loaded
    Replace Virus Data With Test Dataset A
    Register Cleanup  Run IDE update
    Register Cleanup  Revert Virus Data To Live Dataset A
    Run IDE update without SUSI loaded

Replace Virus Data With Test Dataset A And Run IDE update with SUSI loaded
    # Require that SUSI has been initialized
    [Arguments]  ${setOldTimestamps}=${False}
    Replace Virus Data With Test Dataset A  ${setOldTimestamps}
    Register Cleanup  Run IDE update
    Register Cleanup  Revert Virus Data To Live Dataset A
    Run IDE update with SUSI loaded

Start AV
    Check AV Plugin Not Running
    Check Threat Detector Not Running
    Check Threat Detector PID File Does Not Exist
    Check SafeStore Not Running
    Check SafeStore PID File Does Not Exist

    start_sophos_threat_detector_under_fake_watchdog
    Register Cleanup   stop_sophos_threat_detector_under_fake_watchdog_if_running

    ${safestore_mark} =  LogUtils.mark_log_size  ${safestore_log_path}
    ${safestore_handle} =  Start Process  ${SAFESTORE_BIN}
    Set Suite Variable  ${SAFESTORE_HANDLE}  ${safestore_handle}
    Register Cleanup   Terminate And Wait until safestore not running  ${SAFESTORE_HANDLE}

    Remove Files   /tmp/av.stdout  /tmp/av.stderr
    ${fake_management_log_path} =   FakeManagementLog.get_fake_management_log_path
    ${fake_management_mark} =  LogUtils.mark_log_size  ${fake_management_log_path}
    ${av_mark} =  Mark AV Log
    ${handle} =  Start Process  ${AV_PLUGIN_BIN}
    Set Suite Variable  ${AV_PLUGIN_HANDLE}  ${handle}
    Register Cleanup   Terminate And Wait until AV Plugin not running  ${AV_PLUGIN_HANDLE}
    Check AV Plugin Installed from Marks  ${fake_management_mark}

    Wait for log contains from mark   ${safestore_mark}    SafeStore started

Copy And Extract Image
    [Arguments]  ${imagename}
    ${imagetarfile} =  Set Variable  ${RESOURCES_PATH}/filesystem_type_images/${imagename}.img.tgz

    ${UNPACK_DIRECTORY} =  Set Variable  ${NORMAL_DIRECTORY}/${imagename}.IMG
    Create Directory  ${UNPACK_DIRECTORY}
    Register Cleanup  Remove Directory  ${UNPACK_DIRECTORY}  recursive=True
    ${result} =  Run Process  tar  xvzf  ${imagetarfile}  -C  ${UNPACK_DIRECTORY}
    Log  ${result.stdout}
    Log  ${result.stderr}
    Should Be Equal As Integers  ${result.rc}  ${0}
    [Return]  ${UNPACK_DIRECTORY}/${imagename}.img

Unmount Image
    [Arguments]  ${where}
    Unmount Image Internal  ${where}
    deregister cleanup  Unmount Image Internal  ${where}

Unmount Image Internal
    [Arguments]  ${where}   ${errmsg}=Failed to unmount local NFS share
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  2 secs
    ...  Run Shell Process   umount ${where}   OnError=${errmsg}

    Wait Until Keyword Succeeds
    ...  30 secs
    ...  2 secs
    ...  Remove Directory    ${where}  recursive=True


Mount Image
    [Arguments]  ${where}  ${image}  ${type}  ${opts}=loop
    Create Directory  ${where}
    ${result} =  Run Process  mount  -t  ${type}  -o  ${opts}  ${image}  ${where}
    Log  ${result.stdout}
    Log  ${result.stderr}
    Should Be Equal As Integers  ${result.rc}  ${0}
    Register Cleanup  Unmount Image Internal  ${where}

Unmount Test Mount
    [Arguments]  ${where}
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  2 secs
    ...  Run Shell Process   umount ${where}   OnError=Failed to Unmount Test Mount

Bind Mount Directory
    [Arguments]  ${src}  ${dest}
    Create Directory  ${dest}
    ${result} =  Run Process  mount  --bind  ${src}  ${dest}
    Log  ${result.stdout}
    Log  ${result.stderr}
    Should Be Equal As Integers  ${result.rc}  ${0}
    Register Cleanup  Unmount Test Mount  ${dest}

Require Filesystem
    [Arguments]   ${fs_type}
    ${file_does_not_exist} =  Does File Not Exist  /proc/filesystems
    Pass Execution If    ${file_does_not_exist}  /proc/filesystems does not exist - cannot determine supported filesystems
    ${file_does_not_contain} =  Does File Not Contain  /proc/filesystems  ${fs_type}
    Pass Execution If    ${file_does_not_contain}  ${fs_type} is not a supported filesystem with this kernel - skipping test

Require NFS Version
    [Arguments]   ${version}
    Run Keyword And Ignore Error
    ...  Run Process  modprobe  nfsd
    ${status}   ${content} =   Run Keyword And Ignore Error     Get File   /proc/fs/nfsd/versions
    Pass Execution If    '${status}' == 'FAIL'
    ...     /proc/fs/nfsd/versions does not exist - cannot determine supported NFS versions
    Pass Execution If    '-${version}' in ' ${content.strip()} '
    ...     ${version} is not a supported NFS version with this NFS installation - skipping test

Terminate And Wait until threat detector not running
    [Arguments]   ${handle}
    Terminate Process  ${handle}
    Wait until threat detector not running

Terminate And Wait until SafeStore not running
    [Arguments]   ${handle}
    ${result} =  Terminate Process  ${handle}
    Wait until SafeStore not running
    Return From Keyword If  ${result.rc} == 0
    Log  Process failed: ${result.stderr}
    Log  stdout: ${result.stdout}

Terminate And Wait until AV Plugin not running
    [Arguments]   ${handle}
    Terminate Process  ${handle}
    Wait until AV Plugin not running

List AV Plugin Path
    Create Directory  ${TESTTMP}
    ${result} =  Run Process  ls  -lR  ${AV_PLUGIN_PATH}  stdout=${TESTTMP}/lsstdout  stderr=STDOUT
    Log  ls -lR: ${result.stdout}
    Remove File  ${TESTTMP}/lsstdout

Get SHA256
    [Arguments]  ${path}
    ${result} =  Run Process  sha256sum  -b  ${path}
    Log  ${result.stdout}
    Log  ${result.stderr}
    @{parts} =  Split String  ${result.stdout}
    [Return]  ${parts}[0]

Get SHA256 Of String
    [Arguments]  ${string}
    ${result} =  Run Process    echo -n '${string}' | sha256sum    shell=True
    Log  ${result.stdout}
    @{parts} =  Split String  ${result.stdout}
    [Return]  ${parts}[0]

Create Bad Unicode Eicars
   register cleanup  Remove Directory  /tmp_test/  true
   ${result} =  Run Process  bash  ${BASH_SCRIPTS_PATH}/badUnicodeEicarMaker.sh  /tmp_test/  stderr=STDOUT
   Should Be Equal As Integers  ${result.rc}  ${0}
   Log  ${result.stdout}

Create Large PE File Of Size
    [Arguments]  ${size}  ${filepath}
    Copy File  ${RESOURCES_PATH}/file_samples/CertMgr.Exe  ${filepath}
    Register Cleanup  Remove File  ${filepath}
    ${result} =   Run Process   head  -c  ${size}  </dev/urandom  >>${filepath}  shell=True
    Log  ${result.stdout}
    Log  ${result.stderr}
    Should Be Equal As Integers   ${result.rc}  ${0}

Create Archive With Dsa And Eicar
    [Arguments]    ${output}
    Create File    ${NORMAL_DIRECTORY}/1_dsa    ${DSA_BY_NAME_STRING}
    Create File    ${NORMAL_DIRECTORY}/2_eicar    ${EICAR_STRING}
    Create Archive From Files    ${output}    ${NORMAL_DIRECTORY}/1_dsa    ${NORMAL_DIRECTORY}/2_eicar

Create Archive From Files
    [Arguments]    ${output}    @{files}
    ${archive_dir} =    Set Variable    ${NORMAL_DIRECTORY}/archive_dir
    Create Directory    ${archive_dir}
    Move Files    @{files}    ${archive_dir}
    ${file_names} =    Create List
    FOR    ${path}    IN    @{files}
        ${parts} =    Split Path    ${path}
        Append To List    ${file_names}    ${parts}[1]
    END
    # Passing file names explicitly to make sure they have the intended order
    Run Process    tar    --mtime\=UTC 2022-01-01    -C    ${archive_dir}    -cf    ${output}    @{file_names}
    Remove Directory    ${archive_dir}    recursive=True

Create Archive With Eicar
    [Arguments]    ${output}
    Create File    ${NORMAL_DIRECTORY}/eicar    ${EICAR_STRING}
    Create Archive From Files    ${output}    ${NORMAL_DIRECTORY}/eicar