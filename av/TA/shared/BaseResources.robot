*** Settings ***

Library         ../Libs/BaseInteractionTools/PolicyUtils.py
Library         ../Libs/ExclusionHelper.py
Library         ../Libs/HttpsServer.py
Library         ../Libs/OnFail.py
Library         ../Libs/Telemetry.py
Library         String
Library         DateTime

*** Variables ***

${TEMP_SAV_POLICY_FILENAME} =  TempSAVpolicy.xml

${CERT_PATH}   /tmp/cert.pem
${SSPL_BASE}                          ${SOPHOS_INSTALL}/base
${PLUGIN_REGISTRY}                    ${SSPL_BASE}/pluginRegistry
${MCS_ACTION_DIRECTORY}               ${SSPL_BASE}/mcs/action
${MACHINE_ID_FILE}                    ${SSPL_BASE}/etc/machine_id.txt
${EXE_CONFIG_FILE}                    ${SSPL_BASE}/telemetry/var/telemetry-exe.json
${TELEMETRY_OUTPUT_JSON}              ${SSPL_BASE}/telemetry/var/telemetry.json
${TELEMETRY_BACKUP_JSON}              ${SSPL_BASE}/telemetry/cache/av-telemetry.json
${SAFESTORE_TELEMETRY_BACKUP_JSON}    ${SSPL_BASE}/telemetry/cache/safestore-telemetry.json
${TELEMETRY_EXECUTABLE_LOG}           ${SOPHOS_INSTALL}/logs/base/sophosspl/telemetry.log

*** Keywords ***

Send Sav Policy With Imminent Scheduled Scan To Base
    ${time} =  Get Current Date  result_format=%y-%m-%d %H:%M:%S
    Create Sav Policy With Scheduled Scan  ${TEMP_SAV_POLICY_FILENAME}  ${time}
    Send Sav Policy To Base  ${TEMP_SAV_POLICY_FILENAME}

Send Sav Policy With Multiple Imminent Scheduled Scans To Base
    ${time} =  Get Current Date  result_format=%y-%m-%d %H:%M:%S
    Create Sav Policy With Multiple Scheduled Scans  ${TEMP_SAV_POLICY_FILENAME}  ${time}
    Send Sav Policy To Base  ${TEMP_SAV_POLICY_FILENAME}

Send Sav Policy With Multiple Scheduled Scans
    Send Sav Policy To Base  SAV_Policy_Multiple_Scans.xml

Send Sav Policy With No Scheduled Scans
    Send Sav Policy To Base  SAV_Policy_No_Scans.xml

Send Invalid Sav Policy
    Send Sav Policy To Base  SAV_Policy_Invalid.xml

Send Complete Sav Policy
    Create Complete Sav Policy  ${TEMP_SAV_POLICY_FILENAME}
    Send Sav Policy To Base  ${TEMP_SAV_POLICY_FILENAME}

Send Fixed Sav Policy
    Create Fixed Sav Policy  ${TEMP_SAV_POLICY_FILENAME}
    Send Sav Policy To Base  ${TEMP_SAV_POLICY_FILENAME}

Send Policies to enable on-access
    ${mark} =  get_on_access_log_mark
    Register Cleanup   Send Policies to disable on-access
    Send Flags Policy To Base  flags_policy/flags_onaccess_enabled.json
    Send Sav Policy To Base  SAV-2_policy_OA_enabled.xml
    Wait for on access to be enabled  ${mark}

Send Policies to disable on-access
    ${mark} =  get_on_access_log_mark
    Send Sav Policy To Base  SAV-2_policy_OA_disabled.xml
    wait for on access log contains after mark  Finished ProcessPolicy  mark=${mark}
    #TODO: LINUXDAR-5723 re-enable after ticket is fixed
    #Send Flags Policy To Base  flags_policy/flags.json

Send Sav Policy To Base
    [Arguments]  ${policyFile}
    Copy File  ${RESOURCES_PATH}/${policyFile}  ${MCS_PATH}/policy/SAV-2_policy.xml

Send Alc Policy
    Send Alc Policy To Base   ALC_Policy.xml

Send Alc Policy To Base
    [Arguments]  ${policyFile}
    Copy File  ${RESOURCES_PATH}/${policyFile}  ${MCS_PATH}/policy/ALC-1_policy.xml

