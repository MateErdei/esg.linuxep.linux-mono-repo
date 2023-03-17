*** Settings ***
# This test uses the production central account linuxdarwin01@gmail.com
Suite Setup       Set Suite Variable    ${regCommand}     /opt/sophos-spl/base/bin/registerCentral 5ad5322e39575d3fa99cbab82562d7dffa15e89c1cdd4812fe383be5df7d77fe https://mcs2-cloudstation-eu-west-1.prod.hydra.sophos.com/sophos/management/ep   children=true
Test Teardown   Run Keywords
...  MCSRouter Test Teardown  AND
...  Deregister From Central  AND
...  Require Uninstalled

Library     ${LIBS_DIRECTORY}/FullInstallerUtils.py
Library     ${LIBS_DIRECTORY}/LogUtils.py
Library     ${LIBS_DIRECTORY}/LiveQueryUtils.py

Resource    ../GeneralTeardownResource.robot
Resource  ../installer/InstallerResources.robot
Resource  ../mcs_router-nova/McsRouterNovaResources.robot
Resource  ../edr_plugin/EDRResources.robot

Default Tags  CENTRAL  MCS  UPDATE_CACHE  EXCLUDE_AWS

*** Variables ***
${InstalledBaseVersionFile}                 ${SOPHOS_INSTALL}/base/VERSION.ini
${InstalledEDRVersionFile}                 ${SOPHOS_INSTALL}/plugins/edr/VERSION.ini
${SULDownloaderLogDowngrade}                    ${SOPHOS_INSTALL}/logs/base/downgrade-backup/suldownloader.log
*** Test Cases ***
Endpoint Updates Via Update Cache Without Errors
    [Timeout]  10 minutes
    Require Fresh Install
    Install EDR Directly
    Check Installed Correctly
    ${BaseDevVersion} =     Get Version Number From Ini File   ${InstalledBaseVersionFile}
    ${edrDevVersion} =      Get Version Number From Ini File   ${InstalledEDRVersionFile}
    Log File  ${InstalledEDRVersionFile}

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

    Wait Until Keyword Succeeds
    ...  300 secs
    ...  5 secs
    ...  Directory Should Exist   ${SOPHOS_INSTALL}/logs/base/downgrade-backup

    Check Log Contains  Performing Sync using https://linuxuc_tests:8191/v3   ${SULDownloaderLogDowngrade}  backedup suldownloader log
    Check Log Does Not Contain  Connecting to update source directly   ${SULDownloaderLogDowngrade}  backedup suldownloader log

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