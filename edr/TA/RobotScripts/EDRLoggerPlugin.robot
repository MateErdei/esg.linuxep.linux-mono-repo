*** Settings ***
Documentation    Testing the Logger Plugin for XDR Behaviour

Library         Process
Library         OperatingSystem
Library         Collections
Library         ../Libs/XDRLibs.py

Resource        EDRResources.robot
Resource        ComponentSetup.robot

Suite Setup     No Operation
Suite Teardown  No Operation

Test Setup      Install With Base SDDS
Test Teardown   Test Teardown

*** Test Cases ***
EDR Plugin outputs XDR results and Its Answer is available to MCSRouter
    Check EDR Plugin Installed With Base
    Add Uptime Query to Scheduled Queries
    Directory Should Be Empty  ${SOPHOS_INSTALL}/base/mcs/datafeed
    Enable XDR
    Wait Until Keyword Succeeds
    ...  100 secs
    ...  5 secs
    ...  Directory Should Not Be Empty  ${SOPHOS_INSTALL}/base/mcs/datafeed

EDR Plugin Restarts Osquery When Custom Queries Have Changed
    Check EDR Plugin Installed With Base
    Run Keyword And Ignore Error  Remove File  ${SOPHOS_INSTALL}/base/etc/logger.conf
    Create File  ${SOPHOS_INSTALL}/base/etc/logger.conf  [global]\nVERBOSITY = DEBUG\n
    Run Shell Process  ${SOPHOS_INSTALL}/bin/wdctl stop edr   OnError=failed to stop edr
    Run Shell Process  ${SOPHOS_INSTALL}/bin/wdctl start edr   OnError=failed to stop edr

    Enable XDR
    Directory Should Be Empty  ${SOPHOS_INSTALL}/base/mcs/datafeed
    Move File Atomically  ${EXAMPLE_DATA_PATH}/LiveQuery_policy_customquery_limit.xml  /opt/sophos-spl/base/mcs/policy/LiveQuery_policy.xml
    Wait Until Keyword Succeeds
    ...  5 secs
    ...  1 secs
    ...  EDR Plugin Log Contains  Processing LiveQuery Policy
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  File Should Exist  ${SOPHOS_INSTALL}/plugins/edr/etc/osquery.conf.d/sophos-scheduled-query-pack.custom.conf
    Log File  ${SOPHOS_INSTALL}/plugins/edr/etc/osquery.conf.d/sophos-scheduled-query-pack.custom.conf

    Wait Until Keyword Succeeds
    ...  10 secs
    ...  2 secs
    ...  Check All Queries Run  ${SOPHOS_INSTALL}/plugins/edr/log/scheduledquery.log  ${SOPHOS_INSTALL}/plugins/edr/etc/osquery.conf.d/sophos-scheduled-query-pack.custom.conf

EDR Plugin Tags All Queries Correctly
    Check EDR Plugin Installed With Base
    Run Keyword And Ignore Error  Remove File  ${SOPHOS_INSTALL}/base/etc/logger.conf
    Create File  ${SOPHOS_INSTALL}/base/etc/logger.conf  [global]\nVERBOSITY = DEBUG\n
    Directory Should Be Empty  ${SOPHOS_INSTALL}/base/mcs/datafeed
    Move File Atomically  ${EXAMPLE_DATA_PATH}/LiveQuery_policy_customquery_limit.xml  /opt/sophos-spl/base/mcs/policy/LiveQuery_policy.xml
    Enable XDR

    #restart edr so that the altered queries are read in and debug mode applied
    Restart EDR

    Wait Until Keyword Succeeds
    ...  100 secs
    ...  5 secs
    ...  Check All Query Results Contain Correct Tag  ${SOPHOS_INSTALL}/base/mcs/datafeed/  ${SOPHOS_INSTALL}/plugins/edr/etc/osquery.conf.d/sophos-scheduled-query-pack.conf    ${SOPHOS_INSTALL}/plugins/edr/etc/osquery.conf.d/sophos-scheduled-query-pack.mtr.conf  ${SOPHOS_INSTALL}/plugins/edr/etc/osquery.conf.d/sophos-scheduled-query-pack.custom.conf

