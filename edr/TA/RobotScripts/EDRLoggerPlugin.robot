*** Settings ***
Documentation    Testing the Logger Plugin for XDR Behaviour

Library         Process
Library         OperatingSystem
Library         ../Libs/XDRLibs.py

Resource        EDRResources.robot
Resource        ComponentSetup.robot

Suite Setup     No Operation
Suite Teardown  No Operation

Test Setup      Install With Base SDDS
Test Teardown   Test Teardown

*** Test Cases ***
EDR Plugin outputs XDR results and Its Answer is available to MCSRouter
    [Setup]  Install With Base SDDS
    Check EDR Plugin Installed With Base
    Add Uptime Query to Scheduled Queries
    Directory Should Be Empty  ${SOPHOS_INSTALL}/base/mcs/datafeed
    Enable XDR
    Wait Until Keyword Succeeds
    ...  100 secs
    ...  5 secs
    ...  Directory Should Not Be Empty  ${SOPHOS_INSTALL}/base/mcs/datafeed

EDR Plugin Runs All Scheduled Queries
    Check EDR Plugin Installed With Base
    Run Keyword And Ignore Error  Remove File  ${SOPHOS_INSTALL}/base/etc/logger.conf
    Create File  ${SOPHOS_INSTALL}/base/etc/logger.conf  [global]\nVERBOSITY = DEBUG\n
    Directory Should Be Empty  ${SOPHOS_INSTALL}/base/mcs/datafeed
    Copy File  ${TEST_INPUT_PATH}/qp/sophos-scheduled-query-pack.conf  ${SOPHOS_INSTALL}/plugins/edr/etc/osquery.conf.d/sophos-scheduled-query-pack.conf
    Copy File  ${TEST_INPUT_PATH}/qp/sophos-scheduled-query-pack.conf  ${SOPHOS_INSTALL}/plugins/edr/etc/osquery.conf.d/sophos-scheduled-query-pack.conf.DISABLED
    change_all_scheduled_queries_interval  ${SOPHOS_INSTALL}/plugins/edr/etc/osquery.conf.d/sophos-scheduled-query-pack.conf  10
    change_all_scheduled_queries_interval  ${SOPHOS_INSTALL}/plugins/edr/etc/osquery.conf.d/sophos-scheduled-query-pack.conf.DISABLED  10
    Enable XDR

    #restart edr so that the altered queries are read in and debug mode applied
    Restart EDR

    Wait Until Keyword Succeeds
    ...  200 secs
    ...  10 secs
    ...  Check All Queries Run  ${SOPHOS_INSTALL}/plugins/edr/log/edr.log  ${SOPHOS_INSTALL}/plugins/edr/etc/osquery.conf.d/sophos-scheduled-query-pack.conf

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
    Copy File  ${TEST_INPUT_PATH}/qp/sophos-scheduled-query-pack.conf  ${SOPHOS_INSTALL}/plugins/edr/etc/osquery.conf.d/sophos-scheduled-query-pack.conf
    Copy File  ${TEST_INPUT_PATH}/qp/sophos-scheduled-query-pack.conf  ${SOPHOS_INSTALL}/plugins/edr/etc/osquery.conf.d/sophos-scheduled-query-pack.conf.DISABLED
    change_all_scheduled_queries_interval  ${SOPHOS_INSTALL}/plugins/edr/etc/osquery.conf.d/sophos-scheduled-query-pack.conf  10
    change_all_scheduled_queries_interval  ${SOPHOS_INSTALL}/plugins/edr/etc/osquery.conf.d/sophos-scheduled-query-pack.conf.DISABLED  10
    Enable XDR

    # Restart edr so that the altered queries are read in and debug mode applied
    Restart EDR

    Wait Until Keyword Succeeds
    ...  20 secs
    ...  10 secs
    ...  EDR Plugin Log Contains   XDR data limit for this period exceeded

    Wait For LiveQuery Status To Contain  <dailyDataLimitExceeded>true</dailyDataLimitExceeded>


*** Keywords ***
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

Enable XDR
    Create File  ${SOPHOS_INSTALL}/base/mcs/tmp/flags.json  {"xdr.enabled": true}
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
    ...  EDR Plugin Log Contains   Flags running mode is XDR
    ${EDR_CONFIG_CONTENT}=  Get File  ${SOPHOS_INSTALL}/plugins/edr/etc/plugin.conf
    Should Contain  ${EDR_CONFIG_CONTENT}   running_mode=0

Add Uptime Query to Scheduled Queries
    Run Keyword And Ignore Error  Remove File  ${SOPHOS_INSTALL}/plugins/edr/etc/osquery.conf.d/sophos-scheduled-query-pack.conf
    Run Keyword And Ignore Error  Remove File  ${SOPHOS_INSTALL}/plugins/edr/etc/osquery.conf.d/sophos-scheduled-query-pack.conf.DISABLED
    Should Not Exist  ${SOPHOS_INSTALL}/plugins/edr/etc/osquery.conf.d/sophos-scheduled-query-pack.conf
    Should Not Exist  ${SOPHOS_INSTALL}/plugins/edr/etc/osquery.conf.d/sophos-scheduled-query-pack.conf.DISABLED
    Create File  ${SOPHOS_INSTALL}/plugins/edr/etc/osquery.conf.d/sophos-scheduled-query-pack.conf  {"schedule": {"uptime": {"query": "select * from uptime;","interval": 1}}}
    Create File  ${SOPHOS_INSTALL}/plugins/edr/etc/osquery.conf.d/sophos-scheduled-query-pack.conf.DISABLED  {"schedule": {"uptime": {"query": "select * from uptime;","interval": 1}}}

Test Teardown
    EDR And Base Teardown
    Uninstall And Revert Setup