*** Settings ***
Documentation    Verify the UpdateScheduler Plugin using the FakeSulDownloader (replace SulDownloader with a link to the FakeSulDownloader script) to allow verifying many different scenarios without running the real SulDownloader

Test Setup      Setup Current Update Scheduler Environment

Test Teardown   Teardown For Test

Library    Process
Library    OperatingSystem
Library    DateTime
Library    ${LIBS_DIRECTORY}/LogUtils.py
Library    ${LIBS_DIRECTORY}/FakeSulDownloader.py
Library    ${LIBS_DIRECTORY}/UpdateSchedulerHelper.py
Library    ${LIBS_DIRECTORY}/MCSRouter.py
Library    ${LIBS_DIRECTORY}/TelemetryUtils.py

Resource  ../watchdog/LogControlResources.robot
Resource  ../installer/InstallerResources.robot
Resource  ../telemetry/TelemetryResources.robot
Resource  SchedulerUpdateResources.robot
Resource  ../GeneralTeardownResource.robot

Default Tags  UPDATE_SCHEDULER


*** Variables ***
${TELEMETRY_SUCCESS}    0
${TELEMETRY_JSON_FILE}     ${SOPHOS_INSTALL}/base/telemetry/var/telemetry.json

*** Test Cases ***
UpdateScheduler SulDownloader Report Sync With Warehouse Success
    [Tags]  SMOKE  UPDATE_SCHEDULER  TAP_TESTS
    [Documentation]  Demonstrate that Events and Status will be generated during on the first run of Update Scheduler
    Setup Base and Plugin Sync and UpToDate
    Simulate Update Now
    ${eventPath} =  Check Status and Events Are Created
    Check Event Report Success  ${eventPath}

    ${statusContent}        Get File   ${statusPath}
    ${machineID}            Get File   ${MACHINE_ID_FILE}
    Should Contain  ${statusContent}  endpoint id="${machineID}"
    Check Event Source  ${eventPath}  <updateSource>Sophos</updateSource>

    ${time} =  Get Current Time
    Prepare To Run Telemetry Executable
    Run Telemetry Executable  ${EXE_CONFIG_FILE}  ${TELEMETRY_SUCCESS}
    ${telemetryFileContents} =  Get File  ${TELEMETRY_OUTPUT_JSON}
    Check Update Scheduler Telemetry Json Is Correct  ${telemetryFileContents}  0  True  ${time}  sddsid=regruser
    Cleanup Telemetry Server
    Check Update Sheduler Run as Sophos-spl-user


UpdateScheduler Regenerates The Config File If It Does Not Exist
    [Documentation]  Demonstrate that Events and Status will be generated during on the first run of Update Scheduler
    Setup Base and Plugin Sync and UpToDate
    Remove File  ${sulConfigPath}
    Simulate Update Now
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  2 secs
    ...  File Should Exist  ${sulConfigPath}
    ${eventPath} =  Check Status and Events Are Created
    Check Event Report Success  ${eventPath}

UpdateScheduler Second Run with Normal Run will not generate new Event
    Setup Base and Plugin Sync and UpToDate
    Simulate Update Now
    ${eventPath} =  Check Status and Events Are Created
    Check Event Report Success  ${eventPath}
    Log  ${eventPath}
    Log File  ${eventPath}
    ${reportPath} =  Get latest report path
    Log  ${reportPath}
    Log File  ${reportPath}
    Setup Base and Plugin Sync and UpToDate  startTime=2
    Simulate Update Now
    Check No New Event Created  ${eventPath}

    # first report is to be removed as it does not add relevant information
    File Should not Exist  ${reportPath}


UpdateScheduler Third Run with Upgrade will generate event
    Setup Base and Plugin Sync and UpToDate
    Simulate Update Now
    ${eventPath} =  Check Status and Events Are Created
    Check Event Report Success  ${eventPath}

    ${reportPath} =  Get latest report path

    Setup Base and Plugin Sync and UpToDate  startTime=2
    Simulate Update Now
    Check No New Event Created  ${eventPath}
    # first report is to be removed as it does not add relevant information
    File Should not Exist  ${reportPath}

    Setup Base and Plugin Upgraded  startTime=3  syncTime=3
    Simulate Update Now
    ${eventPath} =  Check Status and Events Are Created
    Check Event Report Success  ${eventPath}


