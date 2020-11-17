*** Settings ***

Library         ../Libs/BaseInteractionTools/DiagnoseUtils.py
Library         ../Libs/BaseInteractionTools/PolicyUtils.py
Library         ../Libs/ExclusionHelper.py
Library         ../Libs/HttpsServer.py
Library         String
Library         DateTime

*** Variables ***

${TEMP_SAV_POLICY_FILENAME} =  TempSAVpolicy.xml

${CERT_PATH}   /tmp/cert.pem
${SSPL_BASE}                    ${SOPHOS_INSTALL}/base
${MCS_ACTION_DIRECTORY}         ${SSPL_BASE}/mcs/action
${MACHINE_ID_FILE}              ${SSPL_BASE}/etc/machine_id.txt
${EXE_CONFIG_FILE}              ${SSPL_BASE}/telemetry/var/telemetry-exe.json
${TELEMETRY_OUTPUT_JSON}        ${SSPL_BASE}/telemetry/var/telemetry.json
${TELEMETRY_BACKUP_JSON}        ${SSPL_BASE}/telemetry/cache/av-telemetry.json
${TELEMETRY_EXECUTABLE_LOG}     ${SOPHOS_INSTALL}/logs/base/sophosspl/telemetry.log

*** Keywords ***

Send Sav Policy With Imminent Scheduled Scan To Base
    ${time} =  Get Current Date  result_format=%y-%m-%d %H:%M:%S
    Create Sav Policy With Scheduled Scan  ${TEMP_SAV_POLICY_FILENAME}  ${time}
    Send Sav Policy To Base  ${TEMP_SAV_POLICY_FILENAME}

Send Sav Policy With Multiple Imminent Scheduled Scans To Base
    ${time} =  Get Current Date  result_format=%y-%m-%d %H:%M:%S
    Create Sav Policy With Multiple Scheduled Scans  ${TEMP_SAV_POLICY_FILENAME}  ${time}
    Send Sav Policy To Base  ${TEMP_SAV_POLICY_FILENAME}

Send Sav Policy With Invalid Scan Time
    Create Badly Configured Sav Policy Time  ${TEMP_SAV_POLICY_FILENAME}
    Send Sav Policy To Base  ${TEMP_SAV_POLICY_FILENAME}

Send Sav Policy With Invalid Scan Day
    Create Badly Configured Sav Policy Day  ${TEMP_SAV_POLICY_FILENAME}
    Send Sav Policy To Base  ${TEMP_SAV_POLICY_FILENAME}

Send Sav Policy With Multiple Scheduled Scans
    Send Sav Policy To Base  SAV_Policy_Multiple_Scans.xml

Send Sav Policy With No Scheduled Scans
    Send Sav Policy To Base  SAV_Policy_No_Scans.xml

Send Complete Sav Policy
    Create Complete Sav Policy  ${TEMP_SAV_POLICY_FILENAME}
    Send Sav Policy To Base  ${TEMP_SAV_POLICY_FILENAME}

Send Fixed Sav Policy
    Create Fixed Sav Policy  ${TEMP_SAV_POLICY_FILENAME}
    Send Sav Policy To Base  ${TEMP_SAV_POLICY_FILENAME}

Send Sav Policy To Base
    [Arguments]  ${policyFile}
    Copy File  ${RESOURCES_PATH}/${policyFile}  ${MCS_PATH}/policy/SAV-2_policy.xml

Send Sav Policy To Base With Exclusions Filled In
    [Arguments]  ${policyFile}
    ExclusionHelper.Fill In On Demand Posix Exclusions  ${RESOURCES_PATH}/${policyFile}  ${RESOURCES_PATH}/FilledIn.xml
    Copy File  ${RESOURCES_PATH}/FilledIn.xml  ${MCS_PATH}/policy/SAV-2_policy.xml

Send Sav Action To Base
    [Arguments]  ${actionFile}
    ${savActionFilename}  Generate Random String
    Copy File  ${RESOURCES_PATH}/${actionFile}  ${MCS_PATH}/action/SAV_action_${savActionFilename}.xml

Prepare To Run Telemetry Executable
    Prepare To Run Telemetry Executable With HTTPS Protocol

Prepare To Run Telemetry Executable With HTTPS Protocol
    [Arguments]  ${port}=443  ${TLSProtocol}=tlsv1_2
    HttpsServer.Start Https Server  ${CERT_PATH}  ${port}  ${TLSProtocol}
    Wait Until Keyword Succeeds  10 seconds  1.0 seconds  File Should Exist  ${MACHINE_ID_FILE}
    Create Test Telemetry Config File  ${EXE_CONFIG_FILE}  ${CERT_PATH}

Run Telemetry Executable
    [Arguments]  ${telemetryConfigFilePath}  ${expectedResult}   ${checkResult}=1

    Remove File  ${TELEMETRY_EXECUTABLE_LOG}

    ${result} =  Run Process  sudo  -u  sophos-spl-user  ${SSPL_BASE}/bin/telemetry  ${telemetryConfigFilePath}

    Log  "stdout = ${result.stdout}"
    Log  "stderr = ${result.stderr}"

    Should Be Equal As Integers  ${result.rc}  ${expectedResult}  Telemetry executable returned a non-successful error code: ${result.stderr}

