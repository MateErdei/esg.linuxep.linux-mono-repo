#OnAccessAlternative starts and stops OnAccess,AV and Threat Detector each test.
#Tests should assess behaviour while disrupting their enabled, normal state.
#For tests that want to assess behaviour during standard running use OnAccessStandard

*** Settings ***
Documentation   Product tests for SOAP
Force Tags      PRODUCT  SOAP

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

    Register Cleanup  Clear On Access Log When Nearly Full
    Register Cleanup  Remove File     ${ONACCESS_FLAG_CONFIG}
    Register Cleanup  Check All Product Logs Do Not Contain Error
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
    #deregister cleanup  Wait Until On Access Log Contains With Offset  Scan Queue is empty    timeout=${timeout}
    Dump and Reset Logs
    # Ensure the log is created

    Terminate On Access And AV
    #Wait Until On Access Log Contains With Offset  Scan Queue is empty
    Increase On Access Log To Max Size

    Start AV and On Access
    Wait Until Created   ${AV_PLUGIN_PATH}/log/soapd.log.1   timeout=10s
    Terminate On Access And AV

    ${result} =  Run Process  ls  -altr  ${AV_PLUGIN_PATH}/log/
    Log  ${result.stdout}

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
    Register Cleanup    Exclude On Access Scan Errors
    Require Filesystem  ext2
    ${where} =  Set Variable  ${NORMAL_DIRECTORY}/mount
    ${type} =  Set Variable  ext2
    Mark On Access Log
    Start On Access
    Wait Until On Access Log Contains With Offset  Including mount point:
    On Access Log Does Not Contain With Offset  Including mount point: ${where}
    Sleep  1s
    ${numMountsPreMount} =  get_latest_mount_inclusion_count_from_on_access_log  ${ON_ACCESS_LOG_MARK}

    Mark On Access Log
    ${image} =  Copy And Extract Image  ext2FileSystem
    Mount Image  ${where}  ${image}  ${type}

    Wait Until On Access Log Contains With Offset  Mount points changed - re-evaluating
    Wait Until On Access Log Contains  Including mount point: ${where}
    Sleep  1s
    ${totalNumMountsPostMount} =  get_latest_mount_inclusion_count_from_on_access_log  ${ON_ACCESS_LOG_MARK}
    Should Be Equal As Integers  ${totalNumMountsPostMount}  ${numMountsPreMount+1}

    Mark On Access Log
    Unmount Image  ${where}

    Wait Until On Access Log Contains With Offset  Mount points changed - re-evaluating
    On Access Log Does Not Contain With Offset  Including mount point: ${where}
    Sleep  1s
    ${totalNumMountsPostUmount} =  get_latest_mount_inclusion_count_from_on_access_log  ${ON_ACCESS_LOG_MARK}
    Should Be Equal As Integers  ${totalNumMountsPostUmount}  ${numMountsPreMount}


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


On Access Process Reconnects To Threat Detector
    ${filepath} =  Set Variable  /tmp_test/clean_file_writer/clean.txt
    ${script} =  Set Variable  ${BASH_SCRIPTS_PATH}/cleanFileWriter.sh
    ${HANDLE} =  Start Process  bash  ${script}  stderr=STDOUT
    Register Cleanup  Terminate Process  ${HANDLE}
    Register Cleanup  Remove Directory  /tmp_test/clean_file_writer/  recursive=True

    Wait Until On Access Log Contains With Offset  On-close event for ${filepath}
    FakeWatchdog.Stop Sophos Threat Detector Under Fake Watchdog
    Mark On Access Log
    FakeWatchdog.Start Sophos Threat Detector Under Fake Watchdog
    Wait Until On Access Log Contains With Offset  On-close event for ${filepath}
    #Depending on whether a scan is being processed or it is being requested one of these 2 errors should appear
    File Log Contains One of   ${ON_ACCESS_LOG_PATH}  0  Failed to receive scan response  Failed to send scan request

    On Access Log Does Not Contain With Offset  Failed to scan ${filepath}

On Access Scan Times Out When Unable To Connect To Threat Detector
    [Tags]  MANUAL
    FakeWatchdog.Stop Sophos Threat Detector Under Fake Watchdog
    Restart On Access

    ${filepath} =  Set Variable  /tmp_test/clean_file_writer/clean.txt
    Create File  ${filepath}  clean
    Register Cleanup  Remove File  ${filepath}

    Wait Until On Access Log Contains With Offset  On-close event for ${filepath}
    Wait Until On Access Log Contains With Offset  Failed to connect to Sophos Threat Detector - retrying after sleep
    Wait Until On Access Log Contains With Offset  Reached total maximum number of connection attempts.  timeout=${timeout}