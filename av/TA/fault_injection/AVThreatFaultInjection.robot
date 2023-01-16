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

Fault Injection Test Setup
    Wait Until AV Plugin running

Fault Injection Test Teardown
    Dump and Reset Logs

Dump and Reset Logs
    Dump Log  ${SAFESTORE_LOG_PATH}
    Dump Log  ${AV_LOG_PATH}

Send Threat Object To AV
    [Arguments]  ${filepath}=/tmp/testfile  ${threatname}=ThreatName  ${sha}=sha256
    ${result} =  Run Shell Process  ${SEND_THREAT_DETECTED_TOOL} --socketpath /opt/sophos-spl/plugins/av/chroot/var/threat_report_socket --filepath "${filepath}" --threatname "${threatname}" --sha ${sha}    OnError=Failed to run SendThreatDetectedEvent binary   timeout=10

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
Send A Valid Threat Detected Object To AV with SafeStore Disabled
    Disable SafeStore
    Create File With Automatic Cleanup  /tmp/${TEST NAME}
    ${av_mark} =  Get AV Log Mark
    Send Threat Object To AV    /tmp/${TEST NAME}
    Wait For Log Contains From Mark  ${av_mark}   '/tmp/${TEST NAME}' was not quarantined due to SafeStore being disabled   timeout=60

#Send A Valid Threat Detected Object To AV with SafeStore Enabled
#    Enable SafeStore
#    Create File With Automatic Cleanup  /tmp/${TEST NAME}
#    ${av_mark} =  Get AV Log Mark
#    Send Threat Object To AV    /tmp/${TEST NAME}
#    Wait For Log Contains From Mark  ${av_mark}   something   timeout=60