UpdateScheduler Report Failure to Update
    Setup Plugin Install Failed
    Simulate Update Now
    ${eventPath} =  Check Status and Events Are Created
    Check Event Report Install Failed   ${eventPath}

    ${time} =  Get Current Time
    Prepare To Run Telemetry Executable
    Run Telemetry Executable  ${EXE_CONFIG_FILE}  ${TELEMETRY_SUCCESS}
    ${telemetryFileContents} =  Get File  ${TELEMETRY_JSON_FILE}
    Check Update Scheduler Telemetry Json Is Correct  ${telemetryFileContents}  1  False  sddsid=regruser
    Cleanup Telemetry Server


UpdateScheduler Report Failure to Update Multiple Times In Telemetry
    Setup Plugin Install Failed
    Simulate Update Now
    ${eventPath} =  Check Status and Events Are Created
    Check Event Report Install Failed   ${eventPath}

    Setup Plugin Install Failed  startTime=3  syncTime=3
    Simulate Update Now

    ${time} =  Get Current Time
    Prepare To Run Telemetry Executable
    Wait Until Keyword Succeeds
        ...  5 secs
        ...  1 secs
        ...  Check Log Contains String N Times   ${SOPHOS_INSTALL}/logs/base/sophosspl/updatescheduler.log   Update Scheduler Log   Sending status to Central   2
    Run Telemetry Executable  ${EXE_CONFIG_FILE}  ${TELEMETRY_SUCCESS}
    ${telemetryFileContents} =  Get File  ${TELEMETRY_JSON_FILE}
    Check Update Scheduler Telemetry Json Is Correct  ${telemetryFileContents}  2  False  sddsid=regruser
    Cleanup Telemetry Server

    # Failed count should have been reset.

    Setup Plugin Install Failed  startTime=4  syncTime=4
    Simulate Update Now

    ${time} =  Get Current Time
    Prepare To Run Telemetry Executable
    Wait Until Keyword Succeeds
        ...  5 secs
        ...  1 secs
        ...  Check Log Contains String N Times   ${SOPHOS_INSTALL}/logs/base/sophosspl/updatescheduler.log   Update Scheduler Log   Sending status to Central   3
    Run Telemetry Executable  ${EXE_CONFIG_FILE}  ${TELEMETRY_SUCCESS}
    ${telemetryFileContents} =  Get File  ${TELEMETRY_JSON_FILE}
    Check Update Scheduler Telemetry Json Is Correct  ${telemetryFileContents}  1  False  sddsid=regruser
    Cleanup Telemetry Server


UpdateScheduler Start and Restart
    Setup Plugin Install Failed  startTime=2  syncTime=1
    Simulate Update Now
    ${eventPath} =  Check Status and Events Are Created
    Check Event Report Install Failed   ${eventPath}

    Remove File  ${eventPath}
    Remove File  ${statusPath}

    Restart Update Scheduler
    Send Policy With Cache
    Setup Base and Plugin Upgraded  startTime=3  syncTime=3
    Simulate Update Now
    ${eventPath} =  Check Event File Generated  10
    Check Event Report Success  ${eventPath}
    #All product information is the same so no new status generated
    File Should Not Exist  ${statusPath}


UpdateScheduler SulDownloader Report Sync With Warehouse Success Via Update Cache
    [Documentation]  Demonstrate that Events and Status will be generated during on the first run of Update Scheduler
    Setup Base Uptodate Via Update Cache  2k12-64-ld55-df.eng.sophos:8191
    Send Policy With Cache

    Simulate Update Now
    ${eventPath} =  Check Status and Events Are Created
    Check Event Report Success  ${eventPath}

    Check Event Source  ${eventPath}  <updateSource>4092822d-0925-4deb-9146-fbc8532f8c55</updateSource>