EDR Plugin Applies Folding Rules When Folding Rules Have Changed
    [Setup]  Install With Base SDDS
    Check EDR Plugin Installed With Base
    Run Keyword And Ignore Error  Remove File  ${SOPHOS_INSTALL}/base/etc/logger.conf
    Create File  ${SOPHOS_INSTALL}/base/etc/logger.conf  [global]\nVERBOSITY = DEBUG\n
    Create File  ${SOPHOS_INSTALL}/plugins/edr/etc/query_packs/sophos-scheduled-query-pack.conf  { "schedule": { "uptime": { "query": "SELECT *, 'fixed_value' as fixed_column FROM uptime;", "interval": 3, "removed": false, "denylist": false, "description": "Test query", "tag": "DataLake" }, "uptime_not_folded": { "query": "SELECT *, 'fixed_value' as fixed_column FROM uptime;", "interval": 3, "removed": false, "denylist": false, "description": "Test query", "tag": "DataLake" } } }
    Create File  ${SOPHOS_INSTALL}/plugins/edr/etc/osquery.conf.d/sophos-scheduled-query-pack.conf  { "schedule": { "uptime": { "query": "SELECT *, 'fixed_value' as fixed_column FROM uptime;", "interval": 3, "removed": false, "denylist": false, "description": "Test query", "tag": "DataLake" }, "uptime_not_folded": { "query": "SELECT *, 'fixed_value' as fixed_column FROM uptime;", "interval": 3, "removed": false, "denylist": false, "description": "Test query", "tag": "DataLake" } } }
    Restart EDR
    Enable XDR
    Directory Should Be Empty  ${SOPHOS_INSTALL}/base/mcs/datafeed

    # Inject policy with folding rules
    Apply Live Query Policy And Expect Folding Rules To Have Changed  ${EXAMPLE_DATA_PATH}/LiveQuery_policy_foldingrules_limit.xml

    # Throw away one set of results here so that we are certain they are not from before the folding rules were applied
    ${query_file} =  Clear Datafeed Dir And Wait For Next Result File

    # Wait for a result we know will contain folded and non-folded results
    ${query_file} =  Clear Datafeed Dir And Wait For Next Result File
    ${query_results} =  Get File  ${SOPHOS_INSTALL}/base/mcs/datafeed/${query_file}
    Check Query Results Are Folded  ${query_results}  uptime  fixed_column  fixed_value
    Check Query Results Are Not Folded  ${query_results}  uptime_not_folded  fixed_column  fixed_value

    # Wait for a 2nd batch of result to prove that folding is done per batch, i.e. the folded query shows up again
    ${query_file} =  Clear Datafeed Dir And Wait For Next Result File
    ${query_results} =  Get File  ${SOPHOS_INSTALL}/base/mcs/datafeed/${query_file}
    Check Query Results Are Folded  ${query_results}  uptime  fixed_column  fixed_value
    Check Query Results Are Not Folded  ${query_results}  uptime_not_folded  fixed_column  fixed_value

    # Inject policy without folding rules
    Apply Live Query Policy And Expect Folding Rules To Have Changed  ${EXAMPLE_DATA_PATH}/LiveQuery_policy_100000_limit.xml

    # Throw away one set of results here so that we are certain they are not from before the folding rules were removed
    ${query_file} =  Clear Datafeed Dir And Wait For Next Result File

    # Wait until the results appear and check they are not folded now there are no folding rules
    ${query_file} =  Clear Datafeed Dir And Wait For Next Result File
    ${query_results} =  Get File  ${SOPHOS_INSTALL}/base/mcs/datafeed/${query_file}
    Check Query Results Are Not Folded  ${query_results}  uptime  fixed_column  fixed_value

    # Telemetry
    ${edr_telemetry} =  Get Plugin Telemetry  edr
    ${telemetry_json} =  Evaluate  json.loads('''${edr_telemetry}''')  json
    ${foldable_queries} =  Set Variable  ${telemetry_json['foldable-queries']}
    List Should Contain Value  ${foldable_queries}  uptime
    ${query_json} =  Set Variable  ${telemetry_json['scheduled-queries']['uptime']}
    Dictionary Should Contain Key  ${query_json}  folded-count
    ${folded_count} =  Get From Dictionary  ${query_json}  folded-count
    Should Be True  ${folded_count} > 1
    Should Be True  ${folded_count} < 100

