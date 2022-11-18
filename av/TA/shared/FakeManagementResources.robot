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
    Send Plugin Policy  av  ${SAV_APPID}  ${policy_contents}
    Wait until scheduled scan updated


Run Scheduled Scan With On Access Enabled
    ${time} =  Get Current Date  result_format=%y-%m-%d %H:%M:%S
    Create Sav Policy With Scheduled Scan And On Access Enabled  ${TEMP_SAV_POLICY_FILENAME}  ${time}
    send av policy from file  ${SAV_APPID}  ${RESOURCES_PATH}/${TEMP_SAV_POLICY_FILENAME}
    Wait until scheduled scan updated


Configure Scan Now Scan
    ${policy_contents} =  Get Complete Sav Policy
    send av policy  ${SAV_APPID}  ${policy_contents}
    Wait until scheduled scan updated

Configure Scan Now Scan With On Access Enabled
    ${mark} =  get_av_log_mark
    Create Sav Policy With On Access Enabled  ${TEMP_SAV_POLICY_FILENAME}
    send av policy from file  ${SAV_APPID}  ${RESOURCES_PATH}/${TEMP_SAV_POLICY_FILENAME}
    Wait until scheduled scan updated After Mark  ${mark}

Trigger Scan Now Scan
    Send Plugin Action  av  ${SAV_APPID}  corr123  ${ACTION_CONTENT}

Run Scan Now Scan
    ${mark} =  get_av_log_mark
    ${policy_contents} =  Get Complete Sav Policy
    send av policy  ${SAV_APPID}  ${policy_contents}
    Wait until scheduled scan updated After Mark  ${mark}

    Send Plugin Action  av  ${SAV_APPID}  corr123  ${ACTION_CONTENT}

Run Scan Now Scan For Excluded Files Test
    ${mark} =  get_av_log_mark
    ${policy_contents} =  Replace Exclusions For Exclusion Test  ${RESOURCES_PATH}/${SAV_POLICY_FOR_SCAN_NOW_TEST}
    send av policy  SAV  ${policy_contents}
    Wait until scheduled scan updated After Mark  ${mark}

    Send Plugin Action  av  ${SAV_APPID}  corr123  ${ACTION_CONTENT}

Run Scan Now Scan With No Exclusions
    ${mark} =  get_av_log_mark
    ${policy_contents} =  Get Sav Policy With No Exclusions  ${RESOURCES_PATH}/${SAV_POLICY_FOR_SCAN_NOW_TEST}
    send av policy  ${SAV_APPID}  ${policy_contents}
    Wait until scheduled scan updated After Mark  ${mark}

    Send Plugin Action  av  ${SAV_APPID}  corr123  ${ACTION_CONTENT}

# list of exclusions: https://wiki.sophos.net/display/SAVLU/Exclusions
Run Scan Now Scan With All Types of Exclusions
    ${mark} =  get_av_log_mark
    send av policy from file  ${SAV_APPID}  ${RESOURCES_PATH}/${SAV_POLICY_FOR_EXCLUSION_TYPE_TEST}
    Wait until scheduled scan updated After Mark  ${mark}

    Send Plugin Action  av  ${SAV_APPID}  corr123  ${ACTION_CONTENT}
