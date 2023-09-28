*** Settings ***
Documentation    Suite description

Library         Process
Library         OperatingSystem
Library         ../Libs/FileSystemLibs.py
Library         ${COMMON_TEST_LIBS}/LogUtils.py
Library         ../Libs/rsyslogUtils.py

Resource        EDRResources.robot
Resource        ComponentSetup.robot

Suite Setup     Install With Base SDDS
Suite Teardown  Uninstall All

Test Setup      No Operation
Test Teardown   EDR And Base Teardown

Default Tags    TAP_PARALLEL2

*** Test Cases ***
LiveQuery is Distributed to EDR Plugin and Its Answer is available to MCSRouter
    Check EDR Plugin Installed With Base
    Simulate Live Query  RequestProcesses.json
    Wait Until Created    ${SOPHOS_INSTALL}/base/mcs/response/LiveQuery_123-4_response.json
    ...  15 secs

LiveQuery Response is Chowned to Sophos Spl Local on EDR Startup
    Check EDR Plugin Installed With Base
    Create File  ${SOPHOS_INSTALL}/base/mcs/response/LiveQuery_567-8_response.json
    Stop EDR
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
    [Tags]  rsyslog
    require_rsyslog
    Directory Should Exist    /etc/rsyslog.d
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
    ${is_sles12} =      Does File Contain Word  /etc/os-release  SUSE Linux Enterprise Server 12
    Pass Execution If  ${is_sles12}  LINUXDAR-7106 - test broken on SLES12 - wont be fixed
    Check EDR Plugin Installed With Base

    Remove File  ${SOPHOS_INSTALL}/plugins/edr/bin/sophos_livequery
    create File  ${SOPHOS_INSTALL}/plugins/edr/bin/sophos_livequery    \#!/bin/bash\nsleep 100000
    Log File  ${SOPHOS_INSTALL}/plugins/edr/bin/sophos_livequery
    Run Process  chmod  +x   ${SOPHOS_INSTALL}/plugins/edr/bin/sophos_livequery
    Create File  ${SOPHOS_INSTALL}/base/etc/logger.conf  [global]\nVERBOSITY = DEBUG\n
    Restart EDR

    ${actionContent} =  Set Variable  '{"type": "sophos.mgt.action.RunLiveQuery", "name": "test_query", "query": "select * from users;"}'


    ${mark} =   Mark Log Size   ${EDR_LOG_PATH}
    Send Plugin Actions  edr  LiveQuery  corr  ${actionContent}  ${100}

    Wait For Log Contains From Mark    ${mark}    Early request to stop found.       ${60}
    Wait For Log Contains From Mark    ${mark}    Completed initialisation of EDR    ${240}

    # LINUXDAR-8109 - Until fixed it is possible that we lose a couple of actions while EDR is restarting
    Wait For Log Contains N Times From Mark   ${mark}   Received new Action   ${95}


EDR Plugin Can Have Logging Level Changed Individually
    Check EDR Plugin Installed With Base
    #setting edr_osquery to DEBUG provides all DEBUG logging (these can be hidden by specifying the component with INFO level)
    Create File         ${SOPHOS_INSTALL}/base/etc/logger.conf.local   [edr_osquery]\nVERBOSITY=DEBUG\n
    ${mark} =   Mark Log Size   ${EDR_LOG_PATH}
    Restart EDR
    Wait For Log Contains From Mark    ${mark}    Logger edr_osquery configured for level: DEBUG  ${15}

EDR Plugin Can Have Logging Level Changed Based On Components
    Check EDR Plugin Installed With Base
    #With edr_osquery being set to INFO, the other sub-components can be set to DEBUG and have the specified logging level
    Create File         ${SOPHOS_INSTALL}/base/etc/logger.conf.local   [edr_osquery]\nVERBOSITY=INFO\n[edr]\nVERBOSITY=DEBUG\n
    ${mark} =   Mark Log Size   ${EDR_LOG_PATH}
    Restart EDR
    Wait For Log Contains From Mark    ${mark}    Logger edr_osquery configured for level: INFO  ${15}
    Wait For Log Contains From Mark    ${mark}    Logger edr configured for level: DEBUG  ${1}