EDR Plugin Applies Folding Rules Based Column Value
    [Setup]  Install With Base SDDS
    Check EDR Plugin Installed With Base
    Run Keyword And Ignore Error  Remove File  ${SOPHOS_INSTALL}/base/etc/logger.conf
    Create File  ${SOPHOS_INSTALL}/base/etc/logger.conf  [global]\nVERBOSITY = DEBUG\n
    Create File  ${SOPHOS_INSTALL}/plugins/edr/etc/query_packs/sophos-scheduled-query-pack.conf  { "schedule": { "random": { "query": "SELECT abs(random() % 2) AS number;", "interval": 1, "removed": false, "denylist": false, "description": "Test query", "tag": "DataLake" } } }
    Create File  ${SOPHOS_INSTALL}/plugins/edr/etc/osquery.conf.d/sophos-scheduled-query-pack.conf  { "schedule": { "random": { "query": "SELECT abs(random() % 2) AS number;", "interval": 1, "removed": false, "denylist": false, "description": "Test query", "tag": "DataLake" } } }
    Restart EDR

    Enable XDR
    Directory Should Be Empty  ${SOPHOS_INSTALL}/base/mcs/datafeed

    # Inject policy with folding rules
    Apply Live Query Policy And Expect Folding Rules To Have Changed  ${EXAMPLE_DATA_PATH}/LiveQuery_policy_foldingrules_limit.xml

    # Throw away one set of results here so that we are certain they are not from before the folding rules were applied
    ${query_file} =  Clear Datafeed Dir And Wait For Next Result File

    # Wait for a result we know will contain folded and non-folded results
    ${query_file} =  Clear Datafeed Dir And Wait For Next Result File
    ${query_results} =  Get File  ${SOPHOS_INSTALL}/base/mcs/datafeed/${query_file}
    Check Query Results Are Folded  ${query_results}  random  number  0
    Check Query Results Are Not Folded  ${query_results}  random  number  1

    # Telemetry
    ${edr_telemetry} =  Get Plugin Telemetry  edr
    ${telemetry_json} =  Evaluate  json.loads('''${edr_telemetry}''')  json
    ${foldable_queries} =  Set Variable  ${telemetry_json['foldable-queries']}
    List Should Contain Value  ${foldable_queries}  random
    ${query_json} =  Set Variable  ${telemetry_json['scheduled-queries']['random']}
    Dictionary Should Contain Key  ${query_json}  folded-count
    ${folded_count} =  Get From Dictionary  ${query_json}  folded-count
    Should Be True  ${folded_count} > 1
    Should Be True  ${folded_count} < 100

