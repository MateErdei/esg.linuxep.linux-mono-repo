*** Settings ***
Documentation   Product tests for SOAP
Force Tags      PRODUCT  SOAP

Library         Process
Library         OperatingSystem
Library         String

Library         ../Libs/AVScanner.py
Library         ../Libs/CoreDumps.py
Library         ../Libs/FakeWatchdog.py
Library         ../Libs/FileUtils.py
Library         ../Libs/LockFile.py
Library         ../Libs/OnFail.py
Library         ../Libs/OSUtils.py
Library         ../Libs/LogUtils.py
Library         ../Libs/ThreatReportUtils.py

Resource    ../shared/ErrorMarkers.robot
Resource    ../shared/ComponentSetup.robot
Resource    ../shared/AVResources.robot
Resource    ../shared/OnAccessResources.robot

Suite Teardown  On Access Suite Teardown

Test Setup     On Access Test Setup
Test Teardown  On Access Test Teardown

*** Variables ***
${AV_PLUGIN_PATH}  ${COMPONENT_ROOT_PATH}
${AV_PLUGIN_BIN}   ${COMPONENT_BIN_PATH}
${AV_LOG_PATH}    ${AV_PLUGIN_PATH}/log/av.log
${TESTTMP}  /tmp_test/SSPLAVTests
${SOPHOS_THREAT_DETECTOR_BINARY_LAUNCHER}  ${SOPHOS_THREAT_DETECTOR_BINARY}_launcher
${ONACCESS_FLAG_CONFIG}  ${AV_PLUGIN_PATH}/var/oa_flag.json
${timeout}  ${60}

*** Keywords ***
#Restart On Access/AV/Threat Detector each test

On Access Suite Teardown
    Remove Files    ${ONACCESS_FLAG_CONFIG}  /tmp/av.stdout  /tmp/av.stderr  /tmp/soapd.stdout  /tmp/soapd.stderr

On Access Test Setup
    Component Test Setup
    Mark Sophos Threat Detector Log
    Mark On Access Log
    Set Suite Variable  ${AV_PLUGIN_HANDLE}  ${None}
    Set Suite Variable  ${ON_ACCESS_PLUGIN_HANDLE}  ${None}
    Start On Access And AV With Running Threat Detector
    Mark On Access Log
    Enable OA Scanning

    Register Cleanup  Remove File     ${ONACCESS_FLAG_CONFIG}
    Register Cleanup  Check All Product Logs Do Not Contain Error
    Register Cleanup  Wait Until On Access Log Contains With Offset  Scan Queue is empty    timeout=${timeout}
    Register Cleanup  Exclude On Access Scan Errors
    Register Cleanup  Require No Unhandled Exception
    Register Cleanup  Check For Coredumps  ${TEST NAME}
    Register Cleanup  Check Dmesg For Segfaults
    Register Cleanup  Exclude CustomerID Failed To Read Error


On Access Test Teardown
    List AV Plugin Path
    Terminate On Access And AV
    FakeWatchdog.Stop Sophos Threat Detector Under Fake Watchdog

    run teardown functions

    Dump Log On Failure   ${ON_ACCESS_LOG_PATH}
    Dump Log On Failure   ${THREAT_DETECTOR_LOG_PATH}
    Remove File      ${AV_PLUGIN_PATH}/log/soapd.log*
    Component Test TearDown


Dump and Reset Logs
    Register Cleanup   Remove File      ${AV_PLUGIN_PATH}/log/av.log*
    Register Cleanup   Remove File      ${AV_PLUGIN_PATH}/log/soapd.log*
    Register Cleanup   Dump log         ${AV_PLUGIN_PATH}/log/soapd.log

Verify on access log rotated
    List Directory  ${AV_PLUGIN_PATH}/log/
    Should Exist  ${AV_PLUGIN_PATH}/log/soapd.log.1

*** Test Cases ***

On Access Log Rotates
    Dump and Reset Logs
    # Ensure the log is created
    OnAccessResources.Start AV
    Terminate AV
    Increase On Access Log To Max Size
    OnAccessResources.Start AV
    Wait Until Created   ${AV_PLUGIN_PATH}/log/soapd.log.1   timeout=10s
    Terminate AV
    ${result} =  Run Process  ls  -altr  ${AV_PLUGIN_PATH}/log/
    Log  ${result.stdout}
    deregister cleanup  Wait Until On Access Log Contains With Offset  Scan Queue is empty    timeout=${timeout}
    Verify on access log rotated

On Access Process Parses Policy Config
    Wait Until On Access Log Contains With Offset  New on-access configuration: {"enabled":"true","excludeRemoteFiles":"false","exclusions":["/mnt/","/uk-filer5/"]}
    Wait Until On Access Log Contains With Offset  On-access enabled: "true"
    Wait Until On Access Log Contains With Offset  On-access scan network: "true"
    Wait Until On Access Log Contains With Offset  On-access exclusions: ["/mnt/","/uk-filer5/"]

