*** Settings ***

Library         ../Libs/BaseInteractionTools/PolicyUtils.py
Library         ../Libs/ExclusionHelper.py
Library         ../Libs/HttpsServer.py
Library         ../Libs/LogUtils.py
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
${TELEMETRY_EXECUTABLE_LOG}           ${SOPHOS_INSTALL}/logs/base/sophosspl/telemetry.log

*** Keywords ***

Send Sav Policy With Imminent Scheduled Scan To Base
    ${time} =  Get Current Date  result_format=%y-%m-%d %H:%M:%S
    Create Sav Policy With Scheduled Scan  ${TEMP_SAV_POLICY_FILENAME}  ${time}
    Send Sav Policy To Base  ${TEMP_SAV_POLICY_FILENAME}

Send Sav Policy With Imminent Scheduled Scan To Base Exclusions Added
    ${time} =  Get Current Date  result_format=%y-%m-%d %H:%M:%S
    Create Sav Policy With Scheduled Scan  ${TEMP_SAV_POLICY_FILENAME}  ${time}
    Send Sav Policy To Base With Exclusions Filled In  ${TEMP_SAV_POLICY_FILENAME}

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
    [Arguments]  ${flags_policy}=flags_policy/flags_onaccess_enabled.json  ${oa_mark}=${None}
    ${oa_mark} =  get on access log mark if required  ${oa_mark}
    Register Cleanup If Unique  Send Policies to disable on-access
    Send Flags Policy To Base  ${flags_policy}
    Send CORE Policy To Base  core_policy/CORE-36_oa_enabled.xml
    Send Sav Policy To Base  SAV-2_policy_OA_enabled.xml
    Wait for on access to be enabled  ${oa_mark}

Send Policies to enable on-access with exclusions
    Create File   /tmp_test/oa_file
    Register Cleanup   Remove File   /tmp_test/oa_file
    ${mark} =  get_on_access_log_mark
    Register Cleanup   Send Policies to disable on-access
    # apply policies so that exclusions are applied before OA is enabled
    Send Sav Policy To Base  SAV-2_policy_OA_enabled_with_exclusions.xml
    Send CORE Policy To Base  core_policy/CORE-36_oa_enabled.xml
    Send Flags Policy To Base  flags_policy/flags_onaccess_enabled.json
    Wait for on access to be enabled  ${mark}   file=/tmp_test/oa_file

Send Policies to disable on-access
    ${mark} =  get_on_access_log_mark
    Send Sav Policy To Base  SAV-2_policy_OA_disabled.xml
    Send CORE Policy To Base  core_policy/CORE-36_oa_disabled.xml
    Send Flags Policy To Base  flags_policy/flags.json
    ${expected} =  Create List  on-access will be disabled  On-access scanning disabled  On-access enabled: false
    wait for on access log contains after mark  ${expected}  mark=${mark}

Send Policy To Base
    [Arguments]  ${policyFile}  ${destName}
    Copy File  ${RESOURCES_PATH}/${policyFile}  ${MCS_PATH}/policy/${destName}



Send Sav Policy To Base
    [Arguments]  ${policyFile}
    Send Policy To Base  ${policyFile}  SAV-2_policy.xml

Send Alc Policy
    Send Alc Policy To Base   ALC_Policy.xml

Send Alc Policy To Base
    [Arguments]  ${policyFile}
    Send Policy To Base  ${policyFile}  ALC-1_policy.xml

Send Flags Policy To Base
    [Arguments]  ${policyFile}
    Send Policy To Base  ${policyFile}  flags.json

Send CORE Policy To Base
    [Arguments]  ${policyFile}
    Send Policy To Base  ${policyFile}  CORE-36_policy.xml

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

Send CORC Policy To Base
    [Arguments]  ${policyFile}
    Copy File  ${RESOURCES_PATH}/corc_policy/${policyFile}  ${SOPHOS_INSTALL}/CORC_policy.xml.TEMP
    Run Process  chmod  666  ${SOPHOS_INSTALL}/CORC_policy.xml.TEMP
    Move File  ${SOPHOS_INSTALL}/CORC_policy.xml.TEMP  ${MCS_PATH}/policy/CORC_policy.xml

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

