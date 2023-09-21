*** Settings ***
Documentation    Testing the Logger Plugin for XDR Behaviour

Library         Process
Library         OperatingSystem
Library         Collections
Library         ../Libs/LogUtils.py
Library         ../Libs/XDRLibs.py
Library         ../Libs/InstallerUtils.py
Library         ../Libs/FakeManagement.py

Resource        EDRResources.robot
Resource        ComponentSetup.robot

Suite Setup     Install With Base SDDS Debug
Suite Teardown  Uninstall ALL

Test Setup      Test Setup
Test Teardown   Test Teardown

Default Tags    TAP_TESTS

*** Test Cases ***
EDR Plugin Detects Data Limit From Policy And That A Status Is Sent On Start
    [Setup]  No Operation
    Move File Atomically  ${EXAMPLE_DATA_PATH}/LiveQuery_policy_100000_limit.xml  /opt/sophos-spl/base/mcs/policy/LiveQuery_policy.xml
    # the mocked queries produce a lower amount of data so the 10000 limit will be hit but not the 250000000
    Install EDR Directly from SDDS With mocked scheduled queries

    Wait Until Keyword Succeeds
    ...  5 secs
    ...  1 secs
    ...  EDR Plugin Log Contains  First LiveQuery policy received
    Expect New Datalimit  100000

    Move File Atomically  ${EXAMPLE_DATA_PATH}/LiveQuery_policy_250000000_limit.xml  /opt/sophos-spl/base/mcs/policy/LiveQuery_policy.xml
    Expect New Datalimit  250000000
    Wait For LiveQuery Status To Contain  <dailyDataLimitExceeded>false</dailyDataLimitExceeded>

    Move File Atomically  ${EXAMPLE_DATA_PATH}/LiveQuery_policy_1000GB_limit.xml  /opt/sophos-spl/base/mcs/policy/LiveQuery_policy.xml
    Expect New Datalimit  100000000000

EDR Plugin outputs XDR results and Its Answer is available to MCSRouter
    Add Uptime Query to Scheduled Queries
    Directory Should Be Empty  ${SOPHOS_INSTALL}/base/mcs/datafeed
    Enable XDR
    Wait Until Keyword Succeeds
    ...  100 secs
    ...  5 secs
    ...  Directory Should Not Be Empty  ${SOPHOS_INSTALL}/base/mcs/datafeed

EDR Plugin Restarts Osquery When Custom Queries Have Changed

    Enable XDR
    Directory Should Be Empty  ${SOPHOS_INSTALL}/base/mcs/datafeed
    Move File Atomically  ${EXAMPLE_DATA_PATH}/LiveQuery_policy_customquery_limit.xml  /opt/sophos-spl/base/mcs/policy/LiveQuery_policy.xml
    Wait Until Keyword Succeeds
    ...  5 secs
    ...  1 secs
    ...  EDR Plugin Log Contains  Processing LiveQuery Policy
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  1 secs
    ...  File Should Exist  ${SOPHOS_INSTALL}/plugins/edr/etc/osquery.conf.d/sophos-scheduled-query-pack.custom.conf
    Log File  ${SOPHOS_INSTALL}/plugins/edr/etc/osquery.conf.d/sophos-scheduled-query-pack.custom.conf

    Wait Until Keyword Succeeds
    ...  15 secs
    ...  2 secs
    ...  Check All Queries Run  ${SCHEDULEDQUERY_LOG_FILE}  ${SOPHOS_INSTALL}/plugins/edr/etc/osquery.conf.d/sophos-scheduled-query-pack.custom.conf

EDR Plugin Tags All Queries Correctly
    [Timeout]  10 minutes
    [Setup]  No Operation
    Set Discovery Query Interval In EDR SDDS Directory Config
    # Put policy in place before EDR starts to stop EDR starting osquery and then getting a new policy and immediately restarting it.
    Move File Atomically  ${EXAMPLE_DATA_PATH}/LiveQuery_policy_customquery_limit.xml  /opt/sophos-spl/base/mcs/policy/LiveQuery_policy.xml
    Install EDR Directly from SDDS With mocked scheduled queries
    Check EDR Plugin Installed With Base
    Wait Until Keyword Succeeds
    ...   20 secs
    ...   2 secs
    ...   Check Osquery Running
    Directory Should Be Empty  ${SOPHOS_INSTALL}/base/mcs/datafeed

    Wait Until Keyword Succeeds
    ...  300 secs
    ...  5 secs
    ...  Check All Query Results Contain Correct Tag  ${SOPHOS_INSTALL}/base/mcs/datafeed/  ${SOPHOS_INSTALL}/plugins/edr/etc/osquery.conf.d/sophos-scheduled-query-pack.conf    ${SOPHOS_INSTALL}/plugins/edr/etc/osquery.conf.d/sophos-scheduled-query-pack.mtr.conf  ${SOPHOS_INSTALL}/plugins/edr/etc/osquery.conf.d/sophos-scheduled-query-pack.custom.conf
    ${files} =  List Files In Directory  ${SOPHOS_INSTALL}/base/mcs/datafeed/
    ${content} =  Get File     ${SOPHOS_INSTALL}/base/mcs/datafeed/${files}[0]
    Should Contain  ${content}   "licence":"MTR


EDR Plugin Applies Folding Rules When Folding Rules Have Changed
    [Setup]  Install EDR Directly from SDDS With Fixed Value Queries
    Check EDR Plugin Installed With Base
    Enable XDR
    Directory Should Be Empty  ${SOPHOS_INSTALL}/base/mcs/datafeed

    # Inject policy with folding rules
    Apply Live Query Policy And Expect Folding Rules To Have Changed  ${EXAMPLE_DATA_PATH}/LiveQuery_policy_foldingrules_limit.xml

    # Throw away one set of results here so that we are certain they are not from before the folding rules were applied
    ${query_results} =  Clear Datafeed Dir And Wait For Next Result File

    # Wait for a result we know will contain folded and non-folded results
    ${query_results} =  Clear Datafeed Dir And Wait For Next Result File
    Check Query Results Are Folded  ${query_results}  uptime  fixed_column  fixed_value
    Check Query Results Are Not Folded  ${query_results}  uptime_not_folded  fixed_column  fixed_value

    # Wait for a 2nd batch of result to prove that folding is done per batch, i.e. the folded query shows up again
    ${query_results} =  Clear Datafeed Dir And Wait For Next Result File
    Check Query Results Are Folded  ${query_results}  uptime  fixed_column  fixed_value
    Check Query Results Are Not Folded  ${query_results}  uptime_not_folded  fixed_column  fixed_value

    # Inject policy without folding rules
    Apply Live Query Policy And Expect Folding Rules To Have Changed  ${EXAMPLE_DATA_PATH}/LiveQuery_policy_100000_limit.xml

    # Throw away one set of results here so that we are certain they are not from before the folding rules were removed
    ${query_results} =  Clear Datafeed Dir And Wait For Next Result File

    # Wait until the results appear and check they are not folded now there are no folding rules
    ${query_results} =  Clear Datafeed Dir And Wait For Next Result File
    Check Query Results Are Not Folded  ${query_results}  uptime  fixed_column  fixed_value

    # Telemetry
    ${edr_telemetry} =  Get Plugin Telemetry  edr
    ${telemetry_json} =  Evaluate  json.loads('''${edr_telemetry}''')  json
    ${foldable_queries} =  Set Variable  ${telemetry_json['foldable-queries']}
    List Should Contain Value  ${foldable_queries}  uptime

