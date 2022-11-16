*** Settings ***
Documentation    Telemetry tests
Force Tags      INTEGRATION  AVBASE  TELEMETRY
Library         Collections
Library         OperatingSystem
Library         Process
Library         String
Library         XML
Library         ../Libs/fixtures/AVPlugin.py
Library         ../Libs/FileSampleObfuscator.py
Library         ../Libs/LogUtils.py
Library         ../Libs/OnFail.py
Library         ../Libs/ThreatReportUtils.py
Library         ../Libs/Telemetry.py

Resource        ../shared/AVAndBaseResources.robot
Resource        ../shared/AVResources.robot
Resource        ../shared/BaseResources.robot
Resource        ../shared/ErrorMarkers.robot
Resource        ../shared/OnAccessResources.robot

Suite Setup     Telemetry Suite Setup
Suite Teardown  Telemetry Suite Teardown

Test Setup      AV And Base Setup
Test Teardown   AV And Base Teardown

*** Keywords ***
Telemetry Suite Setup
    Install With Base SDDS
    Send Alc Policy
    Send Sav Policy With No Scheduled Scans

Telemetry Suite Teardown
    Uninstall All

Log Telemetry files
    ${result} =  Run Process  ls -ld ${SSPL_BASE}/bin/telemetry ${SSPL_BASE}/bin ${SSPL_BASE}/bin/telemetry.*  shell=True  stdout=/tmp/telemetry.files  stderr=STDOUT
    Log  Telemetry files: ${result.stdout} code ${result.rc}
    Debug Telemetry  ${SSPL_BASE}/bin/telemetry

*** Test Cases ***

AV plugin runs scheduled scan and updates telemetry
    [Tags]  SLOW  RHEL7
    #Register On Fail  Log Telemetry files
    Log Telemetry files

    # Run telemetry to reset counters to 0
    Run Telemetry Executable With HTTPS Protocol    port=${4421}

    Mark AV Log
    Send Sav Policy With Imminent Scheduled Scan To Base
    File Should Exist  ${MCS_PATH}/policy/SAV-2_policy.xml

    Wait Until AV Plugin Log Contains With Offset  Completed scan  timeout=180

    Run Telemetry Executable With HTTPS Protocol    port=${4421}

    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Log   ${telemetryFileContents}

    ${telemetryJson}=    Evaluate     json.loads("""${telemetryFileContents}""")    json
    ${avDict}=    Set Variable     ${telemetryJson['av']}

    Dictionary Should Contain Item   ${avDict}   scheduled-scan-count   ${1}


AV Plugin Can Send Telemetry
    # Assumes threat health is 1 (good)
    Run Telemetry Executable With HTTPS Protocol    port=${4431}

    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Log  ${telemetryFileContents}
    Check Telemetry  ${telemetryFileContents}
    ${telemetryLogContents} =  Get File    ${TELEMETRY_EXECUTABLE_LOG}
    Should Contain   ${telemetryLogContents}    Gathered telemetry for av

AV Plugin sends non-zero processInfo to Telemetry
    # System file changes can cause ThreatDetector to be restarted while telemetry
    # is being fetched
    Create File  ${COMPONENT_ROOT_PATH}/var/inhibit_system_file_change_restart_threat_detector
    Register Cleanup  Remove File  ${COMPONENT_ROOT_PATH}/var/inhibit_system_file_change_restart_threat_detector

    Restart sophos_threat_detector and mark logs
    Run Telemetry Executable With HTTPS Protocol    port=${4432}

    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Log   ${telemetryFileContents}

    ${telemetryJson}=    Evaluate     json.loads("""${telemetryFileContents}""")    json
    ${avDict}=    Set Variable     ${telemetryJson['av']}

    ${memUsage}=    Get From Dictionary   ${avDict}   threatMemoryUsage
    ${processAge}=    Get From Dictionary   ${avDict}   threatProcessAge

    Should Not Be Equal As Integers  ${memUsage}  ${0}
    Should Be True  ${0} < ${processAge} < ${10}

