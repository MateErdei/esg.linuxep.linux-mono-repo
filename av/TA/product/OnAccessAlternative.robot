#OnAccessAlternative starts and stops OnAccess,AV and Threat Detector each test.
#Tests should assess behaviour while disrupting their enabled, normal state.
#For tests that want to assess behaviour during standard running use OnAccessStandard

*** Settings ***
Documentation   Product tests for SOAP
Force Tags      PRODUCT  SOAP  oa_alternative

Resource    ../shared/ErrorMarkers.robot
Resource    ../shared/FakeManagementResources.robot
Resource    ../shared/ComponentSetup.robot
Resource    ../shared/AVResources.robot
Resource    ../shared/OnAccessResources.robot

Suite Setup     On Access Alternative Suite Setup
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
${DEFAULT_EXCLUSIONS}   ["/mnt/","/uk-filer5/","*excluded*","/opt/test/inputs/test_scripts/","/vagrant/"]

*** Keywords ***
On Access Alternative Suite Setup
    Set Suite Variable  ${AV_PLUGIN_HANDLE}  ${None}
    Set Suite Variable  ${ON_ACCESS_PLUGIN_HANDLE}  ${None}
    ${customerIdFile} =   Set Variable   ${AV_PLUGIN_PATH}/var/customer_id.txt
    Create File  ${customerIdFile}  c1cfcf69a42311a6084bcefe8af02c8a

On Access Suite Teardown
    Terminate On Access And AV
    Remove Files    ${ONACCESS_FLAG_CONFIG}  /tmp/av.stdout  /tmp/av.stderr  /tmp/soapd.stdout  /tmp/soapd.stderr
    Remove Directory   ${TESTTMP}   recursive=True

On Access Test Setup
    Component Test Setup

    Terminate On Access And AV

    save_on_access_log_mark_at_start_of_test

    ${oa_mark} =  get_on_access_log_mark
    Start On Access And AV With Running Threat Detector
    Enable OA Scanning   mark=${oa_mark}

    Register Cleanup  Clear On Access Log When Nearly Full
    Register Cleanup  Remove File     ${ONACCESS_FLAG_CONFIG}
    Register Cleanup  Check All Product Logs Do Not Contain Error
    Register Cleanup  Exclude On Access Scan Errors
    Register Cleanup  Require No Unhandled Exception
    Register Cleanup  Check For Coredumps  ${TEST NAME}
    Register Cleanup  Check Dmesg For Segfaults
    Register Cleanup  Exclude CustomerID Failed To Read Error
    Register On Fail  Dump log  ${ON_ACCESS_LOG_PATH}


On Access Test Teardown
    List AV Plugin Path
    Terminate On Access And AV
    FakeWatchdog.Stop Sophos Threat Detector Under Fake Watchdog

    Run Teardown Functions

    Dump Log On Failure   ${ON_ACCESS_LOG_PATH}
    Dump Log On Failure   ${THREAT_DETECTOR_LOG_PATH}
    Dump Log On Failure   ${AV_LOG_PATH}

    Remove File      ${AV_PLUGIN_PATH}/log/soapd.log*
    Component Test TearDown


Dump and Reset Logs
    Register Cleanup   Remove File      ${AV_PLUGIN_PATH}/log/av.log*
    Register Cleanup   Remove File      ${AV_PLUGIN_PATH}/log/soapd.log*
    Register Cleanup   Dump log         ${AV_PLUGIN_PATH}/log/soapd.log

Verify on access log rotated
    List Directory  ${AV_PLUGIN_PATH}/log/
    Should Exist  ${AV_PLUGIN_PATH}/log/soapd.log.1

Configure on access log to trace level
    Set Log Level  TRACE
    Register Cleanup       Set Log Level  DEBUG
    ${mark} =  get_on_access_log_mark
    Restart On Access
    wait for on access log contains after mark  Logger soapd configured for level: TRACE  mark=${mark}
    wait for on access log contains after mark  mount points in on-access scanning  mark=${mark}

*** Test Cases ***

On Access Log Rotates
    Dump and Reset Logs
    # Ensure the log is created

    Terminate On Access
    Increase On Access Log To Max Size

    Start On Access without Log check
    Wait Until Created   ${AV_PLUGIN_PATH}/log/soapd.log.1   timeout=10s
    Terminate On Access And AV

    ${result} =  Run Process  ls  -altr  ${AV_PLUGIN_PATH}/log/
    Log  ${result.stdout}

    Verify on access log rotated