EDR Plugin Applies Folding Rules Based Column Value
    [Setup]  Install EDR Directly from SDDS With Random Queries
    Check EDR Plugin Installed With Base

    Enable XDR
    Directory Should Be Empty  ${SOPHOS_INSTALL}/base/mcs/datafeed

    # Inject policy with folding rules
    Apply Live Query Policy And Expect Folding Rules To Have Changed  ${EXAMPLE_DATA_PATH}/LiveQuery_policy_foldingrules_limit.xml

    # Throw away one set of results here so that we are certain they are not from before the folding rules were applied
    ${query_results} =  Clear Datafeed Dir And Wait For Next Result File

    # Wait for a result we know will contain folded and non-folded results
    ${query_results} =  Clear Datafeed Dir And Wait For Next Result File
    Check Query Results Are Folded  ${query_results}  random  number  0
    Check Query Results Are Not Folded  ${query_results}  random  number  1

    # Telemetry
    ${edr_telemetry} =  Get Plugin Telemetry  edr
    ${telemetry_json} =  Evaluate  json.loads('''${edr_telemetry}''')  json
    ${foldable_queries} =  Set Variable  ${telemetry_json['foldable-queries']}
    List Should Contain Value  ${foldable_queries}  random


EDR Plugin Applies Regex Folding Rules
    [Setup]  Install EDR Directly from SDDS With Random Queries
    Check EDR Plugin Installed With Base

    Enable XDR
    Directory Should Be Empty  ${SOPHOS_INSTALL}/base/mcs/datafeed

    # Inject policy with folding rules
    Apply Live Query Policy And Expect Folding Rules To Have Changed  ${EXAMPLE_DATA_PATH}/LiveQuery_policy_regex_foldingrules_100kB_limit.xml

    # Throw away one set of results here so that we are certain they are not from before the folding rules were applied
    ${query_results} =  Clear Datafeed Dir And Wait For Next Result File

    # Wait for a result we know will contain folded and non-folded results
    ${query_results} =  Clear Datafeed Dir And Wait For Next Result File
    Check Query Results Are Folded  ${query_results}  random  number

    # Telemetry
    ${edr_telemetry} =  Get Plugin Telemetry  edr
    ${telemetry_json} =  Evaluate  json.loads('''${edr_telemetry}''')  json
    ${foldable_queries} =  Set Variable  ${telemetry_json['foldable-queries']}
    List Should Contain Value  ${foldable_queries}  random


EDR Plugin Runs All Canned Queries
    [Setup]  No Operation
    #  Originally sophos-query-pack.json
    #  A recent fetch gave endpoint_query_pack.json only locally
    ${src_query_pack} =  Set Variable  ${TEST_INPUT_PATH}/lp/endpoint_query_pack.json
    File Should Exist  ${src_query_pack}
    Set Discovery Query Interval In EDR SDDS Directory Config
    Test Setup
    Directory Should Be Empty  ${SOPHOS_INSTALL}/base/mcs/datafeed
    Remove File   ${SOPHOS_INSTALL}/plugins/edr/etc/osquery.conf.d/sophos-scheduled-query-pack.conf
    Remove File   ${SOPHOS_INSTALL}/plugins/edr/etc/osquery.conf.d/sophos-scheduled-query-pack.mtr.conf
    convert_canned_query_json_to_query_pack   ${src_query_pack}  ${SOPHOS_INSTALL}/plugins/edr/etc/osquery.conf.d/sophos-scheduled-query-pack.conf
    Run Process  chmod  600  ${SOPHOS_INSTALL}/plugins/edr/etc/osquery.conf.d/sophos-scheduled-query-pack.conf
    # Inserting policy will cause an osquery restart to apply new policy settings
    Move File Atomically  ${EXAMPLE_DATA_PATH}/LiveQuery_policy_customquery_limit.xml  /opt/sophos-spl/base/mcs/policy/LiveQuery_policy.xml

    Wait Until Keyword Succeeds
    ...  120 secs
    ...  10 secs
    ...  Check All Queries Run  ${SCHEDULEDQUERY_LOG_FILE}  ${SOPHOS_INSTALL}/plugins/edr/etc/osquery.conf.d/sophos-scheduled-query-pack.conf
    ${content} =  Get File  ${EDR_LOG_FILE}
    Should Not Contain  ${content}  query-error-count


EDR Plugin Runs All Scheduled Queries
    [Setup]  No Operation
    Directory Should Be Empty  ${SOPHOS_INSTALL}/base/mcs/datafeed
    Move File Atomically  ${EXAMPLE_DATA_PATH}/LiveQuery_policy_customquery_limit.xml  /opt/sophos-spl/base/mcs/policy/LiveQuery_policy.xml
    Set Discovery Query Interval In EDR SDDS Directory Config
    Test Setup

    Run Process  mkdir  -p  ${SOPHOS_INSTALL}/plugins/eventjournaler/data/eventjournals/SophosSPL/Detections
    Run Process  cp  -r  ${EXAMPLE_DATA_PATH}/TestEventJournalFiles/Detections-0000000000000001-0000000000001e00-132766178770000000-132766182670000000.xz  ${SOPHOS_INSTALL}/plugins/eventjournaler/data/eventjournals/SophosSPL/Detections
    Run Process  chown  -R  sophos-spl-user:sophos-spl-group  ${SOPHOS_INSTALL}/plugins/eventjournaler/
    ${edrMark} =    Mark Log Size    ${EDR_LOG_PATH}

    #restart edr so that the altered queries are read in and debug mode applied
    Restart EDR

    Wait Until Keyword Succeeds
    ...  120 secs
    ...  10 secs
    ...  Check All Queries Run  ${SCHEDULEDQUERY_LOG_FILE}  ${SOPHOS_INSTALL}/plugins/edr/etc/osquery.conf.d/sophos-scheduled-query-pack.conf

    Wait Until Keyword Succeeds
    ...  10 secs
    ...  2 secs
    ...  Check All Queries Run  ${SCHEDULEDQUERY_LOG_FILE}  ${SOPHOS_INSTALL}/plugins/edr/etc/osquery.conf.d/sophos-scheduled-query-pack.mtr.conf

    Wait Until Keyword Succeeds
    ...  10 secs
    ...  2 secs
    ...  Check All Queries Run  ${SCHEDULEDQUERY_LOG_FILE}  ${SOPHOS_INSTALL}/plugins/edr/etc/osquery.conf.d/sophos-scheduled-query-pack.custom.conf
    Wait Until Keyword Succeeds
    ...  100 secs
    ...  5 secs
    ...  Directory Should Not Be Empty  ${SOPHOS_INSTALL}/base/mcs/datafeed
    check_log_does_not_contain_after_mark  ${EDR_LOG_PATH}  query-error-count  ${edrMark}