On Access Process Parses Flags Config On startup
    ${policyContent}=    Get File   ${RESOURCES_PATH}/flags_policy/flags_enabled.json
    Send Plugin Policy  av  FLAGS  ${policyContent}

    Wait Until Created  ${ONACCESS_FLAG_CONFIG}

    Start On Access

    Wait Until On Access Log Contains With Offset   Found Flag config on startup
    Wait Until On Access Log Contains With Offset   Flag is set to not override policy

On Access Does Not Include Remote Files If Excluded In Policy
    [Tags]  NFS
    ${source} =       Set Variable  /tmp_test/nfsshare
    ${destination} =  Set Variable  /testmnt/nfsshare
    Create Directory  ${source}
    Create Directory  ${destination}
    Create Local NFS Share   ${source}   ${destination}
    Register Cleanup  Remove Local NFS Share   ${source}   ${destination}

    Wait Until On Access Log Contains With Offset  Including mount point: /testmnt/nfsshare

    Mark On Access Log
    ${policyContent}=    Get File   ${RESOURCES_PATH}/SAV-2_policy_excludeRemoteFiles.xml
    Send Plugin Policy  av  sav  ${policyContent}

    ${policyContent}=    Get File   ${RESOURCES_PATH}/flags_policy/flags_enabled.json
    Send Plugin Policy  av  FLAGS  ${policyContent}

    Wait Until On Access Log Contains With Offset  New on-access configuration: {"enabled":"true","excludeRemoteFiles":"true","exclusions":["/mnt/","/uk-filer5/"]}
    Wait Until On Access Log Contains With Offset  On-access enabled: "true"
    Wait Until On Access Log Contains With Offset  On-access scan network: "false"
    Wait Until On Access Log Contains With Offset  On-access exclusions: ["/mnt/","/uk-filer5/"]

    Wait Until On Access Log Contains With Offset  OA config changed, re-enumerating mount points
    On Access Log Does Not Contain With Offset  Including mount point: /testmnt/nfsshare

On Access Monitors Addition And Removal Of Mount Points
    [Tags]  NFS
    Mark On Access Log
    Restart On Access
    Wait Until On Access Log Contains With Offset  Including mount point:
    On Access Log Does Not Contain With Offset  Including mount point: /testmnt/nfsshare
    Sleep  1s
    ${numMountsPreNFSmount} =  Count Lines In Log With Offset  ${ON_ACCESS_LOG_PATH}  Including mount point:  ${ON_ACCESS_LOG_MARK}
    Log  Number of Mount Points: ${numMountsPreNFSmount}

    Mark On Access Log
    ${source} =       Set Variable  /tmp_test/nfsshare
    ${destination} =  Set Variable  /testmnt/nfsshare
    Create Directory  ${source}
    Create Directory  ${destination}
    Register Cleanup  Remove Directory  ${source}  recursive=True
    Register Cleanup  Remove Directory  ${destination}  recursive=True
    Create Local NFS Share   ${source}   ${destination}
    Register Cleanup  Remove Local NFS Share   ${source}   ${destination}

    Wait Until On Access Log Contains With Offset  Mount points changed - re-evaluating
    Wait Until On Access Log Contains With Offset  Including mount point: /testmnt/nfsshare
    Sleep  1s
    ${totalNumMountsPostNFSmount} =  Count Lines In Log With Offset  ${ON_ACCESS_LOG_PATH}  Including mount point:  ${ON_ACCESS_LOG_MARK}
    Log  Number of Mount Points: ${totalNumMountsPostNFSmount}
    #TODO Why does it record two mount updates in quick succession
    Should Be True  ${totalNumMountsPostNFSmount} >= ${numMountsPreNFSmount+1}

    Mark On Access Log
    Remove Local NFS Share   ${source}   ${destination}
    Deregister Cleanup  Remove Local NFS Share   ${source}   ${destination}

    Wait Until On Access Log Contains With Offset  Mount points changed - re-evaluating
    On Access Log Does Not Contain With Offset  Including mount point: /testmnt/nfsshare
    Sleep  1s
    ${totalNumMountsPostNFSumount} =  Count Lines In Log With Offset  ${ON_ACCESS_LOG_PATH}  Including mount point:  ${ON_ACCESS_LOG_MARK}
    Log  Number of Mount Points: ${totalNumMountsPostNFSumount}
    #TODO Why does it record two mount updates in quick succession
    Should Be True  ${totalNumMountsPostNFSmount} >= ${numMountsPreNFSmount}


