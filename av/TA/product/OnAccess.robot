*** Settings ***
Documentation   Product tests for SOAP
Force Tags      PRODUCT  SOAP

Library         Process
Library         OperatingSystem
Library         String

Library         ../Libs/AVScanner.py
Library         ../Libs/LockFile.py
Library         ../Libs/OnFail.py
Library         ../Libs/LogUtils.py

Resource    ../shared/ErrorMarkers.robot
Resource    ../shared/ComponentSetup.robot
Resource    ../shared/AVResources.robot

Test Setup     On Access Test Setup
Test Teardown  On Access Test Teardown

*** Variables ***
${AV_PLUGIN_PATH}  ${COMPONENT_ROOT_PATH}
${AV_PLUGIN_BIN}   ${COMPONENT_BIN_PATH}
${AV_LOG_PATH}    ${AV_PLUGIN_PATH}/log/av.log
${TESTTMP}  /tmp_test/SSPLAVTests
${SOPHOS_THREAT_DETECTOR_BINARY_LAUNCHER}  ${SOPHOS_THREAT_DETECTOR_BINARY}_launcher

*** Keywords ***

List AV Plugin Path
    Create Directory  ${TESTTMP}
    ${result} =  Run Process  ls  -lR  ${AV_PLUGIN_PATH}  stdout=${TESTTMP}/lsstdout  stderr=STDOUT
    Log  ls -lR: ${result.stdout}
    Remove File  ${TESTTMP}/lsstdout

On Access Test Setup
    Component Test Setup
    Mark Sophos Threat Detector Log
    Register Cleanup  Require No Unhandled Exception
    Register Cleanup  Check For Coredumps  ${TEST NAME}
    Register Cleanup  Check Dmesg For Segfaults

On Access Test Teardown
    List AV Plugin Path
    run teardown functions
    Stop On Access
    Dump Log On Failure   ${ON_ACCESS_LOG_PATH}

    Check All Product Logs Do Not Contain Error
    Component Test TearDown

Start On Access
    Remove Files   /tmp/soapd.stdout  /tmp/soapd.stderr
    ${handle} =  Start Process  ${ON_ACCESS_BIN}   stdout=/tmp/soapd.stdout  stderr=/tmp/soapd.stderr
    Set Test Variable  ${ON_ACCESS_PLUGIN_HANDLE}  ${handle}
    Remove Files   /tmp/av.stdout  /tmp/av.stderr
    ${handle} =  Start Process  ${AV_PLUGIN_BIN}   stdout=/tmp/av.stdout  stderr=/tmp/av.stderr
    Set Test Variable  ${AV_PLUGIN_HANDLE}  ${handle}
    Wait Until On Access running

Stop On Access
    ${result} =  Terminate Process  ${ON_ACCESS_PLUGIN_HANDLE}
    ${result} =  Terminate Process  ${AV_PLUGIN_HANDLE}

Verify on access log rotated
    List Directory  ${AV_PLUGIN_PATH}/log/
    Should Exist  ${AV_PLUGIN_PATH}/log/soapd.log.1

Dump and Reset Logs
    Register Cleanup   Remove File      ${AV_PLUGIN_PATH}/log/av.log*
    Register Cleanup   Remove File      ${AV_PLUGIN_PATH}/log/soapd.log*
    Register Cleanup   Dump log         ${AV_PLUGIN_PATH}/log/soapd.log

*** Test Cases ***

On Access Log Rotates
    Dump and Reset Logs
    # Ensure the log is created
    Start On Access
    Stop On Access
    Increase On Access Log To Max Size
    Start On Access
    Wait Until Created   ${AV_PLUGIN_PATH}/log/soapd.log.1   timeout=10s
    Stop On Access

    ${result} =  Run Process  ls  -altr  ${AV_PLUGIN_PATH}/log/
    Log  ${result.stdout}

    Verify on access log rotated

On Access Process Parses Policy Config
    Start On Access

    Start Fake Management If Required

    ${policyContent}=    Get File   ${RESOURCES_PATH}/SAV-2_policy_OA_enabled.xml
    Send Plugin Policy  av  sav  ${policyContent}

    Wait Until On Access Log Contains  New on-access configuration: {"enabled":"true","excludeRemoteFiles":"false","exclusions":["/mnt/","/uk-filer5/"]}
    Wait Until On Access Log Contains  On-access enabled: "true"
    Wait Until On Access Log Contains  On-access scan network: "true"
    Wait Until On Access Log Contains  On-access exclusions: ["/mnt/","/uk-filer5/"]

On Access Does Not Include Remote Files If Excluded In Policy
    [Tags]  NFS
    ${source} =       Set Variable  /tmp_test/nfsshare
    ${destination} =  Set Variable  /testmnt/nfsshare
    Create Directory  ${source}
    Create Directory  ${destination}
    Create Local NFS Share   ${source}   ${destination}
    Register Cleanup  Remove Local NFS Share   ${source}   ${destination}

    Start On Access
    Wait Until On Access Log Contains  Including mount point: /testmnt/nfsshare

    Start Fake Management If Required

    Mark On Access Log
    ${policyContent}=    Get File   ${RESOURCES_PATH}/SAV-2_policy_excludeRemoteFiles.xml
    Send Plugin Policy  av  sav  ${policyContent}

    Wait Until On Access Log Contains  New on-access configuration: {"enabled":"true","excludeRemoteFiles":"true","exclusions":["/mnt/","/uk-filer5/"]}
    Wait Until On Access Log Contains  On-access enabled: "true"
    Wait Until On Access Log Contains  On-access scan network: "false"
    Wait Until On Access Log Contains  On-access exclusions: ["/mnt/","/uk-filer5/"]

    Wait Until On Access Log Contains  OA config changed, re-enumerating mount points
    On Access Does Not Log Contain With Offset  Including mount point: /testmnt/nfsshare
