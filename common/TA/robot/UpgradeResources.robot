*** Settings ***
Library    Collections
Library    OperatingSystem
Library    Process
Library    String

Library    ${COMMON_TEST_LIBS}/CentralUtils.py
Library    ${COMMON_TEST_LIBS}/FullInstallerUtils.py
Library    ${COMMON_TEST_LIBS}/LogUtils.py
Library    ${COMMON_TEST_LIBS}/MCSRouter.py
Library    ${COMMON_TEST_LIBS}/OSUtils.py
Library    ${COMMON_TEST_LIBS}/ProcessUtils.py
Library    ${COMMON_TEST_LIBS}/TemporaryDirectoryManager.py
Library    ${COMMON_TEST_LIBS}/ThinInstallerUtils.py
Library    ${COMMON_TEST_LIBS}/UpdateServer.py
Library    ${COMMON_TEST_LIBS}/WarehouseUtils.py

Resource    AVResources.robot
Resource    InstallerResources.robot
Resource    McsRouterResources.robot
Resource    SafeStoreResources.robot


*** Variables ***
${InstalledBaseVersionFile}                     ${SOPHOS_INSTALL}/base/VERSION.ini
${sdds3_server_log}                             /tmp/sdds3_server.log
${sdds3_server_std_output}                      /tmp/sdds3_server_stdout.log
${tmpLaunchDarkly}                              /tmp/launchdarkly
${staticflagfile}                               linuxep.json

*** Keywords ***
Upgrade Resources SDDS3 Test Teardown
    [Arguments]    ${installDir}=${SOPHOS_INSTALL}
    Stop Local SDDS3 Server
    Upgrade Resources Test Teardown    installDir=${installDir}

Upgrade Resources Test Teardown
    [Arguments]  ${UninstallAudit}=True    ${installDir}=${SOPHOS_INSTALL}
    Run Keyword If Test Failed    Dump Cloud Server Log
    Run Keyword If Test Failed    Log XDR Intermediary File
    General Test Teardown    ${installDir}
    Run Keyword If Test Failed    Dump Thininstaller Log
    Run Keyword If Test Failed    Log Status Of Sophos Spl
    Run Keyword If Test Failed    Dump Teardown Log    ${installDir}/base/update/var/config.json
    Run Keyword If Test Failed    Dump Teardown Log    /tmp/preserve-sul-downgrade
    Remove File  /tmp/preserve-sul-downgrade
    Stop Local Cloud Server
    Cleanup Local Warehouse And Thininstaller
    Require Uninstalled
    Cleanup Temporary Folders

Upgrade Resources Suite Setup
    Set Suite Variable    ${GL_handle}       ${EMPTY}
    Set Suite Variable    ${GL_UC_handle}    ${EMPTY}
    Generate Local Ssl Certs If They Dont Exist
    Install Local SSL Server Cert To System
    Regenerate Certificates
    Set Local CA Environment Variable

Upgrade Resources Suite Teardown
    Run Process    make    clean    cwd=${SUPPORT_FILES}/https/

Cleanup Local Warehouse And Thininstaller
    Stop Proxy Servers
    Stop Update Server
    Cleanup Files

Install Local SSL Server Cert To System
    Copy File   ${SUPPORT_FILES}/https/ca/root-ca.crt.pem    ${SUPPORT_FILES}/https/ca/root-ca.crt
    Install System Ca Cert  ${SUPPORT_FILES}/https/ca/root-ca.crt

Check AV and Base Components Inside The ALC Status
    ${statusContent} =  Get File  /opt/sophos-spl/base/mcs/status/ALC_status.xml
    Should Contain  ${statusContent}  ServerProtectionLinux-Plugin-AV
    Should Contain  ${statusContent}  ServerProtectionLinux-Base

Log XDR Intermediary File
    Run Keyword And Ignore Error   Log File  ${SOPHOS_INSTALL}/plugins/edr/var/xdr_intermediary

Mark Known Upgrade Errors
    # TODO: LINUXDAR-7318 - expected till bugfix is in released version
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/logs/base/watchdog.log  /opt/sophos-spl/base/bin/UpdateScheduler died with signal 9

    #LINUXDAR-4015 There won't be a fix for this error, please check the ticket for more info
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/plugins/runtimedetections/log/runtimedetections.log  runtimedetections <> Could not enter supervised child process

Mark Known Downgrade Errors
    # Deliberatly missing the last part of these lines so it will work on all plugin registry files.
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/logs/base/sophosspl/mcsrouter.log  mcsrouter.utils.plugin_registry <> Failed to load plugin file: /opt/sophos-spl/base/pluginRegistry
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/logs/base/sophosspl/mcsrouter.log  mcsrouter.utils.plugin_registry <> [Errno 13] Permission denied: '/opt/sophos-spl/base/pluginRegistry
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/logs/base/wdctl.log  wdctlActions <> Plugin "responseactions" not in registry
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/logs/base/wdctl.log  wdctlActions <> Plugin "liveresponse" not in registry

