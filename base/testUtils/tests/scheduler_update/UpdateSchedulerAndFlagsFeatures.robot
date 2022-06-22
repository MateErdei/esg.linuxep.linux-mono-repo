*** Settings ***
Test Setup      Setup Current Update Scheduler Environment
Test Teardown   Teardown Servers For Update Scheduler

Resource  SchedulerUpdateResources.robot

Default Tags   SULDOWNLOADER  UPDATE_SCHEDULER
Force Tags  LOAD6

*** Test Cases ***
UpdateScheduler Will Not Trigger SULDownloader Until Flags Policy is Read
    [Setup]    Setup Current Update Scheduler Environment Without Flags
    Create Empty Config File To Stop First Update On First Policy Received
    Set Log Level For Component And Reset and Return Previous Log  sophos_managementagent   DEBUG
    Set Log Level For Component And Reset and Return Previous Log  updatescheduler   DEBUG
    Setup Plugin Install Failed

    Wait Until Keyword Succeeds
    ...  30 secs
    ...  1 secs
    ...  Check Log Contains    No policy available right now for app: FLAGS    ${SOPHOS_INSTALL}/logs/base/sophosspl/updatescheduler.log    updatescheduler.log

    Send Mock Flags Policy
    Send Policy With Cache

    Wait Until Keyword Succeeds
    ...  30 secs
    ...  1 secs
    ...  Check Log Contains String N Times   ${SOPHOS_INSTALL}/logs/base/sophosspl/updatescheduler.log   updatescheduler.log   First FLAGS policy received.  1

    Check Log Contains    Received sdds3.enabled flag value: 1    ${SOPHOS_INSTALL}/logs/base/sophosspl/updatescheduler.log    updatescheduler.log

    Setup Base and Plugin Sync and UpToDate
    Create File   ${UPGRADING_MARKER_FILE}
    Run process   chown sophos-spl-updatescheduler:sophos-spl-group ${UPGRADING_MARKER_FILE}  shell=yes
    Run process   chmod a+w ${UPGRADING_MARKER_FILE}  shell=yes

    Simulate Update Now
    ${eventPath} =  Check Status and Events Are Created
    Check Event Report Success  ${eventPath}

SDDS3 Enabled Flag is Read By UpdateScheduler
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  1 secs
    ...  Check Log Contains String N Times   ${SOPHOS_INSTALL}/logs/base/sophosspl/updatescheduler.log   updatescheduler.log   First FLAGS policy received.  1

    Wait Until Keyword Succeeds
    ...  30 secs
    ...  1 secs
    ...  Check Log Contains    Received sdds3.enabled flag value: 1    ${SOPHOS_INSTALL}/logs/base/sophosspl/updatescheduler.log    updatescheduler.log

    Send Mock Flags Policy    {"livequery.network-tables.available": true, "scheduled_queries.next": true, "sdds3.enabled": false}
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  1 secs
    ...  Check Log Contains    Received sdds3.enabled flag value: 0    ${SOPHOS_INSTALL}/logs/base/sophosspl/updatescheduler.log    updatescheduler.log