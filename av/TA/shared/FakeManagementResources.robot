*** Settings ***

Library         ../Libs/BaseInteractionTools/PolicyUtils.py
Library         ../Libs/FakeManagement.py
Library         ../Libs/ExclusionHelper.py
Library         String
Library         DateTime
Resource        ../shared/AVResources.robot

*** Variables ***
${TEMP_SAV_POLICY_FILENAME} =  TempSAVpolicy.xml
${SAV_POLICY_FOR_SCAN_NOW_TEST} =  ExclusionsTestPolicy.xml
${SAV_POLICY_FOR_EXCLUSION_TYPE_TEST} =  DifferentExclusionTypesPolicy.xml
${ACTION_CONTENT} =  <?xml version="1.0"?><a:action xmlns:a="com.sophos/msys/action" type="ScanNow" id="" subtype="ScanMyComputer" replyRequired="1"/>

*** Keywords ***

Run Scheduled Scan
    ${time} =  Get Current Date  result_format=%y-%m-%d %H:%M:%S
    Create Sav Policy With Scheduled Scan  ${TEMP_SAV_POLICY_FILENAME}  ${time}
    ${policy_contents} =  Get File  ${RESOURCES_PATH}/${TEMP_SAV_POLICY_FILENAME}
    Send Plugin Policy  av  sav  ${policy_contents}
    Wait until scheduled scan updated


Run Scheduled Scan With On Access Enabled
    ${time} =  Get Current Date  result_format=%y-%m-%d %H:%M:%S
    Create Sav Policy With Scheduled Scan And On Access Enabled  ${TEMP_SAV_POLICY_FILENAME}  ${time}
    ${policy_contents} =  Get File  ${RESOURCES_PATH}/${TEMP_SAV_POLICY_FILENAME}
    Send Plugin Policy  av  sav  ${policy_contents}
    Wait until scheduled scan updated


Configure Scan Now Scan
    ${policy_contents} =  Get Complete Sav Policy
    Send Plugin Policy  av  sav  ${policy_contents}
    Wait until scheduled scan updated

Configure Scan Now Scan With On Access Enabled
    Create Sav Policy With On Access Enabled  ${TEMP_SAV_POLICY_FILENAME}
    ${policy_contents} =  Get File  ${RESOURCES_PATH}/${TEMP_SAV_POLICY_FILENAME}
    Send Plugin Policy  av  sav  ${policy_contents}
    Wait until scheduled scan updated With Offset

Trigger Scan Now Scan
    Send Plugin Action  av  sav  corr123  ${ACTION_CONTENT}

Run Scan Now Scan
    ${policy_contents} =  Get Complete Sav Policy
    Send Plugin Policy  av  sav  ${policy_contents}
    Wait until scheduled scan updated With Offset

    Send Plugin Action  av  sav  corr123  ${ACTION_CONTENT}

Run Scan Now Scan For Excluded Files Test
    ${policy_contents} =  Replace Exclusions For Exclusion Test  ${RESOURCES_PATH}/${SAV_POLICY_FOR_SCAN_NOW_TEST}

    Send Plugin Policy  av  sav  ${policy_contents}
    Wait until scheduled scan updated With Offset

    Send Plugin Action  av  sav  corr123  ${ACTION_CONTENT}

Run Scan Now Scan With No Exclusions
    ${policy_contents} =  Get Sav Policy With No Exclusions  ${RESOURCES_PATH}/${SAV_POLICY_FOR_SCAN_NOW_TEST}
    Send Plugin Policy  av  sav  ${policy_contents}
    Wait until scheduled scan updated With Offset

    Send Plugin Action  av  sav  corr123  ${ACTION_CONTENT}

# list of exclusions: https://wiki.sophos.net/display/SAVLU/Exclusions
Run Scan Now Scan With All Types of Exclusions
    ${policy_contents} =  Get File  ${RESOURCES_PATH}/${SAV_POLICY_FOR_EXCLUSION_TYPE_TEST}
    Send Plugin Policy  av  sav  ${policy_contents}
    Wait until scheduled scan updated With Offset

    Send Plugin Action  av  sav  corr123  ${ACTION_CONTENT}
