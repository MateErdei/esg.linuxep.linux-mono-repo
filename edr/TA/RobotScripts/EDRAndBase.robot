*** Settings ***
Documentation    Suite description

Library         Process
Library         OperatingSystem

Resource        EDRResources.robot
Resource        ComponentSetup.robot

Suite Setup     Install With Base SDDS
Suite Teardown  Uninstall And Revert Setup

Test Setup      No Operation
Test Teardown   EDR And Base Teardown

*** Test Cases ***
LiveQuery is Distributed to EDR Plugin and Its Answer is available to MCSRouter
    [Tags]      TAP_TEST
    Check EDR Plugin Installed With Base
    Simulate Live Query  RequestProcesses.json
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  1 secs
    ...  File Should Exist    ${SOPHOS_INSTALL}/base/mcs/response/LiveQuery_123-4_response.json

LiveQuery Response is Chowned to Sophos Spl Local on EDR Startup
    Check EDR Plugin Installed With Base
    Create File  ${SOPHOS_INSTALL}/base/mcs/response/LiveQuery_567-8_response.json
    Stop EDR
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  1 secs
    ...  EDR Plugin Log Contains      edr <> Plugin Finished
    Start EDR
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  1 secs
    ...  Check Ownership    ${SOPHOS_INSTALL}/base/mcs/response/LiveQuery_567-8_response.json  sophos-spl-local

Incorrect LiveQuery is Distributed to EDR Plugin is handled correclty
    Check EDR Plugin Installed With Base
    Simulate Live Query  FailedRequestProcesses.json
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  1 secs
    ...  LiveQuery Log Contains  Query with name: Incorrect Query and corresponding id: 123-4 failed to execute with error: no such table: proc

EDR plugin Can Start Osquery
    Check EDR Plugin Installed With Base
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  1 secs
    ...  Check Osquery Running

Test EDR Serialize Response Handles Non-UTF8 Characters in Osquery Response
    Check EDR Plugin Installed With Base
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  1 secs
    ...  Check Osquery Running

    Copy File  ${TEST_INPUT_PATH}/componenttests/LiveQueryReport  ${COMPONENT_ROOT_PATH}/extensions/
    Run Process  chmod  +x  ${COMPONENT_ROOT_PATH}/extensions/LiveQueryReport
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  5 secs
    ...  Run Non-UTF8 Query

EDR plugin Configures OSQuery To Enable SysLog Event Collection

    Check EDR Plugin Installed With Base
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  1 secs
    ...  Check Osquery Running

    Should Exist  ${SOPHOS_INSTALL}/shared/syslog_pipe
    File Should Exist  /etc/rsyslog.d/rsyslog_sophos-spl.conf

    # check rsyslog does not report error connecting to named pipe

    ${result} =  Run Process  which  unminimize
    # unminimize is a command that should only exist on ubuntu minimal, if this command does not exist
    # Then the check for rsyslog running without issue can be made.
    # for some reason ubuntu minimum does not have rsyslog installed as a service.
    # but is installed on all other platforms we support.
    Run Keyword If   ${result.rc}==1
    ...   Check Rsyslog Started Without Error

EDR Restarts If File Descriptor Limit Hit

    Check EDR Plugin Installed With Base

    Remove File  ${SOPHOS_INSTALL}/plugins/edr/bin/sophos_livequery
    create File  ${SOPHOS_INSTALL}/plugins/edr/bin/sophos_livequery    \#!/bin/bash\nsleep 100000
    Log File  ${SOPHOS_INSTALL}/plugins/edr/bin/sophos_livequery
    Run Process  chmod  +x   ${SOPHOS_INSTALL}/plugins/edr/bin/sophos_livequery
    Create File  ${SOPHOS_INSTALL}/base/etc/logger.conf  [global]\nVERBOSITY = DEBUG\n
    Restart EDR

    ${actionContent} =  Set Variable  '{"type": "sophos.mgt.action.RunLiveQuery", "name": "test_query", "query": "select * from users;"}'

    Send Plugin Actions  edr  LiveQuery  corr123  ${actionContent}  ${120}
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  5 secs
    ...  EDR Plugin Log Contains X Times  Received new Action   120

    Wait Until Keyword Succeeds
    ...  200 secs
    ...  20 secs
    ...  EDR Plugin Log Contains X Times   Early request to stop found.  1

