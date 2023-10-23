*** Settings ***
Documentation    Suite description

Library     ${COMMON_TEST_LIBS}/LogUtils.py
Library     ${LIBS_DIRECTORY}/LiveQueryUtils.py

Resource  ${COMMON_TEST_ROBOT}/EDRResources.robot
Resource  ${COMMON_TEST_ROBOT}/GeneralTeardownResource.robot
Resource  ${COMMON_TEST_ROBOT}/LogControlResources.robot
Resource  ${COMMON_TEST_ROBOT}/InstallerResources.robot
Resource  ${COMMON_TEST_ROBOT}/TelemetryResources.robot
Resource  ${COMMON_TEST_ROBOT}/UpgradeResources.robot
Resource  ${COMMON_TEST_ROBOT}/WatchdogResources.robot

Test Setup  EDR Telemetry Test Setup With Debug Logging
Test Teardown  EDR Telemetry Test Teardown

Suite Setup   EDR Telemetry Suite Setup
Suite Teardown   EDR Telemetry Suite Teardown

Default Tags   EDR_PLUGIN  TELEMETRY
Force Tags  LOAD1


*** Test Cases ***
EDR Plugin Produces Telemetry When XDR is enabled
    [Tags]  EDR_PLUGIN  MANAGEMENT_AGENT  TELEMETRY
    [Teardown]  EDR Telemetry Test Teardown With Policy Cleanup

    # make sure osquery has started so the new policy is guaranteed to cause a restart
    Wait Until Keyword Succeeds
    ...  20s
    ...  2s
    ...  Check EDR Log Contains   LiveQuery policy has not been sent to the plugin
    Drop LiveQuery Policy Into Place

    Wait Until Keyword Succeeds
    ...   20 secs
    ...   5 secs
    ...   Check EDR Log Contains  Updating running_mode flag settings to: 1
    Wait Until Keyword Succeeds
    ...   30 secs
    ...   5 secs
    ...   Check EDR Log Contains  Process task OSQUERY_PROCESS_FINISHED
    Wait Until EDR OSQuery Running  20
    Wait Until Osquery Socket Exists
    Prepare To Run Telemetry Executable
    Run Telemetry Executable     ${EXE_CONFIG_FILE}     ${SUCCESS}
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Check EDR Telemetry Json Is Correct  ${telemetryFileContents}  0  0  0  True  ignore_xdr=False  folded_query=True

EDR Plugin Counts OSQuery Restarts Correctly And Reports In Telemetry
    Wait Until EDR OSQuery Running  20
    Wait Until Osquery Socket Exists
    Wait Until Keyword Succeeds
    ...  10s
    ...  2s
    ...  Check Log Contains String N Times  ${SOPHOS_INSTALL}/plugins/edr/log/edr.log  edr.log  SophosExtension running  1
    Kill OSQuery And Wait Until Osquery Running Again
    Wait Until Osquery Socket Exists
    Wait Until Keyword Succeeds
    ...  10s
    ...  2s
    ...  Check Log Contains String N Times  ${SOPHOS_INSTALL}/plugins/edr/log/edr.log  edr.log  SophosExtension running  2
    Restart EDR Plugin              #Check telemetry persists after restart

    Wait Until EDR OSQuery Running  20
    Wait Until Osquery Socket Exists
    Wait Until Keyword Succeeds
    ...  10s
    ...  2s
    ...  Check Log Contains String N Times  ${SOPHOS_INSTALL}/plugins/edr/log/edr.log  edr.log  SophosExtension running  3
    Kill OSQuery And Wait Until Osquery Running Again

    Prepare To Run Telemetry Executable

    #TODO LINUXDAR-3974
    ${times} =  Get Number Of Osquery Restarts
    Should Be True  ${times} > 1
    Should Be True  ${times} < 5

    Run Telemetry Executable     ${EXE_CONFIG_FILE}      ${SUCCESS}
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Check EDR Telemetry Json Is Correct  ${telemetryFileContents}  2  0  0