UpdateScheduler Periodically Run SulDownloader
    [Tags]  SLOW  UPDATE_SCHEDULER
    [Timeout]    35 minutes
    Set Log Level For Component And Reset and Return Previous Log  updatescheduler   DEBUG
    Setup Plugin Install Failed
    # status is to be created in start up - regardless of running update or not.
    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  File Should Exist  ${statusPath}
    #Remove noref status
    Remove File  ${statusPath}

    ${eventPath} =  Check Status and Events Are Created  waitTime=30 min  attemptsTime=2 min
    Check Event Report Install Failed   ${eventPath}

    Remove File  ${eventPath}
    #Remove status with the products in
    Remove File  ${statusPath}

    Setup Base and Plugin Upgraded  startTime=3  syncTime=3
    #Wait for up to 30 mins for the update

    ${eventPath} =  Check Event File Generated   1800
    Check Event Report Success  ${eventPath}
    #Products not changing so no new status generated
    File Should Not Exist  ${statusPath}

UpdateScheduler Sends A Default Status If Its Status Is Requested Before An Update Completes
    ${ErrorMessage} =  Set Variable  Failed to get plugin status for: updatescheduler, with errorStatus not set yet.
    ${RegistrationMessage} =  Set Variable  Plugin registration received for plugin: updatescheduler
    ${ManagementAgentRunning} =  Set Variable  Management Agent running
    ${ManagementAgentLog} =    Get File  /opt/sophos-spl/logs/base/sophosspl/sophos_managementagent.log
    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  Management Agent Contains  ${RegistrationMessage}
    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  Management Agent Contains  ${ManagementAgentRunning}
    Should Not Contain  ${ManagementAgentLog}  ${ErrorMessage}


UpdateScheduler Send Status After Receiving Policy
    [Documentation]  Verify LINUXEP-6534
    [Setup]  Setup Update Scheduler Environment and Stop All Services
    Simulate Send Policy  ALC_policy_direct.xml
    Remove File  ${statusPath}
    Create Fake Report
    Create Empty Config File To Stop First Update On First Policy Received
    Start Update Scheduler
    Start Management Agent Via WDCTL
    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  File Should Exist  ${statusPath}


UpdateScheduler Performs Update After Receiving Policy For The First Time
    [Setup]  Setup Current Update Scheduler Environment Without Policy
    Configure Hosts File
    Remove File  ${statusPath}
    Remove File  ${sulConfigPath}
    Start Update Scheduler
    Start Management Agent Via WDCTL
    Send Policy To UpdateScheduler  ALC_policy_direct.xml
    Check Status and Events Are Created
    ${UpdateSchedulerLog} =    Get File  /opt/sophos-spl/logs/base/sophosspl/updatescheduler.log
    Should Contain  ${UpdateSchedulerLog}  Attempting to update from warehouse


UpdateScheduler Schedules a Scheduled Update and Updates as Scheduled
    [Tags]  SLOW  UPDATE_SCHEDULER
    [Timeout]    20 minutes
    [Setup]  Setup Current Update Scheduler Environment Without Policy
    # There are three types of updates scheduled:
    # 1. Immediately after getting the first ALC policy - to install plugins
    # 2. 5-10 minutes after starting up - to ensure machines that have been switched off for a period get updated quickly
    # 3. Updates based off the update period (5 minutes for this policy)
    ${BasicPolicyXml} =  Get File  ${SUPPORT_FILES}/CentralXml/ALC_policy_scheduled_update.xml
    ${Date} =  Get Current Date
    ${ScheduledDate} =  Add Time To Date  ${Date}  11 minutes
    ${ScheduledDay} =  Convert Date  ${ScheduledDate}  result_format=%A
    ${ScheduledTime} =  Convert Date  ${ScheduledDate}  result_format=%H:%M:00
    ${NewPolicyXml} =  Replace String  ${BasicPolicyXml}  REPLACE_DAY  ${ScheduledDay}
    ${NewPolicyXml} =  Replace String  ${NewPolicyXml}  REPLACE_TIME  ${ScheduledTime}
    ${NewPolicyXml} =  Replace String  ${NewPolicyXml}  Frequency="40"  Frequency="5"
    Create File  ${SOPHOS_INSTALL}/tmp/ALC_policy_scheduled_update.xml  ${NewPolicyXml}
    Log File  ${SOPHOS_INSTALL}/tmp/ALC_policy_scheduled_update.xml
    Send Policy To UpdateScheduler  ${SOPHOS_INSTALL}/tmp/ALC_policy_scheduled_update.xml

    #Update on install
    ${eventPath} =  Check Event File Generated  wait_time_seconds=60
    Remove File  ${eventPath}
    ${reportPath} =  Get latest report path
    Log File   ${reportPath}
    Check report was a product update  ${reportPath}
    Remove File  ${reportPath}

    #Update after 5-10 minutes (boot storm avoiding update)
    ${eventPath} =  Check Event File Generated  wait_time_seconds=600
    Remove File  ${eventPath}
    ${reportPath} =  Get latest report path
    Check report was a product update  ${reportPath}
    Remove File  ${reportPath}

    #Scheduled update - 5 minutes after the previous update
    ${eventPath} =  Check Event File Generated  wait_time_seconds=360
    ${reportPath} =  Get latest report path
    Check report was a product update  ${reportPath}

    ${ActualUpdateDate} =  Get Current Date
    ${TimeDiff} =  Subtract Date From Date  ${ActualUpdateDate}  ${ScheduledDate}
    Run Keyword Unless  -60 < ${TimeDiff} < 360  Fail


