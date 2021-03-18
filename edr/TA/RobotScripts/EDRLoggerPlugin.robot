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

Test Setup      Run Keywords
...  Install With Base SDDS  AND
...  Check EDR Plugin Installed With Base

Test Teardown   Test Teardown

*** Test Cases ***
EDR Plugin outputs XDR results and Its Answer is available to MCSRouter
    Add Uptime Query to Scheduled Queries
    Directory Should Be Empty  ${SOPHOS_INSTALL}/base/mcs/datafeed
    Enable XDR
    Wait Until Keyword Succeeds
    ...  100 secs
    ...  5 secs
    ...  Directory Should Not Be Empty  ${SOPHOS_INSTALL}/base/mcs/datafeed

EDR Plugin Restarts Osquery When Custom Queries Have Changed
    Run Keyword And Ignore Error  Remove File  ${SOPHOS_INSTALL}/base/etc/logger.conf
    Create Debug Level Logger Config File
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
    Run Keyword And Ignore Error  Remove File  ${SOPHOS_INSTALL}/base/etc/logger.conf
    Create Debug Level Logger Config File
    Directory Should Be Empty  ${SOPHOS_INSTALL}/base/mcs/datafeed
    Move File Atomically  ${EXAMPLE_DATA_PATH}/LiveQuery_policy_customquery_limit.xml  /opt/sophos-spl/base/mcs/policy/LiveQuery_policy.xml

    #restart edr so that the altered queries are read in and debug mode applied
    Restart EDR

    Wait Until Keyword Succeeds
    ...  100 secs
    ...  5 secs
    ...  Check All Query Results Contain Correct Tag  ${SOPHOS_INSTALL}/base/mcs/datafeed/  ${SOPHOS_INSTALL}/plugins/edr/etc/osquery.conf.d/sophos-scheduled-query-pack.conf    ${SOPHOS_INSTALL}/plugins/edr/etc/osquery.conf.d/sophos-scheduled-query-pack.mtr.conf  ${SOPHOS_INSTALL}/plugins/edr/etc/osquery.conf.d/sophos-scheduled-query-pack.custom.conf

EDR Plugin Applies Folding Rules When Folding Rules Have Changed
    [Setup]  Install With Base SDDS With Fixed Values Queries
    Check EDR Plugin Installed With Base
    Run Keyword And Ignore Error  Remove File  ${SOPHOS_INSTALL}/base/etc/logger.conf
    Create Debug Level Logger Config File
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
    [Setup]  Install With Base SDDS With Random Queries
    Check EDR Plugin Installed With Base
    Run Keyword And Ignore Error  Remove File  ${SOPHOS_INSTALL}/base/etc/logger.conf
    Create Debug Level Logger Config File
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
    Run Keyword And Ignore Error  Remove File  ${SOPHOS_INSTALL}/base/etc/logger.conf
    Create Debug Level Logger Config File
    Directory Should Be Empty  ${SOPHOS_INSTALL}/base/mcs/datafeed
    Move File Atomically  ${EXAMPLE_DATA_PATH}/LiveQuery_policy_customquery_limit.xml  /opt/sophos-spl/base/mcs/policy/LiveQuery_policy.xml

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

EDR Plugin Logs Broken JSON In Scheduled Query Pack
    [Setup]  Install With Base SDDS
    Check EDR Plugin Installed With Base
    Run Keyword And Ignore Error  Remove File  ${SOPHOS_INSTALL}/base/etc/logger.conf
    Create File  ${SOPHOS_INSTALL}/base/etc/logger.conf  [global]\nVERBOSITY = DEBUG\n
    Run Keyword And Ignore Error  Remove File  ${SOPHOS_INSTALL}/plugins/edr/etc/query_packs/sophos-scheduled-query-pack.conf
    Should Not Exist  ${SOPHOS_INSTALL}/plugins/edr/etc/query_packs/sophos-scheduled-query-pack.conf
    Create File  ${SOPHOS_INSTALL}/plugins/edr/etc/osquery.conf.d/sophos-scheduled-query-pack.conf  {"schedule": {"extracomma": {"query": "select * from uptime;","interval": 1},}}
    Run Shell Process  ${SOPHOS_INSTALL}/bin/wdctl stop edr   OnError=failed to stop edr
    Run Shell Process  ${SOPHOS_INSTALL}/bin/wdctl start edr   OnError=failed to stop edr

    Enable XDR

    Wait Until Keyword Succeeds
    ...  15 secs
    ...  1 secs
    ...  EDR Plugin Log Contains  failed to parse config, of source: ${SOPHOS_INSTALL}/plugins/edr/etc/osquery.conf.d/sophos-scheduled-query-pack.conf

