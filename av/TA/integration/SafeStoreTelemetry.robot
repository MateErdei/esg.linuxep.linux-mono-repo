*** Settings ***
Documentation   SafeStore Telemetry tests
Force Tags      INTEGRATION  AVBASE  TELEMETRY

Library         ../Libs/AVScanner.py
Library         ../Libs/Telemetry.py

Resource        ../shared/AVAndBaseResources.robot

Suite Setup     Telemetry Suite Setup
Suite Teardown  Uninstall All

Test Setup      AV And Base Setup
Test Teardown   AV And Base Teardown

*** Keywords ***
Telemetry Suite Setup
    Install With Base SDDS
    Send Alc Policy
    Send Sav Policy With No Scheduled Scans

*** Test Cases ***
SafeStore Can Send Telemetry
    # Assumes threat health is 1 (good)
    Run Telemetry Executable With HTTPS Protocol    port=${4431}

    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Log  ${telemetryFileContents}
    Check SafeStore Telemetry  ${telemetryFileContents}
    ${telemetryLogContents} =  Get File    ${TELEMETRY_EXECUTABLE_LOG}
    Should Contain   ${telemetryLogContents}    Gathered telemetry for safestore

SafeStore Telemetry Counters Are Zero by default
    Run Telemetry Executable With HTTPS Protocol    port=${4436}

    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Log   ${telemetryFileContents}

    ${telemetryJson}=    Evaluate     json.loads("""${telemetryFileContents}""")    json
    ${safeStoreDict}=    Set Variable     ${telemetryJson['safestore']}

    Dictionary Should Contain Item   ${safeStoreDict}   quarantine-successes   ${0}
    Dictionary Should Contain Item   ${safeStoreDict}   quarantine-failures   ${0}
    Dictionary Should Contain Item   ${safeStoreDict}   unlink-failures   ${0}

Telemetry Executable Generates SafeStore Telemetry
    Start SafeStore
    Wait Until SafeStore Log Contains    Successfully initialised SafeStore database

    Check SafeStore Telemetry    dormant-mode   False
    Check SafeStore Telemetry    health   0

Telemetry Executable Generates SafeStore Telemetry When SafeStore Is In Dormant Mode
    Create File    ${SOPHOS_INSTALL}/plugins/av/var/safestore_dormant_flag    ""

    Check SafeStore Telemetry    dormant-mode   True
    Check SafeStore Telemetry    health   1

Telemetry Executable Generates SafeStore Database Size Telemetry
    Run Telemetry Executable With HTTPS Protocol

    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    ${telemetryJson} =    Evaluate     json.loads("""${telemetryFileContents}""")    json

    Log    ${telemetryJson}
    Dictionary Should Contain Key    ${telemetryJson}    safestore
    ${safeStoreTelemetryJson} =    Get From Dictionary    ${telemetryJson}    safestore
    Dictionary Should Contain Key   ${safeStoreTelemetryJson}   database-size

    ${databaseSize} =    Evaluate    ${safeStoreTelemetryJson["database-size"]}
    ${databaseSizeType}=      Evaluate     isinstance(databaseSize, int)

    Check Is Greater Than    ${databaseSize}    ${100}
    Should Be Equal    ${databaseSizeType}    ${True}