*** Settings ***

Library         ../Libs/BaseInteractionTools/DiagnoseUtils.py
Library         ../Libs/BaseInteractionTools/PolicyUtils.py
Library         String
Library         DateTime

*** Variables ***

${TAR_FILE_DIRECTORY} =  /tmp/TestOutputDirectory
${UNPACK_DIRECTORY} =  /tmp/DiagnoseOutput
${UNPACKED_DIAGNOSE_PLUGIN_FILES} =  ${UNPACK_DIRECTORY}/PluginFiles
${TEMP_SAV_POLICY_FILENAME} =  TempSAVpolicy.xml

*** Keywords ***

Run Diagnose
    Create Directory  ${TAR_FILE_DIRECTORY}
    ${retcode} =  Start Diagnose  ${SOPHOS_INSTALL}/bin/sophos_diagnose  ${TAR_FILE_DIRECTORY}
    Should Be Equal As Integers   ${retcode}  0

Check Diagnose Tar Created
    ${Files} =  List Files In Directory  ${TAR_FILE_DIRECTORY}/
    ${fileCount} =    Get length    ${Files}
    Should Be Equal As Numbers  ${fileCount}  1
    Should Contain    ${Files[0]}    sspl-diagnose
    Should Not Contain   ${Files}  BaseFiles
    Should Not Contain   ${Files}  SystemFiles
    Should Not Contain   ${Files}  PluginFiles

Check Diagnose Collects Correct AV Files
    ${Files} =  List Files In Directory  ${TAR_FILE_DIRECTORY}/
    Create Directory  ${UNPACK_DIRECTORY}
    ${result} =   Run Process   tar    xzf    ${TAR_FILE_DIRECTORY}/${Files[0]}    -C    ${UNPACK_DIRECTORY}/
    Should Be Equal As Strings   ${result.rc}  0

    ${PluginFiles} =  List Files In Directory  ${UNPACKED_DIAGNOSE_PLUGIN_FILES}
    ${AVFiles} =  List Files In Directory  ${UNPACKED_DIAGNOSE_PLUGIN_FILES}/av

    Should Contain  ${PluginFiles}  av.log
    Should Contain  ${PluginFiles}  Scan Now.log
    Should Contain  ${PluginFiles}  sophos_threat_detector.log

    Should Contain  ${AVFiles}  VERSION.ini
    Should Contain  ${AVFiles}  VERSION.ini.0


Check Diagnose Logs
    ${Files} =  List Files In Directory  ${TAR_FILE_DIRECTORY}/
    ${contents} =  Get File  /tmp/diagnose.log
    Should Not Contain  ${contents}  error  ignore_case=True
    Should Contain  ${contents}   Created tarfile: ${Files[0]} in directory ${TAR_FILE_DIRECTORY}

Send Sav Policy With Imminent Scheduled Scan To Base
    # TODO LINUXDAR-1482 Change this so it can configure more than just the time
    ${time} =  Get Current Date  result_format=%y-%m-%d %H:%M:%S
    Create Sav Policy With Scheduled Scan  ${time}  ${TEMP_SAV_POLICY_FILENAME}
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

Send Sav Action To Base
    [Arguments]  ${actionFile}
    ${savActionFilename}  Generate Random String
    Copy File  ${RESOURCES_PATH}/${actionFile}  ${MCS_PATH}/action/SAV_action_${savActionFilename}.xml