EDR Plugin Detects Data Limit From Policy And That A Status Is Sent On Start
    [Setup]  No Operation
    Install Base For Component Tests
    Create Debug Level Logger Config File
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
    Create Debug Level Logger Config File
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
    Create Debug Level Logger Config File
    Move File Atomically  ${EXAMPLE_DATA_PATH}/LiveQuery_policy_10000_limit.xml  /opt/sophos-spl/base/mcs/policy/LiveQuery_policy.xml
    Install EDR Directly from SDDS

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
    Create Debug Level Logger Config File
    Move File Atomically  ${EXAMPLE_DATA_PATH}/LiveQuery_policy_10000_limit.xml  /opt/sophos-spl/base/mcs/policy/LiveQuery_policy.xml
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

    Wait For LiveQuery Status To Contain  <dailyDataLimitExceeded>true</dailyDataLimitExceeded>

EDR Plugin Rolls ScheduleEpoch Over When The Previous One Has Elapsed
    [Setup]  No Operation
    Install Base For Component Tests
    Create Debug Level Logger Config File
    Install EDR Directly from SDDS
    Enable XDR
    ${oldScheduleEpochTimestamp} =  Set Variable  1600000000
    Run Shell Process  ${SOPHOS_INSTALL}/bin/wdctl stop edr   OnError=failed to stop edr
    Create File  ${SOPHOS_INSTALL}/plugins/edr/var/persist-xdrScheduleEpoch  ${oldScheduleEpochTimestamp}
    Run Shell Process  ${SOPHOS_INSTALL}/bin/wdctl start edr   OnError=failed to start edr

    Wait Until Keyword Succeeds
    ...  15 secs
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
    Create Debug Level Logger Config File
    Install EDR Directly from SDDS
    Enable XDR

    Run Shell Process  ${SOPHOS_INSTALL}/bin/wdctl stop edr   OnError=failed to stop edr
    Create File  ${SOPHOS_INSTALL}/plugins/edr/var/persist-xdrScheduleEpoch  ${currentEpochTimeMinus3Days.__str__()}
    Run Shell Process  ${SOPHOS_INSTALL}/bin/wdctl start edr   OnError=failed to start edr

    Wait Until Keyword Succeeds
    ...  15 secs
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
    Create Debug Level Logger Config File
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
    [Setup]  Install With Base SDDS And Marked Next And Latest Files
    Check EDR Plugin Installed With Base
    Run Keyword And Ignore Error  Remove File  ${SOPHOS_INSTALL}/base/etc/logger.conf
    Create Debug Level Logger Config File
    Restart EDR

    Directory Should Be Empty  ${SOPHOS_INSTALL}/base/mcs/datafeed

    Enable XDR with MTR
    ${mark} =  Mark File  ${SOPHOS_INSTALL}/plugins/edr/log/scheduledquery.log

    Wait Until Keyword Succeeds
    ...  30 secs
    ...  5 secs
    ...  Marked File Contains  ${SOPHOS_INSTALL}/plugins/edr/log/scheduledquery.log   latest_xdr_query  ${mark}
    Wait Until Keyword Succeeds
    ...  5 secs
    ...  1 secs
    ...  Marked File Contains  ${SOPHOS_INSTALL}/plugins/edr/log/scheduledquery.log   latest_mtr_query  ${mark}
    sleep  4s
    File Log Does Not Contain  ${SOPHOS_INSTALL}/plugins/edr/log/scheduledquery.log   next_xdr_query
    File Log Does Not Contain  ${SOPHOS_INSTALL}/plugins/edr/log/scheduledquery.log   next_mtr_query

    ${edrMark} =  Mark File  ${SOPHOS_INSTALL}/plugins/edr/log/edr.log
    Change Next Query Packs Flag  true
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
    Change Next Query Packs Flag  false
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

