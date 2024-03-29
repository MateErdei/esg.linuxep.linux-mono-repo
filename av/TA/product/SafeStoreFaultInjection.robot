*** Settings ***
Documentation   Product tests for SafeStore
Force Tags      PRODUCT  SAFESTORE    TAP_PARALLEL2

Library         ../Libs/TapTestOutput.py

Resource    ../shared/ComponentSetup.robot
Resource    ../shared/AVAndBaseResources.robot
Resource    ../shared/SafeStoreResources.robot

Suite Setup    SafeStore Fault Injection Suite Setup

Test Setup     SafeStore Test Setup
Test Teardown  SafeStore Test Teardown

*** Keywords ***

SafeStore Fault Injection Suite Setup
    ${SEND_THREAT_DETECTED_TOOL} =  TapTestOutput.get_send_threat_detected_tool
    set suite variable   \${SEND_THREAT_DETECTED_TOOL}  ${SEND_THREAT_DETECTED_TOOL}
    ${SEND_DATA_TO_SOCKET_TOOL} =   TapTestOutput.get_send_data_to_socket_tool
    set suite variable   \${SEND_DATA_TO_SOCKET_TOOL}  ${SEND_DATA_TO_SOCKET_TOOL}



SafeStore Test Setup
    Component Test Setup
    Start SafeStore Manually
    Wait Until SafeStore Started Successfully
    save_log_marks_at_start_of_test

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

mark expected socket error
    mark_expected_error_in_log  ${SAFESTORE_LOG_PATH}  SafeStoreSocket <> Closing SafeStoreServerConnectionThread, error from socket

Send TDO To socket
    [Arguments]  ${socketpath}=/opt/sophos-spl/plugins/av/var/safestore_socket  ${filepath}=/tmp/testfile  ${threat_type}=threatType  ${threatname}=threatName  ${sha}=e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855  ${fd}=0  ${threatid}=00010203-0405-0607-0809-0a0b0c0d0e0f
    ${result} =  Run Shell Process  ${SEND_THREAT_DETECTED_TOOL} --socketpath ${socketpath} --filepath ${filepath} --threattype ${threat_type} --threatname ${threatname} --sha ${sha} --filedescriptor ${fd} --threatid ${threatid}  OnError=Failed to run SendThreatDetectedEvent binary
    ...  timeout=${20}
    # The sender tool closes the connection in a way that can cause a log message in safestore log.
    mark expected socket error
    [Return]  ${result}

*** Test Cases ***
Send A Valid Threat Detected Object To Safestore
    Create File  /tmp/testfile
    ${result} =  Send TDO To socket
    wait_for_safestore_log_contains_after_mark  Quarantined /tmp/testfile successfully      mark=${SAFESTORE_LOG_MARK_FROM_START_OF_TEST}

Send An invalid capnproto message To Safestore
    ${result} =  Run Shell Process  ${SEND_DATA_TO_SOCKET_TOOL} --socketpath /opt/sophos-spl/plugins/av/var/safestore_socket --data 2222222    OnError=Failed to run SendDataToSocket binary   timeout=10
    wait_for_safestore_log_contains_after_mark  Aborting SafeStoreServerConnectionThread: failed to parse detection   mark=${SAFESTORE_LOG_MARK_FROM_START_OF_TEST}
    mark_expected_error_in_log  ${SAFESTORE_LOG_PATH}  Aborting SafeStoreServerConnectionThread: failed to parse detection

Send An valid capnproto message that is not tdo To Safestore
    ${result} =  Run Shell Process  ${SEND_DATA_TO_SOCKET_TOOL} --socketpath /opt/sophos-spl/plugins/av/var/safestore_socket --sendclientscan    OnError=Failed to run SendDataToSocket binary   timeout=10
    wait_for_safestore_log_contains_after_mark  Aborting SafeStoreServerConnectionThread: failed to parse detection   mark=${SAFESTORE_LOG_MARK_FROM_START_OF_TEST}
    mark_expected_error_in_log  ${SAFESTORE_LOG_PATH}  Aborting SafeStoreServerConnectionThread: failed to parse detection
    mark_expected_error_in_log  ${SAFESTORE_LOG_PATH}  Missing file path while parsing report