EDR Plugin Logs Broken JSON In Scheduled Query Pack
    ${query_pack_conf} =  Set Variable  ${SOPHOS_INSTALL}/plugins/edr/etc/query_packs/sophos-scheduled-query-pack.conf
    ${osquery_conf} =  Set Variable  ${SOPHOS_INSTALL}/plugins/edr/etc/osquery.conf.d/sophos-scheduled-query-pack.conf
    Run Keyword And Ignore Error  Remove File  ${query_pack_conf}
    Should Not Exist  ${query_pack_conf}
    Create File  ${osquery_conf}  {"schedule": {"extracomma": {"query": ,"interval": 1},}}

    Stop EDR
    Start EDR

    Enable XDR
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  1 secs
    ...  EDR Plugin Log Contains  Failed to parse ${osquery_conf}

    Stop EDR

    Remove File    ${osquery_conf}
    Start EDR


EDR Plugin writes custom query file when it recieves a Live Query policy and removes it when there are no custom queries
    [Setup]  No Operation
    Move File Atomically  ${EXAMPLE_DATA_PATH}/LiveQuery_policy_customquery_limit.xml  /opt/sophos-spl/base/mcs/policy/LiveQuery_policy.xml
    Install EDR Directly from SDDS
    Wait Until Keyword Succeeds
    ...  5 secs
    ...  1 secs
    ...  EDR Plugin Log Contains  First LiveQuery policy received
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  1 secs
    ...  File Should Exist  ${SOPHOS_INSTALL}/plugins/edr/etc/osquery.conf.d/sophos-scheduled-query-pack.custom.conf

    Move File Atomically  ${EXAMPLE_DATA_PATH}/LiveQuery_policy_10000_limit.xml  /opt/sophos-spl/base/mcs/policy/LiveQuery_policy.xml

    Wait Until Keyword Succeeds
        ...  15 secs
        ...  1 secs
        ...  File Should Not Exist  ${SOPHOS_INSTALL}/plugins/edr/etc/osquery.conf.d/sophos-scheduled-query-pack.custom.conf

EDR Plugin Sends LiveQuery Status On Period Rollover
    [Setup]  No Operation
    Move File Atomically  ${EXAMPLE_DATA_PATH}/LiveQuery_policy_10000_limit.xml  /opt/sophos-spl/base/mcs/policy/LiveQuery_policy.xml
    Install EDR Directly from SDDS

    # wait until EDR hits daily limit
    Wait For LiveQuery Status To Contain  <dailyDataLimitExceeded>true</dailyDataLimitExceeded>

    # Force EDR limit to roll over by setting the target rollover time to be very far into the past and restarting EDR
    Stop EDR

    Create File  ${SOPHOS_INSTALL}/plugins/edr/var/persist-xdrPeriodTimestamp  10
    Start EDR

    # Wait for the rollover to occur
    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  EDR Plugin Log Contains  XDR period has rolled over

    # Once rolled over we expect that the status has gone back to exceeded=false
    # This won't stay as false for long as the policy limit is only 10kB
    Wait For LiveQuery Status To Contain  <dailyDataLimitExceeded>false</dailyDataLimitExceeded>

EDR Plugin Respects Data Limit
    [Setup]  No Operation
    Move File Atomically  ${EXAMPLE_DATA_PATH}/LiveQuery_policy_10000_limit.xml  /opt/sophos-spl/base/mcs/policy/LiveQuery_policy.xml
    ${mark} =    Mark Log Size    ${EDR_LOG_PATH}
    Install EDR Directly from SDDS
    Wait For Log Contains From Mark    ${mark}    Plugin preparation complete
    Wait For Log Contains From Mark    ${mark}    First LiveQuery policy received
    Expect New Datalimit  10000

    Wait For Log Contains From Mark    ${mark}    XDR data limit for this period exceeded
    Wait For Log Contains From Mark    ${mark}    Restarting osquery, reason: XDR data limit exceeded
    Wait For LiveQuery Status To Contain  <dailyDataLimitExceeded>true</dailyDataLimitExceeded>
    ${edr_telemetry} =  Get Plugin Telemetry  edr
    ${telemetry_json} =  Evaluate  json.loads('''${edr_telemetry}''')  json
    ${uploadLimit} =  Set Variable  ${telemetry_json['scheduled-queries']['upload-limit-hit']}
    Should Be Equal  ${uploadLimit}  ${True}

EDR Plugin Rolls ScheduleEpoch Over When The Previous One Has Elapsed
    Enable XDR
    ${oldScheduleEpochTimestamp} =  Set Variable  1600000000
    Stop EDR
    Create File  ${SOPHOS_INSTALL}/plugins/edr/var/persist-xdrScheduleEpoch  ${oldScheduleEpochTimestamp}
    Start EDR

    Wait Until Keyword Succeeds
    ...  60 secs
    ...  1 secs
    ...  EDR Plugin Log Contains   Using osquery schedule_epoch flag as: --schedule_epoch=${oldScheduleEpochTimestamp}

    Wait Until Keyword Succeeds
    ...  15 secs
    ...  1 secs
    ...  EDR Plugin Log Contains   Previous schedule_epoch: ${oldScheduleEpochTimestamp}, has ended.

    ${scheduleEpoch} =  ScheduleEpoch Should Be Recent
    Should Not Be Equal As Strings  ${scheduleEpoch}  ${oldScheduleEpochTimestamp}
    EDR Plugin Log Contains   Starting new schedule_epoch: ${scheduleEpoch}
    Wait Until Keyword Succeeds
    ...  60 secs
    ...  1 secs
    ...  EDR Plugin Log Contains   Using osquery schedule_epoch flag as: --schedule_epoch=${scheduleEpoch}
    Osquery Flag File Should Contain  --schedule_epoch=${scheduleEpoch}

EDR Plugin Does Not Roll ScheduleEpoch Over When The Previous One Has Not Elapsed
    ${currentEpochTime} =  get_current_epoch_time
    ${currentEpochTimeMinus3Days} =  Evaluate  ${currentEpochTime} - (60*60*24*3)

    Enable XDR

    Stop EDR
    Create File  ${SOPHOS_INSTALL}/plugins/edr/var/persist-xdrScheduleEpoch  ${currentEpochTimeMinus3Days.__str__()}
    Start EDR

    Wait Until Keyword Succeeds
    ...  60 secs
    ...  1 secs
    ...  EDR Plugin Log Contains   Using osquery schedule_epoch flag as: --schedule_epoch=${currentEpochTimeMinus3Days}
    Wait Until Keyword Succeeds
    ...  5 secs
    ...  1 secs
    ...  EDR Plugin Log Does Not Contain   Starting new schedule_epoch
    Osquery Flag File Should Contain  --schedule_epoch=${currentEpochTimeMinus3Days}