EDR Plugin Can Have Logging Level Changed Individually
    Check EDR Plugin Installed With Base
    #setting edr_osquery to DEBUG provides all DEBUG logging (these can be hidden by specifying the component with INFO level)
    Create File         ${SOPHOS_INSTALL}/base/etc/logger.conf.local   [edr_osquery]\nVERBOSITY=DEBUG\n
    Restart EDR
    Wait Until Keyword Succeeds
        ...  15 secs
        ...  1 secs
        ...  EDR Plugin Log Contains   Logger edr_osquery configured for level: DEBUG

EDR Plugin Can Have Logging Level Changed Based On Components
    Check EDR Plugin Installed With Base
    #With edr_osquery being set to INFO, the other sub-components can be set to DEBUG and have the specified logging level
    Create File         ${SOPHOS_INSTALL}/base/etc/logger.conf.local   [edr_osquery]\nVERBOSITY=INFO\n[edr]\nVERBOSITY=DEBUG\n
    Restart EDR
    Wait Until Keyword Succeeds
        ...  15 secs
        ...  1 secs
        ...  EDR Plugin Log Contains   Logger edr_osquery configured for level: INFO
    EDR Plugin Log Contains   Logger edr configured for level: DEBUG

EDR clears jrl files when scheduled queries are disabled
    Check EDR Plugin Installed With Base
    Apply Live Query Policy And Wait For Query Pack Changes  ${EXAMPLE_DATA_PATH}/LiveQuery_policy_enabled.xml
    Create File  ${SOPHOS_INSTALL}/plugins/edr/var/jrl/queryid
    Wait Until Keyword Succeeds
        ...  15 secs
        ...  1 secs
        ...  EDR Plugin Log Contains   Updating running_mode flag settings to: 1
    File Should exist  ${SOPHOS_INSTALL}/plugins/edr/var/jrl/queryid
    Apply Live Query Policy And Wait For Query Pack Changes  ${EXAMPLE_DATA_PATH}/LiveQuery_policy_disabled.xml

    Wait Until Keyword Succeeds
        ...  15 secs
        ...  1 secs
        ...  EDR Plugin Log Contains   Updating running_mode flag settings to: 0
    File Should not exist  ${SOPHOS_INSTALL}/plugins/edr/var/jrl/queryid

EDR Recovers From Incomplete Database Purge
    Install With Base SDDS
    Check EDR Plugin Installed With Base
    Apply Live Query Policy And Wait For Query Pack Changes  ${EXAMPLE_DATA_PATH}/LiveQuery_policy_enabled.xml

    Wait Until Keyword Succeeds
    ...  100 secs
    ...  5 secs
    ...  Number Of SST Database Files Is Greater Than  0

    ${canary_file}=  Set Variable  ${COMPONENT_ROOT_PATH}/var/osquery.db/file_should_be_deleted
    Create File  ${canary_file}  foo

    Stop EDR
    Create Debug Level Logger Config File
    Corrupt OSQuery Database
    Start EDR

    Wait Until Keyword Succeeds
    ...  120 secs
    ...  2 secs
    ...  File Log Contains    ${SOPHOS_INSTALL}/plugins/edr/log/edr_osquery.log    Destroying RocksDB database due to corruption

    Should Not Exist  ${canary_file}
    ${edrMark} =  Mark File  ${EDR_LOG_PATH}

    # Perform a query to make sure that osquery is now working
    Wait Until Keyword Succeeds
    ...  120 secs
    ...  5 secs
    ...  Check Simple Query Works

    Marked File Does Not Contain    ${EDR_LOG_PATH}   Osquery health check failed: write() send(): Broken pipe  ${edrMark}

