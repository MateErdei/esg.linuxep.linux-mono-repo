*** Settings ***
# This test uses the production central account linuxdarwin01@gmail.com
Suite Setup       Set Suite Variable    ${regCommand}     /opt/sophos-spl/base/bin/registerCentral 5ad5322e39575d3fa99cbab82562d7dffa15e89c1cdd4812fe383be5df7d77fe https://mcs2-cloudstation-eu-west-1.prod.hydra.sophos.com/sophos/management/ep   children=true
Test Teardown   Run Keywords
...  MCSRouter Test Teardown  AND
...  Deregister From Central  AND
...  Uninstall_SSPL    ${SOPHOS_INSTALL}

Library     ${COMMON_TEST_LIBS}/FullInstallerUtils.py
Library     ${COMMON_TEST_LIBS}/LogUtils.py
Library     ${COMMON_TEST_LIBS}/LiveQueryUtils.py

Resource    ${COMMON_TEST_ROBOT}/EDRResources.robot
Resource    ${COMMON_TEST_ROBOT}/GeneralTeardownResource.robot
Resource    ${COMMON_TEST_ROBOT}/InstallerResources.robot

Force Tags  CENTRAL  MCS  UPDATE_CACHE  EXCLUDE_AWS

*** Variables ***
${InstalledBaseVersionFile}                 ${SOPHOS_INSTALL}/base/VERSION.ini
${InstalledEDRVersionFile}                 ${SOPHOS_INSTALL}/plugins/edr/VERSION.ini
*** Test Cases ***
Endpoint Updates Via Update Cache Without Errors
    [Timeout]  10 minutes
    Require Fresh Install
    Install EDR Directly
    Check Installed Correctly
    ${BaseDevVersion} =     Get Version Number From Ini File   ${InstalledBaseVersionFile}
    ${edrDevVersion} =      Get Version Number From Ini File   ${InstalledEDRVersionFile}
    Log File  ${InstalledEDRVersionFile}

    ${sul_mark} =  mark_log_size  ${SULDOWNLOADER_LOG_PATH}

    Remove File   ${SOPHOS_INSTALL}/logs/base/suldownloader.log
    Override LogConf File as Global Level  DEBUG
    Register With Real Update Cache and Message Relay Account
    Wait For MCS Router To Be Running
    Wait Until Keyword Succeeds
    ...  40 seconds
    ...  5 secs
    ...  Check Mcsrouter Log Contains  Successfully connected to mcs2-cloudstation-eu-west-1.prod.hydra.sophos.com:443
    Wait Until Keyword Succeeds
    ...  120 secs
    ...  10 secs
    ...  ALC contains Update Cache

    wait_for_log_contains_from_mark  ${sul_mark}  Adding connection candidate, URL: https://sus.sophosupd.com, Message Relay: linuxuc_tes    60
    wait_for_log_contains_from_mark  ${sul_mark}  Trying SUS request (https://sus.sophosupd.com) with proxy: linuxuc    60
    wait_for_log_contains_from_mark  ${sul_mark}  SUS request received HTTP response code: 200    70
    wait_for_log_contains_from_mark  ${sul_mark}  Performing Sync using https://linuxuc_tests:8191/v3    60
    wait_for_log_contains_from_mark  ${sul_mark}  Update success    180
    Check Log Does Not Contain  Trying connection candidate, URL: https://sus.sophosupd.com, proxy: noproxy:    ${SULDOWNLOADER_LOG_PATH}  suldownloader log
    Check Log Does Not Contain  Connecting to update source directly    ${SULDOWNLOADER_LOG_PATH}  suldownloader log

    Wait Until Keyword Succeeds
    ...  200 secs
    ...  5 secs
    ...  Check Log Contains  wdctl <> start edr  ${SOPHOS_INSTALL}/logs/base/wdctl.log  wdctl.log

    ${NewBaseDevVersion} =     Get Version Number From Ini File   ${InstalledBaseVersionFile}
    ${newedrDevVersion} =      Get Version Number From Ini File   ${InstalledEDRVersionFile}

    Should Not Be Equal As Strings   ${BaseDevVersion}  ${NewBaseDevVersion}
    Should Not Be Equal As Strings   ${edrDevVersion}  ${newedrDevVersion}


    #This tests check updating via update cache and effectively downgrades
    #Do a sanity test to ensure Sophos MTR is in a good state after downgrading
    Wait Until EDR OSQuery Running  30
    Run Live Query  ${SIMPLE_QUERY_4_ROW}   simple
    Wait Until Keyword Succeeds
    ...  100 secs
    ...  2 secs
    ...  Check Log Contains String N times   ${SOPHOS_INSTALL}/plugins/edr/log/livequery.log   edr_log  Successfully executed query with name: simple  1


*** Keywords ***
ALC contains Update Cache
    ${alc} =  Get File  ${SOPHOS_INSTALL}/base/mcs/policy/ALC-1_policy.xml
    Should Contain  ${alc}  hostname="
    Should Contain  ${alc}  priority="
    Should Contain  ${alc}  <update_cache>


Check Installed Correctly
    Should Exist    ${SOPHOS_INSTALL}
    Check Expected Base Processes Are Running
    ${result}=  Run Process  stat  -c  "%A"  /opt
    ${ExpectedPerms}=  Set Variable  "drwxr-xr-x"
    Should Be Equal As Strings  ${result.stdout}  ${ExpectedPerms}

Register With Real Update Cache and Message Relay Account
    [Arguments]   ${messageRelayOptions}=
    Mark All Logs  Registering with Real Update Cache and Message Relay account
    # Remove the file so that we can use the "SulDownloader Reports Finished" function.
    Remove File  ${SOPHOS_INSTALL}/logs/base/suldownloader.log
    Register With Central  ${regCommand} ${messageRelayOptions}