EDR Plugin Recovers When ScheduleEpoch Is In The Future
    [Documentation]  To test scenario reported in this: LINUXDAR-2973
    Enable XDR
    ${oldScheduleEpochTimestamp} =  Set Variable  3472328296227742266
    Stop EDR
    Create File  ${SOPHOS_INSTALL}/plugins/edr/var/persist-xdrScheduleEpoch  ${oldScheduleEpochTimestamp}

    ${mark} =  Mark Log Size  ${EDR_LOG_PATH}
    Start EDR

    Wait For Log Contains From Mark    ${mark}
    ...  Using osquery schedule_epoch flag as: --schedule_epoch=${oldScheduleEpochTimestamp}
    ...  timeout=${60}

    Wait For Log Contains From Mark    ${mark}
        ...  Schedule Epoch time: ${oldScheduleEpochTimestamp} is in the future, resetting to current time
        ...  timeout=${15}

    Wait For Log Contains From Mark    ${mark}
        ...  Previous schedule_epoch: ${oldScheduleEpochTimestamp}, has ended. Starting new schedule_epoch:
        ...  timeout=${15}

    ${scheduleEpoch} =  ScheduleEpoch Should Be Recent
    Should Not Be Equal As Strings  ${scheduleEpoch}  ${oldScheduleEpochTimestamp}
    Check Log Contains After Mark    ${EDR_LOG_PATH}    Starting new schedule_epoch: ${scheduleEpoch}    ${mark}

    Wait For Log Contains From Mark    ${mark}
    ...  Using osquery schedule_epoch flag as: --schedule_epoch=${scheduleEpoch}
    ...  timeout=${60}

    Osquery Flag File Should Contain  --schedule_epoch=${scheduleEpoch}

    Sleep  ${5}  Sleep to allow EDR to settle before stopping it - try to work out if it crashes anyway

Check XDR Results Contain Correct ScheduleEpoch Timestamp
    ${currentEpochTime} =  get_current_epoch_time
    ${currentEpochTimeMinus3Days} =  Evaluate  ${currentEpochTime} - (60*60*24*3)

    Enable XDR

    Stop EDR
    # Previous run of EDR may have generated an item in the folder
    Clear Datafeed Folder
    Create File  ${SOPHOS_INSTALL}/plugins/edr/var/persist-xdrScheduleEpoch  ${currentEpochTimeMinus3Days.__str__()}
    Start EDR

    Wait Until Keyword Succeeds
    ...  15 secs
    ...  1 secs
    ...  Osquery Flag File Should Contain  --schedule_epoch=${currentEpochTimeMinus3Days}

    ${scheduledQueryFilename} =  wait_for_scheduled_query_file_and_return_filename
    ${scheduledQueryContents} =  Get File  ${SOPHOS_INSTALL}/base/mcs/datafeed/${scheduledQueryFilename}
    Should Contain  ${scheduledQueryContents}  "epoch":${currentEpochTimeMinus3Days}

EDR Plugin Runs Next Scheduled Queries When Flags Configured To Do So
    [Setup]  Install EDR Directly from SDDS With Latest And Next Marked
    Check EDR Plugin Installed With Base

    Directory Should Be Empty  ${SOPHOS_INSTALL}/base/mcs/datafeed

    Enable XDR with MTR
    ${sqMark} =    Mark Log Size    ${SCHEDULEDQUERY_LOG_FILE}

    Wait For Log Contains From Mark    ${sqMark}    latest_xdr_query
    Wait For Log Contains From Mark    ${sqMark}    latest_mtr_query

    sleep  4s
    check_log_does_not_contain_after_mark  ${SCHEDULEDQUERY_LOG_FILE}    next_xdr_query    ${sqMark}
    check_log_does_not_contain_after_mark  ${SCHEDULEDQUERY_LOG_FILE}    next_mtr_query    ${sqMark}

    ${edrMark} =    Mark Log Size    ${EDR_LOG_PATH}
    Change Next Query Packs Flag  true
    Wait For Log Contains From Mark    ${edrMark}    Prepare system for running osquery

    ${sqMark} =    Mark Log Size    ${SCHEDULEDQUERY_LOG_FILE}
    Are Next Query Packs Enabled in Plugin Conf  1

    wait_for_log_contains_from_mark    ${sqMark}    next_xdr_query
    wait_for_log_contains_from_mark    ${sqMark}    next_mtr_query
    sleep  4s
    check_log_does_not_contain_after_mark  ${SCHEDULEDQUERY_LOG_FILE}   latest_xdr_query  ${sqMark}
    check_log_does_not_contain_after_mark  ${SCHEDULEDQUERY_LOG_FILE}   latest_mtr_query  ${sqMark}

    ${edrMark} =    Mark Log Size    ${EDR_LOG_PATH}
    Change Next Query Packs Flag  false
    Wait For Log Contains From Mark    ${edrMark}    Prepare system for running osquery
    ${sqMark} =  Mark Log Size    ${SCHEDULEDQUERY_LOG_FILE}
    Are Next Query Packs Enabled in Plugin Conf  0

    wait_for_log_contains_from_mark    ${sqMark}    latest_xdr_query
    wait_for_log_contains_from_mark    ${sqMark}    latest_mtr_query

    sleep  4s
    check_log_does_not_contain_after_mark  ${SCHEDULEDQUERY_LOG_FILE}   next_xdr_query  ${sqMark}
    check_log_does_not_contain_after_mark  ${SCHEDULEDQUERY_LOG_FILE}   next_mtr_query  ${sqMark}