OSQuery Database is Purged When Previously Installed EDR Version is Below 1.1.2
    Check EDR Plugin Installed With Base
    Wait Until Keyword Succeeds
    ...  100 secs
    ...  5 secs
    ...  Number Of SST Database Files Is Greater Than  0

    ${canary_file}=  Set Variable  ${COMPONENT_ROOT_PATH}/var/osquery.db/file_should_be_deleted
    Create File  ${canary_file}  foo

    Stop EDR
    Remove File  ${COMPONENT_ROOT_PATH}/VERSION.ini
    Create File  ${COMPONENT_ROOT_PATH}/VERSION.ini  PRODUCT_NAME = SPL-Endpoint-Detection-and-Response-Plugin\nPRODUCT_VERSION = 1.1.1.1\nBUILD_DATE = 2021-05-21\nCOMMIT_HASH = bef8c41c4f3a8cd0458f91bed3a076e81428e394\nPLUGIN_API_COMMIT_HASH = 93b8ec8736dcb5b4266f85b1b08110ebe19c7f03
    Create Debug Level Logger Config File
    Corrupt OSQuery Database
    Start EDR

    # Run Installer which has the work around in that will purge the osquery database on an upgrade from a version
    # older than 1.1.2
    ${result} =   Run Process  bash ${EDR_SDDS}/install.sh   shell=True   timeout=120s
    Should Be Equal As Integers  ${result.rc}  0   "Failed to re-run edr installer.\nstdout: \n${result.stdout}\n. stderr: \n{result.stderr}"
    Log  ${result.stdout}
    Log  ${result.stderr}
    Should Not Exist  ${canary_file}

EDR Plugin Can Run Queries For Event Journal Detection Table
    Check EDR Plugin Installed With Base

    Wait Until Keyword Succeeds
    ...  15 secs
    ...  1 secs
    ...  Check Osquery Running

    Run Process  mkdir  -p  ${SOPHOS_INSTALL}/plugins/eventjournaler/data/event-journals/test1
    Run Process  cp  -r  ${EXAMPLE_DATA_PATH}/TestEventJournalFiles/event_journal  ${SOPHOS_INSTALL}/plugins/eventjournaler/data/eventjournals/
    Run Process  chown  -R  sophos-spl-user:sophos-spl-group  ${SOPHOS_INSTALL}/plugins/eventjournaler/

    # Perform a query to make sure that osquery is now working
    Wait Until Keyword Succeeds
    ...  120 secs
    ...  5 secs
    ...  Check Sophos Detections Journal Queries Work

EDR Plugin Can Run Queries For Event Journal Detection Table And Create Jrl
    # Need to make sure test starts of fresh
    Reinstall With Base
    Check EDR Plugin Installed With Base
    Apply Live Query Policy And Wait For Query Pack Changes  ${EXAMPLE_DATA_PATH}/LiveQuery_policy_enabled.xml

    Wait Until Keyword Succeeds
    ...  100 secs
    ...  5 secs
    ...  Number Of SST Database Files Is Greater Than  0

    Run Process  mkdir  -p  ${SOPHOS_INSTALL}/plugins/eventjournaler/data/event-journals/test1
    Run Process  cp  -r  ${EXAMPLE_DATA_PATH}/TestEventJournalFiles/event_journal  ${SOPHOS_INSTALL}/plugins/eventjournaler/data/eventjournals/
    Run Process  chown  -R  sophos-spl-user:sophos-spl-group  ${SOPHOS_INSTALL}/plugins/eventjournaler/

    # Perform a query to make sure that osquery is now working
    Wait Until Keyword Succeeds
    ...  120 secs
    ...  5 secs
    ...  Check Sophos Detections Journal Queries Work With Query Id

    File Should Exist  ${SOPHOS_INSTALL}/plugins/edr/var/jrl/test_query1

EDR Plugin Returns Query Error If Event Journal Contains Too Many Detections
    # Need to make sure test starts of fresh
    Reinstall With Base
    Check EDR Plugin Installed With Base
    Apply Live Query Policy And Wait For Query Pack Changes  ${EXAMPLE_DATA_PATH}/LiveQuery_policy_enabled.xml

    Wait Until Keyword Succeeds
    ...  100 secs
    ...  5 secs
    ...  Number Of SST Database Files Is Greater Than  0

    Run Process  mkdir  -p  ${SOPHOS_INSTALL}/plugins/eventjournaler/data/eventjournals/SophosSPL/Detections
    Run Process  cp  -r  ${EXAMPLE_DATA_PATH}/TestEventJournalFiles/Detections-0000000000000001-0000000000001e00-132766178770000000-132766182670000000.xz  ${SOPHOS_INSTALL}/plugins/eventjournaler/data/eventjournals/SophosSPL/Detections
    Run Process  chown  -R  sophos-spl-user:sophos-spl-group  ${SOPHOS_INSTALL}/plugins/eventjournaler/

    Wait Until Keyword Succeeds
    ...  120 secs
    ...  5 secs
    ...  Check Sophos Detections Journal Queries Return Maximum Exceeded Error

