*** Settings ***

Library         ../Libs/BaseInteractionTools/PolicyUtils.py
Library         ../Libs/ExclusionHelper.py
Library         ${COMMON_TEST_LIBS}/HttpsServer.py
Library         ../Libs/LogUtils.py
Library         ../Libs/OnAccessUtils.py
Library         ../Libs/OnFail.py
Library         ../Libs/Telemetry.py
Library         String
Library         DateTime

*** Variables ***
${TEMP_SAV_POLICY_FILENAME} =  TempSAVpolicy.xml

${CERT_PATH}                          ${COMMON_TEST_UTILS}/server_certs/server.crt
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

Send Sav Policy With Imminent Scheduled Scan To Base PUA Detections Disabled
    ${time} =  Get Current Date  result_format=%y-%m-%d %H:%M:%S
    Create Sav Policy With Scheduled Scan And Pua Detection Disabled  ${TEMP_SAV_POLICY_FILENAME}  ${time}
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
    [Arguments]  ${revId}=None
    Create Sav Policy With No Scheduled Scan    ${TEMP_SAV_POLICY_FILENAME}    revId=${revId}
    Send Sav Policy To Base  ${TEMP_SAV_POLICY_FILENAME}

Send Invalid Sav Policy
    Send Sav Policy To Base  SAV_Policy_Invalid.xml

Send Complete Sav Policy
    Create Complete Sav Policy  ${TEMP_SAV_POLICY_FILENAME}
    Send Sav Policy To Base  ${TEMP_SAV_POLICY_FILENAME}

Send Fixed Sav Policy
    Create Fixed Sav Policy  ${TEMP_SAV_POLICY_FILENAME}
    Send Sav Policy To Base  ${TEMP_SAV_POLICY_FILENAME}

Send Policies to enable on-access
    [Arguments]  ${flags_policy}=flags_policy/flags.json  ${oa_mark}=${None}
    ${oa_mark} =  get on access log mark if required  ${oa_mark}
    Register Cleanup If Unique  Send Policies to disable on-access
    # apply policies so that exclusions are applied before OA is enabled
    Send Sav Policy To Base  SAV-2_policy_OA_enabled.xml
    Send Flags Policy To Base  ${flags_policy}
    Send CORE Policy To Base  core_policy/CORE-36_oa_enabled.xml
    Wait for on access to be enabled  ${oa_mark}

Send Policies to enable on-access with exclusions
    Create File   /tmp_test/oa_file
    Register Cleanup   Remove File   /tmp_test/oa_file
    ${mark} =  get_on_access_log_mark
    Register Cleanup   Send Policies to disable on-access
    # apply policies so that exclusions are applied before OA is enabled
    Send Sav Policy To Base  SAV-2_policy_OA_enabled_with_exclusions.xml
    Send CORE Policy To Base  core_policy/CORE-36_oa_enabled.xml
    Send Flags Policy To Base  flags_policy/flags.json
    Wait for on access to be enabled  ${mark}   file=/tmp_test/oa_file

Send Policies to disable on-access without waiting
    Send Sav Policy To Base  SAV-2_policy_OA_disabled.xml
    Send CORE Policy To Base  core_policy/CORE-36_oa_disabled.xml
    Send Flags Policy To Base  flags_policy/flags.json


Send Policies to disable on-access
    ${mark} =  get_on_access_log_mark
    Send Policies to disable on-access without waiting
    ${expected} =  Create List  on-access will be disabled  On-access scanning disabled  On-access enabled: false
    wait for on access log contains after mark  ${expected}  mark=${mark}

Send Temp Policy To Base
    [Arguments]  ${policyFile}  ${destName}
    Run Process  chmod  666  ${policyFile}
    Move File  ${policyFile}  ${MCS_PATH}/policy/${destName}