EDR Plugin Hits Data Limit And Queries Resume After Period
    [Setup]  Uninstall All
    [Timeout]  600

    Install Base For Component Tests
    Create Debug Level Logger Config File
    Move File Atomically  ${EXAMPLE_DATA_PATH}/LiveQuery_policy_10000_limit_with_MTR.xml  /opt/sophos-spl/base/mcs/policy/LiveQuery_policy.xml

    # Create specific EDR dirs to drop in files before running installer.
    Should Not Exist  ${SOPHOS_INSTALL}/plugins/edr
    Create Directory  ${SOPHOS_INSTALL}/plugins/edr/etc/osquery.conf.d/

    # These are not included in the build output of EDR, they are supplements so must be manually copied into place
    Should Not Exist  ${SOPHOS_INSTALL}/plugins/edr/etc/osquery.conf.d/sophos-scheduled-query-pack.conf
    Should Not Exist  ${SOPHOS_INSTALL}/plugins/edr/etc/osquery.conf.d/sophos-scheduled-query-pack.conf.DISABLED

    # Inject query pack and initial data limit period into SDDS
    Create File  ${EDR_SDDS}/files/plugins/edr/var/persist-xdrPeriodInSeconds   120

    Install EDR Directly from SDDS

    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  EDR Plugin Log Contains  First LiveQuery policy received
    Expect New Datalimit  10000

    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  XDR Pack Should Be Enabled
    MTR Pack Should Be Enabled
    Custom Pack Should Be Enabled

    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  EDR Plugin Log Contains X Times  Run osquery process  2

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
    ...  60 secs
    ...  1 secs
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
    EDR Plugin Log Contains X Times  Restarting osquery, reason: LiveQuery policy has changed. Restarting osquery to apply changes  1

    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  EDR Plugin Log Contains X Times  Stopping LoggerExtension  2

    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  EDR Plugin Log Contains X Times  LoggerExtension::Stopped  2

    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  EDR Plugin Log Contains X Times  Stopping SophosExtension  2

    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  EDR Plugin Log Contains X Times  SophosExtension::Stopped  2

    # Osquery restart from query pack disabling due to data limit
    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  EDR Plugin Log Contains X Times  Run osquery process  3

    # Prove that scheduled queries are not being executed
    ${empty_marker} =  Set Variable    empty-marker
    Create File  ${SOPHOS_INSTALL}/plugins/edr/log/scheduledquery.log  ${empty_marker}
    # Wait for the possibility of queries to be running, in this time many queriers would have been executed.
    Sleep  20
    File Should Contain Only  ${SOPHOS_INSTALL}/plugins/edr/log/scheduledquery.log  ${empty_marker}

    # Wait until the 2 mins data limit period rolls over
    Wait Until Keyword Succeeds
    ...  200 secs
    ...  5 secs
    ...  EDR Plugin Log Contains  XDR period has rolled over

    # Osquery has started up for the final time.
    Wait Until Keyword Succeeds
    ...  60 secs
    ...  1 secs
    ...  EDR Plugin Log Contains X Times  Run osquery process  4

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
    Create File  ${SOPHOS_INSTALL}/plugins/edr/log/scheduledquery.log  ${empty_marker}
    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  File Should Not Contain Only  ${SOPHOS_INSTALL}/plugins/edr/log/scheduledquery.log  ${empty_marker}

    File Should Contain  ${SOPHOS_INSTALL}/plugins/edr/log/scheduledquery.log  Executing query

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
    [Setup]  No Operation
    Install Base For Component Tests
    Create Debug Level Logger Config File
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

    Apply Live Query Policy And Wait For Query Pack Changes  ${EXAMPLE_DATA_PATH}/LiveQuery_policy_10000_limit_and_different_custom_queries_with_xdr_only.xml

    XDR Pack Should Be Disabled
    MTR Pack Should Be Disabled
    Custom Pack Should Be Disabled

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
    Create File  ${SOPHOS_INSTALL}/plugins/edr/etc/query_packs/sophos-scheduled-query-pack.conf  {"schedule": {"latest_xdr_query": {"query": "select * from uptime;","interval": 2}}}
    Create File  ${SOPHOS_INSTALL}/plugins/edr/etc/query_packs/sophos-scheduled-query-pack.mtr.conf  {"schedule": {"latest_mtr_query": {"query": "select * from uptime;","interval": 2}}}
    Create File  ${SOPHOS_INSTALL}/plugins/edr/etc/query_packs/sophos-scheduled-query-pack-next.conf  {"schedule": {"next_xdr_query": {"query": "select * from uptime;","interval": 2}}}
    Create File  ${SOPHOS_INSTALL}/plugins/edr/etc/query_packs/sophos-scheduled-query-pack-next.mtr.conf  {"schedule": {"next_mtr_query": {"query": "select * from uptime;","interval": 2}}}
    Directory Should Be Empty  ${SOPHOS_INSTALL}/base/mcs/datafeed
    Restart EDR
    Enable XDR with MTR
    Change Next Query Packs Flag  true

    Wait Until Keyword Succeeds
    ...  30 secs
    ...  5 secs
    ...  EDR Plugin Log Contains  Overwriting existing scheduled query packs with 'NEXT' query packs
    ${mark} =  Mark File  ${SOPHOS_INSTALL}/plugins/edr/log/scheduledquery.log
    Are Next Query Packs Enabled in Plugin Conf  1

    Wait Until Keyword Succeeds
    ...  20 secs
    ...  2 secs
    ...  Marked File Contains  ${SOPHOS_INSTALL}/plugins/edr/log/scheduledquery.log   next_xdr_query  ${mark}
    Wait Until Keyword Succeeds
    ...  5 secs
    ...  1 secs
    ...  Marked File Contains  ${SOPHOS_INSTALL}/plugins/edr/log/scheduledquery.log   next_mtr_query  ${mark}
    sleep  4s
    Marked File Does Not Contain  ${SOPHOS_INSTALL}/plugins/edr/log/scheduledquery.log   latest_xdr_query  ${mark}
    Marked File Does Not Contain  ${SOPHOS_INSTALL}/plugins/edr/log/scheduledquery.log   latest_mtr_query  ${mark}

    ${edrMark} =  Mark File  ${SOPHOS_INSTALL}/plugins/edr/log/edr.log

    # Mimic update of the next query pack supplement
    Create File  ${SOPHOS_INSTALL}/plugins/edr/etc/query_packs/sophos-scheduled-query-pack-next.conf  {"schedule": {"next_updated_xdr_query": {"query": "select * from uptime;","interval": 2}}}
    Create File  ${SOPHOS_INSTALL}/plugins/edr/etc/query_packs/sophos-scheduled-query-pack-next.mtr.conf  {"schedule": {"next_updated_mtr_query": {"query": "select * from uptime;","interval": 2}}}
    Restart EDR

    Wait Until Keyword Succeeds
    ...  30 secs
    ...  5 secs
    ...  Marked File Contains  ${SOPHOS_INSTALL}/plugins/edr/log/edr.log  Overwriting existing scheduled query packs with 'NEXT' query packs  ${edrMark}
    ${mark2} =  Mark File  ${SOPHOS_INSTALL}/plugins/edr/log/scheduledquery.log
    Are Next Query Packs Enabled in Plugin Conf  1

    Wait Until Keyword Succeeds
    ...  20 secs
    ...  2 secs
    ...  Marked File Contains  ${SOPHOS_INSTALL}/plugins/edr/log/scheduledquery.log   next_updated_xdr_query  ${mark2}
    Wait Until Keyword Succeeds
    ...  5 secs
    ...  1 secs
    ...  Marked File Contains  ${SOPHOS_INSTALL}/plugins/edr/log/scheduledquery.log   next_updated_mtr_query  ${mark2}
    sleep  4s
    Marked File Does Not Contain  ${SOPHOS_INSTALL}/plugins/edr/log/scheduledquery.log   next_xdr_query  ${mark2}
    Marked File Does Not Contain  ${SOPHOS_INSTALL}/plugins/edr/log/scheduledquery.log   next_mtr_query  ${mark2}
    Marked File Does Not Contain  ${SOPHOS_INSTALL}/plugins/edr/log/scheduledquery.log   latest_xdr_query  ${mark2}
    Marked File Does Not Contain  ${SOPHOS_INSTALL}/plugins/edr/log/scheduledquery.log   latest_mtr_query  ${mark2}