On Access Process Parses Policy Config
    wait for on access log contains after mark  New on-access configuration: {"enabled":"true","excludeRemoteFiles":"false","exclusions":${DEFAULT_EXCLUSIONS}}  mark=${ON_ACCESS_LOG_MARK_FROM_START_OF_TEST}
    wait for on access log contains after mark  On-access enabled: "true"  mark=${ON_ACCESS_LOG_MARK_FROM_START_OF_TEST}
    wait for on access log contains after mark  On-access scan network: "true"  mark=${ON_ACCESS_LOG_MARK_FROM_START_OF_TEST}
    wait for on access log contains after mark  On-access exclusions: ${DEFAULT_EXCLUSIONS}  mark=${ON_ACCESS_LOG_MARK_FROM_START_OF_TEST}

On Access Process Parses Flags Config On startup
    ${policyContent}=    Get File   ${RESOURCES_PATH}/flags_policy/flags_onaccess_enabled.json
    Send Plugin Policy  av  FLAGS  ${policyContent}

    Wait Until Created  ${ONACCESS_FLAG_CONFIG}

    ${mark} =  get_on_access_log_mark
    Restart On Access

    wait for on access log contains after mark  Flag is set to not override policy  mark=${mark}
    wait for on access log contains after mark  mount points in on-access scanning  mark=${mark}

On Access Does Not Include Remote Files If Excluded In Policy
    [Tags]  NFS
    ${mark} =  get_on_access_log_mark
    ${source} =       Set Variable  /tmp_test/nfsshare
    ${destination} =  Set Variable  /testmnt/nfsshare
    Create Directory  ${source}
    Create Directory  ${destination}
    Create Local NFS Share   ${source}   ${destination}
    wait for on access log contains after mark  Including mount point: /testmnt/nfsshare  mark=${mark}
    wait for on access log contains after mark  mount points in on-access scanning  mark=${mark}

    ${mark} =  get_on_access_log_mark
    ${policyContent}=    Get File   ${RESOURCES_PATH}/SAV-2_policy_excludeRemoteFiles.xml
    Send Plugin Policy  av  ${SAV_APPID}  ${policyContent}

    ${policyContent}=    Get File   ${RESOURCES_PATH}/flags_policy/flags_onaccess_enabled.json
    Send Plugin Policy  av  FLAGS  ${policyContent}

    wait for on access log contains after mark  New on-access configuration: {"enabled":"true","excludeRemoteFiles":"true","exclusions":${DEFAULT_EXCLUSIONS}}  mark=${mark}
    wait for on access log contains after mark  On-access enabled: "true"  mark=${mark}
    wait for on access log contains after mark  On-access scan network: "false"  mark=${mark}
    wait for on access log contains after mark  On-access exclusions: ${DEFAULT_EXCLUSIONS}  mark=${mark}
    wait for on access log contains after mark  OA config changed, re-enumerating mount points  mark=${mark}
    check_on_access_log_does_not_contain_after_mark  Including mount point: /testmnt/nfsshare  mark=${mark}
    wait for on access log contains after mark  mount points in on-access scanning  mark=${mark}


On Access Applies Config Changes When The Mounts Change
    [Tags]  NFS
    ${mark} =  get_on_access_log_mark
    ${source} =       Set Variable  /tmp_test/nfsshare
    ${destination} =  Set Variable  /testmnt/nfsshare
    Create Directory  ${source}
    Create Directory  ${destination}
    Create Local NFS Share   ${source}   ${destination}
    wait for on access log contains after mark  Including mount point: /testmnt/nfsshare  mark=${mark}
    wait for on access log contains after mark  mount points in on-access scanning  mark=${mark}

    ${mark} =  get_on_access_log_mark
    ${filepath} =  Set Variable  /testmnt/nfsshare/clean.txt
    Create File  ${filepath}  clean
    Register Cleanup  Remove File  ${filepath}
    wait for on access log contains after mark  On-close event for ${filepath} from Process  mark=${mark}

    ${mark} =  get_on_access_log_mark
    ${policyContent}=    Get File   ${RESOURCES_PATH}/SAV-2_policy_excludeRemoteFiles.xml
    Send Plugin Policy  av  ${SAV_APPID}  ${policyContent}
    wait for on access log contains after mark  On-access enabled: "true"  mark=${mark}
    wait for on access log contains after mark  On-access scan network: "false"  mark=${mark}
    wait for on access log contains after mark  OA config changed, re-enumerating mount points  mark=${mark}
    wait for on access log contains after mark  mount points in on-access scanning  mark=${mark}
    check_on_access_log_contains_after_mark  Mount point /testmnt/nfsshare has been excluded from scanning  mark=${mark}
    check_on_access_log_does_not_contain_after_mark  Including mount point: /testmnt/nfsshare  mark=${mark}

    ${mark} =  get_on_access_log_mark
    ${filepath2} =  Set Variable  /testmnt/nfsshare/clean2.txt
    Create File  ${filepath2}  clean
    Register Cleanup  Remove File  ${filepath2}

    check_on_access_log_does_not_contain_after_mark  On-close event for ${filepath2} from Process  mark=${mark}

    ${mark} =  get_on_access_log_mark
    ${policyContent}=    Get File   ${RESOURCES_PATH}/SAV-2_policy_OA_enabled.xml
    Send Plugin Policy  av  ${SAV_APPID}  ${policyContent}

    wait for on access log contains after mark  On-access enabled: "true"  mark=${mark}
    wait for on access log contains after mark  On-access scan network: "true"  mark=${mark}
    wait for on access log contains after mark  OA config changed, re-enumerating mount points  mark=${mark}
    wait for on access log contains after mark  mount points in on-access scanning  mark=${mark}
    check_on_access_log_contains_after_mark  Including mount point: /testmnt/nfsshare  mark=${mark}

    ${mark} =  get_on_access_log_mark
    ${filepath3} =  Set Variable  /testmnt/nfsshare/clean3.txt
    Create File  ${filepath3}  clean
    Register Cleanup  Remove File  ${filepath3}

    wait for on access log contains after mark  On-close event for ${filepath3} from Process  mark=${mark}


