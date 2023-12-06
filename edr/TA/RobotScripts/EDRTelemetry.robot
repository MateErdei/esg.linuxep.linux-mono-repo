*** Settings ***
Documentation    Testing EDR Telemetry

Library     Collections
Library     OperatingSystem
Library     Process
Library     ${COMMON_TEST_LIBS}/LogUtils.py
Library     ${COMMON_TEST_LIBS}/LiveQueryUtils.py
Library     ../Libs/XDRLibs.py

Resource    ComponentSetup.robot
Resource    ${COMMON_TEST_ROBOT}/EDRResources.robot
Resource    ${COMMON_TEST_ROBOT}/GeneralTeardownResource.robot
Resource    ${COMMON_TEST_ROBOT}/LogControlResources.robot
Resource    ${COMMON_TEST_ROBOT}/InstallerResources.robot
Resource    ${COMMON_TEST_ROBOT}/TelemetryResources.robot
Resource    ${COMMON_TEST_ROBOT}/UpgradeResources.robot
Resource    ${COMMON_TEST_ROBOT}/WatchdogResources.robot

Test Setup  EDR Telemetry Test Setup With Debug Logging
Test Teardown  EDR Telemetry Test Teardown

Suite Setup   EDR Telemetry Suite Setup
Suite Teardown   EDR Telemetry Suite Teardown

Force Tags    TAP_PARALLEL2

*** Test Cases ***
EDR Plugin reports health correctly
    ${edr_telemetry} =  Get Plugin Telemetry  edr
    ${telemetry_json} =  Evaluate  json.loads('''${edr_telemetry}''')  json
    ${health} =  Set Variable  ${telemetry_json['health']}
    Should Be Equal As Integers  ${health}  0
    Run Process  mv  ${SOPHOS_INSTALL}/plugins/edr/bin/osqueryd  /tmp
    ${mark} =   Mark Log Size   ${EDR_LOG_PATH}
    Restart EDR Plugin
    Wait For Log Contains From Mark    ${mark}    Unable to execute osquery
    
    ${edr_telemetry} =  Get Plugin Telemetry  edr
    ${telemetry_json} =  Evaluate  json.loads('''${edr_telemetry}''')  json
    ${health} =  Set Variable  ${telemetry_json['health']}
    Should Be Equal As Integers  ${health}  1
    ${mark} =   Mark Log Size   ${EDR_LOG_PATH}
    Wait For Log Contains From Mark    ${mark}    The osquery process failed to start    ${90}

    ${edr_telemetry} =  Get Plugin Telemetry  edr
    ${telemetry_json} =  Evaluate  json.loads('''${edr_telemetry}''')  json
    ${health} =  Set Variable  ${telemetry_json['health']}
    Should Be Equal As Integers  ${health}  1
    Run Process  mv  /tmp/osqueryd  ${SOPHOS_INSTALL}/plugins/edr/bin/
    Wait For Log Contains From Mark    ${mark}    SophosExtension running    ${45}

    ${edr_telemetry} =  Get Plugin Telemetry  edr
    ${telemetry_json} =  Evaluate  json.loads('''${edr_telemetry}''')  json
    ${health} =  Set Variable  ${telemetry_json['health']}
    Should Be Equal As Integers  ${health}  0


EDR Plugin Produces Telemetry When XDR is enabled
    [Tags]  EDR_PLUGIN  MANAGEMENT_AGENT  TELEMETRY
    [Teardown]  EDR Telemetry Test Teardown With Policy Cleanup

    # make sure osquery has started so the new policy is guaranteed to cause a restart
    Wait For Log Contains After Last Restart    ${EDR_LOG_PATH}    LiveQuery policy has not been sent to the plugin
    ${mark} =   Mark Log Size   ${EDR_LOG_PATH}
    Drop LiveQuery Policy Into Place

    Wait For Log Contains From Mark    ${mark}    Updating running_mode flag settings to: 1
    Wait For Log Contains From Mark    ${mark}    Process task OSQUERY_PROCESS_FINISHED
    Wait Until EDR OSQuery Running  20
    Wait Until Osquery Socket Exists
    Prepare To Run Telemetry Executable
    Run Telemetry Executable     ${EXE_CONFIG_FILE}     ${SUCCESS}
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Check EDR Telemetry Json Is Correct  ${telemetryFileContents}  0  0  0  True  ignore_xdr=False  folded_query=True

EDR Plugin Counts OSQuery Restarts Correctly And Reports In Telemetry
    Wait Until EDR OSQuery Running  20
    Wait Until Osquery Socket Exists
    Wait For Log Contains After Last Restart    ${EDR_LOG_PATH}    SophosExtension running
    
    ${mark} =   Mark Log Size   ${EDR_LOG_PATH}
    Kill OSQuery And Wait Until Osquery Running Again
    Wait Until Osquery Socket Exists
    Wait For Log Contains From Mark    ${mark}    SophosExtension running
    
    ${mark} =   Mark Log Size   ${EDR_LOG_PATH}
    Restart EDR Plugin              #Check telemetry persists after restart
    Wait Until EDR OSQuery Running  20
    Wait Until Osquery Socket Exists
    Wait For Log Contains From Mark    ${mark}    SophosExtension running
    
    ${mark} =   Mark Log Size   ${EDR_LOG_PATH}
    Kill OSQuery And Wait Until Osquery Running Again
    Wait Until Osquery Socket Exists
    Wait For Log Contains From Mark    ${mark}    SophosExtension running
    
    Prepare To Run Telemetry Executable
    Wait For Log Contains From Mark    ${mark}    OSQUERY_PROCESS_FINISHED

    Run Telemetry Executable     ${EXE_CONFIG_FILE}      ${SUCCESS}
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Check EDR Telemetry Json Is Correct  ${telemetryFileContents}  2  0  0