EDR Plugin Hits Data Limit And Queries Resume After Period
    [Setup]  Data Limit Test Setup  ${EXAMPLE_DATA_PATH}/LiveQuery_policy_50kB_limit_with_MTR.xml  180
    [Teardown]  Data Limit Test Teardown

    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  EDR Plugin Log Contains  First LiveQuery policy received
    Expect New Datalimit  50000

    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  XDR Pack Should Be Enabled
    MTR Pack Should Be Enabled
    Custom Pack Should Be Enabled

    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  EDR Plugin Log Contains X Times  Run osquery process  1

    # 2 because custom queries file is written, not enabled
    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  EDR Plugin Log Contains X Times   Enabled query pack conf file  2

    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  File Should Not Be Empty  ${SOPHOS_INSTALL}/plugins/edr/var/xdr_intermediary

    # Wait for data limit to be hit, EDR starts query pack disabling and osquery restart behaviour
    Wait Until Keyword Succeeds
    ...  100 secs
    ...  5 secs
    ...  EDR Plugin Log Contains   XDR data limit for this period exceeded

    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  EDR Plugin Log Contains  Datafeed limit has been hit. Disabling scheduled queries

    # all 3 packs disabled when we hit the limit
    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  EDR Plugin Log Contains X Times   Disabled query pack conf file  3

    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  XDR Pack Should Be Disabled
    MTR Pack Should Be Disabled
    Custom Pack Should Be Disabled

    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  EDR Plugin Log Contains  Sending LiveQuery Status

    Wait For LiveQuery Status To Contain  <dailyDataLimitExceeded>true</dailyDataLimitExceeded>

    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  EDR Plugin Log Contains X Times  Restarting osquery, reason: XDR data limit exceeded  1

    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  EDR Plugin Log Contains X Times  Stopping LoggerExtension  1

    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  EDR Plugin Log Contains X Times  LoggerExtension::Stopped  1

    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  EDR Plugin Log Contains X Times  Stopping SophosExtension  1

    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  EDR Plugin Log Contains X Times  SophosExtension::Stopped  1

    # Osquery restart from query pack disabling due to data limit
    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  EDR Plugin Log Contains X Times  Run osquery process  2

    # Prove that scheduled queries are not being executed
    ${empty_marker} =  Set Variable    empty-marker
    Create File  ${SCHEDULEDQUERY_LOG_FILE}  ${empty_marker}
    # Wait for the possibility of queries to be running, in this time many queriers would have been executed.
    Sleep  20
    File Should Contain Only  ${SCHEDULEDQUERY_LOG_FILE}  ${empty_marker}

    # Wait until the data limit period rolls over
    Wait Until Keyword Succeeds
    ...  200 secs
    ...  5 secs
    ...  EDR Plugin Log Contains  XDR period has rolled over

    # Osquery has started up for the final time.
    Wait Until Keyword Succeeds
    ...  60 secs
    ...  1 secs
    ...  EDR Plugin Log Contains X Times  Run osquery process  3

    # up from 2 to 5 because all 3 were enable after the data period rolled over
    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  EDR Plugin Log Contains X Times   Enabled query pack conf file  5

    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  XDR Pack Should Be Enabled
    MTR Pack Should Be Enabled
    Custom Pack Should Be Enabled

    # Prove that scheduled queries are running again
    Create File  ${SCHEDULEDQUERY_LOG_FILE}  ${empty_marker}
    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  File Should Not Contain Only  ${SCHEDULEDQUERY_LOG_FILE}  ${empty_marker}

    File Should Contain  ${SCHEDULEDQUERY_LOG_FILE}  Executing query

OSQuery Does Not Restart After Period Elapses If Data Limit Not Hit
    [Setup]  Data Limit Test Setup  ${EXAMPLE_DATA_PATH}/LiveQuery_policy_100000_limit.xml  20
    [Teardown]  Data Limit Test Teardown

    Wait Until Keyword Succeeds
    ...  5 secs
    ...  1 secs
    ...  EDR Plugin Log Contains  First LiveQuery policy received
    Expect New Datalimit  100000

    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  XDR Pack Should Be Enabled

    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  EDR Plugin Log Contains X Times  Run osquery process  1

    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  File Should Not Be Empty  ${SOPHOS_INSTALL}/plugins/edr/var/xdr_intermediary

    EDR Plugin Log Contains X Times  Sending LiveQuery Status  1

    # Wait until the data limit period rolls over
    Wait Until Keyword Succeeds
    ...  35 secs
    ...  5 secs
    ...  EDR Plugin Log Contains  XDR period has rolled over

    Wait Until Keyword Succeeds
    ...  40 secs
    ...  2 secs
    ...  EDR Plugin Log Contains X Times  Sending LiveQuery Status  2

    EDR Plugin Log Does Not Contain  Restarting osquery to apply changes after re-enabling query packs following a data limit rollover

    # Check OSQuery has not been restarted
    EDR Plugin Log Contains X Times  Run osquery process  1

Test Query Packs Are Enabled And Disabled By Policy
    Create Debug Level Logger Config File
    Restart EDR

    XDR Pack Should Be Disabled
    MTR Pack Should Be Disabled
    Custom Pack Should Not Exist

    Apply Live Query Policy And Wait For Query Pack Changes  ${EXAMPLE_DATA_PATH}/LiveQuery_policy_enabled.xml

    XDR Pack Should Be Enabled
    MTR Pack Should Be Disabled
    Custom Pack Should Not Exist

    Apply Live Query Policy And Wait For Query Pack Changes  ${EXAMPLE_DATA_PATH}/LiveQuery_policy_enabled_with_MTR.xml

    XDR Pack Should Be Enabled
    MTR Pack Should Be Enabled
    Custom Pack Should Not Exist

    Apply Live Query Policy And Wait For Query Pack Changes  ${EXAMPLE_DATA_PATH}/LiveQuery_policy_enabled_with_MTR_and_custom.xml

    XDR Pack Should Be Enabled
    MTR Pack Should Be Enabled
    Custom Pack Should Be Enabled

    Apply Live Query Policy And Wait For Query Pack Changes  ${EXAMPLE_DATA_PATH}/LiveQuery_policy_enabled_with_custom_only.xml

    XDR Pack Should Be Disabled
    MTR Pack Should Be Disabled
    Custom Pack Should Be Enabled

    Apply Live Query Policy And Wait For Query Pack Changes  ${EXAMPLE_DATA_PATH}/LiveQuery_policy_enabled_with_MTR_only.xml

    XDR Pack Should Be Disabled
    MTR Pack Should Be Enabled
    Custom Pack Should Not Exist

    Apply Live Query Policy And Wait For Query Pack Changes  ${EXAMPLE_DATA_PATH}/LiveQuery_policy_enabled.xml

    XDR Pack Should Be Enabled
    MTR Pack Should Be Disabled
    Custom Pack Should Not Exist

EDR Plugin Respects Data Limit When Applying New Live Query Policy With Different Packs Enabled And New Custom Queries
    Move File Atomically  ${EXAMPLE_DATA_PATH}/LiveQuery_policy_10000_limit_and_different_custom_queries_with_xdr_only.xml  /opt/sophos-spl/base/mcs/policy/LiveQuery_policy.xml
    Install EDR Directly from SDDS
    Wait Until Keyword Succeeds
    ...  5 secs
    ...  1 secs
    ...  EDR Plugin Log Contains  First LiveQuery policy received
    Expect New Datalimit  10000

    Wait Until Keyword Succeeds
    ...  20 secs
    ...  10 secs
    ...  EDR Plugin Log Contains   XDR data limit for this period exceeded

    Wait Until Keyword Succeeds
    ...  5s
    ...  1s
    ...  XDR Pack Should Be Disabled
    MTR Pack Should Be Disabled
    Custom Pack Should Be Disabled

    ${edrMark} =    Mark Log Size    ${EDR_LOG_PATH}
    Apply Live Query Policy And Wait For Query Pack Changes  ${EXAMPLE_DATA_PATH}/LiveQuery_policy_10000_limit_and_different_custom_queries_with_xdr_only.xml

    XDR Pack Should Be Disabled
    MTR Pack Should Be Disabled
    Custom Pack Should Be Disabled
    Wait For Log Contains From Mark    ${edrMark}    Sophos Extension running in thread
    Apply Live Query Policy And Wait For Query Pack Changes  ${EXAMPLE_DATA_PATH}/LiveQuery_policy_enabled.xml

    XDR Pack Should Be Disabled
    MTR Pack Should Be Disabled
    Custom Pack Should Not Exist

    Apply Live Query Policy And Wait For Query Pack Changes  ${EXAMPLE_DATA_PATH}/LiveQuery_policy_10000_limit_and_different_custom_queries_with_xdr_only.xml

    XDR Pack Should Be Disabled
    MTR Pack Should Be Disabled
    Custom Pack Should Be Disabled

