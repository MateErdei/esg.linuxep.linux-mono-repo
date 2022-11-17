*** Settings ***
Resource    AVResources.robot
Resource    BaseResources.robot

Library         ../Libs/CoreDumps.py

*** Keywords ***

AV and Base Setup
    # ignore occasional SXL4 timeouts
    Register Cleanup  Exclude Globalrep Timeout Errors
    Register Cleanup  Require No Unhandled Exception
    Register Cleanup  analyse Journalctl   print_always=True
    Register Cleanup  Dump All Sophos Processes
    Register Cleanup  Log Status Of Sophos Spl
    Register Cleanup  Check For Coredumps  ${TEST NAME}
    Register Cleanup  Check Dmesg For Segfaults
    Clear AV Plugin Logs If They Are Close To Rotating For Integration Tests
    Require Plugin Installed and Running
    Send Alc Policy
    Send Sav Policy With No Scheduled Scans
    Send Flags Policy

    Remove Directory  /tmp/DiagnoseOutput  true

    ${result} =  Run Process  id  sophos-spl-user  stderr=STDOUT
    Log  "id sophos-spl-user = ${result.stdout}"

Check avscanner in /usr/local/bin
    File Should Exist  /usr/local/bin/avscanner

Stop AV Plugin Process
    ${result} =    Run Process    ${SOPHOS_INSTALL}/bin/wdctl   stop   av
    Should Be Equal As Integers    ${result.rc}    ${0}

Start AV Plugin Process
    ${result} =    Run Process    ${SOPHOS_INSTALL}/bin/wdctl   start   av
    Should Be Equal As Integers    ${result.rc}    ${0}

Restart AV Plugin
    Stop AV Plugin
    Start AV Plugin

Stop AV Plugin
    Stop AV Plugin Process
    Stop Sophos_Threat_Detector

Start AV Plugin
    Start AV Plugin Process
    Start Sophos_Threat_Detector

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

User Should Exist
    [Arguments]  ${user}
    File Log Contains   /etc/passwd   ${user}

Check AV Plugin Not Installed
    Directory Should Not Exist  ${SOPHOS_INSTALL}/plugins/${COMPONENT}/sbin/
    File Should Not Exist  ${PLUGIN_REGISTRY}/av.json
    File Should Not Exist  ${PLUGIN_REGISTRY}/sophos_threat.json
    User Should Not Exist  sophos-spl-av
    User Should Not Exist  sophos-spl-threat-detector

Check Logs Saved On Downgrade
    Directory Should Exist  ${AV_BACKUP_DIR}
    File Should Exist  ${AV_BACKUP_DIR}/soapd.log
    File Should Exist  ${AV_BACKUP_DIR}/av.log
    File Should Exist  ${AV_BACKUP_DIR}/sophos_threat_detector.log

Run plugin uninstaller
    ${result} =   Run Process  ${COMPONENT_SBIN_DIR}/uninstall.sh   stderr=STDOUT
    Log    ${result.stdout}

Run plugin uninstaller with downgrade flag
    ${result} =   Run Process  ${COMPONENT_SBIN_DIR}/uninstall.sh  --downgrade   stderr=STDOUT
    Log    ${result.stdout}

Configure and check scan now with offset
    Configure scan now
    Check scan now with Offset

Configure and check scan now with lookups disabled
    Configure scan now with lookups disabled
    Check scan now with Offset

Configure scan now
    Mark AV Log
    Send Sav Policy To Base With Exclusions Filled In  SAV_Policy_Scan_Now.xml
    # Run Keyword and Ignore Error
    Wait until scheduled scan updated With Offset

Configure scan now with lookups disabled
    Send Sav Policy To Base With Exclusions Filled In  SAV_Policy_Scan_Now_Lookup_Disabled.xml
    Wait Until AV Plugin Log Contains With Offset  Reloading susi as policy configuration has changed
    AV Plugin Log Does Not Contain With Offset  Failed to send shutdown request: Failed to connect to unix socket

    # Force SUSI to be loaded:
    Check avscanner can detect eicar

    Wait Until Sophos Threat Detector Log Contains With Offset  SXL Lookups will be disabled   timeout=10
    Run Keyword and Ignore Error  Wait until scheduled scan updated

Check scan now with Offset
    Mark AV Log
    Register On Fail If Unique  dump log  ${SCANNOW_LOG_PATH}
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
   Should Be Equal As Integers  ${result.rc}  ${0}
   Log  ${result.stdout}

Stop sophos_threat_detector
    ${result} =    Run Process    ${SOPHOS_INSTALL}/bin/wdctl   stop   threat_detector
    Should Be Equal As Integers    ${result.rc}    ${0}

Start sophos_threat_detector
    ${result} =    Run Process    ${SOPHOS_INSTALL}/bin/wdctl   start  threat_detector
    Should Be Equal As Integers    ${result.rc}    ${0}

Stop soapd
    ${result} =    Run Process    ${SOPHOS_INSTALL}/bin/wdctl   stop   on_access_process
    Should Be Equal As Integers    ${result.rc}    ${0}

Start soapd
    ${result} =    Run Process    ${SOPHOS_INSTALL}/bin/wdctl   start  on_access_process
    Should Be Equal As Integers    ${result.rc}    ${0}

Restart sophos_threat_detector
    # with added checks/debugging for LINUXDAR-5808
    ${status} =      Run Keyword And Return Status   Really Restart sophos_threat_detector
    Run Keyword If   ${status} != True   Debug Restart sophos_threat_detector Failure

Really Restart sophos_threat_detector
    Stop sophos_threat_detector
    Start sophos_threat_detector
    Wait until threat detector running

Restart sophos_threat_detector and mark logs
    # with added checks/debugging for LINUXDAR-5808
    ${status} =      Run Keyword And Return Status   Really Restart sophos_threat_detector and mark logs
    Run Keyword If   ${status} != True   Debug Restart sophos_threat_detector Failure

Really Restart sophos_threat_detector and mark logs
    Stop sophos_threat_detector
    Mark AV Log
    Mark Sophos Threat Detector Log
    Mark Susi Debug Log
    Start sophos_threat_detector
    Wait until threat detector running with offset

Debug Restart sophos_threat_detector Failure
    # do some debug work before killing the stuck watchdog process
    Analyse Journalctl   print_always=True
    Dump All Sophos Processes
    Log Status Of Sophos Spl

    # force the stuck watchdog process to quit
    ${wd_pid} =   Get Pid   ${WATCHDOG_BINARY}
    Evaluate      os.kill(${wd_pid}, signal.SIGQUIT)   modules=os, signal

    Fail          Failed to restart sophos_threat_detector, so killed watchdog. (LINUXDAR-5808)

Scan GR Test File
    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${RESOURCES_PATH}/file_samples/gui.exe
    Log  ${output}
    BuiltIn.Should Be Equal As Integers  ${rc}  ${0}  Failed to scan gui.exe
