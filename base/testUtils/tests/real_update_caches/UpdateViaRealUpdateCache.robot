*** Settings ***
Suite Setup       Setup MCS Tests Nova
Test Teardown   Post NOVA Update Teardown

Library     ${libs_directory}/FullInstallerUtils.py
Library     ${libs_directory}/LogUtils.py

Resource    ../GeneralTeardownResource.robot
Resource  ../installer/InstallerResources.robot
Resource  ../scheduler_update/SchedulerUpdateResources.robot
Resource  ../mcs_router-nova/McsRouterNovaResources.robot
Resource  ../mdr_plugin/MDRResources.robot

Default Tags  CENTRAL  MCS  UPDATE_CACHE  EXCLUDE_AWS  SLOW

*** Variables ***
${InstalledBaseVersionFile}                 ${SOPHOS_INSTALL}/base/VERSION.ini
${InstalledMTRVersionFile}                 ${SOPHOS_INSTALL}/plugins/mtr/VERSION.ini

*** Test Cases ***
Endpoint Updates Via Update Cache Without Errors
    [Timeout]  10 minutes
    Install MTR From Fake Component Suite
    Check Installed Correctly
    ${BaseDevVersion} =     Get Version Number From Ini File   ${InstalledBaseVersionFile}
    ${MtrDevVersion} =      Get Version Number From Ini File   ${InstalledMTRVersionFile}
    Log File  ${InstalledMTRVersionFile}

    Remove File   ${SOPHOS_INSTALL}/logs/base/suldownloader.log

    Register With Real Update Cache and Message Relay Account
    Wait For MCS Router To Be Running
    Wait For Server In Cloud
    Wait Until Keyword Succeeds
        ...  60 secs
        ...  5 secs
        ...  ALC contains Update Cache

    Wait Until Keyword Succeeds
    ...  300 secs
    ...  5 secs
    ...  Check SulDownloader Run Installers for Base and MTR
    Check Suldownloader Log Contains  Successfully connected to: Update cache at sspluc1:8191

    ${NewBaseDevVersion} =     Get Version Number From Ini File   ${InstalledBaseVersionFile}

    Should Not Be Equal As Strings   ${BaseDevVersion}  ${NewBaseDevVersion}
    Log File  ${InstalledMTRVersionFile}

    #This tests check updating via updace cache and effectively downgrades
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
    Should Contain  ${alc}  ssplucmr


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