Create Local SDDS3 Override
    [Arguments]  ${URLS}=https://localhost:8080  ${CDN_URL}=https://localhost:8080
    ${override_file_contents} =  Catenate    SEPARATOR=\n
    # these settings will instruct SulDownloader to update using SDDS3 via a local test HTTP server.
    ...  URLS = ${URLS}
    ...  CDN_URL = ${CDN_URL}
    Create File    ${SDDS3_OVERRIDE_FILE}    content=${override_file_contents}

Start Local SDDS3 Server
    [Arguments]    ${launchdarklyPath}=${VUT_LAUNCH_DARKLY}    ${sdds3repoPath}=${VUT_WAREHOUSE_ROOT}
    ...  ${port}=8080
    ${handle}=  Start Process
    ...  bash  -x
    ...  ${SUPPORT_FILES}/jenkins/runCommandFromPythonVenvIfSet.sh
    ...  python3  ${COMMON_TEST_LIBS}/SDDS3server.py
    ...  --launchdarkly  ${launchdarklyPath}
    ...  --sdds3  ${sdds3repoPath}
    ...  --port   ${port}
    ...  --certpath   ${SUPPORT_FILES}/https/server-private.pem
    ...  stdout=${sdds3_server_std_output}
    ...  stderr=STDOUT
    Set Suite Variable    $GL_handle    ${handle}
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  Can Curl Url    https://localhost:${port}
    [Return]  ${handle}

Start Local Dogfood SDDS3 Server
    Start Local SDDS3 Server    ${INPUT_DIRECTORY}/dogfood_launch_darkly    ${DOGFOOD_WAREHOUSE_REPO_ROOT}

Start Local Current Shipping SDDS3 Server
    Start Local SDDS3 Server    ${INPUT_DIRECTORY}/current_shipping_launch_darkly    ${CURRENT_SHIPPING_WAREHOUSE_REPO_ROOT}

Start Local SDDS3 Server With Empty Repo
    Create Directory  /tmp/FakeFlags
    Create Directory    /tmp/FakeRepo
    ${handle}=    Start Local SDDS3 Server    /tmp/FakeFlags    /tmp/FakeRepo
    [Return]  ${handle}

Debug Local SDDS3 Server
    [Arguments]  ${handle}
    Run Keyword If Test Passed  Return From Keyword
    ${result} =  Run Process  pstree  -a  stderr=STDOUT
    Log  pstree: ${result.rc} : ${result.stdout}
    ${result} =  Run Process  netstat  -pnl  --inet  stderr=STDOUT
    Log  netstat -pnl: ${result.rc} : ${result.stdout}
    ${result} =  Run Process  netstat  -pn  --inet  stderr=STDOUT
    Log  netstat -pn: ${result.rc} : ${result.stdout}
    ${result} =  Run Process  ss  -plt  stderr=STDOUT
    Log  ss: ${result.rc} : ${result.stdout}
    return from keyword if  "${handle}" == "${EMPTY}"
    ProcessUtils.dump_threads_from_process  ${handle}

Stop Local SDDS3 Server
    return from keyword if  "${GL_handle}" == "${EMPTY}"
    Run Keyword and Ignore Error  Debug Local SDDS3 Server  ${GL_handle}
    ${result} =  Terminate Process  ${GL_handle}  True
    Set Suite Variable    $GL_handle    ${EMPTY}
    Dump Teardown Log    ${sdds3_server_std_output}
    Dump Teardown Log    ${sdds3_server_log}
    Log  SDDS3_server rc = ${result.rc}
    Terminate All Processes  True

Send ALC Policy And Prepare For Upgrade
    [Arguments]    ${policyPath}
    Prepare Installation For Upgrade Using Policy    ${policyPath}
    send_policy_file    alc    ${policyPath}    wait_for_policy=${True}

Prepare Installation For Upgrade Using Policy
    [Arguments]    ${policyPath}
    install_upgrade_certs_for_policy  ${policyPath}

Check For downgraded logs
    # AV logs
    File Should Exist  ${SOPHOS_INSTALL}/plugins/av/log/downgrade-backup/av.log
    File Should Exist  ${SOPHOS_INSTALL}/plugins/av/log/downgrade-backup/soapd.log
    File Should Exist  ${SOPHOS_INSTALL}/plugins/av/log/downgrade-backup/sophos_threat_detector.log
    File Should Exist  ${SOPHOS_INSTALL}/plugins/av/log/downgrade-backup/safestore.log

    # Liveresponse logs
    File Should Exist  ${SOPHOS_INSTALL}/plugins/liveresponse/log/downgrade-backup/liveresponse.log

    # Event journaler logs
    File Should Exist  ${SOPHOS_INSTALL}/plugins/eventjournaler/log/downgrade-backup/eventjournaler.log
    # Edr
    File Should Exist  ${SOPHOS_INSTALL}/plugins/edr/log/downgrade-backup/edr.log

    # Response actions logs
    File Should Exist  ${SOPHOS_INSTALL}/plugins/responseactions/log/downgrade-backup/responseactions.log

    # RTD logs
    File Should Exist  ${SOPHOS_INSTALL}/plugins/runtimedetections/log/downgrade-backup/runtimedetections.log