EDR Plugin Updates Next Scheduled Queries When Supplement Updated And Flag Already Set
    Check EDR Plugin Installed With Base
    Run Keyword And Ignore Error  Remove File  ${SOPHOS_INSTALL}/base/etc/logger.conf
    Create File  ${SOPHOS_INSTALL}/base/etc/logger.conf  [global]\nVERBOSITY = DEBUG\n

    # TODO: When LINUXDAR-3943 is implemented remove denylist option from the query configs, and change test accordingly if required.
    Create File  ${SOPHOS_INSTALL}/plugins/edr/etc/query_packs/sophos-scheduled-query-pack.conf  {"schedule": {"latest_xdr_query": {"query": "select * from uptime;","interval": 2, "denylist": false}}}
    Create File  ${SOPHOS_INSTALL}/plugins/edr/etc/query_packs/sophos-scheduled-query-pack.mtr.conf  {"schedule": {"latest_mtr_query": {"query": "select * from uptime;","interval": 2, "denylist": false}}}
    Create File  ${SOPHOS_INSTALL}/plugins/edr/etc/query_packs/sophos-scheduled-query-pack-next.conf  {"schedule": {"next_xdr_query": {"query": "select * from uptime;","interval": 2, "denylist": false}}}
    Create File  ${SOPHOS_INSTALL}/plugins/edr/etc/query_packs/sophos-scheduled-query-pack-next.mtr.conf  {"schedule": {"next_mtr_query": {"query": "select * from uptime;","interval": 2, "denylist": false}}}
    Directory Should Be Empty  ${SOPHOS_INSTALL}/base/mcs/datafeed
    Restart EDR
    Enable XDR with MTR
    Change Next Query Packs Flag  true

    Wait Until Keyword Succeeds
    ...  30 secs
    ...  5 secs
    ...  EDR Plugin Log Contains  Overwriting existing scheduled query packs with 'NEXT' query packs
    ${sqMark} =    Mark Log Size    ${SCHEDULEDQUERY_LOG_FILE}
    Are Next Query Packs Enabled in Plugin Conf  1

    wait_for_log_contains_from_mark    ${sqMark}    next_xdr_query
    wait_for_log_contains_from_mark    ${sqMark}    next_mtr_query
    sleep  4s
    check_log_does_not_contain_after_mark  ${SCHEDULEDQUERY_LOG_FILE}   latest_xdr_query  ${sqMark}
    check_log_does_not_contain_after_mark  ${SCHEDULEDQUERY_LOG_FILE}   latest_mtr_query  ${sqMark}

    ${edrMark} =    Mark Log Size    ${EDR_LOG_FILE}

    # Mimic update of the next query pack supplement
    # TODO: When LINUXDAR-3943 is implemented remove denylist option from the query configs, and change test accordingly if required.
    Create File  ${SOPHOS_INSTALL}/plugins/edr/etc/query_packs/sophos-scheduled-query-pack-next.conf  {"schedule": {"next_updated_xdr_query": {"query": "select * from uptime;","interval": 2, "denylist": false}}}
    Create File  ${SOPHOS_INSTALL}/plugins/edr/etc/query_packs/sophos-scheduled-query-pack-next.mtr.conf  {"schedule": {"next_updated_mtr_query": {"query": "select * from uptime;","interval": 2, "denylist": false}}}
    Restart EDR

    wait_for_log_contains_from_mark    ${edrMark}    Overwriting existing scheduled query packs with 'NEXT' query packs

    ${sqMark} =    Mark Log Size    ${SCHEDULEDQUERY_LOG_FILE}
    Are Next Query Packs Enabled in Plugin Conf  1

    wait_for_log_contains_from_mark    ${sqMark}    next_updated_xdr_query
    wait_for_log_contains_from_mark    ${sqMark}    next_updated_mtr_query
    sleep  4s
    check_log_does_not_contain_after_mark  ${SCHEDULEDQUERY_LOG_FILE}   next_xdr_query  ${sqMark}
    check_log_does_not_contain_after_mark  ${SCHEDULEDQUERY_LOG_FILE}   next_mtr_query  ${sqMark}
    check_log_does_not_contain_after_mark  ${SCHEDULEDQUERY_LOG_FILE}   latest_xdr_query  ${sqMark}
    check_log_does_not_contain_after_mark  ${SCHEDULEDQUERY_LOG_FILE}   latest_mtr_query  ${sqMark}

EDR Plugin Sends XDR Results After Batch Time
    Add Uptime Query to Scheduled Queries  60
    Directory Should Be Empty  ${SOPHOS_INSTALL}/base/mcs/datafeed
    Enable XDR
    Wait Until Keyword Succeeds
    ...  100 secs
    ...  5 secs
    ...  Directory Should Not Be Empty  ${SOPHOS_INSTALL}/base/mcs/datafeed

    Wait Until Keyword Succeeds
    ...  100 secs
    ...  5 secs
    ...  File Should Not Exist  ${SOPHOS_INSTALL}/plugins/edr/var/xdr_intermediary
    Empty Directory  ${SOPHOS_INSTALL}/base/mcs/datafeed

    Wait Until Keyword Succeeds
    ...  120 secs
    ...  5 secs
    ...  File Should Not Be Empty  ${SOPHOS_INSTALL}/plugins/edr/var/xdr_intermediary

    Wait Until Keyword Succeeds
    ...  20 secs
    ...  2 secs
    ...  Directory Should Not Be Empty  ${SOPHOS_INSTALL}/base/mcs/datafeed