EDR Plugin Counts OSQuery Restarts Correctly when XDR is enabled And Reports In Telemetry
    [Tags]  EDR_PLUGIN  MANAGEMENT_AGENT  TELEMETRY
    [Setup]  EDR Telemetry Test Setup With Debug Logging
    [Teardown]  EDR Telemetry Test Teardown With Policy Cleanup
    Copy File  ${SUPPORT_FILES}/xdr-query-packs/error-queries.conf  ${SOPHOS_INSTALL}/plugins/edr/etc/osquery.conf.d/sophos-scheduled-query-pack.conf

    # make sure osquery has started so the new policy is guaranteed to cause a restart
    Wait For Log Contains After Last Restart    ${EDR_LOG_PATH}    LiveQuery policy has not been sent to the plugin
    Wait For Log Contains After Last Restart    ${EDR_LOG_PATH}    SophosExtension running
    
    ${mark} =   Mark Log Size   ${EDR_LOG_PATH}
    Drop LiveQuery Policy Into Place

    Wait For Log Contains From Mark    ${mark}    Updating running_mode flag settings to: 1
    Wait For Log Contains From Mark    ${mark}    Restarting osquery, reason: LiveQuery policy has changed. Restarting osquery to apply changes
    Wait For Log Contains From Mark    ${mark}    OSQUERY_PROCESS_FINISHED
    Wait Until EDR OSQuery Running  20
    Wait Until Osquery Socket Exists

    Wait For Log Contains From Mark    ${mark}    SophosExtension running
    # sleep to give osquery a chance to stabilise so this test doesn't flake
    # TODO - LINUXDAR-2839 Use new logline to replace this sleep with a smarter wait
    Sleep  10s

    ${mark} =   Mark Log Size   ${EDR_LOG_PATH}
    Kill OSQuery And Wait Until Osquery Running Again
    Wait Until Osquery Socket Exists
    Wait For Log Contains From Mark    ${mark}    SophosExtension running
    
    ${mark} =   Mark Log Size   ${EDR_LOG_PATH}
    Restart EDR Plugin              #Check telemetry persists after restart
    Wait Until EDR OSQuery Running  20
    Wait Until Osquery Socket Exists
    Wait For Log Contains From Mark    ${mark}    SophosExtension running
    
    ${mark} =   Mark Log Size   ${EDR_LOG_PATH}
    Kill OSQuery And Wait Until Osquery Running Again
    Wait For Log Contains From Mark    ${mark}    SophosExtension running

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
    ${edr_mark} =   Mark Log Size   ${EDR_LOG_PATH}
    ${lq_mark} =    Mark Log Size   ${LIVEQUERY_LOG_FILE}
    Run Live Query  ${CRASH_QUERY}  Crash

    Wait For Log Contains From Mark    ${lq_mark}    Extension exited while running    100
    Wait For Log Contains From Mark    ${edr_mark}   OSQUERY_PROCESS_FINISHED
    Prepare To Run Telemetry Executable
    Run Telemetry Executable     ${EXE_CONFIG_FILE}     ${SUCCESS}
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}

    Check EDR Telemetry Json Is Correct  ${telemetryFileContents}  1  1  0  failed_count=1

EDR Reports Telemetry And Stats Correctly After Plugin Restart For Live Query
    ${lq_mark} =   Mark Log Size   ${LIVEQUERY_LOG_FILE}
    Run Live Query  ${SIMPLE_QUERY_1_ROW}   simple
    Wait For Log Contains From Mark    ${lq_mark}    Successfully executed query with name: simple

    Restart EDR Plugin              #Check telemetry persists after restart

    Run Live Query  ${SIMPLE_QUERY_4_ROW}   simple
    Wait For Log Contains From Mark    ${lq_mark}    Successfully executed query with name: simple

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
    
    ${edr_mark} =   Mark Log Size   ${EDR_LOG_PATH}
    Kill OSQuery And Wait Until Osquery Running Again
    Wait For Log Contains From Mark    ${edr_mark}   OSQUERY_PROCESS_FINISHED
    Wait Until Osquery Socket Exists
    
    ${edr_mark} =   Mark Log Size   ${EDR_LOG_PATH}
    Kill OSQuery And Wait Until Osquery Running Again
    Wait For Log Contains From Mark    ${edr_mark}   OSQUERY_PROCESS_FINISHED

    ${edr_mark} =   Mark Log Size   ${EDR_LOG_PATH}
    ${lq_mark} =   Mark Log Size   ${LIVEQUERY_LOG_FILE}
    Run Live Query  ${CRASH_QUERY}  Crash
    Wait For Log Contains From Mark    ${lq_mark}   Extension exited while running    100
    Wait For Log Contains From Mark    ${edr_mark}   OSQUERY_PROCESS_FINISHED
    
    Prepare To Run Telemetry Executable
    Run Telemetry Executable     ${EXE_CONFIG_FILE}     ${SUCCESS}
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}

    #TODO LINUXDAR-3974
    ${times} =    Get Number Of Occurrences Of Substring In Log    ${EDR_LOG_PATH}    OSQUERY_PROCESS_FINISHED
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
    Run Keyword And Ignore Error   LogUtils.Dump Log  ${TELEMETRY_OUTPUT_JSON}
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

Wait Until Osquery Socket Exists
    Wait Until Keyword Succeeds
    ...  10s
    ...  1s
    ...  Should Exist  ${SOPHOS_INSTALL}/plugins/edr/var/osquery.sock