EDR Plugin Can Run Event Journal Scheduled Queries And Create Jrl When Data Is Greater Than Maximum Entries
    # Need to make sure test starts of fresh
    Reinstall With Base
    Check EDR Plugin Installed With Base
    Apply Live Query Policy And Wait For Query Pack Changes  ${EXAMPLE_DATA_PATH}/LiveQuery_policy_enabled.xml

    Wait Until Keyword Succeeds
    ...  100 secs
    ...  5 secs
    ...  Number Of SST Database Files Is Greater Than  0

    Run Process  mkdir  -p  ${SOPHOS_INSTALL}/plugins/eventjournaler/data/eventjournals/SophosSPL/Detections
    Run Process  cp  -r  ${EXAMPLE_DATA_PATH}/TestEventJournalFiles/Detections-0000000000000001-0000000000001e00-132766178770000000-132766182670000000.xz  ${SOPHOS_INSTALL}/plugins/eventjournaler/data/eventjournals/SophosSPL/Detections
    Run Process  chown  -R  sophos-spl-user:sophos-spl-group  ${SOPHOS_INSTALL}/plugins/eventjournaler/

    Wait Until Keyword Succeeds
    ...  120 secs
    ...  5 secs
    ...  Check Sophos Detections Journal Queries Work With Query Id And Time Greater Than  0  2500

    File Should Exist  ${SOPHOS_INSTALL}/plugins/edr/var/jrl/test_query1
    File Should Not Be Empty  ${SOPHOS_INSTALL}/plugins/edr/var/jrl/test_query1
    ${jrl}=  Get File  ${SOPHOS_INSTALL}/plugins/edr/var/jrl/test_query1
    Should Contain  ${jrl}  Detections-0000000000000001-0000000000001e00-132766178770000000-132766182670000000.xz
    File Should Exist  ${SOPHOS_INSTALL}/plugins/edr/var/jrl_tracker/test_query1
    ${contents}=  Get File  ${SOPHOS_INSTALL}/plugins/edr/var/jrl_tracker/test_query1
    Should be equal as Strings  ${contents}   1

    Wait Until Keyword Succeeds
    ...  120 secs
    ...  5 secs
    ...  Check Sophos Detections Journal Queries Work With Query Id And Time Greater Than  0  2500

    File Should Exist  ${SOPHOS_INSTALL}/plugins/edr/var/jrl/test_query1
    File Should Not Be Empty  ${SOPHOS_INSTALL}/plugins/edr/var/jrl/test_query1
    ${jrl}=  Get File  ${SOPHOS_INSTALL}/plugins/edr/var/jrl/test_query1
    Should Contain  ${jrl}  Detections-0000000000000001-0000000000001e00-132766178770000000-132766182670000000.xz

EDR Plugin Can Run Queries For Event Journal Detection Table With Start Time
    # Need to make sure test starts of fresh
    Reinstall With Base
    Check EDR Plugin Installed With Base
    Apply Live Query Policy And Wait For Query Pack Changes  ${EXAMPLE_DATA_PATH}/LiveQuery_policy_enabled.xml

    Wait Until Keyword Succeeds
    ...  100 secs
    ...  5 secs
    ...  Number Of SST Database Files Is Greater Than  0

    Run Process  mkdir  -p  ${SOPHOS_INSTALL}/plugins/eventjournaler/data/eventjournals/SophosSPL/Detections
    Run Process  cp  -r  ${EXAMPLE_DATA_PATH}/TestEventJournalFiles/Detections-0000000000000001-0000000000001e00-132766178770000000-132766182670000000.xz  ${SOPHOS_INSTALL}/plugins/eventjournaler/data/eventjournals/SophosSPL/Detections
    Run Process  chown  -R  sophos-spl-user:sophos-spl-group  ${SOPHOS_INSTALL}/plugins/eventjournaler/

    Wait Until Keyword Succeeds
    ...  120 secs
    ...  5 secs
    ...  Check Sophos Detections Journal Queries Work With Time Greater Than  1632144665  32

