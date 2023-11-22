*** Settings ***
Documentation    Verify the UpdateScheduler Plugin using the FakeSulDownloader (replace SulDownloader with a link to the FakeSulDownloader script) to allow verifying many different scenarios without running the real SulDownloader

Test Setup      Setup Current Update Scheduler Environment

Test Teardown   Teardown For Test

Library    Process
Library    OperatingSystem
Library    DateTime
Library    ${COMMON_TEST_LIBS}/LogUtils.py
Library    ${COMMON_TEST_LIBS}/FakeSulDownloader.py
Library    ${COMMON_TEST_LIBS}/UpdateSchedulerHelper.py
Library    ${COMMON_TEST_LIBS}/MCSRouter.py
Library    ${COMMON_TEST_LIBS}/TelemetryUtils.py

Resource    ${COMMON_TEST_ROBOT}/GeneralTeardownResource.robot
Resource    ${COMMON_TEST_ROBOT}/InstallerResources.robot
Resource    ${COMMON_TEST_ROBOT}/LogControlResources.robot
Resource    ${COMMON_TEST_ROBOT}/McsRouterResources.robot
Resource    ${COMMON_TEST_ROBOT}/SchedulerUpdateResources.robot
Resource    ${COMMON_TEST_ROBOT}/TelemetryResources.robot

Force Tags  UPDATE_SCHEDULER  TAP_PARALLEL6

*** Variables ***
${TELEMETRY_SUCCESS}    0
${TELEMETRY_JSON_FILE}              ${SOPHOS_INSTALL}/base/telemetry/var/telemetry.json