AV plugin Saves and Restores Scan Now Counter
    # Run telemetry to reset counters to 0
    Run Telemetry Executable With HTTPS Protocol    port=${4433}

    # run a scan, count should increase to 1
    Configure and check scan now with offset

    Stop AV Plugin

    Wait Until Keyword Succeeds
                 ...  10 secs
                 ...  1 secs
                 ...  File Should Exist  ${TELEMETRY_BACKUP_JSON}

    ${backupfileContents} =  Get File    ${TELEMETRY_BACKUP_JSON}
    Log   ${backupfileContents}
    ${backupJson}=    Evaluate     json.loads("""${backupfileContents}""")    json
    ${rootkeyDict}=    Set Variable     ${backupJson['rootkey']}
    Dictionary Should Contain Item   ${rootkeyDict}   scan-now-count   ${1}
    Dictionary Should Contain Item   ${rootkeyDict}   threatHealth   ${1}

    Mark AV Log
    Start AV Plugin
    Wait Until AV Plugin Log Contains With Offset  Restoring telemetry from disk for plugin: av
    Run Telemetry Executable With HTTPS Protocol    port=${4434}

    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Log   ${telemetryFileContents}

    ${telemetryJson}=    Evaluate     json.loads("""${telemetryFileContents}""")    json
    ${avDict}=    Set Variable     ${telemetryJson['av']}

    Dictionary Should Contain Item   ${avDict}   scan-now-count   ${1}
    Dictionary Should Contain Item   ${avDict}   threatHealth   ${1}

Command Line Scan Increments Telemety Threat Eicar Count
    ${dirtyfile} =  Set Variable  /tmp_test/dirty_file.txt

    # Run telemetry to reset counters to 0
    Run Telemetry Executable With HTTPS Protocol  port=${4435}

    Create File  ${dirtyfile}  ${EICAR_STRING}
    Register Cleanup  Remove File  ${dirtyfile}

    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} /tmp_test/
    Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}

    Run Telemetry Executable With HTTPS Protocol  port=${4435}

    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Log   ${telemetryFileContents}

    ${telemetryJson}=    Evaluate     json.loads("""${telemetryFileContents}""")    json
    ${avDict}=    Set Variable     ${telemetryJson['av']}
    Dictionary Should Contain Item   ${avDict}   on-demand-threat-eicar-count   ${1}

    Dictionary Should Contain Item   ${avDict}   scan-now-count   ${0}
    Dictionary Should Contain Item   ${avDict}   on-demand-threat-count   ${0}
    Dictionary Should Contain Item   ${avDict}   on-access-threat-eicar-count   ${0}
    Dictionary Should Contain Item   ${avDict}   on-access-threat-count   ${0}
    Dictionary Should Contain Item   ${avDict}   scheduled-scan-count   ${0}


Command Line Scan Increments Telemety Threat Detection Count
    # Run telemetry to reset counters to 0
    Run Telemetry Executable With HTTPS Protocol  port=${4435}

    DeObfuscate File  ${RESOURCES_PATH}/file_samples_obfuscated/MLengHighScore.exe  /tmp_test/MLengHighScore-excluded.exe
    Register Cleanup  Remove File  /tmp_test/MLengHighScore-excluded.exe

    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} /tmp_test/
    Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}

    Run Telemetry Executable With HTTPS Protocol  port=${4435}

    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Log   ${telemetryFileContents}

    ${telemetryJson}=    Evaluate     json.loads("""${telemetryFileContents}""")    json
    ${avDict}=    Set Variable     ${telemetryJson['av']}
    Dictionary Should Contain Item   ${avDict}   on-demand-threat-count   ${1}

    Dictionary Should Contain Item   ${avDict}   scan-now-count   ${0}
    Dictionary Should Contain Item   ${avDict}   on-demand-threat-eicar-count   ${0}
    Dictionary Should Contain Item   ${avDict}   on-access-threat-eicar-count   ${0}
    Dictionary Should Contain Item   ${avDict}   on-access-threat-count   ${0}
    Dictionary Should Contain Item   ${avDict}   scheduled-scan-count   ${0}