Ensure Default Osquery Flags Are Contained in flags file
    [Setup]  No Operation
    Install Base For Component Tests
    Create File  ${SOPHOS_INSTALL}/base/etc/logger.conf  [global]\nVERBOSITY = DEBUG\n
    Install EDR Directly from SDDS
    Wait Until Keyword Succeeds    5 secs    1 secs    EDR Plugin Log Contains  LiveQuery policy has not been sent to the plugin
    Wait Until Created  ${OSQUERY_FLAG_FILE}
    Osquery Flag File Should Contain    --host_identifier=uuid
    Osquery Flag File Should Contain    --log_result_events=true
    Osquery Flag File Should Contain    --utc
    Osquery Flag File Should Contain    --logger_stderr=false
    Osquery Flag File Should Contain    --logger_mode=420
    Osquery Flag File Should Contain    --logger_min_stderr=1
    Osquery Flag File Should Contain    --logger_min_status=1
    Osquery Flag File Should Contain    --disable_watchdog=false
    Osquery Flag File Should Contain    --watchdog_level=0
    Osquery Flag File Should Contain    --watchdog_memory_limit=250
    Osquery Flag File Should Contain    --watchdog_utilization_limit=30
    Osquery Flag File Should Contain    --watchdog_delay=60
    Osquery Flag File Should Contain    --enable_extensions_watchdog=true
    Osquery Flag File Should Contain    --disable_extensions=false
    Osquery Flag File Should Contain    --audit_persist=true
    Osquery Flag File Should Contain    --enable_syslog=true
    Osquery Flag File Should Contain    --audit_allow_config=true
    Osquery Flag File Should Contain    --audit_allow_process_events=true
    Osquery Flag File Should Contain    --audit_allow_fim_events=false
    Osquery Flag File Should Contain    --audit_allow_selinux_events=true
    Osquery Flag File Should Contain    --audit_allow_apparmor_events=true
    Osquery Flag File Should Contain    --audit_allow_sockets=false
    Osquery Flag File Should Contain    --audit_allow_user_events=true
    Osquery Flag File Should Contain    --syslog_events_expiry=604800
    Osquery Flag File Should Contain    --events_expiry=604800
    Osquery Flag File Should Contain    --force=true
    Osquery Flag File Should Contain    --disable_enrollment=true
    Osquery Flag File Should Contain    --enable_killswitch=false
    Osquery Flag File Should Contain    --verbose
    Osquery Flag File Should Contain    --config_refresh=3600
    Osquery Flag File Should Contain    --extensions_require=SophosExtension
    Osquery Flag File Should Contain    --pack_refresh_interval=3600

    # Flags are different if scheduled queries are enabled, so check they change once we enable scheduled queries.
    ${edr_mark} =    Mark Log Size    ${EDR_LOG_FILE}
    Move File Atomically  ${EXAMPLE_DATA_PATH}/LiveQuery_policy_enabled.xml  /opt/sophos-spl/base/mcs/policy/LiveQuery_policy.xml
    wait_for_log_contains_from_mark    ${edr_mark}    Scheduled queries are enabled in policy
    Wait Until Keyword Succeeds
    ...    30 secs
    ...    2 secs
    ...    Osquery Flag File Should Contain    --extensions_require=SophosLoggerPlugin
   Osquery Flag File Should Contain    --extensions_timeout=10
   Osquery Flag File Should Contain    --logger_plugin=SophosLoggerPlugin


*** Variables ***

${OSQUERY_FLAG_FILE} =  ${SOPHOS_INSTALL}/plugins/edr/etc/osquery.flags

*** Keywords ***
XDR Pack Should Be Enabled
    File Should Exist  ${SOPHOS_INSTALL}/plugins/edr/etc/osquery.conf.d/sophos-scheduled-query-pack.conf
    File Should Not Exist  ${SOPHOS_INSTALL}/plugins/edr/etc/osquery.conf.d/sophos-scheduled-query-pack.conf.DISABLED
XDR Pack Should Be Disabled
    File Should Not Exist  ${SOPHOS_INSTALL}/plugins/edr/etc/osquery.conf.d/sophos-scheduled-query-pack.conf
    File Should Exist  ${SOPHOS_INSTALL}/plugins/edr/etc/osquery.conf.d/sophos-scheduled-query-pack.conf.DISABLED

MTR Pack Should Be Enabled
    File Should Exist  ${SOPHOS_INSTALL}/plugins/edr/etc/osquery.conf.d/sophos-scheduled-query-pack.mtr.conf
    File Should Not Exist  ${SOPHOS_INSTALL}/plugins/edr/etc/osquery.conf.d/sophos-scheduled-query-pack.mtr.conf.DISABLED
MTR Pack Should Be Disabled
    File Should Not Exist  ${SOPHOS_INSTALL}/plugins/edr/etc/osquery.conf.d/sophos-scheduled-query-pack.mtr.conf
    File Should Exist  ${SOPHOS_INSTALL}/plugins/edr/etc/osquery.conf.d/sophos-scheduled-query-pack.mtr.conf.DISABLED

Custom Pack Should Be Enabled
    File Should Exist  ${SOPHOS_INSTALL}/plugins/edr/etc/osquery.conf.d/sophos-scheduled-query-pack.custom.conf
    File Should Not Exist  ${SOPHOS_INSTALL}/plugins/edr/etc/osquery.conf.d/sophos-scheduled-query-pack.custom.conf.DISABLED
Custom Pack Should Be Disabled
    File Should Not Exist  ${SOPHOS_INSTALL}/plugins/edr/etc/osquery.conf.d/sophos-scheduled-query-pack.custom.conf
    File Should Exist  ${SOPHOS_INSTALL}/plugins/edr/etc/osquery.conf.d/sophos-scheduled-query-pack.custom.conf.DISABLED
Custom Pack Should Not Exist
    File Should Not Exist  ${SOPHOS_INSTALL}/plugins/edr/etc/osquery.conf.d/sophos-scheduled-query-pack.custom.conf
    File Should Not Exist  ${SOPHOS_INSTALL}/plugins/edr/etc/osquery.conf.d/sophos-scheduled-query-pack.custom.conf.DISABLED

ScheduleEpoch Should Be Recent
    ${scheduleEpoch} =  Get File  ${SOPHOS_INSTALL}/plugins/edr/var/persist-xdrScheduleEpoch
    ${currentEpochTime} =  get_current_epoch_time
    integer_is_within_range  ${scheduleEpoch}    ${currentEpochTime-60}  ${currentEpochTime}
    [Return]  ${scheduleEpoch}

LiveQuery Status Should Contain
    [Arguments]  ${StringToContain}
    ${status} =  Get File  ${SOPHOS_INSTALL}/base/mcs/status/LiveQuery_status.xml
    Should Contain  ${status}  ${StringToContain}

Wait For LiveQuery Status To Contain
    [Arguments]  ${StringToContain}
    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  LiveQuery Status Should Contain  ${StringToContain}

Expect New Datalimit
    [Arguments]  ${limit}
    Wait Until Keyword Succeeds
    ...  5 secs
    ...  1 secs
    ...  EDR Plugin Log Contains  Using dailyDataLimit from LiveQuery Policy: ${limit}
    Wait Until Keyword Succeeds
    ...  5 secs
    ...  1 secs
    ...  EDR Plugin Log Contains  Setting Data Limit to ${limit}

Change Next Query Packs Flag
    [Arguments]  ${flagValue}=false
    Create File  ${SOPHOS_INSTALL}/base/mcs/tmp/flags.json  {"scheduled_queries.next": ${flagValue}}
    ${result} =  Run Process  chown  sophos-spl-local:sophos-spl-group  ${SOPHOS_INSTALL}/base/mcs/tmp/flags.json
    Should Be Equal As Strings  0  ${result.rc}
    Move File  ${SOPHOS_INSTALL}/base/mcs/tmp/flags.json  ${SOPHOS_INSTALL}/base/mcs/policy/flags.json

