*** Settings ***

Library         ../Libs/BaseInteractionTools/DiagnoseUtils.py
Library         ../Libs/BaseInteractionTools/PolicyUtils.py
Library         ../Libs/FakeManagement.py
Library         String
Library         DateTime
Resource        ../shared/AVResources.robot

*** Variables ***

${TAR_FILE_DIRECTORY} =  /tmp/TestOutputDirectory
${UNPACK_DIRECTORY} =  /tmp/DiagnoseOutput
${UNPACKED_DIAGNOSE_PLUGIN_FILES} =  ${UNPACK_DIRECTORY}/PluginFiles
${TEMP_SAV_POLICY_FILENAME} =  TempSAVpolicy.xml
${ACTION_CONTENT} =  <?xml version="1.0"?><a:action xmlns:a="com.sophos/msys/action" type="ScanNow" id="" subtype="ScanMyComputer" replyRequired="1"/>

*** Keywords ***

Run Scheduled Scan
    ${time} =  Get Current Date  result_format=%y-%m-%d %H:%M:%S
    Create Sav Policy With Scheduled Scan  ${TEMP_SAV_POLICY_FILENAME}  ${time}
    ${policy_contents} =  Get File  resources/${TEMP_SAV_POLICY_FILENAME}
    Send Plugin Policy  av  sav  ${policy_contents}
    Wait Until AV Plugin Log Contains  Updating scheduled scan configuration  timeout=240
#    Wait Until AV Plugin Log Contains  Starting scan Sophos Cloud Scheduled Scan  timeout=240

Run Scan Now Scan
    Create Complete Sav Policy  ${TEMP_SAV_POLICY_FILENAME}
    ${policy_contents} =  Get File  resources/${TEMP_SAV_POLICY_FILENAME}
    Send Plugin Policy  av  sav  ${policy_contents}
    Wait Until AV Plugin Log Contains  Updating scheduled scan configuration  timeout=240
    Send Plugin Action  av  sav  corr123  ${ACTION_CONTENT}