On Access Does Not Scan Files If They Match Absolute Directory Exclusion In Policy
    ${mark} =  get_on_access_log_mark
    ${filepath1} =  Set Variable  /tmp_test/eicar.com
    ${filepath2} =  Set Variable  /tmp_test/eicar2.com
    ${policyContent} =  Get Complete Sav Policy  ["/tmp_test/"]  True
    Send Plugin Policy  av  ${SAV_APPID}  ${policyContent}
    wait for on access log contains after mark  On-access exclusions: ["/tmp_test/"]  mark=${mark}
    wait for on access log contains after mark  Updating on-access exclusions with: ["/tmp_test/"]  mark=${mark}
    wait for on access log contains after mark  mount points in on-access scanning  mark=${mark}

    ${mark} =  get_on_access_log_mark
    Create File  ${filepath1}  ${EICAR_STRING}
    Register Cleanup  Remove File  ${filepath1}
    check_on_access_log_does_not_contain_after_mark  On-close event for ${filepath1} from  mark=${mark}

    ${policyContent} =  Get Complete Sav Policy  []  True
    Send Plugin Policy  av  ${SAV_APPID}  ${policyContent}
    wait for on access log contains after mark  On-access exclusions: []  mark=${mark}
    wait for on access log contains after mark  Updating on-access exclusions  mark=${mark}

    Create File  ${filepath2}  ${EICAR_STRING}
    Register Cleanup  Remove File  ${filepath2}
    wait for on access log contains after mark  On-close event for ${filepath2} from  mark=${mark}  timeout=60


