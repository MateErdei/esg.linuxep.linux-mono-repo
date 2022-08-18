*** Settings ***
Test Setup      Setup Current Update Scheduler Environment
Test Teardown   Teardown Servers For Update Scheduler

Resource  SchedulerUpdateResources.robot

Default Tags   SULDOWNLOADER  UPDATE_SCHEDULER
Force Tags  LOAD6

*** Test Cases ***
SDDS3 Enabled Flag is Set to False by Default When Not Present in Flags Json
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  1 secs
    ...  Check Log Contains    ${SOPHOS_INSTALL}/base/etc/sophosspl/flags-mcs.json does not exist, performing a short wait for Central flags    ${SOPHOS_INSTALL}/logs/base/sophosspl/updatescheduler.log    updatescheduler.log
    Check Update Config Contains Expected useSDDS3 Value    false

    Send Mock Flags Policy    {"livequery.network-tables.available": true, "scheduled_queries.next": true}
    Wait Until Keyword Succeeds
        ...  30 secs
        ...  1 secs
        ...  Check Log Contains    Flags: {"livequery.network-tables.available": true, "scheduled_queries.next": true}    ${SOPHOS_INSTALL}/logs/base/sophosspl/updatescheduler.log    updatescheduler.log
    Check Update Config Contains Expected useSDDS3 Value    false

UpdateScheduler Will Not Trigger SULDownloader Until Flags Policy is Read
    Setup Plugin Install Failed
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  1 secs
    ...  Check Log Contains    ${SOPHOS_INSTALL}/base/etc/sophosspl/flags-mcs.json does not exist, performing a short wait for Central flags    ${SOPHOS_INSTALL}/logs/base/sophosspl/updatescheduler.log    updatescheduler.log
    Check Update Config Contains Expected useSDDS3 Value    false

    Send Mock Flags Policy    {"livequery.network-tables.available": true, "scheduled_queries.next": true, "sdds3.enabled": true}
    Check Log Contains    Received sdds3.enabled flag value: 1    ${SOPHOS_INSTALL}/logs/base/sophosspl/updatescheduler.log    updatescheduler.log
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  10 secs
    ...  Check Update Config Contains Expected useSDDS3 Value    true

    @{features}=  Create List   CORE
    Setup Base and Plugin Sync and UpToDate  ${features}
    Create File   ${UPGRADING_MARKER_FILE}
    Run process   chown sophos-spl-updatescheduler:sophos-spl-group ${UPGRADING_MARKER_FILE}  shell=yes
    Run process   chmod a+w ${UPGRADING_MARKER_FILE}  shell=yes

    Simulate Update Now
    ${eventPath} =  Check Status and Events Are Created
    Check Event Report Success  ${eventPath}

SDDS3 Enabled Flag is Read By UpdateScheduler
    Send Mock Flags Policy    {"livequery.network-tables.available": true, "scheduled_queries.next": true, "sdds3.enabled": false}
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  1 secs
    ...  Check Log Contains    Received sdds3.enabled flag value: 0    ${SOPHOS_INSTALL}/logs/base/sophosspl/updatescheduler.log    updatescheduler.log
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  10 secs
    ...  Check Update Config Contains Expected useSDDS3 Value    false

    Send Mock Flags Policy    {"livequery.network-tables.available": true, "scheduled_queries.next": true, "sdds3.enabled": true}
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  1 secs
    ...  Check Log Contains    Received sdds3.enabled flag value: 1    ${SOPHOS_INSTALL}/logs/base/sophosspl/updatescheduler.log    updatescheduler.log
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  10 secs
    ...  Check Update Config Contains Expected useSDDS3 Value    true

*** Keywords ***
Check Update Config Contains Expected useSDDS3 Value
    [Arguments]   ${expectedValue}
    ${updateConfigJson} =  Get File  ${UPDATE_CONFIG}
    Log File   ${UPDATE_CONFIG}
    Should Contain  ${updateConfigJson}   useSDDS3": ${expectedValue}