*** Settings ***
Resource    AVResources.robot
Resource    BaseResources.robot

Library         ${COMMON_TEST_LIBS}/CoreDumps.py

*** Variables ***
${SUSI_DISTRIBUTION_VERSION}        ${COMPONENT_ROOT_PATH}/chroot/susi/distribution_version
${SUSI_UPDATE_SOURCE}               ${COMPONENT_ROOT_PATH}/chroot/susi/update_source
${VDL_DIRECTORY}                    ${SUSI_UPDATE_SOURCE}/vdl
${VDL_TEMP_DESTINATION}             ${COMPONENT_ROOT_PATH}/moved_vdl

*** Keywords ***

AV and Base Setup
    # ignore occasional SXL4 timeouts
    Register Cleanup  clear outbreak mode if required
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
    Send CORE Policy To Base    core_policy/CORE-36_oa_disabled.xml
    Reset ThreatDatabase

    Remove Directory  /tmp/DiagnoseOutput  true

    ${result} =  Run Process  id  sophos-spl-user  stderr=STDOUT
    Log  "id sophos-spl-user = ${result.stdout}"

Check avscanner in /usr/local/bin
    File Should Exist  /usr/local/bin/avscanner

Restart AV Plugin And Threat Detector
    Stop AV Plugin And Threat Detector
    Start AV Plugin And Threat Detector

Stop AV Plugin And Threat Detector
    Stop AV Plugin
    Stop Sophos_Threat_Detector

Start AV Plugin And Threat Detector
    Start AV Plugin
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
    File Should Exist  ${AV_BACKUP_DIR}/safestore.log

Run plugin uninstaller
    ${result} =   Run Process  ${COMPONENT_SBIN_DIR}/uninstall.sh   stderr=STDOUT
    Log    ${result.stdout}

Run plugin uninstaller with downgrade flag
    ${result} =   Run Process  ${COMPONENT_SBIN_DIR}/uninstall.sh  --downgrade   stderr=STDOUT
    Log    ${result.stdout}

Configure and run scan now
    ${av_mark} =  Mark AV Log
    Configure scan now
    Run Scan Now After Mark  ${av_mark}

Configure and check scan now with lookups disabled
    ${av_mark} =  Mark AV Log
    Configure scan now with lookups disabled
    Run Scan Now After Mark  ${av_mark}

Configure scan now
    ${av_mark} =  Mark AV Log
    ${revid} =   Generate Random String
    ${policyContent} =   create_corc_policy  revid=${revid}  sxlLookupEnabled=${true}
    Send CORC Policy To Base From Content  ${policyContent}
    Send Sav Policy To Base With Exclusions Filled In  SAV_Policy_Scan_Now.xml
    # Run Keyword and Ignore Error
    Wait until scheduled scan updated After Mark  ${av_mark}

Configure scan now with lookups disabled
    ${av_mark} =  Mark AV Log
    ${threat_detector_mark} =  Mark Sophos Threat Detector Log

    ${revid} =   Generate Random String
    ${policyContent} =   create_corc_policy  revid=${revid}  sxlLookupEnabled=${false}
    Send CORC Policy To Base From Content  ${policyContent}

    Send Sav Policy To Base With Exclusions Filled In  SAV_Policy_Scan_Now_Lookup_Disabled.xml

    Wait For AV Log Contains After Mark  Restarting sophos_threat_detector as the configuration has changed  ${av_mark}
    Check AV Log Does Not Contain After Mark  Failed to send shutdown request: Failed to connect to unix socket  ${av_mark}

    # Force SUSI to be loaded:
    Check avscanner can detect eicar

    Wait For Sophos Threat Detector Log Contains After Mark  SXL Lookups will be disabled   ${threat_detector_mark}  timeout=10
    Run Keyword and Ignore Error  Wait until scheduled scan updated After Mark  ${av_mark}


Run Scan Now After Mark
    [Arguments]  ${av_mark}
    Register On Fail If Unique  dump log  ${SCANNOW_LOG_PATH}
    Send Sav Action To Base  ScanNow_Action.xml
    Wait For AV Log Contains After Mark  Completed scan Scan Now  ${av_mark}  timeout=180
    Check AV Log Contains After Mark  Evaluating Scan Now  ${av_mark}
    Check AV Log Contains After Mark  Starting scan Scan Now  ${av_mark}


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

Restart soapd
    Stop soapd
    Start soapd

Restart sophos_threat_detector
    # with added checks/debugging for LINUXDAR-5808
    ${status} =      Run Keyword And Return Status   Really Restart sophos_threat_detector
    Run Keyword If   ${status} != True   Debug Restart sophos_threat_detector Failure

Really Restart sophos_threat_detector
    Stop sophos_threat_detector
    ${threat_detector_mark} =  Mark Sophos Threat Detector Log
    Start sophos_threat_detector
    Wait until threat detector running after mark  ${threat_detector_mark}

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

AV Plugin uninstalls
    Register Cleanup    Exclude MCS Router is dead
    Register Cleanup    Install With Base SDDS
    Check avscanner in /usr/local/bin
    Run plugin uninstaller
    Check avscanner not in /usr/local/bin
    Check AV Plugin Not Installed

Remove OA local settings and restart in integration test
    Remove File   ${OA_LOCAL_SETTINGS}
    Restart soapd
    Wait Until On Access running

Set number of scanning threads in integration test
    [Arguments]  ${count}
    Create File   ${OA_LOCAL_SETTINGS}   { "numThreads" : ${count} }
    Register Cleanup   Remove OA local settings and restart in integration test
    Restart soapd
    Wait Until On Access running

Reset ThreatDatabase
    ${avmark} =  Mark AV Log
    ${actionFilename} =  Generate Random String
    ${full_action_filename} =  Set Variable  CORE_action_${actionFilename}.xml
    Copy File  ${RESOURCES_PATH}/core_action/Core_reset_threat.xml  ${SOPHOS_INSTALL}/base/mcs/action/${full_action_filename}
    wait_for_av_log_contains_after_mark   Threat Database has been reset  ${avmark}


Create SUSI Initialisation Error
    Stop sophos_threat_detector
    Remove Directory  ${SUSI_DISTRIBUTION_VERSION}  ${true}
    Move Directory  ${VDL_DIRECTORY}  ${VDL_TEMP_DESTINATION}
    Register Cleanup    Remove Directory    ${VDL_TEMP_DESTINATION}  recursive=True
    ${td_mark} =  Mark Sophos Threat Detector Log
    Start sophos_threat_detector
    Wait until threat detector running after mark  ${td_mark}