UpdateScheduler Performs Update After Receiving Policy With Different Features
    [Setup]  Setup Current Update Scheduler Environment Without Policy
    Configure Hosts File
    Remove File  ${statusPath}
    Remove File  ${sulConfigPath}
    Start Update Scheduler
    Start Management Agent Via WDCTL
    Send Policy To UpdateScheduler  ALC_policy_direct_just_base.xml
    Check Status and Events Are Created
    ${UpdateSchedulerLog} =    Get File  /opt/sophos-spl/logs/base/sophosspl/updatescheduler.log
    Should Contain  ${UpdateSchedulerLog}  Attempting to update from warehouse

    # Now we know we are in a good state simulate previous update state for the test
    Copy File  ${SUPPORT_FILES}/update_config/previous_update_config_CORE_SENSORS_featues_and_base_only.json   ${SOPHOS_INSTALL}/base/update/var/previous_update_config.json
    Run Process  chown  sophos-spl-user:sophos-spl-group   ${SOPHOS_INSTALL}/base/update/var/previous_update_config.json

    # Ensure update will be invoked when previous config subscriptions differ from current, when feature set is the same.
    Send Policy To UpdateScheduler  ALC_policy_direct_with_sdu_feature.xml
    ${UpdateSchedulerLog} =    Get File  /opt/sophos-spl/logs/base/sophosspl/updatescheduler.log
    Should Contain  ${UpdateSchedulerLog}  Detected product configuration change, triggering update.


UpdateScheduler Performs Update After Receiving Policy With Different Subscriptions
    [Setup]  Setup Current Update Scheduler Environment Without Policy
    Configure Hosts File
    Remove File  ${statusPath}
    Remove File  ${sulConfigPath}
    Start Update Scheduler
    Start Management Agent Via WDCTL
    Send Policy To UpdateScheduler  ALC_policy_direct_just_base.xml
    Check Status and Events Are Created
    ${UpdateSchedulerLog} =    Get File  /opt/sophos-spl/logs/base/sophosspl/updatescheduler.log
    Should Contain  ${UpdateSchedulerLog}  Attempting to update from warehouse

    # Now we know we are in a good state simulate previous update state for the test
    Copy File  ${SUPPORT_FILES}/update_config/previous_update_config_base_subscription_only.json   ${SOPHOS_INSTALL}/base/update/var/previous_update_config.json
    Run Process  chown  sophos-spl-user:sophos-spl-group   ${SOPHOS_INSTALL}/base/update/var/previous_update_config.json

    # Ensure update will be invoked when previous config subscriptions differ from current, when feature set is the same.
    Send Policy To UpdateScheduler  ALC_policy_direct_base_and_example_plugin.xml
    ${UpdateSchedulerLog} =    Get File  /opt/sophos-spl/logs/base/sophosspl/updatescheduler.log
    Should Contain  ${UpdateSchedulerLog}  Detected product configuration change, triggering update.