Send Only Message To Safestore
    Create File  /tmp/testfile
    ${result} =  Run Shell Process  ${SEND_THREAT_DETECTED_TOOL} --socketpath /opt/sophos-spl/plugins/av/var/safestore_socket --filepath /tmp/testfile --threatname threatName --sha e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855 --nosendfd    OnError=Failed to run SendThreatDetectedEvent binary   timeout=10
    wait_for_safestore_log_contains_after_mark  Aborting SafeStoreServerConnectionThread: failed to read fd   mark=${SAFESTORE_LOG_MARK_FROM_START_OF_TEST}
    mark_expected_error_in_log  ${SAFESTORE_LOG_PATH}  Aborting SafeStoreServerConnectionThread: failed to read fd

Send Only FD To Safestore
    Create File  /tmp/testfile
    ${result} =  Run Shell Process  ${SEND_THREAT_DETECTED_TOOL} --socketpath /opt/sophos-spl/plugins/av/var/safestore_socket --filepath /tmp/testfile --nosendmessage    OnError=Failed to run SendThreatDetectedEvent binary   timeout=10
    wait_for_safestore_log_contains_after_mark   SafeStoreServerConnectionThread ignoring length of zero / No new messages       mark=${SAFESTORE_LOG_MARK_FROM_START_OF_TEST}

Send Empty ThreatType To Safestore
    Create File  /tmp/testfile
    ${result} =  Send TDO To socket  threat_type=""
    wait_for_safestore_log_contains_after_mark  Quarantined /tmp/testfile successfully   mark=${SAFESTORE_LOG_MARK_FROM_START_OF_TEST}

Send ThreatType with foreign chars To Safestore
    Create File  /tmp/testfile
    ${result} =  Send TDO To socket  threat_type=threatTypeこんにちは
    wait_for_safestore_log_contains_after_mark  Quarantined /tmp/testfile successfully   mark=${SAFESTORE_LOG_MARK_FROM_START_OF_TEST}

Send ThreatType with xml To Safestore
    Create File  /tmp/testfile
    ${result} =  Send TDO To socket  threat_type="<tag></tag>"
    wait_for_safestore_log_contains_after_mark  Quarantined /tmp/testfile successfully   mark=${SAFESTORE_LOG_MARK_FROM_START_OF_TEST}

Send ThreatType with json To Safestore
    Create File  /tmp/testfile
    ${result} =  Send TDO To socket  threat_type="{\"blob\":1000}"
    wait_for_safestore_log_contains_after_mark  Quarantined /tmp/testfile successfully   mark=${SAFESTORE_LOG_MARK_FROM_START_OF_TEST}

Send very long ThreatType To Safestore
    Create File  /tmp/testfile
    ${s}=     Produce long string  50000
    ${result} =  Send TDO To socket  threat_type=${s}
    Wait for SafeStore log contains after mark    Failed to store threats: Couldn't set object custom data 'threats' to    mark=${SAFESTORE_LOG_MARK_FROM_START_OF_TEST}
    Mark expected error in log    ${SAFESTORE_LOG_PATH}    Failed to store threat type

Send Empty ThreatName To Safestore
    Create File  /tmp/testfile
    ${result} =  Send TDO To socket  threatname=""
    wait_for_safestore_log_contains_after_mark  Aborting SafeStoreServerConnectionThread: failed to parse detection   mark=${SAFESTORE_LOG_MARK_FROM_START_OF_TEST}
    mark_expected_error_in_log  ${SAFESTORE_LOG_PATH}  Aborting SafeStoreServerConnectionThread: failed to parse detection

Send ThreatName with foreign chars To Safestore
    Create File  /tmp/testfile
    ${result} =  Send TDO To socket  threatname=threatNameこんにちは
    wait_for_safestore_log_contains_after_mark  Quarantined /tmp/testfile successfully   mark=${SAFESTORE_LOG_MARK_FROM_START_OF_TEST}

Send ThreatName with xml To Safestore
    Create File  /tmp/testfile
    ${result} =  Send TDO To socket  threatname="<tag></tag>"
    wait_for_safestore_log_contains_after_mark  Quarantined /tmp/testfile successfully   mark=${SAFESTORE_LOG_MARK_FROM_START_OF_TEST}

Send ThreatName with json To Safestore
    Create File  /tmp/testfile
    ${result} =  Send TDO To socket  threatname="{\"blob\":1000}"
    wait_for_safestore_log_contains_after_mark  Quarantined /tmp/testfile successfully   mark=${SAFESTORE_LOG_MARK_FROM_START_OF_TEST}

Send very long threatname To Safestore
    Create File  /tmp/testfile
    ${s}=     Produce long string  50000
    ${result} =  Send TDO To socket  threatname=${s}
    wait_for_safestore_log_contains_after_mark  Quarantined /tmp/testfile successfully   mark=${SAFESTORE_LOG_MARK_FROM_START_OF_TEST}