EDR Plugin Runs All Scheduled Queries
    [Setup]  Install With Base SDDS
    Check EDR Plugin Installed With Base
    Run Keyword And Ignore Error  Remove File  ${SOPHOS_INSTALL}/base/etc/logger.conf
    Create File  ${SOPHOS_INSTALL}/base/etc/logger.conf  [global]\nVERBOSITY = DEBUG\n
    Directory Should Be Empty  ${SOPHOS_INSTALL}/base/mcs/datafeed
    Move File Atomically  ${EXAMPLE_DATA_PATH}/LiveQuery_policy_customquery_limit.xml  /opt/sophos-spl/base/mcs/policy/LiveQuery_policy.xml
    Enable XDR

    #restart edr so that the altered queries are read in and debug mode applied
    Restart EDR

    Wait Until Keyword Succeeds
    ...  50 secs
    ...  10 secs
    ...  Check All Queries Run  ${SOPHOS_INSTALL}/plugins/edr/log/scheduledquery.log  ${SOPHOS_INSTALL}/plugins/edr/etc/osquery.conf.d/sophos-scheduled-query-pack.conf

    Wait Until Keyword Succeeds
    ...  10 secs
    ...  2 secs
    ...  Check All Queries Run  ${SOPHOS_INSTALL}/plugins/edr/log/scheduledquery.log  ${SOPHOS_INSTALL}/plugins/edr/etc/osquery.conf.d/sophos-scheduled-query-pack.mtr.conf

    Wait Until Keyword Succeeds
    ...  10 secs
    ...  2 secs
    ...  Check All Queries Run  ${SOPHOS_INSTALL}/plugins/edr/log/scheduledquery.log  ${SOPHOS_INSTALL}/plugins/edr/etc/osquery.conf.d/sophos-scheduled-query-pack.custom.conf
    Wait Until Keyword Succeeds
    ...  100 secs
    ...  5 secs
    ...  Directory Should Not Be Empty  ${SOPHOS_INSTALL}/base/mcs/datafeed

EDR Plugin Detects Data Limit From Policy And That A Status Is Sent On Start
    [Setup]  No Operation
    Install Base For Component Tests
    Create File  ${SOPHOS_INSTALL}/base/etc/logger.conf  [global]\nVERBOSITY = DEBUG\n
    Move File Atomically  ${EXAMPLE_DATA_PATH}/LiveQuery_policy_100000_limit.xml  /opt/sophos-spl/base/mcs/policy/LiveQuery_policy.xml
    Install EDR Directly from SDDS

    Wait Until Keyword Succeeds
    ...  5 secs
    ...  1 secs
    ...  EDR Plugin Log Contains  First LiveQuery policy received
    Expect New Datalimit  100000

    Move File Atomically  ${EXAMPLE_DATA_PATH}/LiveQuery_policy_250000000_limit.xml  /opt/sophos-spl/base/mcs/policy/LiveQuery_policy.xml
    Expect New Datalimit  250000000
    Wait For LiveQuery Status To Contain  <dailyDataLimitExceeded>false</dailyDataLimitExceeded>

EDR Plugin writes custom query file when it recieves a Live Query policy and removes it when there are no custom queries
    [Setup]  No Operation
    Install Base For Component Tests
    Create File  ${SOPHOS_INSTALL}/base/etc/logger.conf  [global]\nVERBOSITY = DEBUG\n
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

EDR Plugin Send LiveQuery Status On Period Rollover
    [Setup]  No Operation
    Install Base For Component Tests
    Create File  ${SOPHOS_INSTALL}/base/etc/logger.conf  [global]\nVERBOSITY = DEBUG\n
    Move File Atomically  ${EXAMPLE_DATA_PATH}/LiveQuery_policy_10000_limit.xml  /opt/sophos-spl/base/mcs/policy/LiveQuery_policy.xml
    Install EDR Directly from SDDS
    Enable XDR

    Run Shell Process  ${SOPHOS_INSTALL}/bin/wdctl stop edr   OnError=failed to stop edr
    Create File  ${SOPHOS_INSTALL}/plugins/edr/var/persist-xdrPeriodTimestamp  10
    Run Shell Process  ${SOPHOS_INSTALL}/bin/wdctl start edr   OnError=failed to start edr
    Wait For LiveQuery Status To Contain  <dailyDataLimitExceeded>false</dailyDataLimitExceeded>
    Remove File  ${SOPHOS_INSTALL}/base/mcs/status/cache/LiveQuery.xml
    Remove File  ${SOPHOS_INSTALL}/base/mcs/status/LiveQuery_status.xml
    Run Shell Process  ${SOPHOS_INSTALL}/bin/wdctl stop managementagent   OnError=failed to stop managementagent
    Run Shell Process  ${SOPHOS_INSTALL}/bin/wdctl start managementagent   OnError=failed to start managementagent

    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  EDR Plugin Log Contains  XDR period has rolled over
    Wait For LiveQuery Status To Contain  <dailyDataLimitExceeded>false</dailyDataLimitExceeded>

