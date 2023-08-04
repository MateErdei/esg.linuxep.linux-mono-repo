*** Settings ***
Library    OperatingSystem

Library    ${LIB_FILES}/FullInstallerUtils.py
Library    ${LIB_FILES}/LogUtils.py
Library    ${LIB_FILES}/MCSRouter.py

*** Variables ***
${SHS_STATUS_FILE}    ${MCS_DIR}/status/SHS_status.xml

*** Keywords ***
Check Watchdog Running
    ${result} =     Run Process     pgrep  -f   sophos_watchdog
    Should Be Equal As Integers     ${result.rc}    0

Check Management Agent Running
    ${result} =     Run Process     pgrep  -f   sophos_managementagent
    Should Be Equal As Integers     ${result.rc}    0

Check Update Scheduler Running
    ${result} =     Run Process     pgrep  -f   UpdateScheduler
    Should Be Equal As Integers     ${result.rc}    0

Check Telemetry Scheduler Is Running
    ${result} =     Run Process     pgrep  -f   tscheduler
    Should Be Equal As Integers     ${result.rc}    0

Check Expected Base Processes Except SDU Are Running
    Check Watchdog Running
    Check Management Agent Running
    Check Update Scheduler Running
    Check Telemetry Scheduler Is Running
    Check MCS Router Running


SHS Status File Contains
    [Arguments]  ${content_to_contain}    ${shsStatusFile}=${SHS_STATUS_FILE}
    ${shsStatus} =  Get File   ${shsStatusFile}
    Log  ${shsStatus}
    Should Contain  ${shsStatus}  ${content_to_contain}

Check MCS Router Running
    [Arguments]    ${baseLogsDir}=${BASE_LOGS_DIR}
    ${pid} =  check_mcs_router_process_running    require_running=True
    Wait Until Created   ${baseLogsDir}/sophosspl/mcsrouter.log    timeout=5 secs
    [Return]    ${pid}

Check MCS Envelope Contains Event Success On N Event Sent
    [Arguments]  ${Event_Number}
    ${string}=  check_log_and_return_nth_occurrence_between_strings    <event><appId>ALC</appId>    </event>    ${BASE_LOGS_DIR}/sophosspl/mcs_envelope.log    ${Event_Number}
    Should contain    ${string}    &lt;number&gt;0&lt;/number&gt;

Check Comms Component Is Not Present
    verify_group_removed    sophos-spl-network
    verify_user_removed    sophos-spl-network

    File Should Not Exist    ${SOPHOS_INSTALL}/base/bin/CommsComponent
    File Should Not Exist    ${BASE_LOGS_DIR}/sophosspl/comms_component.log
    File Should Not Exist    ${SOPHOS_INSTALL}/base/pluginRegistry/commscomponent.json

    Directory Should Not Exist   ${SOPHOS_INSTALL}/var/sophos-spl-comms
    Directory Should Not Exist   ${SOPHOS_INSTALL}/var/comms
    Directory Should Not Exist   ${BASE_LOGS_DIR}/sophos-spl-comms