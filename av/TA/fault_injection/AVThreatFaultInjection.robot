*** Settings ***
Documentation   Fault injection test for the AV threat ingestion socket
Force Tags      PRODUCT  TAP_PARALLEL3

Library         ../Libs/LogUtils.py
Library         ../Libs/ProcessUtils.py
Library         ../Libs/TapTestOutput.py

Resource    ../shared/ComponentSetup.robot
Resource    ../shared/AVAndBaseResources.robot
Resource    ../shared/AVResources.robot
Resource    ${COMMON_TEST_ROBOT}/SafeStoreResources.robot
Resource    ../shared/ErrorMarkers.robot

Test Setup     Fault Injection Test Setup
Test Teardown  Fault Injection Test Teardown

*** Keywords ***
Dump Process Information
    ProcessUtils.dump_threads    ${SAFESTORE_BIN}
    ProcessUtils.dump_threads    ${AV_PLUGIN_BIN}
    ProcessUtils.dump_threads    ${SOPHOS_THREAT_DETECTOR_BINARY}
    ${ss_pid} =   ProcessUtils.pidof  ${SAFESTORE_BIN}
    ${result} =   Run Shell Process   lsof -p ${ss_pid}   OnError=Failed to lsof ${SAFESTORE_BIN}
    Log  ${result.stdout}
    ${av_pid} =   ProcessUtils.pidof  ${AV_PLUGIN_BIN}
    ${result} =   Run Shell Process   lsof -p ${av_pid}   OnError=Failed to lsof ${AV_PLUGIN_BIN}
    Log  ${result.stdout}
    ${td_pid} =   ProcessUtils.pidof  ${SOPHOS_THREAT_DETECTOR_BINARY}
    ${result} =   Run Shell Process   lsof -p ${td_pid}   OnError=Failed to lsof ${SOPHOS_THREAT_DETECTOR_BINARY}
    Log  ${result.stdout}

Check Product Logs For Errors
    check_all_product_logs_do_not_contain_error

Mark Expected Error In AV Log
    [Arguments]  ${error_log}
    mark_expected_error_in_log  ${AV_LOG_PATH}  ${error_log}

Fault Injection Test Setup
    Wait Until AV Plugin running
    register on fail  Dump Process Information

Fault Injection Test Teardown
    Exclude MCS Router is dead
    run_teardown_functions

    # Check that we can still cleanup files after each fault injection test case.
    ${av_mark} =  Get AV Log Mark
    Create File With Automatic Cleanup  /tmp/testfile
    Send Threat Object To AV    /tmp/testfile
    Wait For Log Contains From Mark  ${av_mark}   Threat cleaned up at path: '/tmp/testfile'   timeout=60
    Check Product Logs For Errors
    Dump and Reset Logs



Dump and Reset Logs
    Dump Log  ${SAFESTORE_LOG_PATH}
    Dump Log  ${AV_LOG_PATH}
    Dump Log  ${THREAT_DETECTOR_LOG_PATH}
    Dump Log  ${SUSI_DEBUG_LOG_PATH}


Send Threat Object To AV
    [Arguments]  ${file_path}=/tmp/testfile  ${threat_name}=ThreatName  ${sha}=sha256    ${report_source}=vdl    ${threat_type}=virus    ${userid}=userid
    ${SEND_THREAT_DETECTED_TOOL} =  TapTestOutput.get_send_threat_detected_tool
    ${result} =  Run Shell Process  ${SEND_THREAT_DETECTED_TOOL} --socketpath /opt/sophos-spl/plugins/av/chroot/var/threat_report_socket --filepath "${file_path}" --threatname "${threat_name}" --threattype "${threat_type}" --sha "${sha}" --reportsource "${report_source}" --userid "${userid}"    OnError=Failed to run SendThreatDetectedEvent binary   timeout=10

Create File With Automatic Cleanup
    [Arguments]  ${filepath}
    Create File  ${filepath}
    Register Cleanup  Remove File  ${filepath}

*** Test Cases ***
# SafeStore enabled tests
Send A Valid Threat Detected Object To AV with SafeStore Enabled
    Create File With Automatic Cleanup  /tmp/${TEST NAME}
    ${av_mark} =  Get AV Log Mark
    Send Threat Object To AV    /tmp/${TEST NAME}
    Wait For Log Contains From Mark  ${av_mark}   Threat cleaned up at path: '/tmp/${TEST NAME}'   timeout=60