EDR Plugin Respects Data Limit
    [Setup]  No Operation
    Install Base For Component Tests
    Create File  ${SOPHOS_INSTALL}/base/etc/logger.conf  [global]\nVERBOSITY = DEBUG\n
    Move File Atomically  ${EXAMPLE_DATA_PATH}/LiveQuery_policy_10000_limit.xml  /opt/sophos-spl/base/mcs/policy/LiveQuery_policy.xml
    Install EDR Directly from SDDS
    Wait Until Keyword Succeeds
    ...  5 secs
    ...  1 secs
    ...  EDR Plugin Log Contains  First LiveQuery policy received
    Expect New Datalimit  10000
    Enable XDR

    # Restart edr so that the altered queries are read in and debug mode applied
    Restart EDR

    Wait Until Keyword Succeeds
    ...  20 secs
    ...  10 secs
    ...  EDR Plugin Log Contains   XDR data limit for this period exceeded

    Wait For LiveQuery Status To Contain  <dailyDataLimitExceeded>true</dailyDataLimitExceeded>

EDR Plugin Rolls ScheduleEpoch Over When The Previous One Has Elapsed
    [Setup]  No Operation
    Install Base For Component Tests
    Create File  ${SOPHOS_INSTALL}/base/etc/logger.conf  [global]\nVERBOSITY = DEBUG\n
    Install EDR Directly from SDDS
    Enable XDR
    ${oldScheduleEpochTimestamp} =  Set Variable  1600000000
    Run Shell Process  ${SOPHOS_INSTALL}/bin/wdctl stop edr   OnError=failed to stop edr
    Create File  ${SOPHOS_INSTALL}/plugins/edr/var/persist-xdrScheduleEpoch  ${oldScheduleEpochTimestamp}
    Run Shell Process  ${SOPHOS_INSTALL}/bin/wdctl start edr   OnError=failed to start edr

    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  EDR Plugin Log Contains   Using osquery schedule_epoch flag as: --schedule_epoch=${oldScheduleEpochTimestamp}

    Wait Until Keyword Succeeds
    ...  5 secs
    ...  1 secs
    ...  EDR Plugin Log Contains   Previous schedule_epoch: ${oldScheduleEpochTimestamp}, has ended.
    ${scheduleEpoch} =  ScheduleEpoch Should Be Recent
    Should Not Be Equal As Strings  ${scheduleEpoch}  ${oldScheduleEpochTimestamp}
    EDR Plugin Log Contains   Starting new schedule_epoch: ${scheduleEpoch}
    Wait Until Keyword Succeeds
    ...  5 secs
    ...  1 secs
    ...  EDR Plugin Log Contains   Using osquery schedule_epoch flag as: --schedule_epoch=${scheduleEpoch}
    Osquery Flag File Should Contain  --schedule_epoch=${scheduleEpoch}