Send Absolute Policy To Base
    [Arguments]  ${policyFile}  ${destName}
    Copy File  ${policyFile}  ${MCS_PATH}/tmp/${destName}
    Send Temp Policy To Base  ${MCS_PATH}/tmp/${destName}  ${destName}

Send Policy To Base
    [Arguments]  ${policyFile}  ${destName}
    Send Absolute Policy To Base  ${RESOURCES_PATH}/${policyFile}  ${destName}

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

Send CORE Policy To Base From Content
    [Arguments]  ${policyContent}
    Create File   ${MCS_PATH}/tmp/CORE_policy.xml.TEMP   ${policyContent}
    Send Temp Policy To Base  ${MCS_PATH}/tmp/CORE_policy.xml.TEMP   CORE-36_policy.xml

Send CORC Policy To Base
    [Arguments]  ${policyFile}
    Send Policy To Base  corc_policy/${policyFile}  CORC_policy.xml

Send CORC Policy To Base From Content
    [Arguments]  ${policyContent}
    Create File   ${MCS_PATH}/tmp/CORC_policy.xml.TEMP   ${policyContent}
    Send Temp Policy To Base  ${MCS_PATH}/tmp/CORC_policy.xml.TEMP   CORC_policy.xml

Send CORC Policy to Disable SXL
    ${policyContent} =   create_corc_policy  sxlLookupEnabled=${false}
    Send CORC Policy To Base From Content  ${policyContent}

Send SAV Policy To Base From Content
    [Arguments]  ${policyContent}
    Create File   ${MCS_PATH}/tmp/SAV_policy.xml.TEMP   ${policyContent}
    Send Temp Policy To Base  ${MCS_PATH}/tmp/SAV_policy.xml.TEMP   SAV-2_policy.xml

Send Flags Policy
    Send Flags Policy To Base   flags_policy/flags.json

Send Sav Policy To Base With Exclusions Filled In
    [Arguments]  ${policyFile}
    ExclusionHelper.Fill In On Demand Posix Exclusions  ${RESOURCES_PATH}/${policyFile}  ${RESOURCES_PATH}/FilledIn.xml
    Give Policy Unique Revision Id    ${RESOURCES_PATH}/FilledIn.xml    FilledInWithRevId.xml
    Send Sav Policy To Base  FilledInWithRevId.xml

Send Sav Action To Base
    [Arguments]  ${actionFile}
    Register Cleanup If Unique  Empty Directory  ${MCS_PATH}/action/
    ${savActionFilename} =  Generate Random String
    Copy File  ${RESOURCES_PATH}/${actionFile}  ${MCS_PATH}/tmp/SAV_action_${savActionFilename}.xml
    Move File  ${MCS_PATH}/tmp/SAV_action_${savActionFilename}.xml  ${MCS_PATH}/action/SAV_action_${savActionFilename}.xml

Send RA Action To Base
    Register Cleanup If Unique  Empty Directory  ${MCS_PATH}/action/
    Copy File  ${RESOURCES_PATH}/ResponseAction.json  ${MCS_PATH}/tmp/CORE_id1_request_2030-02-27T13:45:35.699544Z_144444000000004.json
    Move File  ${MCS_PATH}/tmp/CORE_id1_request_2030-02-27T13:45:35.699544Z_144444000000004.json
    ...  ${MCS_PATH}/action/CORE_id1_request_2030-02-27T13:45:35.699544Z_144444000000004.json

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
    Wait Until Created   ${TELEMETRY_OUTPUT_JSON}

Check AV Telemetry
    [Arguments]    ${telemetryKey}    ${telemetryValue}
    Run Telemetry Executable With HTTPS Protocol

    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    ${telemetryJson} =    Evaluate     json.loads("""${telemetryFileContents}""")    json

    Log    ${telemetryJson}
    Dictionary Should Contain Key    ${telemetryJson}    av
    Dictionary Should Contain Item   ${telemetryJson["av"]}   ${telemetryKey}   ${telemetryValue}