On Access Logs When A File Is Closed Following Write After Being Disabled
    ${filepath} =  Set Variable  /tmp_test/eicar.com
    ${pid} =  Get Robot Pid

    Remove File  ${filepath}

    ${enabledPolicyContent}=    Get File   ${RESOURCES_PATH}/SAV-2_policy_OA_enabled.xml
    ${disabledPolicyContent}=    Get File   ${RESOURCES_PATH}/SAV-2_policy_OA_disabled.xml

    Mark On Access Log
    Send Plugin Policy  av  sav  ${disabledPolicyContent}
    Wait Until On Access Log Contains With Offset  On-access enabled: "false"
    Wait Until On Access Log Contains With Offset  Joining eventReader

    Mark On Access Log
    Send Plugin Policy  av  sav  ${enabledPolicyContent}
    Wait Until On Access Log Contains With Offset  On-access enabled: "true"
    Wait Until On Access Log Contains With Offset  Starting eventReader

    Mark On Access Log
    Create File  ${filepath}  ${EICAR_STRING}
    Register Cleanup  Remove File  ${filepath}

    Wait Until On Access Log Contains With Offset  On-close event for ${filepath} from  timeout=${timeout}
    #Wait Until On Access Log Contains With Offset  (PID ${pid}) and UID 0
    Wait Until On Access Log Contains With Offset  On-open event for ${filepath} from
    #Wait Until On Access Log Contains With Offset  (PID ${pid}) and UID 0


On Access Process Handles Consecutive Process Control Requests
    ${policyContent}=    Get File   ${RESOURCES_PATH}/flags_policy/flags_enabled.json
    Send Plugin Policy  av  FLAGS  ${policyContent}
    On Access Log Does Not Contain  Using on-access settings from policy

    ${policyContent}=    Get File   ${RESOURCES_PATH}/SAV-2_policy_OA_enabled.xml
    Send Plugin Policy  av  sav  ${policyContent}
    Wait Until On Access Log Contains With Offset  New on-access configuration: {"enabled":"true"

    ${policyContent}=    Get File   ${RESOURCES_PATH}/SAV-2_policy_OA_disabled.xml
    Send Plugin Policy  av  sav  ${policyContent}
    Wait Until On Access Log Contains With Offset  No policy override, following policy settings
    Wait Until On Access Log Contains With Offset  New on-access configuration: {"enabled":"false"

    ${policyContent}=    Get File   ${RESOURCES_PATH}/flags_policy/flags.json
    Send Plugin Policy  av  FLAGS  ${policyContent}
    Wait Until On Access Log Contains With Offset  Policy override is enabled, on-access will be disabled

    On-access No Eicar Scan

On Access Is Disabled By Default If No Flags Policy Arrives
    Disable OA Scanning

    ${policyContent}=    Get File   ${RESOURCES_PATH}/SAV-2_policy_OA_enabled.xml
    Send Plugin Policy  av  sav  ${policyContent}

    On-access No Eicar Scan


On Access Uses Policy Settings If Flags Dont Override Policy
    ${policyContent}=    Get File   ${RESOURCES_PATH}/SAV-2_policy_OA_enabled.xml
    Send Plugin Policy  av  sav  ${policyContent}

    ${policyContent}=    Get File   ${RESOURCES_PATH}/flags_policy/flags_enabled.json
    Send Plugin Policy  av  FLAGS  ${policyContent}

    Wait Until On Access Log Contains With Offset   No policy override, following policy settings
    Wait Until On Access Log Contains With Offset   New on-access configuration: {"enabled":"true","excludeRemoteFiles":"false","exclusions":["/mnt/","/uk-filer5/"]}

    On-access Scan Eicar Close


On Access Is Disabled After it Receives Disable Flags
    ${policyContent}=    Get File   ${RESOURCES_PATH}/flags_policy/flags_enabled.json
    Send Plugin Policy  av  FLAGS  ${policyContent}

    Wait Until On Access Log Contains With Offset   No policy override, following policy settings

    ${policyContent}=    Get File   ${RESOURCES_PATH}/SAV-2_policy_OA_enabled.xml
    Send Plugin Policy  av  sav  ${policyContent}

    Wait Until On Access Log Contains With Offset   New on-access configuration: {"enabled":"true","excludeRemoteFiles":"false","exclusions":["/mnt/","/uk-filer5/"]}

    ${policyContent}=    Get File   ${RESOURCES_PATH}/flags_policy/flags.json
    Send Plugin Policy  av  FLAGS  ${policyContent}

    Wait Until On Access Log Contains With Offset   Policy override is enabled, on-access will be disabled
    Wait Until On Access Log Contains With Offset   Stopping the reading of Fanotify events

    On-access No Eicar Scan

    Dump Log  ${on_access_log_path}


On Access Does not Use Policy Setttings If Flags Have Overriden Policy
    ${policyContent}=    Get File   ${RESOURCES_PATH}/flags_policy/flags.json
    Send Plugin Policy  av  FLAGS  ${policyContent}

    Wait Until On Access Log Contains With Offset  Policy override is enabled, on-access will be disabled

    Mark On Access Log
    ${policyContent}=    Get File   ${RESOURCES_PATH}/SAV-2_policy_OA_enabled.xml
    Send Plugin Policy  av  sav  ${policyContent}

    Wait Until On Access Log Contains With Offset   Policy override is enabled, on-access will be disable
    On-access No Eicar Scan

    Dump Log  ${on_access_log_path}