EDR clears jrl files when scheduled queries are disabled
    Check EDR Plugin Installed With Base
    ${mark} =   Mark Log Size   ${EDR_LOG_PATH}
    Apply Live Query Policy And Wait For Query Pack Changes  ${EXAMPLE_DATA_PATH}/LiveQuery_policy_enabled.xml
    Create File  ${SOPHOS_INSTALL}/plugins/edr/var/jrl/queryid
    Wait For Log Contains From Mark    ${mark}    Updating running_mode flag settings to: 1  ${15}
    File Should exist  ${SOPHOS_INSTALL}/plugins/edr/var/jrl/queryid
    ${mark} =   Mark Log Size   ${EDR_LOG_PATH}
    Apply Live Query Policy And Wait For Query Pack Changes  ${EXAMPLE_DATA_PATH}/LiveQuery_policy_disabled.xml

    Wait For Log Contains From Mark    ${mark}    Updating running_mode flag settings to: 0  ${15}
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
    ${osquery_mark} =   Mark Log Size   ${SOPHOS_INSTALL}/plugins/edr/log/edr_osquery.log
    Start EDR

    Wait For Log Contains From Mark    ${osquery_mark}  Destroying RocksDB database due to corruption  ${120}

    Should Not Exist  ${canary_file}
    ${edrMark} =  Mark Log Size  ${EDR_LOG_PATH}

    # Perform a query to make sure that osquery is now working
    Wait Until Keyword Succeeds
    ...  120 secs
    ...  5 secs
    ...  Check Simple Query Works

    check_log_does_not_contain_after_mark  ${EDR_LOG_PATH}   Osquery health check failed: write() send(): Broken pipe  ${edrMark}

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
    File Should Not Be Empty  ${SOPHOS_INSTALL}/plugins/edr/var/jrl/test_query1
    ${jrl1}=  Get File  ${SOPHOS_INSTALL}/plugins/edr/var/jrl/test_query1

    # Perform a second query to make sure the last JRL is stored
    Wait Until Keyword Succeeds
    ...  120 secs
    ...  5 secs
    ...  Check Sophos Detections Journal Queries Work With Query Id

    File Should Exist  ${SOPHOS_INSTALL}/plugins/edr/var/jrl/test_query1
    File Should Not Be Empty  ${SOPHOS_INSTALL}/plugins/edr/var/jrl/test_query1
    ${jrl2}=  Get File  ${SOPHOS_INSTALL}/plugins/edr/var/jrl/test_query1

    Should Be Equal As Strings  ${jrl1}  ${jrl2}


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
    Run Process  cp  -r  ${EXAMPLE_DATA_PATH}/TestEventJournalFiles/Detections-00000000000a1115-00000000000a3be7-133288204220000000-133288222940000000.xz  ${SOPHOS_INSTALL}/plugins/eventjournaler/data/eventjournals/SophosSPL/Detections
    Run Process  chown  -R  sophos-spl-user:sophos-spl-group  ${SOPHOS_INSTALL}/plugins/eventjournaler/

    Wait Until Keyword Succeeds
    ...  120 secs
    ...  5 secs
    ...  Check Sophos Detections Journal Queries Return Maximum Exceeded Error