UpdateScheduler Performs Update After Receiving Policy With Different Primary Subscription release tag Values
    [Setup]  Setup Current Update Scheduler Environment Without Policy
    Configure Hosts File
    Remove File  ${statusPath}
    Remove File  ${sulConfigPath}
    Start Update Scheduler
    Start Management Agent Via WDCTL
    Send Policy To UpdateScheduler  ALC_policy_direct_just_base.xml
    Check Status and Events Are Created
    ${UpdateSchedulerLog} =    Get File  /opt/sophos-spl/logs/base/sophosspl/updatescheduler.log
    Should Contain  ${UpdateSchedulerLog}  Attempting to update from warehouse

    # Now we know we are in a good state simulate previous update state for the test
    Copy File  ${SUPPORT_FILES}/update_config/previous_update_config_base_subscription_only.json   ${SOPHOS_INSTALL}/base/update/var/previous_update_config.json
    Run Process  chown  sophos-spl-user:sophos-spl-group   ${SOPHOS_INSTALL}/base/update/var/previous_update_config.json

    # Ensure update will be invoked when previous config subscriptions differ from current, when feature set is the same.
    Send Policy To UpdateScheduler  ALC_BaseOnlyBetaPolicy.xml
    ${UpdateSchedulerLog} =    Get File  /opt/sophos-spl/logs/base/sophosspl/updatescheduler.log
    Should Contain  ${UpdateSchedulerLog}  Detected product configuration change, triggering update.

UpdateScheduler Performs Update After Receiving Policy With Different Primary Subscription Fixed Version Values
    [Setup]  Setup Current Update Scheduler Environment Without Policy
    Configure Hosts File
    Remove File  ${statusPath}
    Remove File  ${sulConfigPath}
    Start Update Scheduler
    Start Management Agent Via WDCTL
    Send Policy To UpdateScheduler  ALC_policy_direct_just_base.xml
    Check Status and Events Are Created
    ${UpdateSchedulerLog} =    Get File  /opt/sophos-spl/logs/base/sophosspl/updatescheduler.log
    Should Contain  ${UpdateSchedulerLog}  Attempting to update from warehouse

    # Now we know we are in a good state simulate previous update state for the test
    Copy File  ${SUPPORT_FILES}/update_config/previous_update_config_base_subscription_only.json   ${SOPHOS_INSTALL}/base/update/var/previous_update_config.json
    Run Process  chown  sophos-spl-user:sophos-spl-group   ${SOPHOS_INSTALL}/base/update/var/previous_update_config.json

    # Ensure update will be invoked when previous config subscriptions differ from current, when feature set is the same.
    Send Policy To UpdateScheduler  ALC_BaseOnlyFixedVersionPolicy.xml
    ${UpdateSchedulerLog} =    Get File  /opt/sophos-spl/logs/base/sophosspl/updatescheduler.log
    Should Contain  ${UpdateSchedulerLog}  Detected product configuration change, triggering update.



UpdateScheduler Performs Update After Receiving Policy With Different Non Primary Subscription release tag Values
    [Setup]  Setup Current Update Scheduler Environment Without Policy
    Configure Hosts File
    Remove File  ${statusPath}
    Remove File  ${sulConfigPath}
    Start Update Scheduler
    Start Management Agent Via WDCTL
    Send Policy To UpdateScheduler  ALC_policy_direct_just_base.xml
    Check Status and Events Are Created
    ${UpdateSchedulerLog} =    Get File  /opt/sophos-spl/logs/base/sophosspl/updatescheduler.log
    Should Contain  ${UpdateSchedulerLog}  Attempting to update from warehouse

    # Now we know we are in a good state simulate previous update state for the test
    Copy File  ${SUPPORT_FILES}/update_config/previous_update_config_base_and_example_plugin.json   ${SOPHOS_INSTALL}/base/update/var/previous_update_config.json
    Run Process  chown  sophos-spl-user:sophos-spl-group   ${SOPHOS_INSTALL}/base/update/var/previous_update_config.json

    # Ensure update will be invoked when previous config subscriptions differ from current, when feature set is the same.
    Send Policy To UpdateScheduler  ALC_policy_direct_base_and_example_plugin_beta.xml
    ${UpdateSchedulerLog} =    Get File  /opt/sophos-spl/logs/base/sophosspl/updatescheduler.log

    Should Contain  ${UpdateSchedulerLog}  Detected product configuration change, triggering update.