EDR Plugin Does Not Roll ScheduleEpoch Over When The Previous One Has Not Elapsed
    [Setup]  No Operation
    Install Base For Component Tests
    ${currentEpochTime} =  get_current_epoch_time
    ${currentEpochTimeMinus3Days} =  Evaluate  ${currentEpochTime} - (60*60*24*3)
    Create File  ${SOPHOS_INSTALL}/base/etc/logger.conf  [global]\nVERBOSITY = DEBUG\n
    Install EDR Directly from SDDS
    Enable XDR

    Run Shell Process  ${SOPHOS_INSTALL}/bin/wdctl stop edr   OnError=failed to stop edr
    Create File  ${SOPHOS_INSTALL}/plugins/edr/var/persist-xdrScheduleEpoch  ${currentEpochTimeMinus3Days.__str__()}
    Run Shell Process  ${SOPHOS_INSTALL}/bin/wdctl start edr   OnError=failed to start edr

    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  EDR Plugin Log Contains   Using osquery schedule_epoch flag as: --schedule_epoch=${currentEpochTimeMinus3Days}
    Wait Until Keyword Succeeds
    ...  5 secs
    ...  1 secs
    ...  EDR Plugin Log Does Not Contain   Starting new schedule_epoch
    Osquery Flag File Should Contain  --schedule_epoch=${currentEpochTimeMinus3Days}

Check XDR Results Contain Correct ScheduleEpoch Timestamp
    [Setup]  No Operation
    Install Base For Component Tests
    ${currentEpochTime} =  get_current_epoch_time
    ${currentEpochTimeMinus3Days} =  Evaluate  ${currentEpochTime} - (60*60*24*3)
    Create File  ${SOPHOS_INSTALL}/base/etc/logger.conf  [global]\nVERBOSITY = DEBUG\n
    Install EDR Directly from SDDS

    Enable XDR

    Run Shell Process  ${SOPHOS_INSTALL}/bin/wdctl stop edr   OnError=failed to stop edr
    Create File  ${SOPHOS_INSTALL}/plugins/edr/var/persist-xdrScheduleEpoch  ${currentEpochTimeMinus3Days.__str__()}
    Run Shell Process  ${SOPHOS_INSTALL}/bin/wdctl start edr   OnError=failed to start edr

    Wait Until Keyword Succeeds
    ...  15 secs
    ...  1 secs
    ...  Osquery Flag File Should Contain  --schedule_epoch=${currentEpochTimeMinus3Days}

    ${scheduledQueryFilename} =  wait_for_scheduled_query_file_and_return_filename
    ${scheduledQueryContents} =  Get File  ${SOPHOS_INSTALL}/base/mcs/datafeed/${scheduledQueryFilename}
    Should Contain  ${scheduledQueryContents}  "epoch":${currentEpochTimeMinus3Days}

