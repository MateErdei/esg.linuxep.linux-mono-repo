*** Settings ***
Documentation    Testing the Logger Plugin for XDR Behaviour

Library         Process
Library         OperatingSystem

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

EDR Plugin Detects Data Limit From Policy
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


*** Keywords ***
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