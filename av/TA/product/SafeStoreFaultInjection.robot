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

send TDO To socket
    [Arguments]  ${socketpath}=/opt/sophos-spl/plugins/av/var/safestore_socket  ${filepath}=/tmp/testfile  ${threatname}=threatName  ${sha}=e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855  ${fd}=0  ${threatid}=00010203-0405-0607-0809-0a0b0c0d0e0f
    ${result} =  Run Shell Process  ${SEND_THREAT_DETECTED_TOOL} --socketpath ${socketpath} --filepath ${filepath} --threatname ${threatname} --sha ${sha} --filedescriptor ${fd} --threatid ${threatid}  OnError=Failed to run SendThreatDetectedEvent binary   timeout=10
    [Return]  ${result}

*** Test Cases ***
Send A Valid Threat Detected Object To Safestore
    Create File  /tmp/testfile
    ${result} =  send TDO To socket
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

Send Empty ThreatName To Safestore
    Create File  /tmp/testfile
    ${result} =  send TDO To socket  threatname=""
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  SafeStore Log Contains  Aborting SafeStore connection thread: failed to parse detection
    mark_expected_error_in_log  ${SAFESTORE_LOG_PATH}  Aborting SafeStore connection thread: failed to parse detection

Send ThreatName with foreign chars To Safestore
    Create File  /tmp/testfile
    ${result} =  send TDO To socket  threatname=threatNameこんにちは
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  SafeStore Log Contains  Quarantined /tmp/testfile successfully

Send ThreatName with xml To Safestore
    Create File  /tmp/testfile
    ${result} =  send TDO To socket  threatname="<tag></tag>"
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  SafeStore Log Contains  Quarantined /tmp/testfile successfully

Send ThreatName with json To Safestore
    Create File  /tmp/testfile
    ${result} =  send TDO To socket  threatname="{\"blob\":1000}"
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  SafeStore Log Contains  Quarantined /tmp/testfile successfully

