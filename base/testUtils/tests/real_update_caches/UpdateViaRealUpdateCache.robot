*** Settings ***
Suite Setup       Set Suite Variable    ${regCommand}     /opt/sophos-spl/base/bin/registerCentral 5ad5322e39575d3fa99cbab82562d7dffa15e89c1cdd4812fe383be5df7d77fe https://mcs2-cloudstation-eu-west-1.prod.hydra.sophos.com/sophos/management/ep   children=true
Test Teardown   Run Keywords
...  MCSRouter Test Teardown  AND
...  Deregister From Central  AND
...  Require Uninstalled

Library     ${LIBS_DIRECTORY}/FullInstallerUtils.py
Library     ${LIBS_DIRECTORY}/LogUtils.py

Resource    ../GeneralTeardownResource.robot
Resource  ../installer/InstallerResources.robot
Resource  ../scheduler_update/SchedulerUpdateResources.robot
Resource  ../mcs_router-nova/McsRouterNovaResources.robot
Resource  ../mdr_plugin/MDRResources.robot

Default Tags  CENTRAL  MCS  UPDATE_CACHE  EXCLUDE_AWS

*** Variables ***
${InstalledBaseVersionFile}                 ${SOPHOS_INSTALL}/base/VERSION.ini
${InstalledMTRVersionFile}                 ${SOPHOS_INSTALL}/plugins/mtr/VERSION.ini
${SULDownloaderLogDowngrade}                    ${SOPHOS_INSTALL}/logs/base/downgrade-backup/suldownloader.log
*** Test Cases ***
Endpoint Updates Via Update Cache Without Errors
    [Timeout]  10 minutes
    Require Fresh Install
    Install MTR From Fake Component Suite
    Check Installed Correctly
    ${BaseDevVersion} =     Get Version Number From Ini File   ${InstalledBaseVersionFile}
    ${MtrDevVersion} =      Get Version Number From Ini File   ${InstalledMTRVersionFile}
    Log File  ${InstalledMTRVersionFile}

    Remove File   ${SOPHOS_INSTALL}/logs/base/suldownloader.log
    Override LogConf File as Global Level  DEBUG
    Register With Real Update Cache and Message Relay Account
    Wait For MCS Router To Be Running

    Wait Until Keyword Succeeds
    ...  120 secs
    ...  10 secs
    ...  ALC contains Update Cache

    Wait Until Keyword Succeeds
    ...  300 secs
    ...  5 secs
    ...  Directory Should Exist   ${SOPHOS_INSTALL}/logs/base/downgrade-backup
    Check Log Contains  Successfully connected to: Update cache at lnxdarwin01uc3  ${SULDownloaderLogDowngrade}  backedup suldownloader log

    Wait Until Keyword Succeeds
    ...  200 secs
    ...  5 secs
    ...  Check Log Contains  wdctl <> start mtr  ${SOPHOS_INSTALL}/logs/base/wdctl.log  wdctl.log

    ${NewBaseDevVersion} =     Get Version Number From Ini File   ${InstalledBaseVersionFile}

    Should Not Be Equal As Strings   ${BaseDevVersion}  ${NewBaseDevVersion}
    Log File  ${InstalledMTRVersionFile}

    #This tests check updating via update cache and effectively downgrades
    #Do a sanity test to ensure Sophos MTR is in a good state after downgrading
    Wait for MDR Executable To Be Running
    Stop MDR Plugin
    Check MDR Plugin Not Running
    Start MDR Plugin
    Wait for MDR Executable To Be Running


*** Keywords ***
Check SulDownloader Run Installers for Base and MTR
    Check Suldownloader Log Contains  Installing product: ServerProtectionLinux-Base
    Check Suldownloader Log Contains  Installing product: ServerProtectionLinux-Plugin-MDR
    Check SulDownloader Log Contains  Generating the report file

ALC contains Update Cache
    ${alc} =  Get File  ${SOPHOS_INSTALL}/base/mcs/policy/ALC-1_policy.xml
    Should Contain  ${alc}  hostname="lnxdarwin01uc3


Check Installed Correctly
    Should Exist    ${SOPHOS_INSTALL}
    Check Expected Base Processes Are Running
    ${result}=  Run Process  stat  -c  "%A"  /opt
    ${ExpectedPerms}=  Set Variable  "drwxr-xr-x"
    Should Be Equal As Strings  ${result.stdout}  ${ExpectedPerms}

Post NOVA Update Teardown
    Nova Test Teardown
    Nova Suite Teardown
    Remove Environment Variable  MCS_CONFIG_SET
    Reload Cloud Options