Send Empty Threat Name Threat Detected Object To AV with SafeStore Enabled
    Create File With Automatic Cleanup  /tmp/${TEST NAME}
    ${expected_error} =  Set Variable  Aborting ThreatReporterServerConnectionThread: Failed to parse detection: Empty threat name
    Mark Expected Error In AV Log  ${expected_error}
    ${av_mark} =  Get AV Log Mark
    Send Threat Object To AV    /tmp/${TEST NAME}    threat_name=
    Wait For Log Contains From Mark  ${av_mark}   ${expected_error}   timeout=60

Send Long Threat Name Threat Detected Object To AV with SafeStore Enabled
    Create File With Automatic Cleanup  /tmp/${TEST NAME}
    ${av_mark} =  Get AV Log Mark
    ${long_string}=  Evaluate  "a" * 50000
    Send Threat Object To AV    /tmp/${TEST NAME}    threat_name=${long_string}
    Wait For Log Contains From Mark  ${av_mark}   Threat cleaned up at path: '/tmp/${TEST NAME}'   timeout=60

Send Threat Name That Contains Non-ascii and Symbols Threat Detected Object To AV with SafeStore Enabled
    Create File With Automatic Cleanup  /tmp/${TEST NAME}
    ${av_mark} =  Get AV Log Mark
    Send Threat Object To AV    /tmp/${TEST NAME}    threat_name=abc£$%^&*()_+[]{};#:@~あぃいぅあㇱ
    Wait For Log Contains From Mark  ${av_mark}   Threat cleaned up at path: '/tmp/${TEST NAME}'   timeout=60

Send Threat Name That Is Valid XML In Threat Detected Object To AV with SafeStore Enabled
    Create File With Automatic Cleanup  /tmp/${TEST NAME}
    ${av_mark} =  Get AV Log Mark
    Send Threat Object To AV    /tmp/${TEST NAME}    threat_name=<xml> <a>text</a> <b>123</b> </xml>
    Wait For Log Contains From Mark  ${av_mark}   Threat cleaned up at path: '/tmp/${TEST NAME}'   timeout=60

Send Threat Name That Is Valid JSON In Threat Detected Object To AV with SafeStore Enabled
    Create File With Automatic Cleanup  /tmp/${TEST NAME}
    ${av_mark} =  Get AV Log Mark
    Send Threat Object To AV    /tmp/${TEST NAME}    threat_name={"this is":"json", "a":1}
    Wait For Log Contains From Mark  ${av_mark}   Threat cleaned up at path: '/tmp/${TEST NAME}'   timeout=60

Send Many Threat Detected Objects To AV with SafeStore Enabled
    [Timeout]  30 minutes
    ${av_mark} =  Get AV Log Mark
    ${number_to_send} =  Set Variable  ${1000}
    FOR    ${index}    IN RANGE    ${number_to_send}
        Create File With Automatic Cleanup  /tmp/${TEST NAME}_${index}
        Wait Until Created    /tmp/${TEST NAME}_${index}
        Send Threat Object To AV    /tmp/${TEST NAME}_${index}
    END
    Wait For Log Contains From Mark  ${av_mark}   Threat cleaned up at path: '/tmp/${TEST NAME}_999'   timeout=250

Send Empty User ID In Threat Detected Object To AV with SafeStore Enabled
    Create File With Automatic Cleanup  /tmp/${TEST NAME}
    ${av_mark} =  Get AV Log Mark
    Send Threat Object To AV    /tmp/${TEST NAME}    userid=
    Wait For Log Contains From Mark  ${av_mark}   Threat cleaned up at path: '/tmp/${TEST NAME}'   timeout=60

Send Long User ID In Threat Detected Object To AV with SafeStore Enabled
    # Check handling of max Linux username length
    Create File With Automatic Cleanup  /tmp/${TEST NAME}
    ${av_mark} =  Get AV Log Mark
    Send Threat Object To AV    /tmp/${TEST NAME}    userid=abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz
    Wait For Log Contains From Mark  ${av_mark}   Threat cleaned up at path: '/tmp/${TEST NAME}'   timeout=60