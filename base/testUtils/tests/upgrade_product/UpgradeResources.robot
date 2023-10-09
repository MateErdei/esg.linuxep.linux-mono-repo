*** Settings ***
Library     ${LIBS_DIRECTORY}/FullInstallerUtils.py
Library     ${LIBS_DIRECTORY}/MCSRouter.py
Library     ${LIBS_DIRECTORY}/OSUtils.py
Library     ${LIBS_DIRECTORY}/WarehouseUtils.py
Library     ${LIBS_DIRECTORY}/WarehouseGenerator.py
Library     ${LIBS_DIRECTORY}/TeardownTools.py
Library     ${LIBS_DIRECTORY}/TemporaryDirectoryManager.py
Library     ${LIBS_DIRECTORY}/ThinInstallerUtils.py
Library     ${LIBS_DIRECTORY}/UpdateSchedulerHelper.py
Library     Process
Library     OperatingSystem
Library     String

Resource    ../mcs_router/McsRouterResources.robot
Resource    ../thin_installer/ThinInstallerResources.robot
Resource    ../liveresponse_plugin/LiveResponseResources.robot


*** Variables ***
${warehousedir}                             ./tmp
${InstalledBaseVersionFile}                 ${SOPHOS_INSTALL}/base/VERSION.ini
${InstalledMDRPluginVersionFile}            ${SOPHOS_INSTALL}/plugins/mtr/VERSION.ini
${InstalledAVPluginVersionFile}             ${SOPHOS_INSTALL}/plugins/av/VERSION.ini
${InstalledMDRSuiteVersionFile}             ${SOPHOS_INSTALL}/plugins/mtr/dbos/data/VERSION.ini
${InstalledEDRPluginVersionFile}            ${SOPHOS_INSTALL}/plugins/edr/VERSION.ini
${InstalledLRPluginVersionFile}             ${SOPHOS_INSTALL}/plugins/liveresponse/VERSION.ini
${InstalledEJPluginVersionFile}             ${SOPHOS_INSTALL}/plugins/eventjournaler/VERSION.ini
${WarehouseBaseVersionFileExtension}        ./TestInstallFiles/ServerProtectionLinux-Base/files/base/VERSION.ini
${WarehouseMDRPluginVersionFileExtension}   ./TestInstallFiles/ServerProtectionLinux-MDR-Control/files/plugins/mtr/VERSION.ini
${WarehouseMDRSuiteVersionFileExtension}    ./TestInstallFiles/ServerProtectionLinux-MDR-DBOS-Component/files/plugins/mtr/dbos/data/VERSION.ini


*** Keywords ***
Test Setup
    Require Uninstalled
    Set Environment Variable  CORRUPTINSTALL  no

Test Teardown
    [Arguments]  ${UninstallAudit}=True
    Run Keyword If Test Failed    Dump Cloud Server Log
    Run Keyword If Test Failed    Log XDR Intermediary File
    General Test Teardown
    Run Keyword If Test Failed    Dump Thininstaller Log
    Run Keyword If Test Failed    Log Status Of Sophos Spl
    Run Keyword If Test Failed    Dump Teardown Log    ${SOPHOS_INSTALL}/base/update/var/config.json
    Run Keyword If Test Failed    Dump Teardown Log    /tmp/preserve-sul-downgrade
    Stop Local Cloud Server
    Cleanup Local Warehouse And Thininstaller
    Require Uninstalled
    Cleanup Temporary Folders
    Require Uninstalled

Suite Setup
    Regenerate HTTPS Certificates
    Regenerate Certificates
    Set Local CA Environment Variable
    Setup Ostia Warehouse Environment


Suite Teardown
    Teardown Ostia Warehouse Environment
    Run Process    make    clean    cwd=${SUPPORT_FILES}/https/

Cleanup Local Warehouse And Thininstaller
    Stop Proxy Servers
    Stop Update Server
    Cleanup Files
    Empty Directory  ${warehousedir}

Send ALC Policy And Prepare For Upgrade
    [Arguments]  ${PolicyPath}
    Prepare Installation For Upgrade Using Policy  ${PolicyPath}
    Send Policy File  alc  ${PolicyPath}  wait_for_policy=${True}

Prepare Installation For Upgrade Using Policy
    [Arguments]  ${PolicyPath}
    Install Upgrade Certs For Policy  ${PolicyPath}

Install Local SSL Server Cert To System
    Copy File   ${SUPPORT_FILES}/https/ca/root-ca.crt.pem    ${SUPPORT_FILES}/https/ca/root-ca.crt
    Install System Ca Cert  ${SUPPORT_FILES}/https/ca/root-ca.crt

Install Ostia SSL Certs To System
    Install System Ca Cert  ${SUPPORT_FILES}/sophos_certs/OstiaCA.crt

Install Internal SSL Certs To System
    Install System Ca Cert  ${SUPPORT_FILES}/sophos_certs/internal/InternalServerCA.cer
    Install System Ca Cert  ${SUPPORT_FILES}/sophos_certs/internal/InternalServerCA1.crt
    Install System Ca Cert  ${SUPPORT_FILES}/sophos_certs/internal/InternalServerCA2.crt
    Install System Ca Cert  ${SUPPORT_FILES}/sophos_certs/internal/InternalServerCA3.crt

Revert System CA Certs
    Cleanup System Ca Certs

Setup Ostia Warehouse Environment
    Generate Local Ssl Certs If They Dont Exist
    Install Local SSL Server Cert To System
    Install Ostia SSL Certs To System
    Install Internal SSL Certs To System
    Setup Local Warehouses If Needed

Teardown Ostia Warehouse Environment
    Restore Host File After Using Local Ostia Warehouses
    Stop Local Ostia Servers
    Revert System CA Certs

Wait For Initial Update To Fail
    #Only to be used when expecting the first update to fail after using the thininstaller
    Wait Until Keyword Succeeds
    ...  120 secs
    ...  5 secs
    ...  Check SulDownloader Log Contains  suldownloaderdata <> Update failed
    Remove File  ${SOPHOS_INSTALL}/logs/base/suldownloader.log
    Remove File  ${SOPHOS_INSTALL}/logs/base/updatescheduler.log
    Remove File  ${SOPHOS_INSTALL}/logs/base/sophosspl/updatescheduler.log

Check MDR and Base Components Inside The ALC Status
    ${statusContent} =  Get File  /opt/sophos-spl/base/mcs/status/ALC_status.xml
    Should Contain  ${statusContent}  ServerProtectionLinux-MDR-DBOS-Component
    Should Contain  ${statusContent}  ServerProtectionLinux-MDR-Control-Component
    Should Contain  ${statusContent}  ServerProtectionLinux-Base

Check Status Has Changed
     [Arguments]  ${status1}
     ${status2} =  Get File  /opt/sophos-spl/base/mcs/status/ALC_status.xml
     Should Not Be Equal As Strings  ${status1}  ${status2}


Run Shell Process
    [Arguments]  ${Command}   ${OnError}   ${timeout}=20s   ${expectedExitCode}=0
    ${result} =   Run Process  ${Command}   shell=True   timeout=${timeout}
    Should Be Equal As Integers  ${result.rc}  ${expectedExitCode}   "${OnError}.\nstdout: \n${result.stdout} \n.stderr: \n${result.stderr}"
    [Return]  ${result.stdout} + ${result.stderr}

Log XDR Intermediary File
    Run Keyword And Ignore Error   Log File  ${SOPHOS_INSTALL}/plugins/edr/var/xdr_intermediary