*** Test Cases ***
UpdateScheduler SulDownloader Report Sync With Warehouse Success
    [Tags]  SMOKE
    [Documentation]  Demonstrate that Events and Status will be generated during on the first run of Update Scheduler
    ${File}=  Get File   ${UPDATE_CONFIG}
    Should Contain   ${File}  "sophosSusURL": "https://sustest.sophosupd.com"
    Should Contain   ${File}  "sophosCdnURLs": [
    Should Contain   ${File}  "https://sdds3test.sophosupd.com
    @{features}=  Create List   CORE
    Setup Base and Plugin Sync and UpToDate  ${features}
    Create File   ${UPGRADING_MARKER_FILE}
    Run process   chown sophos-spl-updatescheduler:sophos-spl-group ${UPGRADING_MARKER_FILE}  shell=yes
    Run process   chmod a+w ${UPGRADING_MARKER_FILE}  shell=yes

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
    check_update_scheduler_telemetry_json_is_correct  ${telemetryFileContents}  0  True  ${time}
    Check Update Scheduler Run as Sophos-spl-user
    File Should Not Exist  ${UPGRADING_MARKER_FILE}

UpdateScheduler Does Not Create A Config For An Invalid Policy With No Base subscription
    Simulate Send Policy   ALC_policy_invalid.xml

    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  Check Log Contains   Invalid policy: No base subscription   ${SOPHOS_INSTALL}/logs/base/sophosspl/updatescheduler.log   Update Scheduler Log

    Check Update Scheduler Running


Systemctl Can Detect SulDownloader Service Runs Without Error After Error Reported
    # After install replace Sul Downloader with a fake version for test, then run SulDownloader in the same way UpdateScheduler does
    # Check if the expected return code is given.

    # Test is designed to prove the contract for the check will work across all platforms.

    Copy File  ${SOPHOS_INSTALL}/base/bin/SulDownloader.0  ${SOPHOS_INSTALL}/base/bin/SulDownloader.0.bak

    Replace Sul Downloader With Fake Broken Version
    ${startresult} =  Run Process   /bin/systemctl  start  sophos-spl-update.service
    Sleep  1s  We need to allow fake sul downloader to run
    ${result} =    Run Process   /bin/systemctl  is-failed  sophos-spl-update.service
    Should Be Equal As Integers  ${result.rc}  ${0}  msg="Detected error in sophos-spl-update.service error. stdout: ${result.stdout} stderr: ${result.stderr}. Start stdout: ${startresult.stdout}. stderr: ${startresult.stderr}"

    @{features}=  Create List   CORE
    Setup Base and Plugin Sync and UpToDate  ${features}

    ${startresult} =  Run Process   /bin/systemctl  start  sophos-spl-update.service
    ${result} =    Run Process   /bin/systemctl  is-failed  sophos-spl-update.service
    Should Be Equal As Integers  ${result.rc}  ${1}  msg="Failed to detect sophos-spl-update.service error. stdout: ${result.stdout} stderr: ${result.stderr}. Start stdout: ${startresult.stdout}. stderr: ${startresult.stderr}"

UpdateScheduler Regenerates The Config File If It Does Not Exist
    [Documentation]  Demonstrate that Events and Status will be generated during on the first run of Update Scheduler
    @{features}=  Create List   CORE
    Setup Base and Plugin Sync and UpToDate  ${features}
    Remove File  ${UPDATE_CONFIG}
    Simulate Update Now
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  2 secs
    ...  File Should Exist  ${UPDATE_CONFIG}
    ${File}=  Get File   ${UPDATE_CONFIG}
    Should Contain   ${File}  "JWToken": "stuff",
    ${eventPath} =  Check Status and Events Are Created
    Check Event Report Success  ${eventPath}

UpdateScheduler updates the device id and tenant id before an update
    @{features}=  Create List   CORE
    Setup Base and Plugin Sync and UpToDate  ${features}
    Remove File  ${UPDATE_CONFIG}
    Simulate Update Now
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  2 secs
    ...  File Should Exist  ${UPDATE_CONFIG}
    ${File}=  Get File   ${UPDATE_CONFIG}
    Should Contain   ${File}  "JWToken": "stuff",
    ${eventPath} =  Check Status and Events Are Created
    Check Event Report Success  ${eventPath}
    Create File       ${MCS_CONFIG}  jwt_token=newjwt\ndevice_id=newdevice\ntenant_id=newtenant
    Simulate Update Now
    Wait Until Keyword Succeeds
    ...   20 secs
    ...   5 secs
    ...   File Should Contain  ${UPDATE_CONFIG}     "deviceId": "newdevice"
    File Should Contain  ${UPDATE_CONFIG}     "tenantId": "newtenant"
    File Should Contain  ${UPDATE_CONFIG}     "JWToken": "newjwt",

UpdateScheduler Second Run with Normal Run will not generate new Event
    @{features}=  Create List   CORE  LIVETERMINAL
    Setup Base and Plugin Sync and UpToDate  ${features}
    Simulate Update Now
    ${eventPath} =  Check Status and Events Are Created
    Check Event Report Success  ${eventPath}
    Log  ${eventPath}
    Log File  ${eventPath}
    ${reportPath} =  Get latest report path
    Log  ${reportPath}
    Log File  ${reportPath}
    Setup Base and Plugin Sync and UpToDate  ${features}  startTime=2
    Simulate Update Now
    Check No New Event Created  ${eventPath}

    # first report is to be removed as it does not add relevant information
    File Should not Exist  ${reportPath}


UpdateScheduler Third Run With Upgrade Will Not Generate Event
    # If overall result of an update goes from SUCCESS to SUCCESS
    # even when files are being installed, no new event will be generated.
    @{features}=  Create List   CORE
    Setup Base and Plugin Sync and UpToDate  ${features}
    Simulate Update Now
    ${eventPath} =  Check Status and Events Are Created
    ${StatusPath} =  Check Status Is Created
    ${StatusContent1} =  Get File  ${StatusPath}

    Check Event Report Success  ${eventPath}

    ${reportPath} =  Get latest report path

    Setup Base and Plugin Sync and UpToDate  ${features}  startTime=2
    Simulate Update Now
    Check No New Event Created  ${eventPath}

    # first report is to be removed as it does not add relevant information
    File Should not Exist  ${reportPath}

    Setup Base and Plugin Upgraded  ${features}  startTime=3  syncTime=3
    Simulate Update Now

    Check No New Event Created  ${eventPath}
    ${StatusPath} =  Check Status Is Created
    ${StatusContent2} =  Get File  ${StatusPath}

    Should Not Be Equal As Strings   ${StatusContent1}  ${StatusContent2}

UpdateScheduler Report Failure to Update
    Setup Plugin Install Failed
    Simulate Update Now
    ${eventPath} =  Check Status and Events Are Created
    Check Event Report Install Failed   ${eventPath}

    ${time} =  Get Current Time
    Prepare To Run Telemetry Executable
    Run Telemetry Executable  ${EXE_CONFIG_FILE}  ${TELEMETRY_SUCCESS}
    ${telemetryFileContents} =  Get File  ${TELEMETRY_JSON_FILE}
    check_update_scheduler_telemetry_json_is_correct  ${telemetryFileContents}  1  False  install_state=1
    Cleanup Telemetry Server

Test Updatescheduler Adds Features That Get Installed On Subsequent Update
    @{features}=  Create List   CORE
    Setup Base And Plugin Sync And Uptodate  ${features}
    Simulate Update Now
    ${eventPath} =  Check Status and Events Are Created
    Wait Until Keyword Succeeds
    ...  1 minutes
    ...  5 secs
    ...  Check Status Report Contain  Feature id="CORE"
    Check Status Report Does Not Contain  Feature id="MDR"
    @{features}=  Create List   CORE  MDR
    Setup Base And Plugin Sync And Uptodate  ${features}
    Send Policy To UpdateScheduler  ALC_policy_direct.xml
    Wait Until Keyword Succeeds
    ...  1 minutes
    ...  5 secs
    ...  Check Status Report Contain  Feature id="MDR"


Test Updatescheduler Does Not Add Features That Failed To Install
    @{features}=  Create List   CORE
    Setup Base And Plugin Sync And Uptodate  ${features}
    Simulate Update Now
    ${eventPath} =  Check Status and Events Are Created
    Wait Until Keyword Succeeds
    ...  1 minutes
    ...  5 secs
    ...  Check Status Report Contain  Feature id="CORE"
    Check Status Report Does Not Contain  Feature id="MDR"

    Setup Plugin Install Failed
    Remove File  ${statusPath}
    Send Policy To UpdateScheduler  ALC_policy_direct.xml
    Wait Until Keyword Succeeds
    ...  1 minutes
    ...  5 secs
    ...  Check Status Report Contain  Feature id="CORE"
    Check Status Report Does Not Contain  Feature id="MDR"

Test Updatescheduler State Machine Results Show In Status Xml Message Correctly
    @{features}=  Create List   CORE
    Setup Base Only Sync And Uptodate  ${features}
    Simulate Update Now
    Wait Until Keyword Succeeds
    ...  1 minutes
    ...  5 secs
    ...  Check Status Report Contain  downloadState

    Log File  ${SOPHOS_INSTALL}/base/mcs/status/ALC_status.xml

    ${StatusContent} =  Get File  ${SOPHOS_INSTALL}/base/mcs/status/ALC_status.xml
    Should Contain  ${StatusContent}  <downloadState><state>good</state></downloadState>
    Should Contain  ${StatusContent}  <installState><state>good</state><lastGood>
    # not checking actual value of lastGood because this is a time stamp that will change.
    Should Contain  ${StatusContent}   </lastGood></installState>
    Should Contain  ${StatusContent}   <rebootState><required>no</required></rebootState>
    Log File  /opt/sophos-spl/base/update/var/updatescheduler/state_machine_raw_data.json

Test Updatescheduler State Machine Results Show In Status Xml Message Correctly With Success Failure Success Restart
    #Success state
    @{features}=  Create List   CORE
    Setup Base Only Sync And Uptodate  ${features}
    Simulate Update Now
    Wait Until Keyword Succeeds
    ...  1 minutes
    ...  5 secs
    ...  Check Status Report Contain  downloadState

    Log File  ${SOPHOS_INSTALL}/base/mcs/status/ALC_status.xml

    ${StatusContent} =  Get File  ${SOPHOS_INSTALL}/base/mcs/status/ALC_status.xml
    Should Contain  ${StatusContent}  <downloadState><state>good</state></downloadState>
    Should Contain  ${StatusContent}  <installState><state>good</state><lastGood>
    # not checking actual value of lastGood because this is a time stamp that will change.
    Should Contain  ${StatusContent}   </lastGood></installState>
    Should Contain  ${StatusContent}   <rebootState><required>no</required></rebootState>
    Log File  /opt/sophos-spl/base/update/var/updatescheduler/state_machine_raw_data.json

    #Needs to fail 3 times to report Failed state
    Fail Update Install And Check Status Shows Good Install State
    Fail Update Install And Check Status Not ReGenerated
    Fail Update Install And Check Status Shows Bad Install State

    Remove File  ${statusPath}
    #Success state
    @{features}=  Create List   CORE
    Setup Base and Plugin Upgraded  ${features}  startTime=3
    Simulate Update Now
    Wait Until Keyword Succeeds
    ...  1 minutes
    ...  5 secs
    ...  Check Status Report Contain  downloadState

    Log File  ${SOPHOS_INSTALL}/base/mcs/status/ALC_status.xml

    ${StatusContent} =  Get File  ${SOPHOS_INSTALL}/base/mcs/status/ALC_status.xml
    Should Contain  ${StatusContent}  <downloadState><state>good</state></downloadState>
    Should Contain  ${StatusContent}  <installState><state>good</state><lastGood>
    # not checking actual value of lastGood because this is a time stamp that will change.
    Should Contain  ${StatusContent}   </lastGood></installState>
    Should Contain  ${StatusContent}   <rebootState><required>no</required></rebootState>

    # Restart update scheduler and management, also delete old status to force a new one to be generated so we can
    # check that the state machines have been correctly read from disk after a restart.
    Run Process  ${SOPHOS_INSTALL}/bin/wdctl  stop  updatescheduler
    Run Process  ${SOPHOS_INSTALL}/bin/wdctl  stop  managementagent
    Remove file  ${statusPath}
    Remove file  ${MCS_DIR}/status/cache/ALC.xml
    Run Process  ${SOPHOS_INSTALL}/bin/wdctl  start  managementagent
    Run Process  ${SOPHOS_INSTALL}/bin/wdctl  start  updatescheduler

    Wait Until Keyword Succeeds
    ...  1 minutes
    ...  5 secs
    ...  Check Status Report Contain  downloadState

    Log File  ${SOPHOS_INSTALL}/base/mcs/status/ALC_status.xml

    ${StatusContent} =  Get File  ${SOPHOS_INSTALL}/base/mcs/status/ALC_status.xml
    Should Contain  ${StatusContent}  <downloadState><state>good</state></downloadState>
    Should Contain  ${StatusContent}  <installState><state>good</state><lastGood>
    # not checking actual value of lastGood because this is a time stamp that will change.
    Should Contain  ${StatusContent}   </lastGood></installState>
    Should Contain  ${StatusContent}   <rebootState><required>no</required></rebootState>

Test Updatescheduler State Machine Data Is Reset When State Machine File Is Empty Status Still Sent
    @{features}=  Create List   CORE
    Setup Base Only Sync And Uptodate  ${features}
    Remove File  ${SOPHOS_INSTALL}/base/update/var/updatescheduler/state_machine_raw_data.json
    Create File  ${SOPHOS_INSTALL}/base/update/var/updatescheduler/state_machine_raw_data.json
    Log File  ${SOPHOS_INSTALL}/base/update/var/updatescheduler/state_machine_raw_data.json

    Simulate Update Now
    Wait Until Keyword Succeeds
    ...  1 minutes
    ...  5 secs
    ...  Check Status Report Contain  downloadState

    Log File  ${SOPHOS_INSTALL}/base/mcs/status/ALC_status.xml

    ${StatusContent} =  Get File  ${SOPHOS_INSTALL}/base/mcs/status/ALC_status.xml
    Should Contain  ${StatusContent}  <downloadState><state>good</state></downloadState>
    Should Contain  ${StatusContent}  <installState><state>good</state><lastGood>
    # not checking actual value of lastGood because this is a time stamp that will change.
    Should Contain  ${StatusContent}   </lastGood></installState>
    Should Contain  ${StatusContent}   <rebootState><required>no</required></rebootState>
    Log File  ${SOPHOS_INSTALL}/base/update/var/updatescheduler/state_machine_raw_data.json

    @{features}=  Create List   CORE
    Setup Base and Plugin Upgraded  ${features}  startTime=3
    Simulate Update Now
    Wait Until Keyword Succeeds
    ...  1 minutes
    ...  5 secs
    ...  Check Status Report Contain  downloadState

    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  Check Log Contains In Order
        ...  ${SOPHOS_INSTALL}/logs/base/sophosspl/updatescheduler.log
        ...  Reading the state machine data file failed with
        ...  removing file to reset state machine
        ...  Sending event to Central

Test Updatescheduler Features Codes Correct After Success Failure Success Restart
    [Documentation]  Updatescheduler on success adds CORE to the feature codes in ALC status, the next update
    ...  that should install MDR fails and does not add MDR to the feature code list in ALC status. A third
    ...  update happens that does install MDR correctly and then that feature code is added to ALC status.
    ...  On a restart we prove that the feature codes that were saved to disk are being loaded in by forcing a
    ...  status to be generated from what is in updateschedulerprocessor memory.

    @{features}=  Create List   CORE
    Setup Base Only Sync And Uptodate  ${features}
    Simulate Update Now
    Wait Until Keyword Succeeds
    ...  1 minutes
    ...  5 secs
    ...  Check Status Report Contain  Feature id="CORE"
    Check Status Report Does Not Contain  Feature id="MDR"
    Remove File  ${statusPath}
    Setup Plugin Install Failed  startTime=2
    Send Policy To UpdateScheduler  ALC_policy_direct.xml
    Wait Until Keyword Succeeds
    ...  1 minutes
    ...  5 secs
    ...  Check Status Report Contain  Feature id="CORE"
    Check Status Report Does Not Contain  Feature id="MDR"

    Remove File  ${statusPath}

    @{features}=  Create List   CORE  MDR
    Setup Base and Plugin Upgraded  ${features}  startTime=3
    Simulate Update Now
    Wait Until Keyword Succeeds
    ...  1 minutes
    ...  5 secs
    ...  Check Status Report Contain  Feature id="CORE"
    Check Status Report Contain  Feature id="MDR"

    # Restart update scheduler and management, also delete old status to force a new one to be generated so we can
    # check that the feature codes have been correctly read from disk after a restart.
    Run Process  ${SOPHOS_INSTALL}/bin/wdctl  stop  updatescheduler
    Run Process  ${SOPHOS_INSTALL}/bin/wdctl  stop  managementagent
    remove file  ${statusPath}
    remove file  ${MCS_DIR}/status/cache/ALC.xml
    Run Process  ${SOPHOS_INSTALL}/bin/wdctl  start  managementagent
    Run Process  ${SOPHOS_INSTALL}/bin/wdctl  start  updatescheduler

    Wait Until Keyword Succeeds
    ...  1 minutes
    ...  5 secs
    ...  Check Status Report Contain  Feature id="CORE"
    Check Status Report Contain  Feature id="MDR"

Test Status Is Sent On Consecutive Successful Updates With Policy Changes
    Override LogConf File as Global Level  DEBUG
    @{features}=  Create List   CORE
    Setup Base And Plugin Sync And Uptodate  ${features}
    Simulate Update Now
    Wait Until Keyword Succeeds
    ...  1 minutes
    ...  5 secs
    ...  Check Status Report Contain  Feature id="CORE"
    Check Status Report Does Not Contain  Feature id="MDR"

    Remove File  ${statusPath}

    @{features}=  Create List   CORE  MDR
    Setup Base And Plugin Sync And Uptodate  ${features}
    Send Policy To UpdateScheduler  ALC_policy_direct.xml
    Wait Until Keyword Succeeds
    ...  1 minutes
    ...  5 secs
    ...  Check Status Report Contain  Feature id="CORE"
    Check Status Report Contain  Feature id="MDR"

    Remove File  ${statusPath}
    @{features}=  Create List   CORE
    Setup Base And Plugin Sync And Uptodate  ${features}
    Send Policy To UpdateScheduler  ALC_policy_direct_just_base.xml
    Wait Until Keyword Succeeds
    ...  1 minutes
    ...  5 secs
    ...  Check Status Report Contain  Feature id="CORE"
    Check Status Report Does Not Contain  Feature id="MDR"


UpdateScheduler Report Failure to Update Multiple Times In Telemetry
    Setup Plugin Install Failed
    Simulate Update Now
    ${eventPath} =  Check Status and Events Are Created
    Check Event Report Install Failed   ${eventPath}

    ${update_scheduler_mark} =    mark_log_size    ${SOPHOS_INSTALL}/logs/base/sophosspl/updatescheduler.log
    Setup Plugin Install Failed  startTime=3  syncTime=3
    Simulate Update Now

    ${time} =  Get Current Time
    Prepare To Run Telemetry Executable
    wait_for_log_contains_from_mark    ${update_scheduler_mark}    Sending status to Central    ${5}
    Run Telemetry Executable  ${EXE_CONFIG_FILE}  ${TELEMETRY_SUCCESS}
    ${telemetryFileContents} =  Get File  ${TELEMETRY_JSON_FILE}
    check_update_scheduler_telemetry_json_is_correct  ${telemetryFileContents}  2  False  install_state=1
    Cleanup Telemetry Server

    # Failed count should have been reset.
    ${update_scheduler_mark} =    mark_log_size    ${SOPHOS_INSTALL}/logs/base/sophosspl/updatescheduler.log
    Setup Plugin Install Failed  startTime=4  syncTime=4
    Simulate Update Now

    ${time} =  Get Current Time
    Prepare To Run Telemetry Executable
    wait_for_log_contains_from_mark    ${update_scheduler_mark}    Sending status to Central    ${5}
    Run Telemetry Executable  ${EXE_CONFIG_FILE}  ${TELEMETRY_SUCCESS}
    ${telemetryFileContents} =  Get File  ${TELEMETRY_JSON_FILE}
    check_update_scheduler_telemetry_json_is_correct  ${telemetryFileContents}  1  False  install_state=1    alc_policy_received=${False}
    Cleanup Telemetry Server


UpdateScheduler Start and Restart
    Setup Plugin Install Failed  startTime=2  syncTime=1
    Overwrite MCS Flags File
    Simulate Update Now
    ${eventPath} =  Check Status and Events Are Created
    Check Event Report Install Failed   ${eventPath}
    log file  ${statusPath}
    Remove File  ${eventPath}
    Remove File  ${statusPath}

    Restart Update Scheduler
    Send Policy With No Cache And No Proxy
    @{features}=  Create List   CORE
    Setup Base and Plugin Upgraded  ${features}  startTime=3  syncTime=3
    Simulate Update Now
    ${eventPath} =  Check Event File Generated  10
    Check Event Report Success  ${eventPath}

    # After first succesful update feature codes are added to the ALC status
    File Should Exist  ${statusPath}
    log file  ${statusPath}


UpdateScheduler SulDownloader Report Sync With Warehouse Success Via Update Cache
    [Documentation]  Demonstrate that Events and Status will be generated during on the first run of Update Scheduler
    Setup Base Uptodate Via Update Cache  2k12-64-ld55-df.eng.sophos:8191

    Simulate Update Now
    ${eventPath} =  Check Status and Events Are Created
    Check Event Report Success  ${eventPath}

    Check Event Source  ${eventPath}  <updateSource>4092822d-0925-4deb-9146-fbc8532f8c55</updateSource>


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
    Remove File  ${statusPath}
    Remove File  ${UPDATE_CONFIG}
    Start Update Scheduler
    Start Management Agent Via WDCTL
    Send Policy To UpdateScheduler  ALC_policy_direct.xml
    Check Status and Events Are Created
    ${UpdateSchedulerLog} =    Get File  /opt/sophos-spl/logs/base/sophosspl/updatescheduler.log
    Should Contain  ${UpdateSchedulerLog}  Attempting to update from warehouse

UpdateScheduler sends a status after recieving a policy that does not change feature list
    [Setup]  Setup Current Update Scheduler Environment Without Policy
    Remove File  ${statusPath}
    Remove File  ${UPDATE_CONFIG}
    Start Update Scheduler
    Start Management Agent Via WDCTL
    Send Policy To UpdateScheduler  ALC_policy_direct_just_base.xml

    Wait Until Keyword Succeeds
    ...  90 secs
    ...  10 secs
    ...  check_updatescheduler_log_contains_string_n_times   Sending status to Central  1
    Send Policy To UpdateScheduler  ALC_BaseOnlyBetaPolicy.xml
    Wait Until Keyword Succeeds
    ...  90 secs
    ...  10 secs
    ...  check_updatescheduler_log_contains_string_n_times   Sending status to Central  2

UpdateScheduler Performs Scheduled Update
    [Tags]  SLOW  UPDATE_SCHEDULER
    [Timeout]    20 minutes
    [Setup]  Setup Current Update Scheduler Environment Without Policy
    # There are three types of updates scheduled:
    # 1. Immediately after getting the first ALC policy - to install plugins
    # 2. 5-10 minutes after starting up - to ensure machines that have been switched off for a period get updated quickly
    # 3. Updates based off the update period (7 minutes for this policy)
    ${BasicPolicyXml} =  Get File  ${SUPPORT_FILES}/CentralXml/ALC_policy_scheduled_update.xml
    ${Date} =  Get Current Date
    ${ScheduledDate} =  Add Time To Date  ${Date}  11 minutes
    ${ScheduledDay} =  Convert Date  ${ScheduledDate}  result_format=%A
    ${ScheduledTime} =  Convert Date  ${ScheduledDate}  result_format=%H:%M:00
    ${NewPolicyXml} =  Replace String  ${BasicPolicyXml}  REPLACE_DAY  ${ScheduledDay}
    ${NewPolicyXml} =  Replace String  ${NewPolicyXml}  REPLACE_TIME  ${ScheduledTime}
    ${NewPolicyXml} =  Replace String  ${NewPolicyXml}  Frequency="40"  Frequency="7"
    Create File  ${SOPHOS_INSTALL}/tmp/ALC_policy_scheduled_update.xml  ${NewPolicyXml}
    Log File  ${SOPHOS_INSTALL}/tmp/ALC_policy_scheduled_update.xml
    Send Policy To UpdateScheduler  ${SOPHOS_INSTALL}/tmp/ALC_policy_scheduled_update.xml

    # Update on install
    ${eventPath} =  Check Event File Generated  wait_time_seconds=60
    Remove File  ${eventPath}
    ${reportPath} =  Get latest report path
    Check report was a product update  ${reportPath}
    ${sul_log_mark} =    mark_log_size     ${SULDOWNLOADER_LOG_PATH}

    # Update after 5-10 minutes (boot storm avoiding update)
    Log File   ${reportPath}
    Remove File  ${reportPath}
    wait_for_log_contains_from_mark    ${sul_log_mark}    Generating the report file    ${600}

    # Scheduled update - 7 minutes after the previous update
    ${sul_log_mark} =    mark_log_size     ${SULDOWNLOADER_LOG_PATH}
    wait_for_log_contains_from_mark    ${sul_log_mark}    Generating the report file    ${480}
    ${reportPath} =  Get latest report path
    Check report was a product update  ${reportPath}

    ${ActualUpdateDate} =  Get Current Date
    ${TimeDiff} =  Subtract Date From Date  ${ActualUpdateDate}  ${ScheduledDate}
    IF  ${TimeDiff} < -60 or 480 < ${TimeDiff}
      Fail
    END


UpdateScheduler Performs Update After Receiving Policy With Different Features
    [Setup]  Setup Current Update Scheduler Environment Without Policy
    Remove File  ${statusPath}
    Remove File  ${UPDATE_CONFIG}
    Start Update Scheduler
    Start Management Agent Via WDCTL
    Send Policy To UpdateScheduler  ALC_policy_direct_just_base.xml
    Check Status and Events Are Created
    ${UpdateSchedulerLog} =    Get File  /opt/sophos-spl/logs/base/sophosspl/updatescheduler.log
    Should Contain  ${UpdateSchedulerLog}  Attempting to update from warehouse

    # Now we know we are in a good state simulate previous update state for the test
    Copy File  ${SUPPORT_FILES}/update_config/previous_update_config_base_subscription_only.json   ${SOPHOS_INSTALL}/base/update/var/updatescheduler/previous_update_config.json
    Run Process  chown  sophos-spl-updatescheduler:sophos-spl-group   ${SOPHOS_INSTALL}/base/update/var/updatescheduler/previous_update_config.json

    # Ensure update will be invoked when previous config subscriptions differ from current, when feature set is the same.
    Send Policy To UpdateScheduler  ALC_policy_direct_with_sdu_feature.xml
    ${UpdateSchedulerLog} =    Get File  /opt/sophos-spl/logs/base/sophosspl/updatescheduler.log
    Should Contain  ${UpdateSchedulerLog}  Detected product configuration change, triggering update.


UpdateScheduler Performs Update After Receiving Policy With Different Subscriptions
    [Setup]  Setup Current Update Scheduler Environment Without Policy
    Remove File  ${statusPath}
    Remove File  ${UPDATE_CONFIG}
    Start Update Scheduler
    Start Management Agent Via WDCTL
    Send Policy To UpdateScheduler  ALC_policy_direct_just_base.xml
    Check Status and Events Are Created
    ${UpdateSchedulerLog} =    Get File  /opt/sophos-spl/logs/base/sophosspl/updatescheduler.log
    Should Contain  ${UpdateSchedulerLog}  Attempting to update from warehouse

    # Now we know we are in a good state simulate previous update state for the test
    Copy File  ${SUPPORT_FILES}/update_config/previous_update_config_base_subscription_only.json   ${SOPHOS_INSTALL}/base/update/var/updatescheduler/previous_update_config.json
    Run Process  chown  sophos-spl-updatescheduler:sophos-spl-group   ${SOPHOS_INSTALL}/base/update/var/updatescheduler/previous_update_config.json

    # Ensure update will be invoked when previous config subscriptions differ from current, when feature set is the same.
    Send Policy To UpdateScheduler  ALC_policy_direct.xml
    ${UpdateSchedulerLog} =    Get File  /opt/sophos-spl/logs/base/sophosspl/updatescheduler.log
    Should Contain  ${UpdateSchedulerLog}  Detected product configuration change, triggering update.

UpdateScheduler Performs Update After Receiving Policy With Different Primary Subscription release tag Values
    [Setup]  Setup Current Update Scheduler Environment Without Policy
    Remove File  ${statusPath}
    Remove File  ${UPDATE_CONFIG}
    Start Update Scheduler
    Start Management Agent Via WDCTL
    Send Policy To UpdateScheduler  ALC_policy_direct_just_base.xml
    Check Status and Events Are Created
    ${UpdateSchedulerLog} =    Get File  /opt/sophos-spl/logs/base/sophosspl/updatescheduler.log
    Should Contain  ${UpdateSchedulerLog}  Attempting to update from warehouse

    # Now we know we are in a good state simulate previous update state for the test
    Copy File  ${SUPPORT_FILES}/update_config/previous_update_config_base_subscription_only.json   ${SOPHOS_INSTALL}/base/update/var/updatescheduler/previous_update_config.json
    Run Process  chown  sophos-spl-updatescheduler:sophos-spl-group   ${SOPHOS_INSTALL}/base/update/var/updatescheduler/previous_update_config.json

    # Ensure update will be invoked when previous config subscriptions differ from current, when feature set is the same.
    Send Policy To UpdateScheduler  ALC_BaseOnlyBetaPolicy.xml
    ${UpdateSchedulerLog} =    Get File  /opt/sophos-spl/logs/base/sophosspl/updatescheduler.log
    Should Contain  ${UpdateSchedulerLog}  Detected product configuration change, triggering update.

UpdateScheduler Performs Update After Receiving Policy With Different Primary Subscription Fixed Version Values
    [Setup]  Setup Current Update Scheduler Environment Without Policy
    Remove File  ${statusPath}
    Remove File  ${UPDATE_CONFIG}
    Start Update Scheduler
    Start Management Agent Via WDCTL
    Send Policy To UpdateScheduler  ALC_policy_direct_just_base.xml
    Check Status and Events Are Created
    ${UpdateSchedulerLog} =    Get File  /opt/sophos-spl/logs/base/sophosspl/updatescheduler.log
    Should Contain  ${UpdateSchedulerLog}  Attempting to update from warehouse

    # Now we know we are in a good state simulate previous update state for the test
    Copy File  ${SUPPORT_FILES}/update_config/previous_update_config_base_subscription_only.json   ${SOPHOS_INSTALL}/base/update/var/updatescheduler/previous_update_config.json
    Run Process  chown  sophos-spl-updatescheduler:sophos-spl-group   ${SOPHOS_INSTALL}/base/update/var/updatescheduler/previous_update_config.json

    # Ensure update will be invoked when previous config subscriptions differ from current, when feature set is the same.
    Send Policy To UpdateScheduler  ALC_BaseOnlyFixedVersionPolicy.xml

    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  Check Log Contains   Detected product configuration change, triggering update.    ${SOPHOS_INSTALL}/logs/base/sophosspl/updatescheduler.log    Updatescheduler Log


UpdateScheduler Performs Update After Receiving Policy With Different Non Primary Subscription release tag Values
    [Setup]  Setup Current Update Scheduler Environment Without Policy
    Remove File  ${statusPath}
    Remove File  ${UPDATE_CONFIG}
    Start Update Scheduler
    Start Management Agent Via WDCTL
    Send Policy To UpdateScheduler  ALC_policy_direct_just_base.xml
    Check Status and Events Are Created
    ${UpdateSchedulerLog} =    Get File  /opt/sophos-spl/logs/base/sophosspl/updatescheduler.log
    Should Contain  ${UpdateSchedulerLog}  Attempting to update from warehouse

    # Now we know we are in a good state simulate previous update state for the test
    Copy File  ${SUPPORT_FILES}/update_config/previous_update_config_base_subscription_only.json   ${SOPHOS_INSTALL}/base/update/var/updatescheduler/previous_update_config.json
    Run Process  chown  sophos-spl-updatescheduler:sophos-spl-group   ${SOPHOS_INSTALL}/base/update/var/updatescheduler/previous_update_config.json

    # Ensure update will be invoked when previous config subscriptions differ from current, when feature set is the same.
    Send Policy To UpdateScheduler  ALC_policy_direct.xml
    ${UpdateSchedulerLog} =    Get File  /opt/sophos-spl/logs/base/sophosspl/updatescheduler.log

    Should Contain  ${UpdateSchedulerLog}  Detected product configuration change, triggering update.

UpdateScheduler Performs Update After Receiving Policy With Different Non Primary Subscription Fixed Version Values
    [Setup]  Setup Current Update Scheduler Environment Without Policy
    Remove File  ${statusPath}
    Remove File  ${UPDATE_CONFIG}
    Start Update Scheduler
    Start Management Agent Via WDCTL
    Send Policy To UpdateScheduler  ALC_policy_direct_just_base.xml
    Check Status and Events Are Created
    ${UpdateSchedulerLog} =    Get File  /opt/sophos-spl/logs/base/sophosspl/updatescheduler.log
    Should Contain  ${UpdateSchedulerLog}  Attempting to update from warehouse

    # Now we know we are in a good state simulate previous update state for the test
    Copy File  ${SUPPORT_FILES}/update_config/previous_update_config_base_subscription_only.json   ${SOPHOS_INSTALL}/base/update/var/updatescheduler/previous_update_config.json
    Run Process  chown  sophos-spl-updatescheduler:sophos-spl-group   ${SOPHOS_INSTALL}/base/update/var/updatescheduler/previous_update_config.json

    # Ensure update will be invoked when previous config subscriptions differ from current, when feature set is the same.
    Send Policy To UpdateScheduler  ALC_BaseOnlyFixedVersionPolicy.xml
    ${UpdateSchedulerLog} =    Get File  /opt/sophos-spl/logs/base/sophosspl/updatescheduler.log

    Should Contain  ${UpdateSchedulerLog}  Detected product configuration change, triggering update.

Failed Download Telemetry Test
    ${script} =     Catenate    SEPARATOR=\n
    ...    \{
    ...     "DownloadFailedSinceTime": "0",
    ...     "DownloadState": "1",
    ...     "DownloadStateCredit": "72",
    ...     "EventStateLastError": "0",
    ...     "EventStateLastTime": "1637597873",
    ...     "InstallFailedSinceTime": "0",
    ...     "InstallState": "0",
    ...     "InstallStateCredit": "3",
    ...     "LastGoodInstallTime": "1534853540",
    ...     "canSendEvent": true
    ...   }
    ...    \

    Create File   /opt/sophos-spl/base/update/var/updatescheduler/state_machine_raw_data.json  content=${script}
    Run Process  chown  sophos-spl-updatescheduler:sophos-spl-group   /opt/sophos-spl/base/update/var/updatescheduler/state_machine_raw_data.json
    Restart Update Scheduler
    ${time} =  Get Current Time
    Prepare To Run Telemetry Executable
    Run Telemetry Executable  ${EXE_CONFIG_FILE}  ${TELEMETRY_SUCCESS}
    ${telemetryFileContents} =  Get File  ${TELEMETRY_JSON_FILE}
    check_update_scheduler_telemetry_json_is_correct  ${telemetryFileContents}  0  download_state=1
    Cleanup Telemetry Server


Update Scheduler Waits Until Suldownloader Has Finished On Start
    ${suldownloader_lock} =  Set Variable  ${SOPHOS_INSTALL}/var/lock-sophosspl/suldownloader.pid
    Stop Update Scheduler
    Remove File    ${suldownloader_lock}
    Start Process	flock    ${suldownloader_lock}    sleep    20	alias=flock
    Wait Until Keyword Succeeds    3 secs    1 secs    file should exist    ${suldownloader_lock}
    Run process    chown    root:sophos-spl-group    ${suldownloader_lock}
    Run process    chmod    0660    ${suldownloader_lock}
    Start Update Scheduler
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  3 secs
    ...  Check Log Contains In Order    ${SOPHOS_INSTALL}/logs/base/sophosspl/updatescheduler.log
        ...    Waiting for SulDownloader to finish
        ...    Could not acquire suldownloader lock, assuming suldownloader is running
        ...    Took lock /opt/sophos-spl/var/lock-sophosspl/suldownloader.pid, assuming suldownloader not running
        ...    No instance of SulDownloader running
        ...    Running with update period to 60 minutes


*** Keywords ***
Teardown For Test
    Cleanup Telemetry Server
    Log SystemCtl Update Status
    Dump Log On Failure  /opt/sophos-spl/tmp/fakesul.log
    Run Keyword If Test Failed  Dump Mcs Router Dir Contents
    Run Keyword And Ignore Error  Move File  /etc/hosts.bk  /etc/hosts
    General Test Teardown


Replace Original Sul Downloader
    Remove File  ${SOPHOS_INSTALL}/base/bin/SulDownloader.0
    Copy File  ${SOPHOS_INSTALL}/base/bin/SulDownloader.0.bak  ${SOPHOS_INSTALL}/base/bin/SulDownloader.0