EDR Plugin Runs Next Scheduled Queries When Flags Configured To Do So
    Check EDR Plugin Installed With Base
    Run Keyword And Ignore Error  Remove File  ${SOPHOS_INSTALL}/base/etc/logger.conf
    Create File  ${SOPHOS_INSTALL}/base/etc/logger.conf  [global]\nVERBOSITY = DEBUG\n
    Create File  ${SOPHOS_INSTALL}/plugins/edr/etc/query_packs/sophos-scheduled-query-pack.conf  {"schedule": {"latest_xdr_query": {"query": "select * from uptime;","interval": 2}}}
    Create File  ${SOPHOS_INSTALL}/plugins/edr/etc/query_packs/sophos-scheduled-query-pack.mtr.conf  {"schedule": {"latest_mtr_query": {"query": "select * from uptime;","interval": 2}}}
    Create File  ${SOPHOS_INSTALL}/plugins/edr/etc/query_packs/sophos-scheduled-query-pack-next.conf  {"schedule": {"next_xdr_query": {"query": "select * from uptime;","interval": 2}}}
    Create File  ${SOPHOS_INSTALL}/plugins/edr/etc/query_packs/sophos-scheduled-query-pack-next.mtr.conf  {"schedule": {"next_mtr_query": {"query": "select * from uptime;","interval": 2}}}
    Restart EDR

    Directory Should Be Empty  ${SOPHOS_INSTALL}/base/mcs/datafeed

    Enable XDR
    ${mark} =  Mark File  ${SOPHOS_INSTALL}/plugins/edr/log/scheduledquery.log


    Wait Until Keyword Succeeds
    ...  5 secs
    ...  1 secs
    ...  Marked File Contains  ${SOPHOS_INSTALL}/plugins/edr/log/scheduledquery.log   latest_xdr_query  ${mark}
    Wait Until Keyword Succeeds
    ...  5 secs
    ...  1 secs
    ...  Marked File Contains  ${SOPHOS_INSTALL}/plugins/edr/log/scheduledquery.log   latest_mtr_query  ${mark}
    sleep  4s
    File Log Does Not Contain  ${SOPHOS_INSTALL}/plugins/edr/log/scheduledquery.log   next_xdr_query
    File Log Does Not Contain  ${SOPHOS_INSTALL}/plugins/edr/log/scheduledquery.log   next_mtr_query

    ${edrMark} =  Mark File  ${SOPHOS_INSTALL}/plugins/edr/log/edr.log
    Enable XDR And Change Next Query Packs Flag  true
    Wait Until Keyword Succeeds
    ...  5 secs
    ...  1 secs
    ...  Marked File Contains  ${SOPHOS_INSTALL}/plugins/edr/log/edr.log  Prepare system for running osquery  ${edrMark}
    ${mark} =  Mark File  ${SOPHOS_INSTALL}/plugins/edr/log/scheduledquery.log
    Are Next Query Packs Enabled in Plugin Conf  1

    Wait Until Keyword Succeeds
    ...  5 secs
    ...  1 secs
    ...  Marked File Contains  ${SOPHOS_INSTALL}/plugins/edr/log/scheduledquery.log   next_xdr_query  ${mark}
    Wait Until Keyword Succeeds
    ...  5 secs
    ...  1 secs
    ...  Marked File Contains  ${SOPHOS_INSTALL}/plugins/edr/log/scheduledquery.log   next_mtr_query  ${mark}
    sleep  4s
    Marked File Does Not Contain  ${SOPHOS_INSTALL}/plugins/edr/log/scheduledquery.log   latest_xdr_query  ${mark}
    Marked File Does Not Contain  ${SOPHOS_INSTALL}/plugins/edr/log/scheduledquery.log   latest_mtr_query  ${mark}

    ${edrMark} =  Mark File  ${SOPHOS_INSTALL}/plugins/edr/log/edr.log
    Enable XDR And Change Next Query Packs Flag  false
    Wait Until Keyword Succeeds
    ...  5 secs
    ...  1 secs
    ...  Marked File Contains  ${SOPHOS_INSTALL}/plugins/edr/log/edr.log  Prepare system for running osquery  ${edrMark}
    ${mark} =  Mark File  ${SOPHOS_INSTALL}/plugins/edr/log/scheduledquery.log
    Are Next Query Packs Enabled in Plugin Conf  0

    Wait Until Keyword Succeeds
    ...  5 secs
    ...  1 secs
    ...  Marked File Contains  ${SOPHOS_INSTALL}/plugins/edr/log/scheduledquery.log   latest_xdr_query  ${mark}
    Wait Until Keyword Succeeds
    ...  5 secs
    ...  1 secs
    ...  Marked File Contains  ${SOPHOS_INSTALL}/plugins/edr/log/scheduledquery.log   latest_mtr_query  ${mark}
    sleep  4s
    Marked File Does Not Contain  ${SOPHOS_INSTALL}/plugins/edr/log/scheduledquery.log   next_xdr_query  ${mark}
    Marked File Does Not Contain  ${SOPHOS_INSTALL}/plugins/edr/log/scheduledquery.log   next_mtr_query  ${mark}


*** Keywords ***
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
    ...  10 secs
    ...  1 secs
    ...  LiveQuery Status Should Contain  ${StringToContain}