Are Next Query Packs Enabled in Plugin Conf
    [Arguments]  ${settingValue}=0
    Wait Until Created  ${SOPHOS_INSTALL}/plugins/edr/etc/plugin.conf  timeout=15 secs
    Wait Until Keyword Succeeds
    ...  60 secs
    ...  5 secs
    ...  EDR Plugin Log Contains   Updating running_mode flag setting
    ${EDR_CONFIG_CONTENT}=  Get File  ${SOPHOS_INSTALL}/plugins/edr/etc/plugin.conf
    Should Contain  ${EDR_CONFIG_CONTENT}   scheduled_queries_next=${settingValue}

Add Uptime Query to Scheduled Queries
    [Arguments]  ${interval}=1
    # TODO: When LINUXDAR-3943 is implemented remove denylist option from the query config, and change test accordingly if required.
    Create File  ${SOPHOS_INSTALL}/plugins/edr/etc/query_packs/sophos-scheduled-query-pack.conf  {"schedule": {"uptime": {"query": "select * from uptime;","interval": ${interval}, "tag": "stream", "denylist": false}}}
    Create File  ${SOPHOS_INSTALL}/plugins/edr/etc/osquery.conf.d/sophos-scheduled-query-pack.conf.DISABLED  {"schedule": {"uptime": {"query": "select * from uptime;","interval": ${interval}, "tag": "stream", "denylist": false}}}

Dump Scheduled Query Table
    ${response} =    Run Live Query and Return Result  SELECT name,interval,executions,last_executed,denylisted,average_memory FROM osquery_schedule order by name
    print_query_result_as_table    ${response}

Verify RPM DB
    # Sometimes we see this in the osquery log "glog_logger.cpp:49] Could not get RPM header flag." Which seems to
    # cause osquery to get stuck for a bit. Adding this here to verify the RPM DB - if this check fails during a build
    # then we can look into running the rpm rebuild DB command to try and fix it before running tests.
    ${has_rpmdb_verify} =    Does File Exist    /usr/lib/rpm/rpmdb_verify
    IF    ${has_rpmdb_verify}
        ${result} =   Run Process    /usr/lib/rpm/rpmdb_verify    /var/lib/rpm/Packages
        Should Be Equal As Integers    ${result.rc}  0   "Failed to verify RPM DB \n stdout: \n${result.stdout}\n stderr: \n${result.stderr}"
    END

Set Discovery Query Interval In EDR SDDS Directory Config
    [Arguments]  ${interval}=20
    ${sdds_plugin_conf} =    Set Variable    ${EDR_SDDS}/files/plugins/edr/etc/plugin.conf
    ${current_config_content} =  Set Variable    ${EMPTY}
    ${config_exists} =    does_file_exist    ${sdds_plugin_conf}
    IF  ${config_exists}
        ${current_config_content} =     Get File    ${sdds_plugin_conf}
        log file    ${sdds_plugin_conf}
    END
    Run Keyword If    "pack_refresh_interval" not in "${current_config_content}"
    ...    Append To File    ${sdds_plugin_conf}    pack_refresh_interval=${interval}\n
    log file    ${sdds_plugin_conf}


Test Setup
    Install EDR Directly from SDDS
    Check EDR Plugin Installed With Base
    Wait Until Keyword Succeeds
    ...   20 secs
    ...   2 secs
    ...   Check Osquery Running

Test Teardown
    Remove File    ${EDR_SDDS}/files/plugins/edr/etc/plugin.conf
    Run Keyword If Test Failed    Run Keyword And Ignore Error    Clear Datafeed Dir And Wait For Next Result File
    Run Keyword If Test Failed    Run Keyword And Ignore Error    Dump Scheduled Query Table
    Run Keyword If Test Failed    Verify RPM DB
    EDR And Base Teardown Without Starting EDR
    Uninstall EDR
    clear_datafeed_folder
    Remove File  ${SOPHOS_INSTALL}/base/mcs/policy/LiveQuery_policy.xml

Osquery Flag File Should Contain
    [Arguments]  ${stringToContain}
    Should Exist    ${OSQUERY_FLAG_FILE}
    ${flags} =  Get File  ${OSQUERY_FLAG_FILE}
    Should Contain  ${flags}   ${stringToContain}

Clear Datafeed Dir And Wait For Next Result File
    Empty Directory  ${SOPHOS_INSTALL}/base/mcs/datafeed
    ${query_file} =  Wait For Scheduled Query File And Return Filename
    ${query_results} =  Get File  ${SOPHOS_INSTALL}/base/mcs/datafeed/${query_file}
    Log  ${query_results}
    [Return]  ${query_results}

Check Query Results Are Folded
    [Arguments]  ${result_string}  ${query_name}  ${column_name}  ${column_value}=${None}
    ${IsFolded} =  Check Query Results Folded  ${result_string}  ${query_name}  ${column_name}  ${column_value}
    Should Be True  ${IsFolded}

Check Query Results Are Not Folded
    [Arguments]  ${result_string}  ${query_name}  ${column_name}  ${column_value}
    ${IsFolded} =  Check Query Results Folded  ${result_string}  ${query_name}  ${column_name}  ${column_value}
    Should Not Be True  ${IsFolded}

Apply Live Query Policy And Expect Folding Rules To Have Changed
    [Arguments]  ${policy_file}
    Move File Atomically  ${policy_file}  /opt/sophos-spl/base/mcs/policy/LiveQuery_policy.xml
    Wait Until Keyword Succeeds
    ...  5 secs
    ...  1 secs
    ...  EDR Plugin Log Contains  Processing LiveQuery Policy
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  EDR Plugin Log Contains  LiveQuery Policy folding rules have changed


Data Limit Test Setup
    [Arguments]  ${live_query_policy}  ${period}
    Move File Atomically  ${live_query_policy}  /opt/sophos-spl/base/mcs/policy/LiveQuery_policy.xml

    # Create specific EDR dirs to drop in files before running installer.
    Should Not Exist  ${SOPHOS_INSTALL}/plugins/edr
    Create Directory  ${SOPHOS_INSTALL}/plugins/edr/etc/osquery.conf.d/

    # These are not included in the build output of EDR, they are supplements so must be manually copied into place
    Should Not Exist  ${SOPHOS_INSTALL}/plugins/edr/etc/osquery.conf.d/sophos-scheduled-query-pack.conf
    Should Not Exist  ${SOPHOS_INSTALL}/plugins/edr/etc/osquery.conf.d/sophos-scheduled-query-pack.conf.DISABLED

    # Inject query pack and initial data limit period into SDDS
    Create File  ${EDR_SDDS}/files/plugins/edr/var/persist-xdrPeriodInSeconds   ${period}

    Install EDR Directly from SDDS With mocked scheduled queries  10

Data Limit Test Teardown
    Create File  ${EDR_SDDS}/files/plugins/edr/var/persist-xdrPeriodInSeconds   86400
    Test Teardown