EDR Plugin Counts OSQuery Restarts Correctly when XDR is enabled And Reports In Telemetry
    [Tags]  EDR_PLUGIN  MANAGEMENT_AGENT  TELEMETRY
    [Setup]  EDR Telemetry Test Setup With Debug Logging
    [Teardown]  EDR Telemetry Test Teardown With Policy Cleanup
    Copy File  ${SUPPORT_FILES}/xdr-query-packs/error-queries.conf  ${SOPHOS_INSTALL}/plugins/edr/etc/osquery.conf.d/sophos-scheduled-query-pack.conf

    # make sure osquery has started so the new policy is guaranteed to cause a restart
    Wait Until Keyword Succeeds
    ...  20s
    ...  2s
    ...  Check EDR Log Contains   LiveQuery policy has not been sent to the plugin
    Wait Until Keyword Succeeds
    ...  16s
    ...  2s
    ...  Check Log Contains String N Times  ${SOPHOS_INSTALL}/plugins/edr/log/edr.log  edr.log  SophosExtension running  1
    Drop LiveQuery Policy Into Place

    Wait Until Keyword Succeeds
    ...   20 secs
    ...   5 secs
    ...   Check Log Contains In Order
            ...  ${SOPHOS_INSTALL}/plugins/edr/log/edr.log
            ...  Updating running_mode flag settings to: 1
    Wait Until Keyword Succeeds
    ...  20s
    ...  2s
    ...  Check EDR Log Contains  Restarting osquery, reason: LiveQuery policy has changed. Restarting osquery to apply changes
    Wait Until Keyword Succeeds
    ...  20s
    ...  2s
    ...  Check Log Contains String N Times  ${SOPHOS_INSTALL}/plugins/edr/log/edr.log  edr.log   OSQUERY_PROCESS_FINISHED  1
    Wait Until EDR OSQuery Running  20
    Wait Until Osquery Socket Exists

    Wait Until Keyword Succeeds
    ...  10s
    ...  2s
    ...  Check Log Contains String N Times  ${SOPHOS_INSTALL}/plugins/edr/log/edr.log  edr.log  SophosExtension running  2
    # sleep to give osquery a chance to stabilise so this test doesn't flake
    # TODO - LINUXDAR-2839 Use new logline to replace this sleep with a smarter wait
    Sleep  10s

    Kill OSQuery And Wait Until Osquery Running Again
    Wait Until Osquery Socket Exists
    ${times} =  Wait Until Keyword Succeeds
    ...  10s
    ...  2s
    ...  Check Log Contains String At Least N Times  ${SOPHOS_INSTALL}/plugins/edr/log/edr.log  edr.log  SophosExtension running  3
    Restart EDR Plugin              #Check telemetry persists after restart

    Wait Until EDR OSQuery Running  20
    Wait Until Osquery Socket Exists
    ${times} =  Wait Until Keyword Succeeds
    ...  10s
    ...  2s
    ...  Check Log Contains String At Least N Times  ${SOPHOS_INSTALL}/plugins/edr/log/edr.log  edr.log  SophosExtension running  ${times+1}
    Kill OSQuery And Wait Until Osquery Running Again
    ${times} =   Wait Until Keyword Succeeds
    ...  30s
    ...  2s
    ...  Check Log Contains String At Least N Times  ${SOPHOS_INSTALL}/plugins/edr/log/edr.log  edr.log  SophosExtension running  ${times+1}

    Prepare To Run Telemetry Executable

    #TODO LINUXDAR-3974
    ${numOsqueryRestarts} =  Get Number Of Osquery Restarts
    Should Be True  ${numOsqueryRestarts} > 2
    Should Be True  ${numOsqueryRestarts} < 6
    # The first doesn't count in telemetry because it was a controlled restart due to policy change
    ${numOsqueryRestartsInTelemetry} =  Set Variable  ${numOsqueryRestarts-1}

    Run Telemetry Executable     ${EXE_CONFIG_FILE}      ${SUCCESS}
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Check EDR Telemetry Json Is Correct  ${telemetryFileContents}  ${numOsqueryRestartsInTelemetry}  0  0  True  ignore_xdr=False  folded_query=True