*** Keywords ***
Apply Live Query Policy And Wait For Query Pack Changes
    [Arguments]  ${policy}
    ${mark} =  Mark File  ${SOPHOS_INSTALL}/plugins/edr/log/edr.log
    Move File Atomically  ${policy}  /opt/sophos-spl/base/mcs/policy/LiveQuery_policy.xml
    Wait Until Keyword Succeeds
    ...  10s
    ...  2s
    ...  Marked File Contains  ${SOPHOS_INSTALL}/plugins/edr/log/edr.log  LiveQuery policy has changed. Restarting osquery to apply changes  ${mark}

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

Change Next Query Packs Flag
    [Arguments]  ${flagValue}=false
    Create File  ${SOPHOS_INSTALL}/base/mcs/tmp/flags.json  {"scheduled_queries.next": ${flagValue}}
    ${result} =  Run Process  chown  sophos-spl-local:sophos-spl-group  ${SOPHOS_INSTALL}/base/mcs/tmp/flags.json
    Should Be Equal As Strings  0  ${result.rc}
    Move File  ${SOPHOS_INSTALL}/base/mcs/tmp/flags.json  ${SOPHOS_INSTALL}/base/mcs/policy/flags.json

Enable XDR
    Move File Atomically  ${EXAMPLE_DATA_PATH}/LiveQuery_policy_enabled.xml  /opt/sophos-spl/base/mcs/policy/LiveQuery_policy.xml
    Is XDR Enabled in Plugin Conf