Scheduled Scan Increments Telemetry Threat Eicar And Scheduled Scan Count
    ${dirtyfile} =  Set Variable  /tmp_test/dirty_file.txt

    # Run telemetry to reset counters to 0
    Run Telemetry Executable With HTTPS Protocol  port=${4435}

    Create File  ${dirtyfile}  ${EICAR_STRING}
    Register Cleanup  Remove File  ${dirtyfile}

    Mark AV Log
    Send Sav Policy With Imminent Scheduled Scan To Base Exclusions Added
    File Should Exist  ${MCS_PATH}/policy/SAV-2_policy.xml

    Wait Until AV Plugin Log Contains With Offset  Completed scan  timeout=60

    Run Telemetry Executable With HTTPS Protocol  port=${4435}

    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Log   ${telemetryFileContents}

    ${telemetryJson}=    Evaluate     json.loads("""${telemetryFileContents}""")    json
    ${avDict}=    Set Variable     ${telemetryJson['av']}
    Dictionary Should Contain Item   ${avDict}   on-demand-threat-eicar-count   ${1}
    Dictionary Should Contain Item   ${avDict}   scheduled-scan-count   ${1}

    Dictionary Should Contain Item   ${avDict}   on-demand-threat-count   ${0}
    Dictionary Should Contain Item   ${avDict}   on-access-threat-eicar-count   ${0}
    Dictionary Should Contain Item   ${avDict}   on-access-threat-count   ${0}
    Dictionary Should Contain Item   ${avDict}   scan-now-count   ${0}


Scheduled Scan Increments Telemetry Threat And Scheduled Scan Count
    # Run telemetry to reset counters to 0
    Run Telemetry Executable With HTTPS Protocol  port=${4435}

    DeObfuscate File  ${RESOURCES_PATH}/file_samples_obfuscated/MLengHighScore.exe  /tmp_test/MLengHighScore-excluded.exe
    Register Cleanup  Remove File  /tmp_test/MLengHighScore-excluded.exe

    Mark AV Log
    Send Sav Policy With Imminent Scheduled Scan To Base Exclusions Added
    File Should Exist  ${MCS_PATH}/policy/SAV-2_policy.xml

    Wait Until AV Plugin Log Contains With Offset  Completed scan  timeout=60

    Run Telemetry Executable With HTTPS Protocol  port=${4435}

    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Log   ${telemetryFileContents}

    ${telemetryJson}=    Evaluate     json.loads("""${telemetryFileContents}""")    json
    ${avDict}=    Set Variable     ${telemetryJson['av']}
    Dictionary Should Contain Item   ${avDict}   on-demand-threat-count   ${1}
    Dictionary Should Contain Item   ${avDict}   scheduled-scan-count   ${1}

    Dictionary Should Contain Item   ${avDict}   on-demand-threat-eicar-count   ${0}
    Dictionary Should Contain Item   ${avDict}   on-access-threat-eicar-count   ${0}
    Dictionary Should Contain Item   ${avDict}   on-access-threat-count   ${0}
    Dictionary Should Contain Item   ${avDict}   scan-now-count   ${0}