EDR Plugin Reports Telemetry Correctly For OSQuery CPU Restarts
    Run Live Query  ${CRASH_QUERY}  Crash

    Wait Until Keyword Succeeds
    ...  100 secs
    ...  2 secs
    ...  Check Livequery Log Contains    Extension exited while running
    Wait Until Keyword Succeeds
    ...  15
    ...  1
    ...  Check EDR Log Contains  OSQUERY_PROCESS_FINISHED
    Prepare To Run Telemetry Executable
    Run Telemetry Executable     ${EXE_CONFIG_FILE}     ${SUCCESS}
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}

    Check EDR Telemetry Json Is Correct  ${telemetryFileContents}  1  1  0  failed_count=1

EDR Reports Telemetry And Stats Correctly After Plugin Restart For Live Query
    Run Live Query  ${SIMPLE_QUERY_1_ROW}   simple
    Wait Until Keyword Succeeds
    ...  100 secs
    ...  2 secs
    ...  Check Log Contains String N times   ${SOPHOS_INSTALL}/plugins/edr/log/livequery.log   edr_log  Successfully executed query with name: simple  1

    Restart EDR Plugin              #Check telemetry persists after restart

    Run Live Query  ${SIMPLE_QUERY_4_ROW}   simple
    Wait Until Keyword Succeeds
    ...  100 secs
    ...  2 secs
    ...  Check Log Contains String N times   ${SOPHOS_INSTALL}/plugins/edr/log/livequery.log   edr_log  Successfully executed query with name: simple  2

    Prepare To Run Telemetry Executable
    Run Telemetry Executable     ${EXE_CONFIG_FILE}     ${SUCCESS}
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}


    Check EDR Telemetry Json Is Correct  ${telemetryFileContents}  0  0  0  successful_count=2

EDR Reports Telemetry Correctly When Events Max Limit Is Hit For A Table

    ${script} =     Catenate    SEPARATOR=\n
    ...    \Expiring events for subscriber: user_events (overflowed limit 100000)
    ...   Expiring events for subscriber: socket_events (overflowed limit 100000)
    ...   Expiring events for subscriber: selinux_events (overflowed limit 100000)
    ...   Expiring events for subscriber: process_events (overflowed limit 100000)
    ...   Expiring events for subscriber: process_events (overflowed limit 100000)
    ...    \
    Create File    ${SOPHOS_INSTALL}/plugins/edr/log/osqueryd.INFO.20201117-051713.1056   content=${script}



    Prepare To Run Telemetry Executable
    Run Telemetry Executable     ${EXE_CONFIG_FILE}     ${SUCCESS}
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}

    Check EDR Telemetry Json Is Correct  ${telemetryFileContents}  0  0  0  ignore_process_events=False  ignore_selinux_events=False  ignore_socket_events=False  ignore_user_events=False


EDR Plugin Reports Telemetry Correctly For OSQuery CPU Restarts And Restarts by EDR Plugin
    Wait Until EDR OSQuery Running  20
    # osquery will take longer to restart if it is killed before the socket is created
    Wait Until Osquery Socket Exists
    Kill OSQuery And Wait Until Osquery Running Again
    ${times} =  Wait Until Keyword Succeeds
    ...  20s
    ...  2s
    ...  Check Log Contains String At Least N Times  ${SOPHOS_INSTALL}/plugins/edr/log/edr.log  edr.log  OSQUERY_PROCESS_FINISHED  1
    Wait Until Osquery Socket Exists
    Kill OSQuery And Wait Until Osquery Running Again
    ${times} =  Wait Until Keyword Succeeds
    ...  20s
    ...  2s
    ...  Check Log Contains String At Least N Times  ${SOPHOS_INSTALL}/plugins/edr/log/edr.log  edr.log  OSQUERY_PROCESS_FINISHED  ${times+1}

    Run Live Query  ${CRASH_QUERY}  Crash
    Wait Until Keyword Succeeds
    ...  100 secs
    ...  2 secs
    ...  Check Livequery Log Contains    Extension exited while running

    ${times} =  Wait Until Keyword Succeeds
    ...  60s
    ...  2s
    ...  Check Log Contains String At Least N Times  ${SOPHOS_INSTALL}/plugins/edr/log/edr.log  edr.log  OSQUERY_PROCESS_FINISHED  ${times+1}
    Prepare To Run Telemetry Executable
    Run Telemetry Executable     ${EXE_CONFIG_FILE}     ${SUCCESS}
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}


    #TODO LINUXDAR-3974
    Should Be True  ${times} < 7  More restarts than is reasonable were found
    Check EDR Telemetry Json Is Correct  ${telemetryFileContents}  ${times}  1  0  failed_count=1