EDR Plugin Can Run Queries For Event Journal Detection Table With End Time
    # Need to make sure test starts of fresh
    Reinstall With Base
    Check EDR Plugin Installed With Base
    Apply Live Query Policy And Wait For Query Pack Changes  ${EXAMPLE_DATA_PATH}/LiveQuery_policy_enabled.xml

    Wait Until Keyword Succeeds
    ...  100 secs
    ...  5 secs
    ...  Number Of SST Database Files Is Greater Than  0

    Run Process  mkdir  -p  ${SOPHOS_INSTALL}/plugins/eventjournaler/data/eventjournals/SophosSPL/Detections
    Run Process  cp  -r  ${EXAMPLE_DATA_PATH}/TestEventJournalFiles/Detections-0000000000000001-0000000000001e00-132766178770000000-132766182670000000.xz  ${SOPHOS_INSTALL}/plugins/eventjournaler/data/eventjournals/SophosSPL/Detections
    Run Process  chown  -R  sophos-spl-user:sophos-spl-group  ${SOPHOS_INSTALL}/plugins/eventjournaler/

    Wait Until Keyword Succeeds
    ...  120 secs
    ...  5 secs
    ...  Check Sophos Detections Journal Queries Work With Time Less Than  1632144278  12

EDR Plugin Can Run Queries For Event Journal Detection Table With Start Time And Create Jrl
    # Need to make sure test starts of fresh
    Reinstall With Base
    Check EDR Plugin Installed With Base
    Apply Live Query Policy And Wait For Query Pack Changes  ${EXAMPLE_DATA_PATH}/LiveQuery_policy_enabled.xml

    Wait Until Keyword Succeeds
    ...  100 secs
    ...  5 secs
    ...  Number Of SST Database Files Is Greater Than  0

    Run Process  mkdir  -p  ${SOPHOS_INSTALL}/plugins/eventjournaler/data/eventjournals/SophosSPL/Detections
    Run Process  cp  -r  ${EXAMPLE_DATA_PATH}/TestEventJournalFiles/Detections-0000000000000001-0000000000001e00-132766178770000000-132766182670000000.xz  ${SOPHOS_INSTALL}/plugins/eventjournaler/data/eventjournals/SophosSPL/Detections
    Run Process  chown  -R  sophos-spl-user:sophos-spl-group  ${SOPHOS_INSTALL}/plugins/eventjournaler/

    Wait Until Keyword Succeeds
    ...  120 secs
    ...  5 secs
    ...  Check Sophos Detections Journal Queries Work With Query Id And Time Greater Than  1632144665  32

    File Should Exist  ${SOPHOS_INSTALL}/plugins/edr/var/jrl/test_query1

EDR Plugin Can Run Queries For Event Journal Detection Table With End Time And Create Jrl
    # Need to make sure test starts of fresh
    Reinstall With Base
    Check EDR Plugin Installed With Base
    Apply Live Query Policy And Wait For Query Pack Changes  ${EXAMPLE_DATA_PATH}/LiveQuery_policy_enabled.xml

    Wait Until Keyword Succeeds
    ...  100 secs
    ...  5 secs
    ...  Number Of SST Database Files Is Greater Than  0

    Run Process  mkdir  -p  ${SOPHOS_INSTALL}/plugins/eventjournaler/data/eventjournals/SophosSPL/Detections
    Run Process  cp  -r  ${EXAMPLE_DATA_PATH}/TestEventJournalFiles/Detections-0000000000000001-0000000000001e00-132766178770000000-132766182670000000.xz  ${SOPHOS_INSTALL}/plugins/eventjournaler/data/eventjournals/SophosSPL/Detections
    Run Process  chown  -R  sophos-spl-user:sophos-spl-group  ${SOPHOS_INSTALL}/plugins/eventjournaler/

    Wait Until Keyword Succeeds
    ...  120 secs
    ...  5 secs
    ...  Check Sophos Detections Journal Queries Work With Query Id And Time Less Than  1632144278  12

    File Should Exist  ${SOPHOS_INSTALL}/plugins/edr/var/jrl/test_query1