Scan Now Increments Telemetry Threat Eicar And Scan Now Count
    ${dirtyfile} =  Set Variable  /tmp_test/dirty_file.txt

    # Run telemetry to reset counters to 0
    Run Telemetry Executable With HTTPS Protocol  port=${4435}

    Create File  ${dirtyfile}  ${EICAR_STRING}
    Register Cleanup  Remove File  ${dirtyfile}

    Configure and check scan now with offset

    Run Telemetry Executable With HTTPS Protocol  port=${4435}

    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Log   ${telemetryFileContents}

    ${telemetryJson}=    Evaluate     json.loads("""${telemetryFileContents}""")    json
    ${avDict}=    Set Variable     ${telemetryJson['av']}
    Dictionary Should Contain Item   ${avDict}   on-demand-threat-eicar-count   ${1}
    Dictionary Should Contain Item   ${avDict}   scan-now-count   ${1}

    Dictionary Should Contain Item   ${avDict}   on-demand-threat-count   ${0}
    Dictionary Should Contain Item   ${avDict}   on-access-threat-eicar-count   ${0}
    Dictionary Should Contain Item   ${avDict}   on-access-threat-count   ${0}
    Dictionary Should Contain Item   ${avDict}   scheduled-scan-count   ${0}


Scan Now Increments Telemetry Threat And Scan Now Count
    # Run telemetry to reset counters to 0
    Run Telemetry Executable With HTTPS Protocol  port=${4435}

    DeObfuscate File  ${RESOURCES_PATH}/file_samples_obfuscated/MLengHighScore.exe  /tmp_test/MLengHighScore-excluded.exe
    Register Cleanup  Remove File  /tmp_test/MLengHighScore-excluded.exe

    Configure and check scan now with offset

    Run Telemetry Executable With HTTPS Protocol  port=${4435}

    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Log   ${telemetryFileContents}

    ${telemetryJson}=    Evaluate     json.loads("""${telemetryFileContents}""")    json
    ${avDict}=    Set Variable     ${telemetryJson['av']}
    Dictionary Should Contain Item   ${avDict}   on-demand-threat-count   ${1}
    Dictionary Should Contain Item   ${avDict}   scan-now-count   ${1}

    Dictionary Should Contain Item   ${avDict}   on-demand-threat-eicar-count   ${0}
    Dictionary Should Contain Item   ${avDict}   on-access-threat-eicar-count   ${0}
    Dictionary Should Contain Item   ${avDict}   on-access-threat-count   ${0}
    Dictionary Should Contain Item   ${avDict}   scheduled-scan-count   ${0}


On Access Scan Increments Telemetry Threat Eicar Count
    # Run telemetry to reset counters to 0
    Run Telemetry Executable With HTTPS Protocol  port=${4435}

    ${mark} =  Get on access log mark

    Send Policies to enable on-access
    Register Cleanup  Send Policies to disable on-access

    wait for on access log contains after mark  On-access scanning enabled  mark=${mark}

    On-access Scan Eicar Open

    Run Telemetry Executable With HTTPS Protocol  port=${4435}

    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Log   ${telemetryFileContents}

    ${telemetryJson}=    Evaluate     json.loads("""${telemetryFileContents}""")    json
    ${avDict}=    Set Variable     ${telemetryJson['av']}
    Dictionary Should Contain Item   ${avDict}   on-access-threat-eicar-count   ${1}

    Dictionary Should Contain Item   ${avDict}   on-demand-threat-count   ${0}
    Dictionary Should Contain Item   ${avDict}   scan-now-count   ${0}
    Dictionary Should Contain Item   ${avDict}   on-demand-threat-eicar-count   ${0}
    Dictionary Should Contain Item   ${avDict}   on-access-threat-count   ${0}
    Dictionary Should Contain Item   ${avDict}   scheduled-scan-count   ${0}


