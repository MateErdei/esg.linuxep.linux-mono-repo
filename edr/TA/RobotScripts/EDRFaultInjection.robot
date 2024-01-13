*** Settings ***
Library     Process
Resource    ComponentSetup.robot
Resource    EDRResources.robot
Resource    ${COMMON_TEST_ROBOT}/GeneralUtilsResources.robot

Suite Setup     Install With Base SDDS
Suite Teardown  Uninstall All

Test Setup      No Operation
Test Teardown   EDR And Base Teardown

Force Tags    TAP_PARALLEL1

*** Variables ***
${OSQUERY_FLAGS_FILE}   ${SOPHOS_INSTALL}/plugins/edr/etc/osquery.flags

*** Test Cases ***
EDR Plugin stops osquery when killed
    Check EDR Plugin Installed With Base
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  1 secs
    ...  Check Osquery Running
    ${oldPid} =  Get Osquery pid
    ${result} =  Run Process  pgrep edr | xargs kill -9  shell=true
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  Check EDR Executable Not Running
    Check Osquery Running
    # wait for watchdog to bring edr back up
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  1 secs
    ...  Check EDR Executable Running
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  5 secs
    ...  File Should Contain  ${SOPHOS_INSTALL}/plugins/edr/log/edr.log  Stopping process
    Wait Until Keyword Succeeds
    ...  25 secs
    ...  1 secs
    ...  Check Osquery Has Restarted  ${oldPid}

EDR Plugin stops osquery when killed by segv
    CoreDumps.Ignore Coredumps And Segfaults
    Check EDR Plugin Installed With Base
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  Check Osquery Running
    ${oldPid} =  Get Osquery pid
    ${result} =  Run Process  pgrep edr | xargs kill -11  shell=true
    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  Check EDR Executable Not Running
    Check Osquery Running
    # wait for watchdog to bring edr back up
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  1 secs
    ...  Check EDR Executable Running
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  5 secs
    ...  File Should Contain  ${SOPHOS_INSTALL}/plugins/edr/log/edr.log  Stopping process
    Wait Until Keyword Succeeds
    ...  25 secs
    ...  1 secs
    ...  Check Osquery Has Restarted  ${oldPid}

EDR Plugin replaces flags file on startup
    Check EDR Plugin Installed With Base
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  1 secs
    ...  Check Osquery Running
    Stop EDR
    Remove File   ${OSQUERY_FLAGS_FILE}
    Create File   ${OSQUERY_FLAGS_FILE}  content=badcontent
    Start EDR
    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  Check EDR Executable Running
    Wait Until Keyword Succeeds
    ...  25 secs
    ...  5 secs
    ...  File Should Not Contain Only  ${OSQUERY_FLAGS_FILE}  badcontent

EDR Plugin Ignores Flags Unrelated to Plugin
    [Setup]    Install With Base SDDS Debug
    Send Mock Flags Policy  {"livequery.network-tables.available": true, "sdds3.enabled": false}
    Install EDR Directly from SDDS
    Check EDR Plugin Installed With Base

    Wait Until Keyword Succeeds
    ...    10 secs
    ...    1 secs
    ...    EDR Plugin Log Contains    Flags: {"livequery.network-tables.available": true, "sdds3.enabled": false}

    Wait Until Keyword Succeeds
    ...    30 secs
    ...    1 secs
    ...    Check Osquery Running
    ${startPid} =  Get Osquery Pid
    ${edrMark} =  Mark File  ${EDR_LOG_PATH}

    Send Mock Flags Policy  {"livequery.network-tables.available": true, "sdds3.enabled": true}
    Wait Until Keyword Succeeds
    ...    30 secs
    ...    1 secs
    ...    EDR Plugin Log Contains    Flags: {"livequery.network-tables.available": true, "sdds3.enabled": true}

    Check Osquery Has Not Restarted    ${startPid}    30s
    Marked File Does Not Contain    ${EDR_LOG_PATH}   Network table flag has changed. Restarting osquery to apply changes  ${edrMark}


