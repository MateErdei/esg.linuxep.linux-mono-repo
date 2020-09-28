*** Settings ***
Resource    AVResources.robot
Resource    BaseResources.robot

*** Keywords ***

AV and Base Setup
    Remove Directory  /tmp/DiagnoseOutput  true

Check avscanner in /usr/local/bin
    File Should Exist  /usr/local/bin/avscanner

Stop AV Plugin
    ${result} =    Run Process    ${SOPHOS_INSTALL}/bin/wdctl   stop   av
    Should Be Equal As Integers    ${result.rc}    0

Start AV Plugin
    ${result} =    Run Process    ${SOPHOS_INSTALL}/bin/wdctl   start   av
    Should Be Equal As Integers    ${result.rc}    0

Check avscanner not in /usr/local/bin
    File Should Not Exist  /usr/local/bin/avscanner

Check AV Plugin Not Installed
    Directory Should Not Exist  ${SOPHOS_INSTALL}/plugins/${COMPONENT}
    File Should Not Exist  ${SOPHOS_INSTALL}/base/pluginRegistry/av.json

Run plugin uninstaller
    Run Process  ${COMPONENT_SBIN_DIR}/uninstall.sh

Configure and check scan now
    Configure scan now
    Check scan now

Configure scan now
    Send Sav Policy To Base With Exclusions Filled In  SAV_Policy_Scan_Now.xml
    Wait until scheduled scan updated

Check scan now
    Send Sav Action To Base  ScanNow_Action.xml
    Wait Until AV Plugin Log Contains  Completed scan Scan Now  timeout=180  interval=10
    AV Plugin Log Contains  Evaluating Scan Now
    AV Plugin Log Contains  Starting scan Scan Now

Validate latest Event
    [Arguments]  ${now}
    ${eventXml}=  get_latest_event_xml_from_events  base/mcs/event/  ${now}
    Log File  ${eventXml}
    ${parsedXml}=  parse xml  ${eventXml}
    Should Be Equal  ${parsedXml.tag}  event
    ELEMENT TEXT SHOULD MATCH  source=${parsedXml}  pattern=Scan Now  normalize_whitespace=True  xpath=scanComplete
