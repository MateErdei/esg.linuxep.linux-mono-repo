*** Settings ***
Documentation   SafeStore Telemetry tests
Force Tags      INTEGRATION  TELEMETRY    SAFESTORE  TAP_PARALLEL2

Library         Collections

Library         ../Libs/AVScanner.py
Library         ../Libs/FileSampleObfuscator.py
Library         ${COMMON_TEST_LIBS}/LogUtils.py
Library         ../Libs/Telemetry.py

Resource        ../shared/AVAndBaseResources.robot
Resource        ../shared/ErrorMarkers.robot
Resource        ../shared/SafeStoreResources.robot

Suite Setup     SafeStore Telemetry Suite Setup
Suite Teardown  Uninstall All

Test Setup      AV and Base Setup
Test Teardown   AV And Base Teardown

*** Keywords ***
SafeStore Telemetry Suite Setup
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

    Dictionary Should Contain Item   ${safeStoreDict}   quarantine-successes            ${0}
    Dictionary Should Contain Item   ${safeStoreDict}   quarantine-failures             ${0}
    Dictionary Should Contain Item   ${safeStoreDict}   unlink-failures                 ${0}
    Dictionary Should Contain Item   ${safeStoreDict}   database-deletions              ${0}
    Dictionary Should Contain Item   ${safeStoreDict}   successful-file-restorations    ${0}
    Dictionary Should Contain Item   ${safeStoreDict}   failed-file-restorations        ${0}

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

   ${av_mark} =  Mark AV Log

   Wait Until SafeStore Log Contains    Successfully initialised SafeStore database
   ${ss_mark} =    Mark Safestore Log

   Check avscanner can detect eicar
   Wait Until SafeStore Log Contains  Received Threat:
   Wait For Safestore Log Contains After Mark    Quarantined ${SCAN_DIRECTORY}/eicar.com successfully   ${ss_mark}

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
   ${ss_mark} =    Mark Safestore Log

   Check avscanner can detect eicar
   Wait Until SafeStore Log Contains  Received Threat:
   Wait For SafeStore Log Contains After Mark    Quarantined ${SCAN_DIRECTORY}/eicar.com successfully    ${ss_mark}

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

   ${av_mark} =  Mark AV Log
   Wait Until SafeStore Log Contains    Successfully initialised SafeStore database
   ${ss_mark} =    Mark Safestore Log

   Corrupt SafeStore Database
   Check avscanner can detect eicar
   Wait Until SafeStore Log Contains  Received Threat:
   Wait For Safestore Log Contains After Mark    Cannot quarantine ${SCAN_DIRECTORY}/eicar.com, SafeStore is in CORRUPT state    ${ss_mark}

   Stop SafeStore
   Wait Until Keyword Succeeds
   ...  10 secs
   ...  1 secs
   ...  File Should Exist  ${SAFESTORE_TELEMETRY_BACKUP_JSON}

   ${backupfileContents} =  Get File    ${SAFESTORE_TELEMETRY_BACKUP_JSON}
   Log   ${backupfileContents}
   ${backupJson}=    Evaluate     json.loads("""${backupfileContents}""")    json
   ${rootkeyDict}=    Set Variable     ${backupJson['rootkey']}
   Dictionary Should Contain Item   ${rootkeyDict}   unlink-failures   ${1}

   Start SafeStore
   ${ss_mark} =    Mark Safestore Log

   Check avscanner can detect eicar
   Wait Until SafeStore Log Contains  Received Threat:
   Wait For Safestore Log Contains After Mark    Cannot quarantine ${SCAN_DIRECTORY}/eicar.com, SafeStore is in CORRUPT state    ${ss_mark}

   Run Telemetry Executable With HTTPS Protocol  port=${4435}

   ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
   Log   ${telemetryFileContents}
   ${telemetryJson}=    Evaluate     json.loads("""${telemetryFileContents}""")    json
   ${safeStoreDict}=    Set Variable     ${telemetryJson['safestore']}
   Dictionary Should Contain Item   ${safeStoreDict}   unlink-failures        ${2}
   # Verify other counts are not impacted
   Dictionary Should Contain Item   ${safeStoreDict}   quarantine-successes   ${0}
   Dictionary Should Contain Item   ${safeStoreDict}   quarantine-failures    ${0}

   Mark Expected Error In Log    ${SAFESTORE_LOG_PATH}    Failed to initialise SafeStore database: DB_ERROR
   Mark Expected Error In Log    ${SAFESTORE_LOG_PATH}    Quarantine Manager failed to initialise
   Mark Expected Error In Log    ${SAFESTORE_LOG_PATH}    Cannot quarantine ${SCAN_DIRECTORY}/eicar.com, SafeStore is in CORRUPT state

Corrupt Threat Database Telemetry Is Not Reported When Database File Is Not On Disk
    Install With Base SDDS
    File Should Not Exist   ${THREAT_DATABASE_PATH}

    Run Telemetry Executable With HTTPS Protocol    port=${4431}

    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    ${telemetryJson} =    Evaluate     json.loads("""${telemetryFileContents}""")    json
    Log    ${telemetryJson}
    ${avDict}=    Set Variable     ${telemetryJson['av']}
    Dictionary Should Not Contain Key    ${avDict}   corrupt-threat-database

