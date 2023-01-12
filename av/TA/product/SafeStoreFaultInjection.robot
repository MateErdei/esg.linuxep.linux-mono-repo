*** Settings ***
Documentation   Product tests for SafeStore
Force Tags      PRODUCT  SAFESTORE

Resource    ../shared/ComponentSetup.robot
Resource    ../shared/AVResources.robot
Resource    ../shared/SafeStoreResources.robot

Test Setup     SafeStore Test Setup
Test Teardown  SafeStore Test Teardown

*** Keywords ***

SafeStore Test Setup
    Component Test Setup
    Start SafeStore Manually
    Wait Until SafeStore Started Successfully

SafeStore Test Teardown
    Stop SafeStore Manually
    Remove Directory    ${SAFESTORE_DB_DIR}  recursive=True
    List AV Plugin Path
    run teardown functions
    Check All Product Logs Do Not Contain Error
    Dump and Reset Logs
    Component Test TearDown

Dump and Reset Logs
    Dump log  ${SAFESTORE_LOG_PATH}
    Remove File  ${SAFESTORE_LOG_PATH}*

*** Test Cases ***
Send A Valid Threat Detected Object To Safestore
    Create File  /tmp/testfile
    ${result} =  Run Shell Process  ${SEND_THREAT_DETECTED_TOOL} --socketpath /opt/sophos-spl/plugins/av/var/safestore_socket --filepath /tmp/testfile --threatname threatName --sha e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855    OnError=Failed to run SendThreatDetectedEvent binary   timeout=10
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  SafeStore Log Contains  Quarantined /tmp/testfile successfully

Send Only Message To Safestore
    Create File  /tmp/testfile
    ${result} =  Run Shell Process  ${SEND_THREAT_DETECTED_TOOL} --socketpath /opt/sophos-spl/plugins/av/var/safestore_socket --filepath /tmp/testfile --threatname threatName --sha e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855 --nosendfd    OnError=Failed to run SendThreatDetectedEvent binary   timeout=10
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  SafeStore Log Contains  Aborting SafeStore connection thread: failed to read fd
    mark_expected_error_in_log  ${SAFESTORE_LOG_PATH}  Aborting SafeStore connection thread: failed to read fd

Send Only FD To Safestore
    Create File  /tmp/testfile
    ${result} =  Run Shell Process  ${SEND_THREAT_DETECTED_TOOL} --socketpath /opt/sophos-spl/plugins/av/var/safestore_socket --filepath /tmp/testfile --nosendmessage    OnError=Failed to run SendThreatDetectedEvent binary   timeout=10
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  SafeStore Log Contains  Ignoring length of zero / No new messages

Send Short SHA To Safestore
    Create File  /tmp/testfile
    ${result} =  Run Shell Process  ${SEND_THREAT_DETECTED_TOOL} --socketpath /opt/sophos-spl/plugins/av/var/safestore_socket --filepath /tmp/testfile --threatname threatName --sha e3b0c44298fc1c149afbf4c8996f    OnError=Failed to run SendThreatDetectedEvent binary   timeout=10
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  SafeStore Log Contains  Quarantined /tmp/testfile successfully


Send Long SHA To Safestore
    Create File  /tmp/testfile
    ${result} =  Run Shell Process  ${SEND_THREAT_DETECTED_TOOL} --socketpath /opt/sophos-spl/plugins/av/var/safestore_socket --filepath /tmp/testfile --threatname threatName --sha e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b85555    OnError=Failed to run SendThreatDetectedEvent binary   timeout=10
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  SafeStore Log Contains  Quarantined /tmp/testfile successfully

Send invalid SHA with captial letters To Safestore
    Create File  /tmp/testfile
    ${result} =  Run Shell Process  ${SEND_THREAT_DETECTED_TOOL} --socketpath /opt/sophos-spl/plugins/av/var/safestore_socket --filepath /tmp/testfile --threatname threatName --sha E3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855    OnError=Failed to run SendThreatDetectedEvent binary   timeout=10
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  SafeStore Log Contains  Quarantined /tmp/testfile successfully

Send SHA that is in json format To Safestore
    Create File  /tmp/testfile
    ${result} =  Run Shell Process  ${SEND_THREAT_DETECTED_TOOL} --socketpath /opt/sophos-spl/plugins/av/var/safestore_socket --filepath /tmp/testfile --threatname threatName --sha {"hellow":1000}    OnError=Failed to run SendThreatDetectedEvent binary   timeout=10
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  SafeStore Log Contains  Quarantined /tmp/testfile successfully

Send SHA that is in xml format To Safestore
    Create File  /tmp/testfile
    ${result} =  Run Shell Process  ${SEND_THREAT_DETECTED_TOOL} --socketpath /opt/sophos-spl/plugins/av/var/safestore_socket --filepath /tmp/testfile --threatname threatName --sha <xmltag>aaaa<\xmltag>    OnError=Failed to run SendThreatDetectedEvent binary   timeout=10
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  SafeStore Log Contains  Quarantined /tmp/testfile successfully

Send TDO with empty SHA To Safestore
    Create File  /tmp/testfile
    ${result} =  Run Shell Process  ${SEND_THREAT_DETECTED_TOOL} --socketpath /opt/sophos-spl/plugins/av/var/safestore_socket --filepath /tmp/testfile --threatname threatName   OnError=Failed to run SendThreatDetectedEvent binary   timeout=10
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  SafeStore Log Contains  Quarantined /tmp/testfile successfully
    mark_expected_error_in_log  ${SAFESTORE_LOG_PATH}  Got DATA_TAG_NOT_SET while setting custom data, name: SHA256

Send invalid SHA To safestore
    Create File  /tmp/testfile
    ${result} =  Run Shell Process  ${SEND_THREAT_DETECTED_TOOL} --socketpath /opt/sophos-spl/plugins/av/var/safestore_socket --filepath /tmp/testfile --threatname threatName --sha e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b8==    OnError=Failed to run SendThreatDetectedEvent binary   timeout=10
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  SafeStore Log Contains  Quarantined /tmp/testfile successfully

Send SHA with foreign char Safestore
    Create File  /tmp/testfile
    ${result} =  Run Shell Process  ${SEND_THREAT_DETECTED_TOOL} --socketpath /opt/sophos-spl/plugins/av/var/safestore_socket --filepath /tmp/testfile --threatname threatName --sha e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b8សួស្តី    OnError=Failed to run SendThreatDetectedEvent binary   timeout=10
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  SafeStore Log Contains  Quarantined /tmp/testfile successfully