On Access Scan Increments Telemetry Threat Count
    # Run telemetry to reset counters to 0
    Run Telemetry Executable With HTTPS Protocol  port=${4435}

    ${mark} =  Get on access log mark

    Send Policies to enable on-access
    Register Cleanup  Send Policies to disable on-access

    wait for on access log contains after mark  On-access scanning enabled  mark=${mark}

    #Generate ml file in excluded location
    DeObfuscate File  ${RESOURCES_PATH}/file_samples_obfuscated/MLengHighScore.exe  ${NORMAL_DIRECTORY}/MLengHighScore-excluded.exe
    Register Cleanup  Remove File  ${NORMAL_DIRECTORY}/MLengHighScore-excluded.exe

    #Generate a event we can look for
    On-access Scan Clean File

    #Move file
    Move File  ${NORMAL_DIRECTORY}/MLengHighScore-excluded.exe  ${NORMAL_DIRECTORY}/MLengHighScore.exe
    Register Cleanup  Remove File  ${NORMAL_DIRECTORY}/MLengHighScore.exe

    #Do test
    ${mark} =  Get on access log mark
    Get Binary File  ${NORMAL_DIRECTORY}/MLengHighScore.exe
    wait for on access log contains after mark  Detected "${NORMAL_DIRECTORY}/MLengHighScore.exe" is infected with ML/PE-A (Open)  mark=${mark}

    Run Telemetry Executable With HTTPS Protocol  port=${4435}

    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Log   ${telemetryFileContents}

    ${telemetryJson}=    Evaluate     json.loads("""${telemetryFileContents}""")    json
    ${avDict}=    Set Variable     ${telemetryJson['av']}

    Dictionary Should Contain Item   ${avDict}   on-access-threat-count   ${1}

    Dictionary Should Contain Item   ${avDict}   on-demand-threat-count   ${0}
    Dictionary Should Contain Item   ${avDict}   scan-now-count   ${0}
    Dictionary Should Contain Item   ${avDict}   on-demand-threat-eicar-count   ${0}
    Dictionary Should Contain Item   ${avDict}   on-access-threat-eicar-count   ${0}
    Dictionary Should Contain Item   ${avDict}   scheduled-scan-count   ${0}


AV plugin increments Scan Now Counter after Save and Restore
    # Run telemetry to reset counters to 0
    Run Telemetry Executable With HTTPS Protocol  port=${4435}

    # run a scan, count should increase to 1
    Configure and check scan now with offset

    Stop AV Plugin

    Wait Until Keyword Succeeds
                 ...  10 secs
                 ...  1 secs
                 ...  File Should Exist  ${TELEMETRY_BACKUP_JSON}

    ${backupfileContents} =  Get File    ${TELEMETRY_BACKUP_JSON}
    Log   ${backupfileContents}
    ${backupJson}=    Evaluate     json.loads("""${backupfileContents}""")    json
    ${rootkeyDict}=    Set Variable     ${backupJson['rootkey']}
    Dictionary Should Contain Item   ${rootkeyDict}   scan-now-count   ${1}

    Start AV Plugin

    # run a scan, count should increase to 1
    Configure and check scan now with offset

    Run Telemetry Executable With HTTPS Protocol  port=${4435}

    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Log   ${telemetryFileContents}

    ${telemetryJson}=    Evaluate     json.loads("""${telemetryFileContents}""")    json
    ${avDict}=    Set Variable     ${telemetryJson['av']}
    Dictionary Should Contain Item   ${avDict}   scan-now-count   ${2}


SafeStore Can Send Telemetry
    # Assumes threat health is 1 (good)
    Run Telemetry Executable With HTTPS Protocol    port=${4431}

    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Log  ${telemetryFileContents}
    Check Telemetry  ${telemetryFileContents}
    ${telemetryLogContents} =  Get File    ${TELEMETRY_EXECUTABLE_LOG}
    Should Contain   ${telemetryLogContents}    Gathered telemetry for safestore

Telemetry Executable Generates SafeStore Telemetry
    Start SafeStore
    Wait Until SafeStore Log Contains    Successfully initialised SafeStore database

    Check SafeStore Telemetry    dormant-mode   False
    Check SafeStore Telemetry    health   0

Telemetry Executable Generates SafeStore Telemetry When SafeStore Is In Dormant Mode
    Create File    ${SOPHOS_INSTALL}/plugins/av/var/safestore_dormant_flag    ""

    Check SafeStore Telemetry    dormant-mode   True
    Check SafeStore Telemetry    health   1