Corrupt Threat Database Telemetry Is Reported
    Register Cleanup    Exclude ThreatDatabase Failed To Parse Database

    ${avMark} =  Mark AV Log
    Stop AV Plugin
    Wait until AV Plugin Not Running

    Remove File     ${THREAT_DATABASE_PATH}
    Create File     ${THREAT_DATABASE_PATH}    {T26796de6c
    Register Cleanup  Remove File  ${THREAT_DATABASE_PATH}

    Start AV Plugin
    Wait For Log Contains From Mark    ${avMark}     Resetting ThreatDatabase as we failed to parse ThreatDatabase on disk with error
    Wait For Log Contains From Mark    ${avMark}     Initialised Threat Database

    Check AV Telemetry        corrupt-threat-database    ${True}

SafeStore Telemetry Is Incremented When Database Is Deleted
    Install With Base SDDS
    Wait Until SafeStore running
    Check SafeStore Telemetry    database-deletions   ${0}

    ${safestoreMark} =  Mark Log Size    ${SAFESTORE_LOG_PATH}
    Corrupt SafeStore Database
    Wait For Log Contains From Mark  ${safestoreMark}  Quarantine database deletion successful    200

    Check SafeStore Telemetry    database-deletions   ${1}

    Register Cleanup    Install With Base SDDS

    Mark Expected Error In Log    ${SAFESTORE_LOG_PATH}    Failed to initialise SafeStore database: DB_ERROR
    Mark Expected Error In Log    ${SAFESTORE_LOG_PATH}    Quarantine Manager failed to initialise

SafeStore Telemetry Is Incremented When File Is Successfully Restored
    Stop sophos_threat_detector
    Register Cleanup   Remove File  ${MCS_PATH}/policy/CORC_policy.xml
    Send CORC Policy To Base  corc_policy_empty_allowlist.xml
    Start sophos_threat_detector
    ${avMark} =  Mark AV Log
    ${safeStoreMark} =  Mark Log Size  ${SAFESTORE_LOG_PATH}
    Send Flags Policy To Base  flags_policy/flags_safestore_quarantine_ml_enabled.json
    Wait Until SafeStore running
    Check SafeStore Telemetry    successful-file-restorations   ${0}

    # Create threat to scan
    ${threat_file} =  Set Variable  ${NORMAL_DIRECTORY}/MLengHighScore.exe
    Create Directory  ${NORMAL_DIRECTORY}/
    DeObfuscate File  ${RESOURCES_PATH}/file_samples_obfuscated/MLengHighScore.exe  ${threat_file}
    Register Cleanup  Remove File  ${threat_file}
    File Should Exist  ${threat_file}

    # Scan threat
    Run Process    ${AVSCANNER}    ${NORMAL_DIRECTORY}/MLengHighScore.exe
    Wait For Log Contains From Mark    ${safeStoreMark}    Quarantined ${NORMAL_DIRECTORY}/MLengHighScore.exe successfully
    File Should Not Exist  ${threat_file}

    ## Ensure all fallout from previous policy changes has completed before we apply a new policy
    Sleep  ${5}

    # Allow-list the file
    Send CORC Policy To Base  corc_policy.xml
    Wait For Log Contains From Mark    ${avMark}    Added SHA256 to allow list: c88e20178a82af37a51b030cb3797ed144126cad09193a6c8c7e95957cf9c3f9
    Wait For Log Contains From Mark    ${safeStoreMark}    SafeStore Database Rescan request received

    Wait For Log Contains From Mark    ${safeStoreMark}    RestoreReportingClient reports successful restoration of ${threat_file}    timeout=60
    Check SafeStore Telemetry    successful-file-restorations   ${1}

SafeStore Telemetry Is Incremented When File Restoration Fails
    Stop sophos_threat_detector
    Register Cleanup   Remove File  ${MCS_PATH}/policy/CORC_policy.xml
    Send CORC Policy To Base  corc_policy_empty_allowlist.xml
    Start sophos_threat_detector
    ${avMark} =  Mark AV Log
    ${safeStoreMark} =  Mark Log Size  ${SAFESTORE_LOG_PATH}
    Send Flags Policy To Base  flags_policy/flags_safestore_quarantine_ml_enabled.json
    Wait Until SafeStore running
    Check SafeStore Telemetry    failed-file-restorations   ${0}

    # Create threat to scan
    ${threat_file} =  Set Variable  ${NORMAL_DIRECTORY}/MLengHighScore.exe
    Create Directory  ${NORMAL_DIRECTORY}/
    DeObfuscate File  ${RESOURCES_PATH}/file_samples_obfuscated/MLengHighScore.exe  ${threat_file}
    Register Cleanup  Remove File  ${threat_file}
    File Should Exist  ${threat_file}

    # Scan threat
    Run Process    ${AVSCANNER}    ${NORMAL_DIRECTORY}/MLengHighScore.exe
    Wait For Log Contains From Mark    ${safeStoreMark}    Quarantined ${NORMAL_DIRECTORY}/MLengHighScore.exe successfully
    File Should Not Exist  ${threat_file}

    Remove Directory    ${NORMAL_DIRECTORY}
    Register Cleanup    Create Directory  ${NORMAL_DIRECTORY}/

    # Allow-list the file
    Send CORC Policy To Base  corc_policy.xml
    Wait For Log Contains From Mark    ${avMark}    Added SHA256 to allow list: c88e20178a82af37a51b030cb3797ed144126cad09193a6c8c7e95957cf9c3f9
    Wait For Log Contains From Mark    ${safeStoreMark}    SafeStore Database Rescan request received

    Wait For Log Contains From Mark    ${safeStoreMark}    Unable to restore clean file: ${threat_file}    timeout=60
    Check SafeStore Telemetry    failed-file-restorations   ${1}

    Mark Expected Error In Log    ${SAFESTORE_LOG_PATH}    Got RESTORE_FAILED when trying to restore an object
    Mark Expected Error In Log    ${SAFESTORE_LOG_PATH}    Unable to restore clean file: ${threat_file}