Enable XDR with MTR
    Move File Atomically  ${EXAMPLE_DATA_PATH}/LiveQuery_policy_enabled_with_MTR.xml  /opt/sophos-spl/base/mcs/policy/LiveQuery_policy.xml
    Is XDR Enabled in Plugin Conf

Is XDR Enabled in Plugin Conf
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  1 secs
    ...  File Should Exist    ${SOPHOS_INSTALL}/plugins/edr/etc/plugin.conf
    Wait Until Keyword Succeeds
    ...  60 secs
    ...  5 secs
    ...  EDR Plugin Log Contains   Updating running_mode flag setting
    # race condition here between the above log and the file being written
    sleep  0.1s
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
    ...  EDR Plugin Log Contains   Updating running_mode flag setting
    ${EDR_CONFIG_CONTENT}=  Get File  ${SOPHOS_INSTALL}/plugins/edr/etc/plugin.conf
    Should Contain  ${EDR_CONFIG_CONTENT}   scheduled_queries_next=${settingValue}

Add Uptime Query to Scheduled Queries
    Create File  ${SOPHOS_INSTALL}/plugins/edr/etc/query_packs/sophos-scheduled-query-pack.conf  {"schedule": {"uptime": {"query": "select * from uptime;","interval": 1}}}
    Create File  ${SOPHOS_INSTALL}/plugins/edr/etc/osquery.conf.d/sophos-scheduled-query-pack.conf.DISABLED  {"schedule": {"uptime": {"query": "select * from uptime;","interval": 1}}}

Test Teardown
    EDR And Base Teardown
    Uninstall All

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

Create Debug Level Logger Config File
    Create File  ${SOPHOS_INSTALL}/base/etc/logger.conf  [global]\nVERBOSITY = DEBUG\n