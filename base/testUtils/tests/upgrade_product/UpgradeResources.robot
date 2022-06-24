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
${warehousedir}                                 ./tmp
${InstalledBaseVersionFile}                     ${SOPHOS_INSTALL}/base/VERSION.ini
${InstalledMDRPluginVersionFile}                ${SOPHOS_INSTALL}/plugins/mtr/VERSION.ini
${InstalledAVPluginVersionFile}                 ${SOPHOS_INSTALL}/plugins/av/VERSION.ini
${InstalledMDRSuiteVersionFile}                 ${SOPHOS_INSTALL}/plugins/mtr/dbos/data/VERSION.ini
${InstalledEDRPluginVersionFile}                ${SOPHOS_INSTALL}/plugins/edr/VERSION.ini
${InstalledRuntimedetectionsPluginVersionFile}  ${SOPHOS_INSTALL}/plugins/runtimedetections/VERSION.ini
${InstalledLRPluginVersionFile}                 ${SOPHOS_INSTALL}/plugins/liveresponse/VERSION.ini
${InstalledEJPluginVersionFile}                 ${SOPHOS_INSTALL}/plugins/eventjournaler/VERSION.ini
${InstalledHBTPluginVersionFile}                ${SOPHOS_INSTALL}/plugins/heartbeat/VERSION.ini
${WarehouseBaseVersionFileExtension}            ./TestInstallFiles/ServerProtectionLinux-Base/files/base/VERSION.ini
${WarehouseMDRPluginVersionFileExtension}       ./TestInstallFiles/ServerProtectionLinux-MDR-Control/files/plugins/mtr/VERSION.ini
${WarehouseMDRSuiteVersionFileExtension}        ./TestInstallFiles/ServerProtectionLinux-MDR-DBOS-Component/files/plugins/mtr/dbos/data/VERSION.ini
${sdds3_server_output}                             /tmp/sdds3_server.log

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
    Remove File  /tmp/preserve-sul-downgrade
    Stop Local Cloud Server
    Cleanup Local Warehouse And Thininstaller
    Require Uninstalled
    Cleanup Temporary Folders
    Require Uninstalled

Suite Setup
    Suite Setup Without Ostia
    Setup Ostia Warehouse Environment

Suite Setup Without Ostia
    Regenerate HTTPS Certificates
    Regenerate Certificates
    Set Local CA Environment Variable


Suite Teardown
    Teardown Ostia Warehouse Environment
    Suite Teardown Without Ostia

Suite Teardown Without Ostia
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
    Install System Ca Cert  ${SUPPORT_FILES}/sophos_certs/internal/SophosInternalIntCA-A1.crt
    Install System Ca Cert  ${SUPPORT_FILES}/sophos_certs/internal/SophosInternalIntCA-A2.crt
    Install System Ca Cert  ${SUPPORT_FILES}/sophos_certs/internal/SophosInternalIntCA-B1.crt
    Install System Ca Cert  ${SUPPORT_FILES}/sophos_certs/internal/SophosInternalIntCA-B2.crt
    Install System Ca Cert  ${SUPPORT_FILES}/sophos_certs/internal/SophosInternalRootCA-A.crt
    Install System Ca Cert  ${SUPPORT_FILES}/sophos_certs/internal/SophosInternalRootCA-B.crt

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
    Should Contain  ${statusContent}  ServerProtectionLinux-Plugin-MDR
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


Mark Known Upgrade Errors
    #TODO LINUXDAR-3671 remove when this defect is closed
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/logs/base/wdctl.log   Failed to remove runtimedetections: Timeout out connecting to watchdog: No incoming data
    #TODO LINUXDAR-3503 remove when this defect is closed
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/plugins/edr/log/edr.log  edr <> Failed to start extension, extension.Start threw: Failed to register extension: Failed adding registry: Duplicate extension

    #TODO LINUXDAR-2972 remove when this defect is closed
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/logs/base/sophosspl/mcsrouter.log  root <> Atomic write failed with message: [Errno 13] Permission denied: '/opt/sophos-spl/tmp/policy/flags.json'
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/logs/base/sophosspl/mcsrouter.log  root <> Atomic write failed with message: [Errno 2] No such file or directory: '/opt/sophos-spl/tmp/policy/flags.json'
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/logs/base/sophosspl/mcsrouter.log  root <> utf8 write failed with message: [Errno 13] Permission denied: '/opt/sophos-spl/tmp/policy/flags.json'

    # Deliberatly missing the last part of these lines so it will work on all plugin registry files.
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/logs/base/sophosspl/mcsrouter.log  mcsrouter.utils.plugin_registry <> Failed to load plugin file: /opt/sophos-spl/base/pluginRegistry
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/logs/base/sophosspl/mcsrouter.log  mcsrouter.utils.plugin_registry <> [Errno 13] Permission denied: '/opt/sophos-spl/base/pluginRegistry

    #TODO LINUXDAR-3490 remove when this defect is fixed
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/logs/base/watchdog.log    ProcessMonitoringImpl <> /opt/sophos-spl/plugins/runtimedetections/bin/runtimedetections died with 1

Create Local SDDS3 Override
    [Arguments]  ${URLS}=http://127.0.0.1:8080  ${CDN_URL}=http://127.0.0.1:8080  ${USE_SDDS3_OVERRIDE}=true  ${USE_HTTP}=true
    ${override_file_contents} =  Catenate    SEPARATOR=\n
    # these settings will instruct SulDownloader to update using SDDS3 via a local test HTTP server.
    ...  URLS = ${URLS}
    ...  CDN_URL = ${CDN_URL}
    ...  USE_SDDS3 = ${USE_SDDS3_OVERRIDE}
    ...  USE_HTTP = ${USE_HTTP}
    Create File    ${sdds3_override_file}    content=${override_file_contents}

Start Local SDDS3 Server
    ${handle}=  Start Process  bash -x ${SUPPORT_FILES}/jenkins/runCommandFromPythonVenvIfSet.sh python3 ${LIBS_DIRECTORY}/SDDS3server.py --launchdarkly ${SYSTEMPRODUCT_TEST_INPUT}/sdds3/launchdarkly --sdds3 ${SYSTEMPRODUCT_TEST_INPUT}/sdds3/repo  shell=true
    [Return]  ${handle}

Start Local SDDS3 Server With Empty Repo
    Create Directory  /tmp/FakeFlags
    Create Directory    /tmp/FakeRepo
    ${handle}=  Start Process  bash -x ${SUPPORT_FILES}/jenkins/runCommandFromPythonVenvIfSet.sh python3 ${LIBS_DIRECTORY}/SDDS3server.py --launchdarkly /tmp/FakeFlags --sdds3 /tmp/FakeRepo  shell=true
    [Return]  ${handle}

Stop Local SDDS3 Server
     terminate process  ${GL_handle}  True
     Dump Teardown Log    ${sdds3_server_output}
     terminate all processes  True