Move File Atomically
    [Arguments]  ${source}  ${destination}
    Copy File  ${source}  /opt/NotARealFile
    Move File  /opt/NotARealFile  ${destination}

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

Enable XDR And Change Next Query Packs Flag
    [Arguments]  ${flagValue}=false
    Create File  ${SOPHOS_INSTALL}/base/mcs/tmp/flags.json  {"xdr.enabled": true, "scheduled_queries.next": ${flagValue}}
    ${result} =  Run Process  chown  sophos-spl-local:sophos-spl-group  ${SOPHOS_INSTALL}/base/mcs/tmp/flags.json
    Should Be Equal As Strings  0  ${result.rc}
    Move File  ${SOPHOS_INSTALL}/base/mcs/tmp/flags.json  ${SOPHOS_INSTALL}/base/mcs/policy/flags.json

Enable XDR
    Create File  ${SOPHOS_INSTALL}/base/mcs/tmp/flags.json  {"xdr.enabled": true, "scheduled_queries.next": false}
    ${result} =  Run Process  chown  sophos-spl-local:sophos-spl-group  ${SOPHOS_INSTALL}/base/mcs/tmp/flags.json
    Should Be Equal As Strings  0  ${result.rc}
    Move File  ${SOPHOS_INSTALL}/base/mcs/tmp/flags.json  ${SOPHOS_INSTALL}/base/mcs/policy/flags.json
    Is XDR Enabled in Plugin Conf

Is XDR Enabled in Plugin Conf
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  1 secs
    ...  File Should Exist    ${SOPHOS_INSTALL}/plugins/edr/etc/plugin.conf
    Wait Until Keyword Succeeds
    ...  60 secs
    ...  5 secs
    ...  EDR Plugin Log Contains   Flags have changed so restarting osquery
    ${EDR_CONFIG_CONTENT}=  Get File  ${SOPHOS_INSTALL}/plugins/edr/etc/plugin.conf
    Should Contain  ${EDR_CONFIG_CONTENT}   running_mode=1

Are Next Query Packs Enabled in Plugin Conf
    [Arguments]  ${settingValue}=0
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  1 secs
    ...  File Should Exist    ${SOPHOS_INSTALL}/plugins/edr/etc/plugin.conf
    Wait Until Keyword Succeeds
    ...  60 secs
    ...  5 secs
    ...  EDR Plugin Log Contains   Flags have changed so restarting osquery
    ${EDR_CONFIG_CONTENT}=  Get File  ${SOPHOS_INSTALL}/plugins/edr/etc/plugin.conf
    Should Contain  ${EDR_CONFIG_CONTENT}   scheduled_queries_next=${settingValue}

Add Uptime Query to Scheduled Queries
    Run Keyword And Ignore Error  Remove File  ${SOPHOS_INSTALL}/plugins/edr/etc/query_packs/sophos-scheduled-query-pack.conf
    Should Not Exist  ${SOPHOS_INSTALL}/plugins/edr/etc/query_packs/sophos-scheduled-query-pack.conf
    Create File  ${SOPHOS_INSTALL}/plugins/edr/etc/query_packs/sophos-scheduled-query-pack.conf  {"schedule": {"uptime": {"query": "select * from uptime;","interval": 1}}}

Test Teardown
    EDR And Base Teardown
    Uninstall And Revert Setup

Osquery Flag File Should Contain
    [Arguments]  ${stringToContain}
    ${flags} =  Get File  ${SOPHOS_INSTALL}/plugins/edr/etc/osquery.flags
    Should Contain  ${flags}   ${stringToContain}

Clear Datafeed Dir And Wait For Next Result File
    Empty Directory  ${SOPHOS_INSTALL}/base/mcs/datafeed
    ${QueryFile} =  Wait For Scheduled Query File And Return Filename
    [Return]  ${QueryFile}

Check Query Results Are Folded
    [Arguments]  ${result_string}  ${query_name}  ${column_name}  ${column_value}
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