Send Filepath with json To Safestore
    Create File  /tmp/{\'blob\':1000}
    ${result} =  send TDO To socket  filepath="/tmp/{'blob':1000}"
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  SafeStore Log Contains  Quarantined /tmp/{'blob':1000} successfully

Send Filepath with xml To Safestore
    Create File  /tmp/<xml><\xml>
    ${result} =  send TDO To socket  filepath="/tmp/<xml><\xml>"
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  SafeStore Log Contains  Quarantined /tmp/<xml><\xml> successfully

Send Filepath with foreign char Safestore
    Create File  /tmp/こんにちは
    ${result} =  send TDO To socket  filepath="/tmp/こんにちは"
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  SafeStore Log Contains  Quarantined /tmp/こんにちは successfully

Send Filepath that is a dir To Safestore
    Create Directory  /tmp/Dir
    ${result} =  send TDO To socket  filepath=/tmp/Dir
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  SafeStore Log Contains  Failed to quarantine /tmp/Dir due to: MaxObjectSizeExceeded
    mark_expected_error_in_log  ${SAFESTORE_LOG_PATH}  Failed to quarantine /tmp/Dir due to: MaxObjectSizeExceeded

Send empty File To Safestore
    ${result} =  send TDO To socket  filepath=""  fd=1
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  SafeStore Log Contains  Aborting SafeStore connection thread: failed to parse detection
    mark_expected_error_in_log  ${SAFESTORE_LOG_PATH}  Aborting SafeStore connection thread: failed to parse detection

Send Filepath only slashes To Safestore
    ${result} =  send TDO To socket  filepath="//"  fd=1
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  SafeStore Log Contains  Cannot quarantine // as it was moved
    mark_expected_error_in_log  ${SAFESTORE_LOG_PATH}  Cannot quarantine // as it was moved

Send Filepath no slashes To Safestore
    ${result} =  send TDO To socket  filepath="tmp"  fd=1
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  SafeStore Log Contains  Cannot quarantine tmp as it was moved
    mark_expected_error_in_log  ${SAFESTORE_LOG_PATH}  Cannot quarantine tmp as it was moved

Send Short SHA To Safestore
    Create File  /tmp/testfile
    ${result} =  send TDO To socket  sha=e3b0c44298fc1c149afbf4c8996f
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  SafeStore Log Contains  Quarantined /tmp/testfile successfully

Send Long SHA To Safestore
    Create File  /tmp/testfile
    ${result} =  send TDO To socket  sha=e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b85555
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  SafeStore Log Contains  Quarantined /tmp/testfile successfully

Send invalid SHA with captial letters To Safestore
    Create File  /tmp/testfile
    ${result} =  send TDO To socket  sha=E3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  SafeStore Log Contains  Quarantined /tmp/testfile successfully

Send SHA that is in json format To Safestore
    Create File  /tmp/testfile
    ${result} =  send TDO To socket  sha={"hellow":1000}
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  SafeStore Log Contains  Quarantined /tmp/testfile successfully

Send SHA that is in xml format To Safestore
    Create File  /tmp/testfile
    ${result} =  send TDO To socket  sha="<xmltag>aaaa<\xmltag>"
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  SafeStore Log Contains  Quarantined /tmp/testfile successfully

Send TDO with empty SHA To Safestore
    Create File  /tmp/testfile
    ${result} =  send TDO To socket  sha=""
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  SafeStore Log Contains  Quarantined /tmp/testfile successfully
    mark_expected_error_in_log  ${SAFESTORE_LOG_PATH}  Got DATA_TAG_NOT_SET while setting custom data, name: SHA256

Send invalid SHA To safestore
    Create File  /tmp/testfile
    ${result} =  send TDO To socket  sha=e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b8==
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  SafeStore Log Contains  Quarantined /tmp/testfile successfully

Send SHA with foreign char Safestore
    Create File  /tmp/testfile
    ${result} =  send TDO To socket  sha=e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b8សួស្តី
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  SafeStore Log Contains  Quarantined /tmp/testfile successfully

Send empty threatid To Safestore
    Create File  /tmp/testfile
    ${result} =  send TDO To socket  threatid=""
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  SafeStore Log Contains  Aborting SafeStore connection thread: failed to parse detection
    mark_expected_error_in_log  ${SAFESTORE_LOG_PATH}  Aborting SafeStore connection thread: failed to parse detection

Send long threatid To Safestore
    Create File  /tmp/testfile
    ${result} =  send TDO To socket  threatid="00010203-0405-0607-0809-0a0b0c0d0e0ff"
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  SafeStore Log Contains  Aborting SafeStore connection thread: failed to parse detection
    mark_expected_error_in_log  ${SAFESTORE_LOG_PATH}  Aborting SafeStore connection thread: failed to parse detection

Send short threatid To Safestore
    Create File  /tmp/testfile
    ${result} =  send TDO To socket  threatid="00010203-0405-0607-0809-0a0b"
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  SafeStore Log Contains  Aborting SafeStore connection thread: failed to parse detection
    mark_expected_error_in_log  ${SAFESTORE_LOG_PATH}  Aborting SafeStore connection thread: failed to parse detection

Send threatid with non hex chars To Safestore
    Create File  /tmp/testfile
    ${result} =  send TDO To socket  threatid="00010203-0405-0607-0809-0a0b0c0d0e=="
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  SafeStore Log Contains  Aborting SafeStore connection thread: failed to parse detection
    mark_expected_error_in_log  ${SAFESTORE_LOG_PATH}  Aborting SafeStore connection thread: failed to parse detection

Send threatid with uppercase chars To Safestore
    Create File  /tmp/testfile
    ${result} =  send TDO To socket  threatid="00010203-0405-0607-0809-0a0b0c0d0e0F"
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  SafeStore Log Contains  Quarantined /tmp/testfile successfully

Send threatid with no hyphens To Safestore
    Create File  /tmp/testfile
    ${result} =  send TDO To socket  threatid="0001020300405006070080900a0b0c0d0e0F"
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  SafeStore Log Contains  Aborting SafeStore connection thread: failed to parse detection
    mark_expected_error_in_log  ${SAFESTORE_LOG_PATH}  Aborting SafeStore connection thread: failed to parse detection

Send threatid with only hyphens To Safestore
    Create File  /tmp/testfile
    ${result} =  send TDO To socket  threatid="------------------------------------"
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  SafeStore Log Contains  Aborting SafeStore connection thread: failed to parse detection
    mark_expected_error_in_log  ${SAFESTORE_LOG_PATH}  Aborting SafeStore connection thread: failed to parse detection

Send threatid with only four hyphens To Safestore
    Create File  /tmp/testfile
    ${result} =  send TDO To socket  threatid="----"
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  SafeStore Log Contains  Aborting SafeStore connection thread: failed to parse detection
    mark_expected_error_in_log  ${SAFESTORE_LOG_PATH}  Aborting SafeStore connection thread: failed to parse detection

Send threatid with foreign char To Safestore
    Create File  /tmp/testfile
    ${result} =  send TDO To socket  threatid="00010203-0405-0607-0809-0a0b0c0d0e0こ"
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  SafeStore Log Contains  Aborting SafeStore connection thread: failed to parse detection
    mark_expected_error_in_log  ${SAFESTORE_LOG_PATH}  Aborting SafeStore connection thread: failed to parse detection

Send threatid with json To Safestore
    Create File  /tmp/testfile
    ${result} =  send TDO To socket  threatid="{'00010203-0405-0607-0809-0a0b0c0d0e0j':90}"
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  SafeStore Log Contains  Aborting SafeStore connection thread: failed to parse detection
    mark_expected_error_in_log  ${SAFESTORE_LOG_PATH}  Aborting SafeStore connection thread: failed to parse detection

Send threatid with xml To Safestore
    Create File  /tmp/testfile
    ${result} =  send TDO To socket  threatid="<exmapletag></exampletag>"
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  SafeStore Log Contains  Aborting SafeStore connection thread: failed to parse detection
    mark_expected_error_in_log  ${SAFESTORE_LOG_PATH}  Aborting SafeStore connection thread: failed to parse detection