EDR Restarts Extensions If They Exit Due To OSQuery Being Killed
    Check EDR Plugin Installed With Base
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  1 secs
    ...  Check Osquery Running
    ${old_osquery_pid} =    Get Osquery pid
    ${old_osquery_wd_pid} =    Get Osquery WD PID

    # Run a live query to prove EDR is up and running
    Copy File    ${EXAMPLE_DATA_PATH}/example_mcs.config    ${SOPHOS_INSTALL}/base/etc/sophosspl/mcs.config
    Simulate Live Query    QueryEndpointIdFromServerTable.json    1001
    Wait Until Created    ${SOPHOS_INSTALL}/base/mcs/response/LiveQuery_1001_response.json    15 secs
    File Should Contain  ${SOPHOS_INSTALL}/base/mcs/response/LiveQuery_1001_response.json    d43688f6-c547-aaaa-eeee-5332f0ad9819

    # Kill the osquery process (not the osquery WD process)
    ${edr_mark} =  Mark Log Size    ${EDR_LOG_PATH}
    ${result} =  Run Process  kill    -9    ${old_osquery_pid}
    Should Be Equal    ${result.rc}    ${0}
    Wait Until Keyword Succeeds
    ...  25 secs
    ...  1 secs
    ...  Check Osquery Has Restarted  ${old_osquery_pid}
    Wait For Log Contains From Mark    ${edr_mark}    Service extension stopped unexpectedly    ${30}
    Wait For Log Contains From Mark    ${edr_mark}    Attempting to reconnect extensions to osquery after an unexpected extension exit    ${30}
    Wait For Log Contains From Mark    ${edr_mark}    SophosExtension running    ${30}

    # Check that osquery WD has not been restarted.
    ${osquery_wd_pid} =    Get Osquery WD PID
    should be equal as integers    ${old_osquery_wd_pid}    ${osquery_wd_pid}

    # Run another live query to prove queries are working
    Simulate Live Query    QueryEndpointIdFromServerTable.json    1002
    Wait Until Created    ${SOPHOS_INSTALL}/base/mcs/response/LiveQuery_1002_response.json    15 secs
    File Should Contain  ${SOPHOS_INSTALL}/base/mcs/response/LiveQuery_1002_response.json    d43688f6-c547-aaaa-eeee-5332f0ad9819


EDR Restarts Extensions If They Exit Due To OSQuery Hitting CPU Limit
    Check EDR Plugin Installed With Base
    Create File  ${PLUGIN_CONF}  watchdog_utilization_limit=10
    Register Cleanup        Remove File  ${PLUGIN_CONF}
    Restart EDR
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  1 secs
    ...  Check Osquery Running
    ${old_osquery_pid} =    Get Osquery pid
    ${old_osquery_wd_pid} =    Get Osquery WD PID

    ${edr_mark} =  Mark Log Size    ${EDR_LOG_PATH}
    ${edr_osquery_mark} =  Mark Log Size    ${EDR_PLUGIN_PATH}/log/edr_osquery.log
    Simulate Live Query    QueryToInduceCpuLimit.json    2001

    Wait Until Keyword Succeeds
    ...  100 secs
    ...  3 secs
    ...  Check Osquery Has Restarted  ${old_osquery_pid}

    Wait For Log Contains From Mark    ${edr_osquery_mark}    Maximum sustainable CPU utilization limit     ${60}
    Wait For Log Contains From Mark    ${edr_mark}    Increment telemetry: osquery-restarts-cpu    ${60}
    Wait For Log Contains From Mark    ${edr_mark}    Service extension stopped unexpectedly    ${30}
    Wait For Log Contains From Mark    ${edr_mark}    Attempting to reconnect extensions to osquery after an unexpected extension exit    ${30}
    Wait For Log Contains From Mark    ${edr_mark}    SophosExtension running    ${30}

    # Check that osquery WD has not been restarted.
    ${osquery_wd_pid} =    Get Osquery WD PID
    should be equal as integers    ${old_osquery_wd_pid}    ${osquery_wd_pid}

    # Run another live query to prove queries are working
    Copy File    ${EXAMPLE_DATA_PATH}/example_mcs.config    ${SOPHOS_INSTALL}/base/etc/sophosspl/mcs.config
    Simulate Live Query    QueryEndpointIdFromServerTable.json    2002
    Wait Until Created    ${SOPHOS_INSTALL}/base/mcs/response/LiveQuery_2002_response.json    15 secs
    File Should Contain  ${SOPHOS_INSTALL}/base/mcs/response/LiveQuery_2002_response.json    d43688f6-c547-aaaa-eeee-5332f0ad9819


*** Keywords ***
Check Osquery Has Restarted
    [Arguments]  ${oldPid}
    ${newPid} =  Get Osquery Pid
    Should Not be Equal  ${oldPid}  ${newPid}

Check Osquery Has Not Restarted
    [Arguments]    ${originalPid}    ${time}=10s
    Sleep    ${time}
    ${endPid} =  Get Osquery Pid
    Should Be Equal As Integers  ${originalPid}  ${endPid}

Send Mock Flags Policy
    [Arguments]    ${fileContents}
    Create File    /tmp/flags.json    ${fileContents}
    Move File Atomically as sophos-spl-local    /tmp/flags.json    ${SOPHOS_INSTALL}/base/mcs/policy/flags.json