EDR Plugin Stops Without Errors
    Check EDR Plugin Installed With Base
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  1 secs
    ...  Check Osquery Running
    Stop EDR
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  1 secs
    ...  EDR Plugin Log Contains      edr <> Plugin Finished
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  1 secs
    ...  Check EDR Executable Not Running
    Check Osquery Not Running
    EDR Plugin Log Does Not Contain  ERROR [
    EDR Plugin Log Does Not Contain  WARN [
    EDR Plugin Log Does Not Contain  Operation canceled


*** Keywords ***
Reinstall With Base
    Uninstall All
    Install With Base SDDS

Check Simple Query Works
    ${response} =  Run Live Query and Return Result  SELECT * from uptime
    Should Contain  ${response}  "errorCode":0,"errorMessage":"OK"

Check Sophos Detections Journal Queries Work
        ${response} =  Run Live Query and Return Result  SELECT * from sophos_detections_journal WHERE time > 0
        Should Contain  ${response}  "errorCode":0,"errorMessage":"OK"

Check Sophos Detections Journal Queries Work With Time Greater Than
        [Arguments]  ${start_time}  ${expected_count}
        ${response} =  Run Live Query and Return Result  SELECT * from sophos_detections_journal WHERE time > ${start_time}
        Should Contain  ${response}  "errorCode":0,"errorMessage":"OK","rows":${expected_count}

Check Sophos Detections Journal Queries Work With Time Less Than
        [Arguments]  ${end_time}  ${expected_count}
        ${response} =  Run Live Query and Return Result  SELECT * from sophos_detections_journal WHERE time < ${end_time}
        Should Contain  ${response}  "errorCode":0,"errorMessage":"OK","rows":${expected_count}

Check Sophos Detections Journal Queries Work With Query Id
        ${response} =  Run Live Query and Return Result  SELECT * from sophos_detections_journal WHERE time > 0 AND query_id = 'test_query1'
        Should Contain  ${response}  "errorCode":0,"errorMessage":"OK"

Check Sophos Detections Journal Queries Work With Query Id And Time Greater Than
        [Arguments]  ${start_time}  ${expected_count}
        ${response} =  Run Live Query and Return Result  SELECT * from sophos_detections_journal WHERE time > ${start_time} AND query_id = 'test_query1'
        Should Contain  ${response}  "errorCode":0,"errorMessage":"OK","rows":${expected_count}

Check Sophos Detections Journal Queries Work With Query Id And Time Less Than
        [Arguments]  ${end_time}  ${expected_count}
        ${response} =  Run Live Query and Return Result  SELECT * from sophos_detections_journal WHERE time < ${end_time} AND query_id = 'test_query1'
        Should Contain  ${response}  "errorCode":0,"errorMessage":"OK","rows":${expected_count}

Check Sophos Detections Journal Queries Return Maximum Exceeded Error
        ${response} =  Run Live Query and Return Result  SELECT * from sophos_detections_journal WHERE time > 0
        Should Contain  ${response}  "errorCode":1,"errorMessage":"Query returned too many events, please restrict search period to a shorter time period"

Number Of SST Database Files Is Greater Than
    [Arguments]  ${min_sst_files_for_test}
    ${sst_file_count}=  Count Files In Directory  ${COMPONENT_ROOT_PATH}/var/osquery.db  *.sst
    log to console  ${sst_file_count}
    Should Be True 	${sst_file_count} > ${min_sst_files_for_test}

Run Non-UTF8 Query
    ${result} =  Run Process  ${COMPONENT_ROOT_PATH}/extensions/LiveQueryReport  ${COMPONENT_ROOT_PATH}/var/osquery.sock  select '1\xfffd' as h;  shell=true

    Log  ${result.stdout}
    Log  ${result.stderr}
    Should Be Equal As Integers  ${result.rc}  0

    Should Contain  ${result.stdout}   "errorCode": 0

Check Rsyslog Started Without Error
    ${result} =  Run Process  systemctl  status  rsyslog
    Log  ${result.stdout}
    Log  ${result.stderr}
    Should Contain  ${result.stdout}  active (running)
    Should Not Contain  ${result.stdout}  Could not open output pipe '/opt/sophos-spl/shared/syslog_pipe'

Corrupt OSQuery Database
    ${sstFiles}=  List Files In Directory  ${COMPONENT_ROOT_PATH}/var/osquery.db  *.sst
    Remove File  ${COMPONENT_ROOT_PATH}/var/osquery.db/${sstFiles[0]}
    should not exist  ${COMPONENT_ROOT_PATH}/var/osquery.db/${sstFiles[0]}