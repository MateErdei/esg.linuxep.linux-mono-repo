*** Settings ***
Documentation   Fault injection test for the AV threat ingestion socket
Force Tags      PRODUCT

Library         ../Libs/LogUtils.py

Resource    ../shared/ComponentSetup.robot
Resource    ../shared/AVAndBaseResources.robot
Resource    ../shared/AVResources.robot
Resource    ../shared/SafeStoreResources.robot
Resource    ../shared/ErrorMarkers.robot

Test Setup     Fault Injection Test Setup
Test Teardown  Fault Injection Test Teardown

*** Keywords ***

Check Product Logs For Errors
    check_all_product_logs_do_not_contain_error

Mark Expected Error In AV Log
    [Arguments]  ${error_log}
    mark_expected_error_in_log  ${AV_LOG_PATH}  ${error_log}

Fault Injection Test Setup
    Wait Until AV Plugin running

Fault Injection Test Teardown
    Exclude MCS Router is dead
    Check Product Logs For Errors
    # TODO - other checks to make sure nothing else is broken.
    Dump and Reset Logs

Dump and Reset Logs
    Dump Log  ${SAFESTORE_LOG_PATH}
    Dump Log  ${AV_LOG_PATH}

Send Threat Object To AV
    [Arguments]  ${file_path}=/tmp/testfile  ${threat_name}=ThreatName  ${sha}=sha256    ${report_source}=vdl
    ${result} =  Run Shell Process  ${SEND_THREAT_DETECTED_TOOL} --socketpath /opt/sophos-spl/plugins/av/chroot/var/threat_report_socket --filepath "${file_path}" --threatname "${threat_name}" --sha ${sha} --reportsource ${report_source}    OnError=Failed to run SendThreatDetectedEvent binary   timeout=10

Create File With Automatic Cleanup
    [Arguments]  ${filepath}
    Create File  ${filepath}
    Register Cleanup  Remove File  ${filepath}

Enable SafeStore
    ${av_mark} =  Get AV Log Mark
    Send Flags Policy To Base  flags_policy/flags_safestore_enabled.json
    Wait For Log Contains From Mark  ${av_mark}  Setting SafeStore to enabled.    timeout=60

Disable SafeStore
    ${av_mark} =  Get AV Log Mark
    Send Flags Policy To Base  flags_policy/flags.json
    Wait For Log Contains From Mark  ${av_mark}  Setting SafeStore to disabled.    timeout=60


*** Test Cases ***

# SafeStore disabled tests
Send A Valid Threat Detected Object To AV with SafeStore Disabled
    Disable SafeStore
    Create File With Automatic Cleanup  /tmp/${TEST NAME}
    ${av_mark} =  Get AV Log Mark
    Send Threat Object To AV    /tmp/${TEST NAME}
    Wait For Log Contains From Mark  ${av_mark}   '/tmp/${TEST NAME}' was not quarantined due to SafeStore being disabled   timeout=60

Send Empty Threat Name Threat Detected Object To AV with SafeStore Disabled
    Disable SafeStore
    Create File With Automatic Cleanup  /tmp/${TEST NAME}
    ${av_mark} =  Get AV Log Mark
    Send Threat Object To AV    /tmp/${TEST NAME}    threat_name=
    ${expected_error} =  Set Variable  Aborting Threat Reporter connection thread: Failed to parse detection: Empty threat name
    Wait For Log Contains From Mark  ${av_mark}   ${expected_error}   timeout=60
    Mark Expected Error In AV Log  ${expected_error}

# SafeStore enabled tests
Send A Valid Threat Detected Object To AV with SafeStore Enabled
    Enable SafeStore
    Create File With Automatic Cleanup  /tmp/${TEST NAME}
    ${av_mark} =  Get AV Log Mark
    Send Threat Object To AV    /tmp/${TEST NAME}
    Wait For Log Contains From Mark  ${av_mark}   Threat cleaned up at path: '/tmp/${TEST NAME}'   timeout=60

Send Empty Threat Name Threat Detected Object To AV with SafeStore Enabled
    Enable SafeStore
    Create File With Automatic Cleanup  /tmp/${TEST NAME}
    ${av_mark} =  Get AV Log Mark
    Send Threat Object To AV    /tmp/${TEST NAME}    threat_name=
    ${expected_error} =  Set Variable  Aborting Threat Reporter connection thread: Failed to parse detection: Empty threat name
    Wait For Log Contains From Mark  ${av_mark}   ${expected_error}   timeout=60
    Mark Expected Error In AV Log  ${expected_error}