On Access Does Not Scan Files If They Match Relative Directory Exclusion In Policy
    Configure on access log to trace level

    ${mark} =  get_on_access_log_mark
    ${policyContent} =  Get Complete Sav Policy  ["testdir/folder_without_wildcard/","dir/su*ir/","do*er/"]  True
    Send Plugin Policy  av  ${SAV_APPID}  ${policyContent}
    wait for on access log contains after mark  On-access exclusions: ["testdir/folder_without_wildcard/","dir/su*ir/","do*er/"]  mark=${mark}
    wait for on access log contains after mark  Updating on-access exclusions with: ["/testdir/folder_without_wildcard/"] ["*/dir/su*ir/*"] ["*/do*er/*"]  mark=${mark}
    ${TEST_DIR_WITHOUT_WILDCARD} =  Set Variable  /tmp_test/testdir/folder_without_wildcard
    ${TEST_DIR_WITH_WILDCARD} =  Set Variable  /tmp_test/testdir/folder_with_wildcard
    Create Directory  ${TEST_DIR_WITHOUT_WILDCARD}
    Register Cleanup  Remove Directory  ${TEST_DIR_WITHOUT_WILDCARD}  recursive=True
    Create Directory  ${TEST_DIR_WITH_WILDCARD}
    Register Cleanup  Remove Directory  ${TEST_DIR_WITH_WILDCARD}  recursive=True

    ${mark} =  get_on_access_log_mark
    #Relative path to directory
    Create File     ${TEST_DIR_WITHOUT_WILDCARD}/clean_file                                       ${CLEAN_STRING}
    Create File     ${TEST_DIR_WITHOUT_WILDCARD}/naughty_eicar_folder/eicar                       ${EICAR_STRING}
    Create File     ${TEST_DIR_WITHOUT_WILDCARD}/clean_eicar_folder/eicar                         ${CLEAN_STRING}
    #Relative path to directory with wildcard
    Create File     ${TEST_DIR_WITH_WILDCARD}/dir/subpart/subdir/eicar.com                        ${EICAR_STRING}
    Create File     ${TEST_DIR_WITH_WILDCARD}/ddir/subpart/subdir/eicar.com                       ${CLEAN_STRING}
    Create File     ${TEST_DIR_WITH_WILDCARD}/documents/test/subfolder/eicar.com                  ${EICAR_STRING}
    Create File     ${TEST_DIR_WITH_WILDCARD}/ddocuments/test/subfolder/eicar.com                 ${CLEAN_STRING}

    wait for on access log contains after mark  File access on ${TEST_DIR_WITHOUT_WILDCARD}/clean_file will not be scanned due to exclusion: testdir/folder_without_wildcard/  mark=${mark}
    wait for on access log contains after mark  File access on ${TEST_DIR_WITHOUT_WILDCARD}/naughty_eicar_folder/eicar will not be scanned due to exclusion: testdir/folder_without_wildcard/  mark=${mark}
    wait for on access log contains after mark  File access on ${TEST_DIR_WITHOUT_WILDCARD}/clean_eicar_folder/eicar will not be scanned due to exclusion: testdir/folder_without_wildcard/  mark=${mark}
    wait for on access log contains after mark  File access on ${TEST_DIR_WITH_WILDCARD}/dir/subpart/subdir/eicar.com will not be scanned due to exclusion: dir/su*ir/  mark=${mark}
    wait for on access log contains after mark  On-close event for ${TEST_DIR_WITH_WILDCARD}/ddir/subpart/subdir/eicar.com  mark=${mark}
    wait for on access log contains after mark  File access on ${TEST_DIR_WITH_WILDCARD}/documents/test/subfolder/eicar.com will not be scanned due to exclusion: do*er/  mark=${mark}
    wait for on access log contains after mark  On-close event for ${TEST_DIR_WITH_WILDCARD}/ddocuments/test/subfolder/eicar.com  mark=${mark}