UpdateScheduler Performs Update After Receiving Policy With Different Non Primary Subscription Fixed Version Values
    [Setup]  Setup Current Update Scheduler Environment Without Policy
    Configure Hosts File
    Remove File  ${statusPath}
    Remove File  ${sulConfigPath}
    Start Update Scheduler
    Start Management Agent Via WDCTL
    Send Policy To UpdateScheduler  ALC_policy_direct_just_base.xml
    Check Status and Events Are Created
    ${UpdateSchedulerLog} =    Get File  /opt/sophos-spl/logs/base/sophosspl/updatescheduler.log
    Should Contain  ${UpdateSchedulerLog}  Attempting to update from warehouse

    # Now we know we are in a good state simulate previous update state for the test
    Copy File  ${SUPPORT_FILES}/update_config/previous_update_config_base_and_example_plugin.json   ${SOPHOS_INSTALL}/base/update/var/previous_update_config.json
    Run Process  chown  sophos-spl-user:sophos-spl-group   ${SOPHOS_INSTALL}/base/update/var/previous_update_config.json

    # Ensure update will be invoked when previous config subscriptions differ from current, when feature set is the same.
    Send Policy To UpdateScheduler  ALC_policy_direct_base_and_example_plugin_FixedVersion.xml
    ${UpdateSchedulerLog} =    Get File  /opt/sophos-spl/logs/base/sophosspl/updatescheduler.log

    Should Contain  ${UpdateSchedulerLog}  Detected product configuration change, triggering update.


*** Keywords ***

Check Event Source
    [Arguments]   ${eventPath}   ${expectedSource}
    ${fileContent}  Get File  ${eventPath}
    Should Contain  ${fileContent}  ${expectedSource}  msg="Event does not report Source: ${expectedSource}"

Check Update Sheduler Run as Sophos-spl-user
    ${Pid} =   Run Process   pidof UpdateScheduler   shell=yes
    ${UpdateSchedulerProcess} =  Run Process   ps -o user\= -p ${Pid.stdout}    shell=yes
    Should Contain   ${UpdateSchedulerProcess.stdout}   sophos-spl-user    msg="Failed to detect process ownership of UpdateScheduler. Expected it to run as sophos-spl-user. ${UpdateSchedulerProcess.stdout}. Err: ${UpdateSchedulerProcess.stderr}"


Management Agent Contains
    [Arguments]  ${Contents}
    ${ManagementAgentLog} =    Get File  /opt/sophos-spl/logs/base/sophosspl/sophos_managementagent.log
    Should Contain  ${ManagementAgentLog}  ${Contents}

Configure Hosts File
    Copy File  /etc/hosts  /etc/hosts.bk
    Append To File  /etc/hosts  127.0.0.1 dci.sophosupd.net\n127.0.0.1 dci.sophosupd.com\n
    Append To File  /etc/hosts  127.0.0.1 d1.sophosupd.net\n127.0.0.1 d1.sophosupd.com\n
    Append To File  /etc/hosts  127.0.0.1 d2.sophosupd.net\n127.0.0.1 d2.sophosupd.com\n
    Append To File  /etc/hosts  127.0.0.1 d3.sophosupd.net\n127.0.0.1 d3.sophosupd.com
    Append To File  /etc/hosts  127.0.0.1 es-web.sophos.com\n



Check report was a product update
    [Arguments]  ${reportPath}
    ${contents} =    Get File  ${reportPath}
    Should Contain  ${contents}  "supplementOnlyUpdate": false

Check report was supplement-only update
    [Arguments]  ${reportPath}
    ${contents} =    Get File  ${reportPath}
    Should Contain  ${contents}  "supplementOnlyUpdate": true

Teardown For Test
    Log SystemCtl Update Status
    Run Keyword If Test Failed  Dump Mcs Router Dir Contents
    Run Keyword And Ignore Error  Move File  /etc/hosts.bk  /etc/hosts
    General Test Teardown