Check Current Release With AV Installed Correctly
    Check MCS Router Running
    Check AV Plugin Installed
    Check SafeStore Installed Correctly
    Check Installed Correctly
    Check Comms Component Is Not Present

Check Comms Component Is Not Present
    Verify Group Removed    sophos-spl-network
    Verify User Removed    sophos-spl-network

    File Should Not Exist    ${SOPHOS_INSTALL}/base/bin/CommsComponent
    File Should Not Exist    ${SOPHOS_INSTALL}/logs/base/sophosspl/comms_component.log
    File Should Not Exist    ${SOPHOS_INSTALL}/base/pluginRegistry/commscomponent.json

    Directory Should Not Exist   ${SOPHOS_INSTALL}/var/sophos-spl-comms
    Directory Should Not Exist   ${SOPHOS_INSTALL}/var/comms
    Directory Should Not Exist   ${SOPHOS_INSTALL}/logs/base/sophos-spl-comms

Check Installed Correctly
    Should Exist    ${SOPHOS_INSTALL}
    Check Correct MCS Password And ID For Local Cloud Saved

    ${result}=  Run Process  stat  -c  "%A"  /opt
    ${ExpectedPerms}=  Set Variable  "drwxr-xr-x"
    Should Be Equal As Strings  ${result.stdout}  ${ExpectedPerms}
    ${version_number} =  Get Version Number From Ini File  ${InstalledBaseVersionFile}
    Check Expected Base Processes Except SDU Are Running

Check Installed Plugins Are VUT Versions
    # TODO LINUXDAR-8265: Remove is_using_version_workaround
    ${contents} =  Get File  ${EDR_DIR}/VERSION.ini
    ${edr_vut_version} =  get_version_for_rigidname_in_sdds3_warehouse   ${VUT_WAREHOUSE_ROOT}    ${VUT_LAUNCH_DARKLY}    ServerProtectionLinux-Plugin-EDR    is_using_version_workaround=${True}
    Should Contain   ${contents}   PRODUCT_VERSION = ${edr_vut_version}
    ${contents} =  Get File  ${LIVERESPONSE_DIR}/VERSION.ini
    ${liveresponse_vut_version} =  get_version_for_rigidname_in_sdds3_warehouse   ${VUT_WAREHOUSE_ROOT}    ${VUT_LAUNCH_DARKLY}    ServerProtectionLinux-Plugin-liveresponse    is_using_version_workaround=${True}
    Should Contain   ${contents}   PRODUCT_VERSION = ${liveresponse_vut_version}
    ${contents} =  Get File  ${EVENTJOURNALER_DIR}/VERSION.ini
    ${eventjournaler_vut_version} =  get_version_for_rigidname_in_sdds3_warehouse   ${VUT_WAREHOUSE_ROOT}    ${VUT_LAUNCH_DARKLY}    ServerProtectionLinux-Plugin-EventJournaler    is_using_version_workaround=${True}
    Should Contain   ${contents}   PRODUCT_VERSION = ${eventjournaler_vut_version}
    ${contents} =  Get File  ${RUNTIMEDETECTIONS_DIR}/VERSION.ini
    ${runtimedetections_vut_version} =  get_version_for_rigidname_in_sdds3_warehouse   ${VUT_WAREHOUSE_ROOT}    ${VUT_LAUNCH_DARKLY}    ServerProtectionLinux-Plugin-RuntimeDetections    is_using_version_workaround=${True}
    Should Contain   ${contents}   PRODUCT_VERSION = ${runtimedetections_vut_version}

Setup SUS static
    Remove Directory   ${tmpLaunchDarkly}   recursive=True
    Create Directory   ${tmpLaunchDarkly}
    Copy File  ${STATIC_SUITES}/linuxep.json   ${tmpLaunchDarkly}

    Copy File  ${VUT_LAUNCH_DARKLY}/release.linuxep.ServerProtectionLinux-Base.json   ${tmpLaunchDarkly}
    Copy File  ${VUT_LAUNCH_DARKLY}/release.linuxep.ServerProtectionLinux-Plugin-AV.json   ${tmpLaunchDarkly}
    Copy File  ${VUT_LAUNCH_DARKLY}/release.linuxep.ServerProtectionLinux-Plugin-EDR.json  ${tmpLaunchDarkly}


Setup SUS static 999
    Remove Directory   ${tmpLaunchDarkly}   recursive=True
    Create Directory   ${tmpLaunchDarkly}
    Copy File  ${STATIC_SUITES_999}/linuxep.json   ${tmpLaunchDarkly}

    Copy File  ${VUT_LAUNCH_DARKLY_999}/release.linuxep.ServerProtectionLinux-Base.json   ${tmpLaunchDarkly}
    Copy File  ${VUT_LAUNCH_DARKLY_999}/release.linuxep.ServerProtectionLinux-Plugin-AV.json   ${tmpLaunchDarkly}
    Copy File  ${VUT_LAUNCH_DARKLY_999}/release.linuxep.ServerProtectionLinux-Plugin-EDR.json  ${tmpLaunchDarkly}