Send Filepath with json To Safestore
    Create File  /tmp/{\'blob\':1000}
    ${result} =  Send TDO To socket  filepath="/tmp/{'blob':1000}"
    wait_for_safestore_log_contains_after_mark  Quarantined /tmp/{'blob':1000} successfully   mark=${SAFESTORE_LOG_MARK_FROM_START_OF_TEST}

Send Filepath with xml To Safestore
    Create File  /tmp/<xml><\xml>
    ${result} =  Send TDO To socket  filepath="/tmp/<xml><\xml>"
    wait_for_safestore_log_contains_after_mark  Quarantined /tmp/<xml><\xml> successfully   mark=${SAFESTORE_LOG_MARK_FROM_START_OF_TEST}

Send Filepath with foreign char Safestore
    Create File  /tmp/こんにちは
    ${result} =  Send TDO To socket  filepath="/tmp/こんにちは"
    wait_for_safestore_log_contains_after_mark  Quarantined /tmp/こんにちは successfully   mark=${SAFESTORE_LOG_MARK_FROM_START_OF_TEST}

Send Filepath with null char Safestore
    Create File  /tmp/file\000
    ${result} =  Send TDO To socket  filepath="/tmp/file\000"
    wait_for_safestore_log_contains_after_mark  Quarantined /tmp/file\000 successfully   mark=${SAFESTORE_LOG_MARK_FROM_START_OF_TEST}

Send very long Filepath To Safestore
    ${s}=     Produce long string
    ${long_filepath}=  Set Variable   /tmp/${s}/${s}/${s}/${s}/${s}/${s}/${s}/${s}/${s}/${s}/${s}/${s}/${s}/${s}/${s}
    Create File  ${long_filepath}
    Register Cleanup   Remove Directory  /tmp/${s}  recursive=True
    ${result} =  Send TDO To socket  filepath=${long_filepath}
    wait_for_safestore_log_contains_after_mark  Quarantined ${long_filepath} successfully   mark=${SAFESTORE_LOG_MARK_FROM_START_OF_TEST}

Send Filepath that is a dir To Safestore
    Create Directory  /opt/Dir
    Register Cleanup   Remove Directory  /opt/Dir  recursive=True
    ${result} =  Send TDO To socket  filepath=/opt/Dir
    wait_for_safestore_log_contains_after_mark  Failed to quarantine /opt/Dir due to:   mark=${SAFESTORE_LOG_MARK_FROM_START_OF_TEST}
    mark_expected_error_in_log  ${SAFESTORE_LOG_PATH}  Failed to quarantine /opt/Dir due to: MaxObjectSizeExceeded
    mark_expected_error_in_log  ${SAFESTORE_LOG_PATH}  safestore <> Failed to quarantine /opt/Dir due to: FileReadFailed

Send empty File To Safestore
    ${result} =  Send TDO To socket  filepath=""  fd=1
    wait_for_safestore_log_contains_after_mark  Aborting SafeStoreServerConnectionThread: failed to parse detection   mark=${SAFESTORE_LOG_MARK_FROM_START_OF_TEST}
    mark_expected_error_in_log  ${SAFESTORE_LOG_PATH}  Aborting SafeStoreServerConnectionThread: failed to parse detection

Send Filepath only slashes To Safestore
    ${result} =  Send TDO To socket  filepath="//"  fd=1
    wait_for_safestore_log_contains_after_mark  Cannot quarantine // as it was moved   mark=${SAFESTORE_LOG_MARK_FROM_START_OF_TEST}
    mark_expected_error_in_log  ${SAFESTORE_LOG_PATH}  Cannot quarantine // as it was moved
    mark expected socket error

Send Filepath no slashes To Safestore
    ${result} =  Send TDO To socket  filepath="tmp"  fd=1
    wait_for_safestore_log_contains_after_mark  Cannot quarantine tmp as it was moved   mark=${SAFESTORE_LOG_MARK_FROM_START_OF_TEST}
    mark_expected_error_in_log  ${SAFESTORE_LOG_PATH}  Cannot quarantine tmp as it was moved

Send Short SHA To Safestore
    Create File  /tmp/testfile
    ${result} =  Send TDO To socket  sha=e3b0c44298fc1c149afbf4c8996f
    wait_for_safestore_log_contains_after_mark  Quarantined /tmp/testfile successfully   mark=${SAFESTORE_LOG_MARK_FROM_START_OF_TEST}

Send Long SHA To Safestore
    Create File  /tmp/testfile
    ${result} =  Send TDO To socket  sha=e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b85555
    wait_for_safestore_log_contains_after_mark  Quarantined /tmp/testfile successfully   mark=${SAFESTORE_LOG_MARK_FROM_START_OF_TEST}

Send invalid SHA with captial letters To Safestore
    Create File  /tmp/testfile
    ${result} =  Send TDO To socket  sha=E3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855
    wait_for_safestore_log_contains_after_mark  Quarantined /tmp/testfile successfully   mark=${SAFESTORE_LOG_MARK_FROM_START_OF_TEST}

Send SHA that is in json format To Safestore
    Create File  /tmp/testfile
    ${result} =  Send TDO To socket  sha={"hellow":1000}
    wait_for_safestore_log_contains_after_mark  Quarantined /tmp/testfile successfully   mark=${SAFESTORE_LOG_MARK_FROM_START_OF_TEST}

Send SHA that is in xml format To Safestore
    Create File  /tmp/testfile
    ${result} =  Send TDO To socket  sha="<xmltag>aaaa<\xmltag>"
    wait_for_safestore_log_contains_after_mark  Quarantined /tmp/testfile successfully   mark=${SAFESTORE_LOG_MARK_FROM_START_OF_TEST}

Send TDO with empty SHA To Safestore
    Create File  /tmp/testfile
    ${result} =  Send TDO To socket  sha=""
    wait_for_safestore_log_contains_after_mark  Quarantined /tmp/testfile successfully   mark=${SAFESTORE_LOG_MARK_FROM_START_OF_TEST}
    mark_expected_error_in_log  ${SAFESTORE_LOG_PATH}  Got DATA_TAG_NOT_SET while setting custom data, name: SHA256

Send invalid SHA To safestore
    Create File  /tmp/testfile
    ${result} =  Send TDO To socket  sha=e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b8==
    wait_for_safestore_log_contains_after_mark  Quarantined /tmp/testfile successfully   mark=${SAFESTORE_LOG_MARK_FROM_START_OF_TEST}

Send SHA with foreign char Safestore
    Create File  /tmp/testfile
    ${result} =  Send TDO To socket  sha=e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b8សួស្តី
    wait_for_safestore_log_contains_after_mark  Quarantined /tmp/testfile successfully   mark=${SAFESTORE_LOG_MARK_FROM_START_OF_TEST}

Send very long SHA To Safestore
    Create File  /tmp/testfile
    ${long_string}=  Evaluate  "a" * 50000
    ${mark} =  Mark Safestore Log
    ${result} =  Send TDO To socket  sha=${long_string}
    wait_for_safestore_log_contains_after_mark  Quarantined /tmp/testfile successfully   mark=${mark}

Send empty threatid To Safestore
    Create File  /tmp/testfile
    ${result} =  Send TDO To socket  threatid=""
    wait_for_safestore_log_contains_after_mark  Aborting SafeStoreServerConnectionThread: failed to parse detection   mark=${SAFESTORE_LOG_MARK_FROM_START_OF_TEST}
    mark_expected_error_in_log  ${SAFESTORE_LOG_PATH}  Aborting SafeStoreServerConnectionThread: failed to parse detection

Send long threatid To Safestore
    Create File  /tmp/testfile
    ${result} =  Send TDO To socket  threatid="00010203-0405-0607-0809-0a0b0c0d0e0ff"
    wait_for_safestore_log_contains_after_mark  Aborting SafeStoreServerConnectionThread: failed to parse detection   mark=${SAFESTORE_LOG_MARK_FROM_START_OF_TEST}
    mark_expected_error_in_log  ${SAFESTORE_LOG_PATH}  Aborting SafeStoreServerConnectionThread: failed to parse detection

Send short threatid To Safestore
    Create File  /tmp/testfile
    ${result} =  Send TDO To socket  threatid="00010203-0405-0607-0809-0a0b"
    wait_for_safestore_log_contains_after_mark  Aborting SafeStoreServerConnectionThread: failed to parse detection   mark=${SAFESTORE_LOG_MARK_FROM_START_OF_TEST}
    mark_expected_error_in_log  ${SAFESTORE_LOG_PATH}  Aborting SafeStoreServerConnectionThread: failed to parse detection

Send threatid with non hex chars To Safestore
    Create File  /tmp/testfile
    ${result} =  Send TDO To socket  threatid="00010203-0405-0607-0809-0a0b0c0d0e=="
    wait_for_safestore_log_contains_after_mark  Aborting SafeStoreServerConnectionThread: failed to parse detection   mark=${SAFESTORE_LOG_MARK_FROM_START_OF_TEST}
    mark_expected_error_in_log  ${SAFESTORE_LOG_PATH}  Aborting SafeStoreServerConnectionThread: failed to parse detection

Send threatid with uppercase chars To Safestore
    Create File  /tmp/testfile
    ${result} =  Send TDO To socket  threatid="00010203-0405-0607-0809-0a0b0c0d0e0F"
    wait_for_safestore_log_contains_after_mark  Quarantined /tmp/testfile successfully   mark=${SAFESTORE_LOG_MARK_FROM_START_OF_TEST}

Send threatid with no hyphens To Safestore
    Create File  /tmp/testfile
    ${result} =  Send TDO To socket  threatid="0001020300405006070080900a0b0c0d0e0F"
    wait_for_safestore_log_contains_after_mark  Aborting SafeStoreServerConnectionThread: failed to parse detection   mark=${SAFESTORE_LOG_MARK_FROM_START_OF_TEST}
    mark_expected_error_in_log  ${SAFESTORE_LOG_PATH}  Aborting SafeStoreServerConnectionThread: failed to parse detection

Send threatid with only hyphens To Safestore
    Create File  /tmp/testfile
    ${result} =  Send TDO To socket  threatid="------------------------------------"
    wait_for_safestore_log_contains_after_mark  Aborting SafeStoreServerConnectionThread: failed to parse detection   mark=${SAFESTORE_LOG_MARK_FROM_START_OF_TEST}
    mark_expected_error_in_log  ${SAFESTORE_LOG_PATH}  Aborting SafeStoreServerConnectionThread: failed to parse detection

Send threatid with only four hyphens To Safestore
    Create File  /tmp/testfile
    ${result} =  Send TDO To socket  threatid="----"
    wait_for_safestore_log_contains_after_mark  Aborting SafeStoreServerConnectionThread: failed to parse detection   mark=${SAFESTORE_LOG_MARK_FROM_START_OF_TEST}
    mark_expected_error_in_log  ${SAFESTORE_LOG_PATH}  Aborting SafeStoreServerConnectionThread: failed to parse detection

Send threatid with foreign char To Safestore
    Create File  /tmp/testfile
    ${result} =  Send TDO To socket  threatid="00010203-0405-0607-0809-0a0b0c0d0e0こ"
    wait_for_safestore_log_contains_after_mark  Aborting SafeStoreServerConnectionThread: failed to parse detection   mark=${SAFESTORE_LOG_MARK_FROM_START_OF_TEST}
    mark_expected_error_in_log  ${SAFESTORE_LOG_PATH}  Aborting SafeStoreServerConnectionThread: failed to parse detection

Send threatid with json To Safestore
    Create File  /tmp/testfile
    ${result} =  Send TDO To socket  threatid="{'00010203-0405-0607-0809-0a0b0c0d0e0j':90}"
    wait_for_safestore_log_contains_after_mark  Aborting SafeStoreServerConnectionThread: failed to parse detection   mark=${SAFESTORE_LOG_MARK_FROM_START_OF_TEST}
    mark_expected_error_in_log  ${SAFESTORE_LOG_PATH}  Aborting SafeStoreServerConnectionThread: failed to parse detection

Send threatid with xml To Safestore
    Create File  /tmp/testfile
    ${result} =  Send TDO To socket  threatid="<exmapletag></exampletag>"
    wait_for_safestore_log_contains_after_mark  Aborting SafeStoreServerConnectionThread: failed to parse detection   mark=${SAFESTORE_LOG_MARK_FROM_START_OF_TEST}
    mark_expected_error_in_log  ${SAFESTORE_LOG_PATH}  Aborting SafeStoreServerConnectionThread: failed to parse detection

Send very long threatid To Safestore
    Create File  /tmp/testfile
    ${s}=     Produce long string  50000
    ${result} =  Send TDO To socket  threatid=${s}
    wait_for_safestore_log_contains_after_mark  Aborting SafeStoreServerConnectionThread: failed to parse detection   mark=${SAFESTORE_LOG_MARK_FROM_START_OF_TEST}
    mark_expected_error_in_log  ${SAFESTORE_LOG_PATH}  Aborting SafeStoreServerConnectionThread: failed to parse detection
