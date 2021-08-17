*** Settings ***
# This test uses the qa central account LinuxDarwinSSPLAutomation@mail.com
Suite Setup       Set Suite Variable    ${regCommand}     /opt/sophos-spl/base/bin/registerCentral 8f4b3234643165725ca6a9a2b87a0893880f4041ff7d2887a89d74ca329af4ce https://mcs2-cloudstation-eu-central-1.qa.hydra.sophos.com/sophos/management/ep   children=true
Test Teardown   Run Keywords
...  Remove Environment Variable  https_proxy  AND
...  Remove Environment Variable  MCS_CA  AND
...  MCSRouter Test Teardown  AND
...  Deregister From Central  AND
...  Require Uninstalled
Test Setup  Setup QA account certs
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
Test we can reach central through an enviroment proxy
    [Timeout]  10 minutes
    Require Fresh Install
    Create File  /opt/sophos-spl/base/mcs/certs/ca_env_override_flag
    Install EDR Directly
    Check Expected Base Processes Are Running
    ${BaseDevVersion} =     Get Version Number From Ini File   ${InstalledBaseVersionFile}
    ${edrDevVersion} =      Get Version Number From Ini File   ${InstalledEDRVersionFile}
    Log File  ${InstalledEDRVersionFile}

    Remove File   ${SOPHOS_INSTALL}/logs/base/suldownloader.log
    Override LogConf File as Global Level  DEBUG
    Set Environment Variable  https_proxy   http://ssplsecureproxyserver.eng.sophos:8888
    Register With Real Update Cache and Message Relay Account
    Wait For MCS Router To Be Running
    Wait Until Keyword Succeeds
    ...  40 seconds
    ...  5 secs
    ...  Check Mcsrouter Log Contains  Successfully connected to mcs2-cloudstation-eu-central-1.qa.hydra.sophos.com:443 via ssplsecureproxyserver.eng.sophos:8888
*** Keywords ***
Setup QA account certs
    Set Environment Variable  MCS_CA   ${SUPPORT_FILES}/CloudAutomation/hmr-qa-sha256.pem
    Run Process  chmod  +r  ${SUPPORT_FILES}/CloudAutomation/hmr-qa-sha256.pem

