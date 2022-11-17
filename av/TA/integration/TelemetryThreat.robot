*** Settings ***
Documentation    Telemetry Threat tests
Force Tags      INTEGRATION  AVBASE  TELEMETRYTHREAT

Library         Collections

Library         ../Libs/FileSampleObfuscator.py
#Library         ../Libs/Telemetry.py

Resource        ../shared/OnAccessResources.robot
Resource        ../shared/ErrorMarkers.robot
Resource        ../shared/AVAndBaseResources.robot

Suite Setup     Telemetry Suite Setup
Suite Teardown  Telemetry Suite Teardown

Test Setup      AV And Base Setup
Test Teardown   AV And Base Teardown

*** Keywords ***
Telemetry Suite Setup
    Install With Base SDDS

Telemetry Suite Teardown
    Uninstall All

*** Test Cases ***
Command Line Scan Increments Telemetry Threat Eicar Count
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


Command Line Scan Increments Telemetry Threat Detection Count
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

    ${av_mark} =  Get AV Log Mark
    Send Sav Policy With Imminent Scheduled Scan To Base Exclusions Added
    File Should Exist  ${MCS_PATH}/policy/SAV-2_policy.xml

    wait_for_log_contains_from_mark  ${av_mark}  Completed scan  timeout=60

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

    ${av_mark} =  Get AV Log Mark
    Send Sav Policy With Imminent Scheduled Scan To Base Exclusions Added
    File Should Exist  ${MCS_PATH}/policy/SAV-2_policy.xml

    wait_for_log_contains_from_mark  ${av_mark}  Completed scan  timeout=60

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

    #This registers disable with cleanup
    Send Policies to enable on-access

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

    #This registers disable with cleanup
    Send Policies to enable on-access

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