EDR Plugin Produces Telemetry With OSQuery Max Events Override Value
    Prepare To Run Telemetry Executable

    Remove File  ${SOPHOS_INSTALL}/plugins/edr/etc/plugin.conf
    Create File  ${SOPHOS_INSTALL}/plugins/edr/etc/plugin.conf  events_max=345678

    Restart EDR Plugin
    Wait Until EDR OSQuery Running  20
    Wait Until Osquery Socket Exists

    ${LogContents} =  Get File  ${EDR_DIR}/etc/osquery.flags
    Should Contain  ${LogContents}  --events_max=345678

    Run Telemetry Executable     ${EXE_CONFIG_FILE}     ${SUCCESS}
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Check EDR Telemetry Json Is Correct  ${telemetryFileContents}  0  0  0  events_max=345678

*** Keywords ***

Get Number Of Osquery Restarts
    ${times} =  Check Log Contains String At Least N Times  ${SOPHOS_INSTALL}/plugins/edr/log/edr.log  edr.log  OSQUERY_PROCESS_FINISHED  1
    [Return]  ${times}

EDR Telemetry Suite Setup
    Require Fresh Install
    Override LogConf File as Global Level  DEBUG
    Copy Telemetry Config File in To Place


EDR Telemetry Suite Teardown
    Uninstall SSPL

EDR Telemetry Test Setup
    Require Installed
    Install EDR Directly
    Create Directory   ${COMPONENT_TEMP_DIR}
    Wait Until EDR OSQuery Running  20

EDR Telemetry Test Setup With Debug Logging
    Require Installed
    Create File  ${SOPHOS_INSTALL}/base/etc/logger.conf  [global]\nVERBOSITY = DEBUG\n
    Install EDR Directly
    Create Directory   ${COMPONENT_TEMP_DIR}
    Wait Until EDR OSQuery Running  20

EDR Telemetry Test Teardown
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Log  ${telemetryFileContents}
    General Test Teardown
    Remove file  ${TELEMETRY_OUTPUT_JSON}
    Run Keyword If Test Failed  LogUtils.Dump Log  ${HTTPS_LOG_FILE_PATH}
    Cleanup Telemetry Server
    Remove Directory   ${COMPONENT_TEMP_DIR}  recursive=true
    Remove File  ${EXE_CONFIG_FILE}
    Uninstall EDR Plugin
    Run Keyword And Ignore Error   Remove File   ${SOPHOS_INSTALL}/base/mcs/policy/LiveQuery-1_policy.xml
    Run Keyword And Ignore Error   Remove File   ${SOPHOS_INSTALL}/base/telemetry/cache/edr-telemetry.json

EDR Telemetry Test Teardown With Policy Cleanup
    EDR Telemetry Test Teardown
    Cleanup MCSRouter Directories



Trigger EDR Osquery Database Purge
    Should Exist  /opt/sophos-spl/plugins/edr/var/osquery.db
    ${result} =  Run Process  dd if\=/dev/urandom bs\=1024 count\=200 | split -a 4 -b 1k - /opt/sophos-spl/plugins/edr/var/osquery.db/  shell=true
    Log  ${result.stdout}
    Log  ${result.stderr}
    Should Be Equal As Strings  ${result.rc}  0

Wait For EDR Osquery To Purge Database
    Wait Until Keyword Succeeds
    ...  120s
    ...  10s
    ...  Check EDR Log Contains   Purging Database

Wait Until Osquery Socket Exists
    Wait Until Keyword Succeeds
    ...  10s
    ...  1s
    ...  Should Exist  ${SOPHOS_INSTALL}/plugins/edr/var/osquery.sock