On Access Does Not Scan Files If They Match Wildcard Exclusion In Policy
    Configure on access log to trace level

    ${TEST_DIR} =   Set Variable  /tmp_test/globExclDir
    Create Directory  ${TEST_DIR}
    Register Cleanup  Remove Directory  ${TEST_DIR}  recursive=True

    ${mark} =  get_on_access_log_mark
    ${exclusionList} =  Set Variable  ["eicar","${TEST_DIR}/eicar.???","${TEST_DIR}/hi_i_am_dangerous.*","${TEST_DIR}/*.js"]
    ${policyContent} =  Get Complete Sav Policy  ${exclusionList}  True
    Send Plugin Policy  av  ${SAV_APPID}  ${policyContent}
    wait for on access log contains after mark  On-access exclusions: ${exclusionList}  mark=${mark}
    wait for on access log contains after mark  Updating on-access exclusions with: ["/eicar"] ["/tmp_test/globExclDir/eicar.???"] ["/tmp_test/globExclDir/hi_i_am_dangerous.*"] ["/tmp_test/globExclDir/*.js"]  mark=${mark}

    ${mark} =  get_on_access_log_mark
    Create File     ${TEST_DIR}/clean_file.txt             ${CLEAN_STRING}
    Create File     ${TEST_DIR}/eicar                      ${EICAR_STRING}
    #Absolute path with character suffix
    Create File     ${TEST_DIR}/eicar.com                  ${EICAR_STRING}
    Create File     ${TEST_DIR}/eicar.comm                 ${CLEAN_STRING}
    Create File     ${TEST_DIR}/eicar.co                   ${CLEAN_STRING}
    #Absolute path with filename prefix
    Create File     ${TEST_DIR}/hi_i_am_dangerous.txt      ${EICAR_STRING}
    Create File     ${TEST_DIR}/hi_i_am_dangerous.exe      ${EICAR_STRING}
    Create File     ${TEST_DIR}/hi_i_am_dangerous          ${CLEAN_STRING}
    #Absolute path with filename suffix
    Create File     ${TEST_DIR}/bird.js                    ${EICAR_STRING}
    Create File     ${TEST_DIR}/exe.js                     ${EICAR_STRING}
    Create File     ${TEST_DIR}/clean_file.jss             ${CLEAN_STRING}
    wait for on access log contains after mark  On-close event for ${TEST_DIR}/clean_file.txt  mark=${mark}
    wait for on access log contains after mark  File access on ${TEST_DIR}/eicar will not be scanned due to exclusion: eicar  mark=${mark}
    wait for on access log contains after mark  File access on ${TEST_DIR}/eicar.com will not be scanned due to exclusion: ${TEST_DIR}/eicar.???  mark=${mark}
    wait for on access log contains after mark  On-close event for ${TEST_DIR}/eicar.comm  mark=${mark}
    wait for on access log contains after mark  On-close event for ${TEST_DIR}/eicar.co  mark=${mark}
    wait for on access log contains after mark  File access on ${TEST_DIR}/hi_i_am_dangerous.txt will not be scanned due to exclusion: ${TEST_DIR}/hi_i_am_dangerous.*  mark=${mark}
    wait for on access log contains after mark  File access on ${TEST_DIR}/hi_i_am_dangerous.exe will not be scanned due to exclusion: ${TEST_DIR}/hi_i_am_dangerous.*  mark=${mark}
    wait for on access log contains after mark  On-close event for ${TEST_DIR}/hi_i_am_dangerous  mark=${mark}
    wait for on access log contains after mark  File access on ${TEST_DIR}/bird.js will not be scanned due to exclusion: ${TEST_DIR}/*.js  mark=${mark}
    wait for on access log contains after mark  File access on ${TEST_DIR}/exe.js will not be scanned due to exclusion: ${TEST_DIR}/*.js  mark=${mark}
    wait for on access log contains after mark  On-close event for ${TEST_DIR}/clean_file.jss  mark=${mark}


On Access Does Not Monitor A Mount Point If It Matches An Exclusion In Policy
    ${mark} =  get_on_access_log_mark
    ${policyContent} =  Get Complete Sav Policy  ["/"]  True
    Send Plugin Policy  av  ${SAV_APPID}  ${policyContent}
    wait for on access log contains after mark  On-access exclusions: ["/"]  mark=${mark}
    wait for on access log contains after mark  Updating on-access exclusions  mark=${mark}
    wait for on access log contains after mark  Mount point / matches an exclusion in the policy and will be excluded from scanning  mark=${mark}


On Access Does Not Monitor A Bind-mounted File If It Matches A File Exclusion In Policy
    ${source} =       Set Variable  /tmp_test/src_file
    ${destination} =  Set Variable  /tmp_test/bind_mount
    Register Cleanup  Remove Directory   /tmp_test   recursive=true
    Remove Directory  /tmp_test   recursive=true
    Create File  ${source}
    Create File  ${destination}

    Run Shell Process   mount --bind ${source} ${destination}     OnError=Failed to create bind mount
    Register Cleanup  Unmount Test Mount  ${destination}

    ${mark} =  get_on_access_log_mark
    ${policyContent} =  Get Complete Sav Policy  ["/tmp_test/bind_mount"]  True
    Send Plugin Policy  av  ${SAV_APPID}  ${policyContent}
    wait for on access log contains after mark  On-access exclusions: ["/tmp_test/bind_mount"]  mark=${mark}
    wait for on access log contains after mark  Updating on-access exclusions  mark=${mark}
    wait for on access log contains after mark  Mount point /tmp_test/bind_mount matches an exclusion in the policy and will be excluded from scanning  mark=${mark}


On Access Logs When A File Is Closed Following Write After Being Disabled
    ${filepath} =  Set Variable  /tmp_test/eicar.com
    ${pid} =  Get Robot Pid

    Remove File  ${filepath}

    ${enabledPolicyContent}=    Get File   ${RESOURCES_PATH}/SAV-2_policy_OA_enabled.xml
    ${disabledPolicyContent}=    Get File   ${RESOURCES_PATH}/SAV-2_policy_OA_disabled.xml

    ${mark} =  get_on_access_log_mark
    Send Plugin Policy  av  ${SAV_APPID}  ${disabledPolicyContent}
    wait_for_on_access_log_contains_after_mark  On-access enabled: "false"  mark=${mark}
    wait_for_on_access_log_contains_after_mark  Joining eventReader  mark=${mark}
    Sleep   1s

    ${mark} =  get_on_access_log_mark
    Send Plugin Policy  av  ${SAV_APPID}  ${enabledPolicyContent}
    wait_for_on_access_log_contains_after_mark  On-access enabled: "true"  mark=${mark}
    wait_for_on_access_log_contains_after_mark  Starting eventReader  mark=${mark}
    Wait for on access to be enabled  ${mark}

    ${mark} =  get_on_access_log_mark
    Create File  ${filepath}  ${CLEAN_STRING}
    Register Cleanup  Remove File  ${filepath}

    wait_for_on_access_log_contains_after_mark  On-close event for ${filepath} from  mark=${mark}


On Access Process Handles Consecutive Process Control Requests
    ${mark} =  get_on_access_log_mark
    ${policyContent}=    Get File   ${RESOURCES_PATH}/flags_policy/flags_onaccess_enabled.json
    Send Plugin Policy  av  FLAGS  ${policyContent}
    wait for on access log contains after mark  No policy override, following policy settings  mark=${mark}

    ${policyContent}=    Get File   ${RESOURCES_PATH}/SAV-2_policy_OA_enabled.xml
    Send Plugin Policy  av  ${SAV_APPID}  ${policyContent}
    wait for on access log contains after mark  New on-access configuration: {"enabled":"true"  mark=${mark}

    ${mark} =  get_on_access_log_mark
    ${policyContent}=    Get File   ${RESOURCES_PATH}/SAV-2_policy_OA_disabled.xml
    Send Plugin Policy  av  ${SAV_APPID}  ${policyContent}
    wait for on access log contains after mark  No policy override, following policy settings  mark=${mark}
    wait for on access log contains after mark  New on-access configuration: {"enabled":"false"  mark=${mark}

    ${policyContent}=    Get File   ${RESOURCES_PATH}/flags_policy/flags.json
    Send Plugin Policy  av  FLAGS  ${policyContent}
    wait for on access log contains after mark  Overriding policy, on-access will be disabled  mark=${mark}
    wait for log to not contain after mark  ${ON_ACCESS_LOG_PATH}  mount points in on-access scanning  mark=${mark}  timeout=${5}

    On-access No Eicar Scan

On Access Process Handles Fast Process Control Requests Last Flag is OA Enabled
    ${enabledFlags}=    Get File   ${RESOURCES_PATH}/flags_policy/flags_enabled.json
    Send Plugin Policy  av  FLAGS  ${enabledFlags}

    ${enabledPolicy}=    Get File   ${RESOURCES_PATH}/SAV-2_policy_OA_enabled.xml
    Send Plugin Policy  av  ${SAV_APPID}  ${enabledPolicy}

    ${mark} =  get_on_access_log_mark
    ${disabledFlags}=    Get File   ${RESOURCES_PATH}/flags_policy/flags.json
    Send Plugin Policy  av  FLAGS  ${disabledFlags}
    #need to ensure that the disable flag has been read by soapd,
    #the avp process can ovewrite the flag config before soapd gets a change to read it.
    #this is fine because soapd will always read the latest flag settings as we get a notification after the a config is written
    #but for the purpose of this test it's we want to avoid this situation
    wait for on access log contains after mark   New flag configuration: {"oa_enabled":false}    mark=${mark}  timeout=${2}

    ${mark} =  get_on_access_log_mark
    Send Plugin Policy  av  FLAGS  ${enabledFlags}
    wait for on access log contains after mark  No policy override, following policy settings  mark=${mark}
    wait for on access log contains after mark  New on-access configuration: {"enabled":"true"  mark=${mark}
    wait for on access log contains after mark  Finished ProcessPolicy  mark=${mark}
    wait for on access log contains after mark  Setting poll timeout to  mark=${mark}
    wait for on access log contains after mark  On-access scanning enabled  mark=${mark}

    On-access Scan Eicar Open

On Access Is Disabled By Default If No Flags Policy Arrives
    Disable OA Scanning

    ${policyContent}=    Get File   ${RESOURCES_PATH}/SAV-2_policy_OA_enabled.xml
    Send Plugin Policy  av  ${SAV_APPID}  ${policyContent}

    On-access No Eicar Scan


On Access Uses Policy Settings If Flags Dont Override Policy
    ${mark} =  get_on_access_log_mark
    ${policyContent}=    Get File   ${RESOURCES_PATH}/SAV-2_policy_OA_enabled.xml
    Send Plugin Policy  av  ${SAV_APPID}  ${policyContent}

    ${policyContent}=    Get File   ${RESOURCES_PATH}/flags_policy/flags_onaccess_enabled.json
    Send Plugin Policy  av  FLAGS  ${policyContent}

    wait for on access log contains after mark   No policy override, following policy settings  mark=${mark}
    wait for on access log contains after mark   New on-access configuration: {"enabled":"true","excludeRemoteFiles":"false","exclusions":${DEFAULT_EXCLUSIONS}}  mark=${mark}

    On-access Scan Eicar Close


On Access Is Disabled After it Receives Disable Flags
    ${mark} =  get_on_access_log_mark
    ${policyContent}=    Get File   ${RESOURCES_PATH}/flags_policy/flags_onaccess_enabled.json
    Send Plugin Policy  av  FLAGS  ${policyContent}

    wait for on access log contains after mark   No policy override, following policy settings  mark=${mark}

    ${mark} =  get_on_access_log_mark
    ${policyContent}=    Get File   ${RESOURCES_PATH}/SAV-2_policy_OA_enabled.xml
    Send Plugin Policy  av  ${SAV_APPID}  ${policyContent}

    wait for on access log contains after mark   New on-access configuration: {"enabled":"true","excludeRemoteFiles":"false","exclusions":${DEFAULT_EXCLUSIONS}}  mark=${mark}

    ${mark} =  get_on_access_log_mark
    ${policyContent}=    Get File   ${RESOURCES_PATH}/flags_policy/flags.json
    Send Plugin Policy  av  FLAGS  ${policyContent}

    wait for on access log contains after mark   Overriding policy, on-access will be disabled  mark=${mark}
    wait for on access log contains after mark   Stopping the reading of Fanotify events  mark=${mark}

    On-access No Eicar Scan


On Access Does not Use Policy Settings If Flags Have Overriden Policy
    ${mark} =  get_on_access_log_mark
    ${policyContent}=    Get File   ${RESOURCES_PATH}/flags_policy/flags.json
    Send Plugin Policy  av  FLAGS  ${policyContent}

    wait for on access log contains after mark   Overriding policy, on-access will be disabled  mark=${mark}

    ${mark} =  get_on_access_log_mark
    ${policyContent}=    Get File   ${RESOURCES_PATH}/SAV-2_policy_OA_enabled.xml
    Send Plugin Policy  av  ${SAV_APPID}  ${policyContent}

    wait for on access log contains after mark    Overriding policy, on-access will be disabled  mark=${mark}
    On-access No Eicar Scan


On Access Process Reconnects To Threat Detector
    ${mark} =  get_on_access_log_mark
    ${filepath} =  Set Variable  /tmp_test/clean_file_writer/clean.txt
    ${script} =  Set Variable  ${BASH_SCRIPTS_PATH}/cleanFileWriter.sh
    ${HANDLE} =  Start Process  bash  ${script}  stderr=STDOUT
    #Register cleanups are First In Last Out, register Terminate Process last
    Register Cleanup  Remove Directory  /tmp_test/clean_file_writer/  recursive=True
    Register Cleanup  Terminate Process  ${HANDLE}

    wait for on access log contains after mark  On-close event for ${filepath}  mark=${mark}
    FakeWatchdog.Stop Sophos Threat Detector Under Fake Watchdog
    ${mark} =  get_on_access_log_mark
    FakeWatchdog.Start Sophos Threat Detector Under Fake Watchdog
    wait for on access log contains after mark  On-close event for ${filepath}  mark=${mark}
    #Depending on whether a scan is being processed or it is being requested one of these 2 errors should appear
    File Log Contains One of   ${ON_ACCESS_LOG_PATH}  0  Failed to receive scan response  Failed to send scan request

    check_on_access_log_does_not_contain_after_mark  Failed to scan ${filepath}  mark=${mark}

On Access Scan Times Out When Unable To Connect To Threat Detector
    [Tags]  MANUAL
    FakeWatchdog.Stop Sophos Threat Detector Under Fake Watchdog
    ${mark} =  get_on_access_log_mark
    Restart On Access
    wait for on access log contains after mark   mount points in on-access scanning  mark=${mark}

    ${filepath} =  Set Variable  /tmp_test/clean_file_writer/clean.txt
    Create File  ${filepath}  clean
    Register Cleanup  Remove File  ${filepath}

    wait for on access log contains after mark  On-close event for ${filepath}  mark=${mark}
    wait for on access log contains after mark  Failed to connect to Sophos Threat Detector - retrying after sleep  mark=${mark}
    wait for on access log contains after mark  Reached total maximum number of connection attempts.  timeout=${60}  mark=${mark}

On Access Logs Scan time in TRACE
    ${mark} =  get_on_access_log_mark
    Configure on access log to trace level
    wait for on access log contains after mark  Starting eventReader  mark=${mark}
    wait for on access log contains after mark   mount points in on-access scanning  mark=${mark}

    ${filepath} =  Set Variable  /tmp_test/clean_file_writer/clean.txt
    Create File  ${filepath}  clean
    Register Cleanup  Remove File  ${filepath}

    wait for on access log contains after mark  Scan for /tmp_test/clean_file_writer/clean.txt completed in  mark=${mark}

On Access logs if the kernel queue overflows
    # set loglevel to INFO to avoid log rotation due to large number of events
    Set Log Level  INFO
    Register Cleanup  Set Log Level  DEBUG
    ${mark} =  get_on_access_log_mark
    Terminate On Access
    Start On Access without Log check
    wait for on access log contains after mark  Starting scanHandler  mark=${mark}
    Sleep  1s

    Mark On Access Log
    # suspend soapd, so that we stop reading from the kernel queue
    ${soapd_pid} =  Record Soapd Plugin PID
    Evaluate  os.kill(${soapd_pid}, signal.SIGSTOP)  modules=os, signal

    # create 16384 + 1 file events
    Create Directory   /tmp_test
    Register Cleanup   Remove Directory   /tmp_test   recursive=True
    Run Shell Process
    ...   for i in `seq 0 1 16384`; do echo clean > /tmp_test/clean_file_\$i; done
    ...   OnError=Failed to create clean files

    # resume soapd
    Evaluate  os.kill(${soapd_pid}, signal.SIGCONT)  modules=os, signal

    wait for on access log contains after mark   Fanotify queue overflowed, some files will not be scanned.  mark=${mark}

On Access Uses Multiple Scanners
    register on fail  dump threads  ${ON_ACCESS_BIN}
    ${mark} =  get_on_access_log_mark
    Configure on access log to trace level
    wait for on access log contains after mark   Starting scanHandler  mark=${mark}
    Sleep  1s

    Run Process  bash  ${BASH_SCRIPTS_PATH}/fanotifyMarkSpammer.sh  timeout=10s    on_timeout=continue

    check_multiple_on_access_threads_are_scanning   mark=${mark}


On Access Can Handle Unlimited Marks
    ${mark} =  get_on_access_log_mark

    #Default MARK limit is 8192 marks https://man7.org/linux/man-pages/man2/fanotify_init.2.html
    Create Directory   /tmp_test
    Register Cleanup   Remove Directory   /tmp_test   recursive=True
    Run Shell Process
        ...   for i in `seq 0 1 8500`; do echo clean > /tmp_test/clean_file_\$i; done
        ...   OnError=Failed to create clean files

    wait for on access log contains after mark  On-open event for /tmp_test/clean_file_8500  mark=${mark}
    check_on_access_log_does_not_contain_after_mark   fanotify_mark failed: cacheFd : No space left on device  mark=${mark}

On Access Can Be enabled After It Gets Disabled In Policy
    Disable OA Scanning
    Enable OA Scanning

    On-access Scan Eicar Close

On Access Doesnt Cache Close Events With Detections After Rewrite
    [Tags]   manual
    # switch to 1 scanning thread
    Create File   ${AV_PLUGIN_PATH}/var/onaccessproductconfig.json   { "maxthreads" : 1 }
    Register Cleanup   Remove File   {AV_PLUGIN_PATH}/var/onaccessproductconfig.json
    ${mark} =  Get on access log mark
    Restart On Access
    wait for on access log contains after mark  On-access scanning enabled  mark=${mark}

    # create test file, wait for OA queue to clear
    ${srcfile} =   Set Variable   ${COMPONENT_ROOT_PATH}/chroot/susi/update_source/susicore/libicui18n.so.70.1
    ${testfile} =  Set Variable  /tmp_test/dirtyfile.txt
    Evaluate  shutil.copyfile("${srcfile}", "${testfile}-excluded")
    Move File  ${testfile}-excluded  ${testfile}
    Register Cleanup   Remove File   ${testfile}
    Sleep   1s

    # replace test file with EICAR
    ${oamark} =  Get on access log mark
    Write File After Delay  ${testfile}  ${EICAR_STRING}  ${0.005}
    Wait for on access log contains after mark  On-close event for ${testfile} from  mark=${oamark}
    Wait for on access log contains after mark  Detected "${testfile}" is infected with EICAR-AV-Test (Close-Write)  mark=${oamark}
    Sleep   1s  #Let the event (hopefully not) be cached

    # see if the file is cached
    ${oamark2} =  Get on access log mark
    Get File  ${testfile}
    Sleep  1s
    Dump On Access Log After Mark   ${oamark}
    Wait for on access log contains after mark  On-open event for ${testfile} from  mark=${oamark2}
    Wait for on access log contains after mark  Detected "${testfile}" is infected with EICAR-AV-Test (Open)  mark=${oamark2}
