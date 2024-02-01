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
Library    ${COMMON_TEST_LIBS}/IniUtils.py

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
Upgrade Resources SDDS3 Test Setup
    Require Uninstalled
    Register Cleanup  Check All Product Logs Do Not Contain Error
    Register Cleanup  Mark Expected Error In Log  ${SULDOWNLOADER_LOG_PATH}  suldownloader <> Failed to connect to repository:
    Register Cleanup  Mark Expected Error In Log  ${UPDATESCHEDULER_LOG_PATH}  updatescheduler <> Update Service (sophos-spl-update.service) failed.
    Register Cleanup  Mark Expected Error In Log  ${SOPHOS_INSTALL}/logs/base/watchdog.log  ProcessMonitoringImpl <> /opt/sophos-spl/base/bin/mcsrouter died with 1
    Register Cleanup  Mark Expected Error In Log  ${SOPHOS_INSTALL}/logs/base/watchdog.log  ProcessMonitoringImpl <> /opt/sophos-spl/base/bin/mcsrouter died with exit code 1

Upgrade Resources SDDS3 Test Teardown
    [Arguments]    ${installDir}=${SOPHOS_INSTALL}
    Run Teardown Functions
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
    Run Keyword If Test Failed    Dump Teardown Log    ${installDir}/base/update/var/package_config.xml
    Remove File  /tmp/preserve-sul-downgrade
    Stop Local Cloud Server
    Cleanup Local Warehouse And Thininstaller
    Require Uninstalled
    Cleanup Temporary Folders

Upgrade Resources Suite Setup
    Set Suite Variable    ${GL_handle}       ${EMPTY}
    Set Suite Variable    ${GL_UC_handle}    ${EMPTY}
    Setup_MCS_Cert_Override
    ${kernel_version_too_old_for_rtd} =    Check Kernel Version Is Older    5.3    aarch64
    Set Suite Variable    ${KERNEL_VERSION_TOO_OLD_FOR_RTD}    ${kernel_version_too_old_for_rtd}
    ${kernel_version_new_enough_for_rtd} =  invert boolean  ${KERNEL_VERSION_TOO_OLD_FOR_RTD}
    Set Suite Variable    ${KERNEL_VERSION_NEW_ENOUGH_FOR_RTD}    ${kernel_version_new_enough_for_rtd}

Cleanup Local Warehouse And Thininstaller
    Stop Proxy Servers
    Stop Update Server
    Cleanup Files

Check AV and Base Components Inside The ALC Status
    ${statusContent} =  Get File  /opt/sophos-spl/base/mcs/status/ALC_status.xml
    Should Contain  ${statusContent}  ServerProtectionLinux-Plugin-AV
    Should Contain  ${statusContent}  ServerProtectionLinux-Base

Log XDR Intermediary File
    Run Keyword And Ignore Error   Log File  ${SOPHOS_INSTALL}/plugins/edr/var/xdr_intermediary

Mark Known Upgrade Errors
    # TODO: LINUXDAR-7318 - expected till bugfix is in released version
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/logs/base/watchdog.log  /opt/sophos-spl/base/bin/UpdateScheduler died with signal 9

    # Something seems to trigger this in the shipping version as of 2024-01-10
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/logs/base/watchdog.log  /opt/sophos-spl/plugins/av/sbin/av died with signal 9

    #LINUXDAR-4015 There won't be a fix for this error, please check the ticket for more info
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/plugins/runtimedetections/log/runtimedetections.log  runtimedetections <> Could not enter supervised child process

    #TODO LINUXDAR-8575 Once RTD <-> MA comms are stable remove this error mark line.
    Mark Expected Error In Log    ${BASE_LOGS_DIR}/sophosspl/sophos_managementagent.log     managementagent <> Failure on sending message to runtimedetections. Reason: No incoming data on ZMQ socket from getReply in PluginProxy

Mark Known Downgrade Errors
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/logs/base/wdctl.log  wdctlActions <> Plugin "responseactions" not in registry
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/logs/base/wdctl.log  wdctlActions <> Plugin "liveresponse" not in registry
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/base/sophosspl/mcsrouter.log  mcsrouter.utils.plugin_registry <> Failed to load plugin file:
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/base/sophosspl/mcsrouter.log  mcsrouter.utils.plugin_registry <> [Errno 2] No such file or directory:

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
    ...  --certpath   ${COMMON_TEST_UTILS}/server_certs/server.crt
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


Check For downgraded logs
    # AV logs
    File Should Exist  ${SOPHOS_INSTALL}/plugins/av/log/downgrade-backup/av.log
    File Should Exist  ${SOPHOS_INSTALL}/plugins/av/log/downgrade-backup/soapd.log
    File Should Exist  ${SOPHOS_INSTALL}/plugins/av/log/downgrade-backup/sophos_threat_detector.log
    File Should Exist  ${SOPHOS_INSTALL}/plugins/av/log/downgrade-backup/safestore.log
    # Device Isolation logs
    # TODO: LINUXDAR-8348: Uncomment device isolation log file check following release
    # File Should Exist  ${SOPHOS_INSTALL}/plugins/deviceisolation/log/downgrade-backup/deviceisolation.log
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

Check Installed Plugin Is VUT Version
	[Arguments]  ${plugin_directory}  ${rigid_name}
	${contents} =  Get File  ${plugin_directory}/VERSION.ini
    ${plugin_vut_version} =  get_version_for_rigidname_in_sdds3_warehouse   ${VUT_WAREHOUSE_ROOT}    ${VUT_LAUNCH_DARKLY}    ${rigid_name}
    Should Contain   ${contents}   PRODUCT_VERSION = ${plugin_vut_version}

Check Installed Plugins Are VUT Versions
    Check Installed Plugin Is VUT Version  ${EDR_DIR}  ServerProtectionLinux-Plugin-EDR
    Check Installed Plugin Is VUT Version  ${LIVERESPONSE_DIR}  ServerProtectionLinux-Plugin-liveresponse
    Check Installed Plugin Is VUT Version  ${EVENTJOURNALER_DIR}  ServerProtectionLinux-Plugin-EventJournaler
    Check Installed Plugin Is VUT Version  ${RUNTIMEDETECTIONS_DIR}  ServerProtectionLinux-Plugin-RuntimeDetections
    Check Installed Plugin Is VUT Version  ${DEVICEISOLATION_DIR}  ServerProtectionLinux-Plugin-DeviceIsolation

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