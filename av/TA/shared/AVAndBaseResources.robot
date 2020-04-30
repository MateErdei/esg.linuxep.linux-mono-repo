*** Keywords ***

AV and Base Setup
    Remove Directory  /tmp/DiagnoseOutput  true

Check avscanner in /usr/local/bin
    File Should Exist  /usr/local/bin/avscanner

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
    Send Sav Policy To Base  SAV_Policy_Scan_Now.xml
    Wait Until AV Plugin Log Contains  Updating scheduled scan configuration

Check scan now
    Send Sav Action To Base  ScanNow_Action.xml
    Wait Until AV Plugin Log Contains  Completed scan Scan Now  timeout=180
    AV Plugin Log Contains  Starting Scan Now scan
    AV Plugin Log Contains  Starting scan Scan Now

Validate latest Event
     ${eventXml}=  get_latest_xml_from_events  base/mcs/event/
     ${parsedXml}=  parse xml  ${eventXml}
     ELEMENT TEXT SHOULD MATCH  source=${parsedXml}  pattern=Scan Now  normalize_whitespace=True  xpath=scanComplete
