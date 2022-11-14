*** Settings ***
Documentation   SafeStore Telemetry tests
Force Tags      INTEGRATION  AVBASE  TELEMETRY

Library         Collections

Library         ../Libs/AVScanner.py
Library         ../Libs/LogUtils.py
Library         ../Libs/Telemetry.py

Resource        ../shared/AVAndBaseResources.robot
Resource        ../shared/ErrorMarkers.robot
Resource        ../shared/SafeStoreResources.robot

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
    Check SafeStore Telemetry Dict    ${telemetryFileContents}
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

    ${databaseSize} =    Get From Dictionary    ${safeStoreTelemetryJson}    database-size
    ${databaseSizeType}=      Evaluate     isinstance(${databaseSize}, int)

    Check Is Greater Than    ${databaseSize}    ${100}
    Should Be Equal    ${databaseSizeType}    ${True}

SafeStore Increments Quarantine Counter After Successful Quarantine
   # Run telemetry to reset counters to 0
   Run Telemetry Executable With HTTPS Protocol  port=${4435}

   ${av_mark} =  Get AV Log Mark
   Send Flags Policy To Base  flags_policy/flags_safestore_enabled.json
   Wait For AV Log Contains After Mark    SafeStore flag set. Setting SafeStore to enabled.  ${av_mark}   timeout=60
   Wait Until SafeStore Log Contains    Successfully initialised SafeStore database
   ${ss_mark} =    Get SafeStore Log Mark

   Check avscanner can detect eicar
   Wait Until SafeStore Log Contains  Received Threat:
   Wait For Safestore Log Contains After Mark    Finalised file: eicar.com    ${ss_mark}

   Stop SafeStore
   Wait Until Keyword Succeeds
   ...  10 secs
   ...  1 secs
   ...  File Should Exist  ${SAFESTORE_TELEMETRY_BACKUP_JSON}

   ${backupfileContents} =  Get File    ${SAFESTORE_TELEMETRY_BACKUP_JSON}
   Log   ${backupfileContents}
   ${backupJson}=    Evaluate     json.loads("""${backupfileContents}""")    json
   ${rootkeyDict}=    Set Variable     ${backupJson['rootkey']}
   Dictionary Should Contain Item   ${rootkeyDict}   quarantine-successes   ${1}

   Start SafeStore
   Wait For Safestore Log Contains After Mark    Successfully initialised SafeStore database     ${ss_mark}
   ${ss_mark} =    Get SafeStore Log Mark

   Check avscanner can detect eicar
   Wait Until SafeStore Log Contains  Received Threat:
   LogUtils.Wait For SafeStore Log Contains After Mark    Finalised file: eicar.com    ${ss_mark}

   Run Telemetry Executable With HTTPS Protocol  port=${4435}

   ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
   Log   ${telemetryFileContents}
   ${telemetryJson}=    Evaluate     json.loads("""${telemetryFileContents}""")    json
   ${safeStoreDict}=    Set Variable     ${telemetryJson['safestore']}
   Dictionary Should Contain Item   ${safeStoreDict}   quarantine-successes   ${2}
   # Verify other counts are not impacted
   Dictionary Should Contain Item   ${safeStoreDict}   quarantine-failures   ${0}
   Dictionary Should Contain Item   ${safeStoreDict}   unlink-failures   ${0}

SafeStore Increments Quarantine Counter After Failed Quarantine
   # Run telemetry to reset counters to 0
   Run Telemetry Executable With HTTPS Protocol  port=${4435}

   ${av_mark} =  Get AV Log Mark
   Send Flags Policy To Base  flags_policy/flags_safestore_enabled.json
   Wait For AV Log Contains After Mark    SafeStore flag set. Setting SafeStore to enabled.  ${av_mark}   timeout=60
   Wait Until SafeStore Log Contains    Successfully initialised SafeStore database
   ${ss_mark} =    Get SafeStore Log Mark

   Corrupt SafeStore Database
   Check avscanner can detect eicar
   Wait Until SafeStore Log Contains  Received Threat:
   Wait For Safestore Log Contains After Mark    Cannot quarantine file, SafeStore is in    ${ss_mark}

   Stop SafeStore
   Wait Until Keyword Succeeds
   ...  10 secs
   ...  1 secs
   ...  File Should Exist  ${SAFESTORE_TELEMETRY_BACKUP_JSON}

   ${backupfileContents} =  Get File    ${SAFESTORE_TELEMETRY_BACKUP_JSON}
   Log   ${backupfileContents}
   ${backupJson}=    Evaluate     json.loads("""${backupfileContents}""")    json
   ${rootkeyDict}=    Set Variable     ${backupJson['rootkey']}
   Dictionary Should Contain Item   ${rootkeyDict}   quarantine-failures   ${1}

   Start SafeStore
   ${ss_mark} =    Get SafeStore Log Mark

   Check avscanner can detect eicar
   Wait Until SafeStore Log Contains  Received Threat:
   Wait For Safestore Log Contains After Mark    Cannot quarantine file, SafeStore is in    ${ss_mark}

   Run Telemetry Executable With HTTPS Protocol  port=${4435}

   ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
   Log   ${telemetryFileContents}
   ${telemetryJson}=    Evaluate     json.loads("""${telemetryFileContents}""")    json
   ${safeStoreDict}=    Set Variable     ${telemetryJson['safestore']}
   Dictionary Should Contain Item   ${safeStoreDict}   quarantine-failures   ${2}
   # Verify other counts are not impacted
   Dictionary Should Contain Item   ${safeStoreDict}   quarantine-successes   ${0}
   Dictionary Should Contain Item   ${safeStoreDict}   unlink-failures   ${0}

   Mark Expected Error In Log    ${SAFESTORE_LOG_PATH}    Quarantine Manager failed to initialise