Send Flags Policy To Base
    [Arguments]  ${policyFile}
    Copy File  ${RESOURCES_PATH}/${policyFile}  ${MCS_PATH}/policy/flags.json

Send Flags Policy
    Send Flags Policy To Base   flags_policy/flags.json

Send Sav Policy To Base With Exclusions Filled In
    [Arguments]  ${policyFile}
    ExclusionHelper.Fill In On Demand Posix Exclusions  ${RESOURCES_PATH}/${policyFile}  ${RESOURCES_PATH}/FilledIn.xml
    Copy File  ${RESOURCES_PATH}/FilledIn.xml  ${MCS_PATH}/policy/SAV-2_policy.xml

Send Sav Action To Base
    [Arguments]  ${actionFile}
    Register Cleanup If Unique  Empty Directory  ${MCS_PATH}/action/
    ${savActionFilename} =  Generate Random String
    Copy File  ${RESOURCES_PATH}/${actionFile}  ${MCS_PATH}/action/SAV_action_${savActionFilename}.xml

Prepare To Run Telemetry Executable
    Prepare To Run Telemetry Executable With HTTPS Protocol

Prepare To Run Telemetry Executable With HTTPS Protocol
    [Arguments]  ${port}=${443}  ${TLSProtocol}=tlsv1_2
    HttpsServer.Start Https Server  ${CERT_PATH}  ${port}  ${TLSProtocol}
    Register Cleanup If Unique  Stop Https Server

    Wait Until Keyword Succeeds  10 seconds  1.0 seconds  File Should Exist  ${MACHINE_ID_FILE}
    Create Test Telemetry Config File  ${EXE_CONFIG_FILE}  ${CERT_PATH}  port=${port}

Run Telemetry Executable
    [Arguments]  ${telemetryConfigFilePath}  ${expectedResult}   ${checkResult}=1

    Remove File  ${TELEMETRY_EXECUTABLE_LOG}

    ${result}=  Telemetry.run_telemetry  ${SSPL_BASE}/bin/telemetry  ${telemetryConfigFilePath}

    Should Be Equal As Integers  ${result.rc}  ${expectedResult}  Telemetry executable returned a non-successful error code: ${result.stdout}

Run Telemetry Executable With HTTPS Protocol
    [Arguments]  ${port}=${443}  ${TLSProtocol}=tlsv1_2
    Prepare To Run Telemetry Executable With HTTPS Protocol   port=${port}  TLSProtocol=${TLSProtocol}
    Remove File  ${TELEMETRY_OUTPUT_JSON}
    Run Telemetry Executable     ${EXE_CONFIG_FILE}     ${0}
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  File Should Exist  ${TELEMETRY_OUTPUT_JSON}

Check AV Telemetry
    [Arguments]    ${telemetryKey}    ${telemetryValue}
    Run Telemetry Executable With HTTPS Protocol

    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    ${telemetryJson} =    Evaluate     json.loads("""${telemetryFileContents}""")    json

    Log    ${telemetryJson}
    Dictionary Should Contain Key    ${telemetryJson}    av
    Dictionary Should Contain Item   ${telemetryJson["av"]}   ${telemetryKey}   ${telemetryValue}

Check SafeStore Telemetry
    [Arguments]    ${telemetryKey}    ${telemetryValue}
    Run Telemetry Executable With HTTPS Protocol

    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    ${telemetryJson} =    Evaluate     json.loads("""${telemetryFileContents}""")    json

    Log    ${telemetryJson}
    Dictionary Should Contain Key    ${telemetryJson}    safestore
    Dictionary Should Contain Item   ${telemetryJson["safestore"]}   ${telemetryKey}   ${telemetryValue}

Dump All Sophos Processes
    ${result}=  Run Process    ps -elf | grep sophos    shell=True
    Log  ${result.stdout}

Log Status Of Sophos Spl
    ${result} =  Run Process    systemctl  status  sophos-spl
    Log  ${result.stdout}
    ${result} =  Run Process    systemctl  status  sophos-spl-update
    Log  ${result.stdout}
    ${result} =  Run Process    systemctl  status  sophos-spl-diagnose
    Log  ${result.stdout}

