*** Settings ***
Force Tags      INTEGRATION
Library         Collections

Resource        ../shared/AVAndBaseResources.robot
Resource        ../shared/ErrorMarkers.robot

Suite Setup     AV Health Suite Setup
Suite Teardown  Uninstall All

Test Setup      AV And Base Setup
Test Teardown   AV Health Teardown

*** Keywords ***
AV Health Suite Setup
    Install With Base SDDS

AV Health Teardown
    Remove File    /tmp_test/naughty_eicar
    AV And Base Teardown
    Uninstall All

Check AV Threat Health Telemetry
    [Arguments]    ${threatStatus}
    Prepare To Run Telemetry Executable
    Run Telemetry Executable     ${EXE_CONFIG_FILE}     0
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  File Should Exist  ${TELEMETRY_OUTPUT_JSON}

    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    ${telemetryJson} =    Evaluate     json.loads("""${telemetryFileContents}""")    json

    Log    ${telemetryJson}
    Dictionary Should Contain Key    ${telemetryJson}    av
    Dictionary Should Contain Item   ${telemetryJson["av"]}   threatHealth   ${threatStatus}

SHS Status File Contains
    [Arguments]  ${content_to_contain}
    ${shsStatus} =  Get File   ${MCS_DIR}/status/SHS_status.xml
    Log  ${shsStatus}
    Should Contain  ${shsStatus}  ${content_to_contain}

Check Threat Health is Reporting Correctly
    [Arguments]    ${threatStatus}
    Wait Until Keyword Succeeds
    ...  40 secs
    ...  5 secs
    ...  Check AV Threat Health Telemetry    ${threatStatus}

    Wait Until Keyword Succeeds
    ...  40 secs
    ...  5 secs
    ...  SHS Status File Contains   <item name="threat" value="${threatStatus}" />

*** Test Cases ***
Clean CLS Result Does Not Reset Threat Health
    Check Threat Health is Reporting Correctly    1

    Create File     /tmp_test/naughty_eicar    ${EICAR_STRING}
    Create File     /tmp_test/clean_file       ${CLEAN_STRING}

    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} /tmp_test/naughty_eicar
    Log  return code is ${rc}
    Log  output is ${output}
    Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}
    Sophos Threat Detector Log Contains With Offset   Detected "EICAR-AV-Test" in /tmp_test/naughty_eicar

    Check Threat Health is Reporting Correctly    2

    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} /tmp_test/clean_file
    Log  return code is ${rc}
    Log  output is ${output}
    Should Be Equal As Integers  ${rc}  ${CLEAN_RESULT}

    Check Threat Health is Reporting Correctly    2


Clean Scan Now Result Resets Threat Health
    Check Threat Health is Reporting Correctly    1

    Create File     /tmp_test/naughty_eicar    ${EICAR_STRING}

    Configure and check scan now with offset
    Wait Until AV Plugin Log Contains With Offset  Completed scan  timeout=180

    Check Threat Health is Reporting Correctly    2

    Remove File  /tmp_test/naughty_eicar
    Register Cleanup    Remove File  /tmp_test/naughty_eicar

    Configure and check scan now with offset
    Wait Until AV Plugin Log Contains With Offset  Completed scan  timeout=180

    Check Threat Health is Reporting Correctly    1


Clean Scheduled Scan Result Resets Threat Health
    Check Threat Health is Reporting Correctly    1

    Create File  /tmp_test/naughty_eicar  ${EICAR_STRING}

    Mark AV Log
    Send Sav Policy With Imminent Scheduled Scan To Base
    File Should Exist  /opt/sophos-spl/base/mcs/policy/SAV-2_policy.xml
    Wait until scheduled scan updated With Offset
    Wait Until AV Plugin Log Contains With Offset  Starting scan Sophos Cloud Scheduled Scan  timeout=180
    Wait Until AV Plugin Log Contains With Offset  Completed scan  timeout=210

    Check Threat Health is Reporting Correctly    2

    Remove File  /tmp_test/naughty_eicar
    Register Cleanup    Remove File  /tmp_test/naughty_eicar

    Mark AV Log
    Send Sav Policy With Imminent Scheduled Scan To Base
    File Should Exist  /opt/sophos-spl/base/mcs/policy/SAV-2_policy.xml
    Wait until scheduled scan updated With Offset
    Wait Until AV Plugin Log Contains With Offset  Starting scan Sophos Cloud Scheduled Scan  timeout=180
    Wait Until AV Plugin Log Contains With Offset  Completed scan  timeout=210

    Check Threat Health is Reporting Correctly    1
