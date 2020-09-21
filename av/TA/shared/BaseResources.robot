*** Settings ***

Library         ../Libs/BaseInteractionTools/DiagnoseUtils.py
Library         ../Libs/BaseInteractionTools/PolicyUtils.py
Library         ../Libs/ExclusionHelper.py
Library         ../Libs/HttpsServer.py
Library         String
Library         DateTime

*** Variables ***

${TEMP_SAV_POLICY_FILENAME} =  TempSAVpolicy.xml

${EXE_CONFIG_FILE}  ${SOPHOS_INSTALL}/base/telemetry/var/telemetry-exe.json
${CERT_PATH}   /tmp/cert.pem
${MACHINE_ID_FILE}  ${SOPHOS_INSTALL}/base/etc/machine_id.txt
${TELEMETRY_OUTPUT_JSON}    ${SOPHOS_INSTALL}/base/telemetry/var/telemetry.json
${TELEMETRY_EXECUTABLE_LOG}    ${SOPHOS_INSTALL}/logs/base/sophosspl/telemetry.log


*** Keywords ***

Send Sav Policy With Imminent Scheduled Scan To Base
    ${time} =  Get Current Date  result_format=%y-%m-%d %H:%M:%S
    Create Sav Policy With Scheduled Scan  ${TEMP_SAV_POLICY_FILENAME}  ${time}
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

    #Run Process  chown  sophos-spl-user:sophos-spl-group  ${SOPHOS_INSTALL}/base/bin/telemetry
    #Run Process  chown  sophos-spl-user:sophos-spl-group  ${SOPHOS_INSTALL}/base/lib64/libtelemetry*.so.*   shell=True
    ${result} =  Run Process  sudo  -u  sophos-spl-user  ${SOPHOS_INSTALL}/base/bin/telemetry  ${telemetryConfigFilePath}   shell=True

    Log  "stdout = ${result.stdout}"
    Log  "stderr = ${result.stderr}"

    ${ls_result} =  Run Process  ls  -l  ${SOPHOS_INSTALL}/base/lib64/
    Log To Console  "ls = ${ls_result.stdout}"

    ${ldd_result} =  Run Process  ldd  ${SOPHOS_INSTALL}/base/bin/telemetry
    Log To Console  "ldd = ${ldd_result.stdout}"

    Log To Console  -----PASSWD------
    ${passwd_file} =  Get File  /etc/passwd
    Log To Console  ${passwd_file}

    Log To Console  -----GROUP------
    ${group_file} =  Get File  /etc/group
    Log To Console  ${group_file}

    Should Be Equal As Integers  ${result.rc}  ${expectedResult}  Telemetry executable returned a non-successful error code: ${result.stderr}

