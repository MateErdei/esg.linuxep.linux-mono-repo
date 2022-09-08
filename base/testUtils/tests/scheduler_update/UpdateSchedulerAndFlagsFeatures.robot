*** Settings ***
Test Setup      Setup Current Update Scheduler Environment
Test Teardown   Teardown Servers For Update Scheduler

Resource  SchedulerUpdateResources.robot

Default Tags   SULDOWNLOADER  UPDATE_SCHEDULER
Force Tags  LOAD6

*** Test Cases ***
useSDDS3 is False by Default When Flags Policy is Missing
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  1 secs
    ...  Check UpdateScheduler Log Contains In Order
         ...  ${SOPHOS_INSTALL}/base/etc/sophosspl/flags-mcs.json does not exist, performing a short wait for Central flags
         ...  FLAGS policy has not been sent to the plugin
         ...  Return from waitForPolicy
         ...  Processing Flags
         ...  Flags:

    Check Update Config Contains Expected useSDDS3 Value    false
    Check UpdateScheduler Log Does Not Contain    Received sdds3.enabled flag value:

useSDDS3 is False by When SDDS3 Enabled Flag Is Not Present in Flags Policy
    Send Mock Flags Policy    {"livequery.network-tables.available": true, "scheduled_queries.next": true}
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  1 secs
    ...  Check UpdateScheduler Log Contains    Flags: {"livequery.network-tables.available": true, "scheduled_queries.next": true}

    Check Update Config Contains Expected useSDDS3 Value    false

UpdateScheduler Will Not Trigger SULDownloader Until Flags Policy is Read
    Setup Plugin Install Failed
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  1 secs
    ...  Check UpdateScheduler Log Contains    ${SOPHOS_INSTALL}/base/etc/sophosspl/flags-mcs.json does not exist, performing a short wait for Central flags
    Check Update Config Contains Expected useSDDS3 Value    false

    Send Mock Flags Policy    {"livequery.network-tables.available": true, "scheduled_queries.next": true, "sdds3.enabled": true}
    Check UpdateScheduler Log Contains    Received sdds3.enabled flag value: 1
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
    ...  Check UpdateScheduler Log Contains    Received sdds3.enabled flag value: 0
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  10 secs
    ...  Check Update Config Contains Expected useSDDS3 Value    false

    Send Mock Flags Policy    {"livequery.network-tables.available": true, "scheduled_queries.next": true, "sdds3.enabled": true}
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  1 secs
    ...  Check UpdateScheduler Log Contains    Received sdds3.enabled flag value: 1
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  10 secs
    ...  Check Update Config Contains Expected useSDDS3 Value    true
