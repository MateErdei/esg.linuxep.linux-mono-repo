*** Settings ***
Resource    AVResources.robot
Resource    BaseResources.robot

*** Keywords ***

AV and Base Setup
    Require Plugin Installed and Running
    Clear AV Plugin Logs If They Are Close To Rotating For Integration Tests
    Remove Directory  /tmp/DiagnoseOutput  true

Check avscanner in /usr/local/bin
    File Should Exist  /usr/local/bin/avscanner

Stop AV Plugin
    ${result} =    Run Process    ${SOPHOS_INSTALL}/bin/wdctl   stop   av
    Should Be Equal As Integers    ${result.rc}    0
    ${result} =    Run Process    ${SOPHOS_INSTALL}/bin/wdctl   stop   threat_detector
    Should Be Equal As Integers    ${result.rc}    0

Start AV Plugin
    ${result} =    Run Process    ${SOPHOS_INSTALL}/bin/wdctl   start   threat_detector
    Should Be Equal As Integers    ${result.rc}    0
    ${result} =    Run Process    ${SOPHOS_INSTALL}/bin/wdctl   start   av
    Should Be Equal As Integers    ${result.rc}    0

Check avscanner not in /usr/local/bin
    File Should Not Exist  /usr/local/bin/avscanner

Wait Until Logs Exist
    Wait Until Keyword Succeeds
    ...  5 secs
    ...  1 secs
    ...  File Should Exist  ${SOPHOS_INSTALL}/plugins/${COMPONENT}/log/av.log
    Wait Until Keyword Succeeds
    ...  5 secs
    ...  1 secs
    ...  File Should Exist  ${SOPHOS_INSTALL}/plugins/${COMPONENT}/chroot/log/sophos_threat_detector.log

User Should Not Exist
    [Arguments]  ${user}
    File Log Should Not Contain   /etc/passwd   ${user}

Check AV Plugin Not Installed
    Directory Should Not Exist  ${SOPHOS_INSTALL}/plugins/${COMPONENT}/sbin/
    File Should Not Exist  ${PLUGIN_REGISTRY}/av.json
    File Should Not Exist  ${PLUGIN_REGISTRY}/sophos_threat.json
    User Should Not Exist  sophos-spl-av
    User Should Not Exist  sophos-spl-threat-detector

Check Logs Saved On Downgrade
    Directory Should Exist  ${SOPHOS_INSTALL}/tmp/av_downgrade/
    File Should Exist  ${SOPHOS_INSTALL}/tmp/av_downgrade/av.log
    File Should Exist  ${SOPHOS_INSTALL}/tmp/av_downgrade/sophos_threat_detector.log

Run plugin uninstaller
    Run Process  ${COMPONENT_SBIN_DIR}/uninstall.sh

Run plugin uninstaller with downgrade flag
    Run Process  ${COMPONENT_SBIN_DIR}/uninstall.sh  --downgrade

Configure and check scan now with offset
    Configure scan now
    Check scan now with Offset

Configure and check scan now with lookups disabled
    Configure scan now with lookups disabled
    Check scan now with Offset

Configure scan now
    Mark Sophos Threat Detector Log
    Send Sav Policy To Base With Exclusions Filled In  SAV_Policy_Scan_Now.xml
    Wait until scheduled scan updated

Configure scan now with lookups disabled
    Send Sav Policy To Base With Exclusions Filled In  SAV_Policy_Scan_Now_Lookup_Disabled.xml
    Wait Until AV Plugin Log Contains With Offset  Restarting sophos_threat_detector as the system/susi configuration has changed
    AV Plugin Log Does Not Contain With Offset  Failed to send shutdown request: Failed to connect to unix socket

    # Force SUSI to be loaded:
    Check avscanner can detect eicar

    Wait Until Sophos Threat Detector Log Contains With Offset  SXL Lookups will be disabled   timeout=10
    Wait until scheduled scan updated

Check scan now with Offset
    Mark AV Log
    Send Sav Action To Base  ScanNow_Action.xml
    Wait Until AV Plugin Log Contains With Offset  Completed scan Scan Now  timeout=180  interval=10
    AV Plugin Log Contains With Offset  Evaluating Scan Now
    AV Plugin Log Contains With Offset  Starting scan Scan Now

Validate latest Event
    [Arguments]  ${now}
    ${eventXml}=  get_latest_event_xml_from_events  base/mcs/event/  ${now}
    Log File  ${eventXml}
    ${parsedXml}=  parse xml  ${eventXml}
    Should Be Equal  ${parsedXml.tag}  event
    ELEMENT TEXT SHOULD MATCH  source=${parsedXml}  pattern=Scan Now  normalize_whitespace=True  xpath=scanComplete

Create Encoded Eicars
   ${result} =  Run Process  bash  ${BASH_SCRIPTS_PATH}/createEncodingEicars.sh  stderr=STDOUT
   Should Be Equal As Integers  ${result.rc}  0
   Log  ${result.stdout}

Stop sophos_threat_detector
    ${result} =    Run Process    ${SOPHOS_INSTALL}/bin/wdctl   stop   threat_detector
    Should Be Equal As Integers    ${result.rc}    0

Start sophos_threat_detector
    ${result} =    Run Process    ${SOPHOS_INSTALL}/bin/wdctl   start  threat_detector
    Should Be Equal As Integers    ${result.rc}    0

Restart sophos_threat_detector
    Stop sophos_threat_detector
    Start sophos_threat_detector
    Wait until threat detector running

Restart sophos_threat_detector and mark logs
    Stop sophos_threat_detector
    Mark AV Log
    Mark Sophos Threat Detector Log
    Start sophos_threat_detector
    Wait until threat detector running