EDR Plugin Can Run Event Journal Scheduled Queries And Create Jrl When Data Is Greater Than Maximum Entries
    [Documentation]  This test puts a large journal (~90MB) into place and then queries it to make sure that
    ...    when we hit the journal reading limit that we create a JRL file to indicate where we read up to and
    ...    that we move the JRL onwards each query. A tracker file is also created to keep track of
    ...    how many times we've tried to read too many results, which should be removed once we start getting back
    ...    fewer results than the journal reader limit - i.e. stop hitting the limit.
    ...    We limit the query results to be 10 in this test because there is a smaller (10MB) limit in the response
    ...    size we send to Central compared to the larger (50MB) limit of data that we send to osquery.
    ...    Osquery can filter down that 50MB of data based on the query.
    ...    E.g. with a limit of 10 rows here, EDR will send 50MB of data to osquery
    ...    and then osquery will take only 10 rows to send to Central, this means we can test the journal read
    ...    limit without the Central response limit interfering with the test.

    ${expectedNumEvents} =  Set Variable  ${10}

    # Need to make sure test starts off fresh
    Reinstall With Base
    Check EDR Plugin Installed With Base
    Apply Live Query Policy And Wait For Query Pack Changes  ${EXAMPLE_DATA_PATH}/LiveQuery_policy_enabled.xml

    Wait Until Keyword Succeeds
    ...  100 secs
    ...  5 secs
    ...  Number Of SST Database Files Is Greater Than  0

    Run Process  mkdir  -p  ${SOPHOS_INSTALL}/plugins/eventjournaler/data/eventjournals/SophosSPL/Detections
    Run Process  cp  -r  ${EXAMPLE_DATA_PATH}/TestEventJournalFiles/Detections-00000000000a1115-00000000000a3be7-133288204220000000-133288222940000000.xz  ${SOPHOS_INSTALL}/plugins/eventjournaler/data/eventjournals/SophosSPL/Detections
    Run Process  chown  -R  sophos-spl-user:sophos-spl-group  ${SOPHOS_INSTALL}/plugins/eventjournaler/

    Wait Until Keyword Succeeds
    ...  120 secs
    ...  5 secs
    ...  Check Sophos Detections Journal Queries Work With Query Id And Time Greater Than With Limit    0    ${expectedNumEvents}    ${expectedNumEvents}

    Wait Until Created  ${SOPHOS_INSTALL}/plugins/edr/var/jrl/test_query1    5s
    File Should Not Be Empty  ${SOPHOS_INSTALL}/plugins/edr/var/jrl/test_query1
    ${jrl}=  Get File  ${SOPHOS_INSTALL}/plugins/edr/var/jrl/test_query1
    log    ${jrl}
    Should Contain  ${jrl}  Detections-00000000000a1115-00000000000a3be7-133288204220000000-133288222940000000.xz
    Should Contain  ${jrl}    position="50130696"
    Wait Until Created  ${SOPHOS_INSTALL}/plugins/edr/var/jrl_tracker/test_query1    5s
    ${contents}=  Get File  ${SOPHOS_INSTALL}/plugins/edr/var/jrl_tracker/test_query1
    log    ${contents}
    Should be equal as Strings  ${contents}   1

    Wait Until Keyword Succeeds
    ...  120 secs
    ...  5 secs
    ...  Check Sophos Detections Journal Queries Work With Query Id And Time Greater Than With Limit    0    ${expectedNumEvents}    ${expectedNumEvents}

    File Should Exist  ${SOPHOS_INSTALL}/plugins/edr/var/jrl/test_query1
    File Should Not Be Empty  ${SOPHOS_INSTALL}/plugins/edr/var/jrl/test_query1
    ${jrl}=  Get File  ${SOPHOS_INSTALL}/plugins/edr/var/jrl/test_query1
    log    ${jrl}
    Should Contain  ${jrl}  Detections-00000000000a1115-00000000000a3be7-133288204220000000-133288222940000000.xz
    Should Contain  ${jrl}  position="99993176"
    # We should no longer be hitting the limit once we have read the first 50MB of the file. The next query would
    # have read less than that and then removed the tracker as it is no longer returning too many results.
    Should Not Exist    ${SOPHOS_INSTALL}/plugins/edr/var/jrl_tracker/test_query1

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
    [Teardown]  EDR And Base Teardown No Stop

    Check EDR Plugin Installed With Base
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  1 secs
    ...  Check Osquery Running
    Sleep    ${1}    Sleep to allow EDR to settle before stopping it

    Stop EDR  ${30}
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  1 secs
    ...  Check EDR Executable Not Running
    Check Osquery Not Running
    EDR Plugin Log Does Not Contain  ERROR [
    EDR Plugin Log Does Not Contain  WARN [
    EDR Plugin Log Does Not Contain  Operation canceled

EDR Plugin Can clean up old osquery info and warning files
    Check EDR Plugin Installed With Base
    Stop EDR
    Create File  ${EDR_LOG_DIR}/osqueryd.INFO.20200117-042121.1001
    Create File  ${EDR_LOG_DIR}/osqueryd.INFO.20200117-042121.1002
    Create File  ${EDR_LOG_DIR}/osqueryd.INFO.20200117-042121.1003
    Create File  ${EDR_LOG_DIR}/osqueryd.INFO.20200117-042121.1004
    Create File  ${EDR_LOG_DIR}/osqueryd.INFO.20200117-042121.1005
    Create File  ${EDR_LOG_DIR}/osqueryd.INFO.20200117-042121.1006
    Create File  ${EDR_LOG_DIR}/osqueryd.INFO.20200117-042121.1007
    Create File  ${EDR_LOG_DIR}/osqueryd.INFO.20200117-042121.1008
    Create File  ${EDR_LOG_DIR}/osqueryd.INFO.20200117-042121.1009
    Create File  ${EDR_LOG_DIR}/osqueryd.INFO.20200117-042121.1010
    Create File  ${EDR_LOG_DIR}/osqueryd.INFO.20200117-042121.1011

    Create File  ${EDR_LOG_DIR}/osqueryd.WARNING.20200117-042121.1001
    Create File  ${EDR_LOG_DIR}/osqueryd.WARNING.20200117-042121.1002
    Create File  ${EDR_LOG_DIR}/osqueryd.WARNING.20200117-042121.1003
    Create File  ${EDR_LOG_DIR}/osqueryd.WARNING.20200117-042121.1004
    Create File  ${EDR_LOG_DIR}/osqueryd.WARNING.20200117-042121.1005
    Create File  ${EDR_LOG_DIR}/osqueryd.WARNING.20200117-042121.1006
    Create File  ${EDR_LOG_DIR}/osqueryd.WARNING.20200117-042121.1007
    Create File  ${EDR_LOG_DIR}/osqueryd.WARNING.20200117-042121.1008
    Create File  ${EDR_LOG_DIR}/osqueryd.WARNING.20200117-042121.1009
    Create File  ${EDR_LOG_DIR}/osqueryd.WARNING.20200117-042121.1010
    Create File  ${EDR_LOG_DIR}/osqueryd.WARNING.20200117-042121.1011

    ${mark} =  Mark Log Size  ${EDR_LOG_PATH}
    Start EDR
    Wait For Log Contains From Mark    ${mark}    Removed old osquery WARNING file:  ${30}
    Wait For Log Contains From Mark    ${mark}    Removed old osquery INFO file:  ${30}

sophos_endpoint_info Has installed_versions Field Which Has Base And Edr Version
    Check EDR Plugin Installed With Base
    ${response} =  Run Live Query and Return Result  SELECT installed_versions FROM sophos_endpoint_info
    Should Contain  ${response}  "errorCode":0,"errorMessage":"OK"
    Should Contain  ${response}  [{\\"Name\\":\\"SPL-Base-Component\\",\\"installed_version\\":\\"1.2
    Should Contain  ${response}  ,{\\"Name\\":\\"SPL-Endpoint-Detection-and-Response-Plugin\\",\\"installed_version\\":\\"1.1

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

Check Sophos Detections Journal Queries Work With Query Id And Time Greater Than With Limit
        [Arguments]  ${start_time}    ${expected_count}    ${limit}
        ${response} =  Run Live Query and Return Result  SELECT * from sophos_detections_journal WHERE time > ${start_time} AND query_id = 'test_query1' limit ${limit}
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
