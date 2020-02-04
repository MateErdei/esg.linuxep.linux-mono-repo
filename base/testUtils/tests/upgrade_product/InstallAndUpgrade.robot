*** Settings ***
Suite Setup      Suite Setup
Suite Teardown   Suite Teardown

Test Setup       Test Setup
Test Teardown    Test Teardown

Library     ${libs_directory}/WarehouseGenerator.py
Library     ${libs_directory}/ThinInstallerUtils.py
Library     ${libs_directory}/LogUtils.py
Library     ${libs_directory}/FullInstallerUtils.py
Library     ${libs_directory}/MCSRouter.py
Library     ${libs_directory}/UpdateSchedulerHelper.py
Library     ${libs_directory}/TemporaryDirectoryManager.py
Library     ${libs_directory}/OSUtils.py
Library     ${libs_directory}/WarehouseUtils.py
Library     ${libs_directory}/TeardownTools.py
Library     ${libs_directory}/UpgradeUtils.py
Library     Process
Library     OperatingSystem
Library     String

Resource    ../mcs_router/McsRouterResources.robot
Resource    ../mcs_router-nova/McsRouterNovaResources.robot
Resource    ../installer/InstallerResources.robot
Resource    ../thin_installer/ThinInstallerResources.robot
Resource    ../example_plugin/ExamplePluginResources.robot
Resource    ../mdr_plugin/MDRResources.robot
Resource    ../scheduler_update/SchedulerUpdateResources.robot
Resource    ../GeneralTeardownResource.robot
Resource    UpgradeResources.robot

*** Variables ***
${BaseAndMtrReleasePolicy}                  ${GeneratedWarehousePolicies}/base_and_mtr_VUT-1.xml
${BaseAndMtrVUTPolicy}                      ${GeneratedWarehousePolicies}/base_and_mtr_VUT.xml
${BaseAndMtrWithFakeLibs}                   ${GeneratedWarehousePolicies}/base_and_mtr_0_6_0.xml
${BaseAndEdrVUTPolicy}                      ${GeneratedWarehousePolicies}/base_and_edr_VUT.xml
${LocalWarehouseDir}                        ./local_warehouses
${Testpolicy}                               ${SUPPORT_FILES}/CentralXml/RealWarehousePolicies/GeneratedAlcPolicies/base_and_mtr_VUT.xml
${base_removed_files_manifest}              ${SOPHOS_INSTALL}/tmp/ServerProtectionLinux-Base/removedFiles_manifest.dat
${mtr_removed_files_manifest}               ${SOPHOS_INSTALL}/tmp/ServerProtectionLinux-Plugin-MDR/removedFiles_manifest.dat
${base_files_to_delete}                     ${SOPHOS_INSTALL}/base/update/cache/primary/ServerProtectionLinux-Base/filestodelete.dat
${mtr_files_to_delete}                      ${SOPHOS_INSTALL}/base/update/cache/primary/ServerProtectionLinux-Plugin-MDR/filestodelete.dat

*** Test Cases ***
We Can Upgrade From A Release To Master Without Unexpected Errors
    [Tags]  INSTALLER  THIN_INSTALLER  UNINSTALL  UPDATE_SCHEDULER  SULDOWNLOADER  OSTIA

    Start Local Cloud Server  --initial-alc-policy  ${BaseAndMtrReleasePolicy}

    Log File  /etc/hosts
    Configure And Run Thininstaller Using Real Warehouse Policy  0  ${BaseAndMtrReleasePolicy}  real=True
    Wait For Initial Update To Fail

    Send ALC Policy And Prepare For Upgrade  ${BaseAndMtrReleasePolicy}
    Trigger Update Now
    # waiting for 2nd because the 1st is a guaranteed failure
    Wait Until Keyword Succeeds
    ...   200 secs
    ...   10 secs
    ...   Check MCS Envelope Contains Event Success On N Event Sent  2

    Check EAP Release Installed Correctly
    ${BaseReleaseVersion} =     Get Version Number From Ini File   ${InstalledBaseVersionFile}
    ${MtrReleaseVersion} =      Get Version Number From Ini File   ${InstalledMDRPluginVersionFile}


    Send ALC Policy And Prepare For Upgrade  ${BaseAndMtrVUTPolicy}
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  2 secs
    ...  Check Policy Written Match File  ALC-1_policy.xml  ${BaseAndMtrVUTPolicy}

    Mark Watchdog Log
    Mark Managementagent Log

    Trigger Update Now
    Wait Until Keyword Succeeds
    ...   200 secs
    ...   10 secs
    ...   Check MCS Envelope Contains Event Success On N Event Sent  3

#     If mtr is installed for the first time, this will appear
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/logs/base/wdctl.log  wdctlActions <> Plugin "mtr" not in registry
    # If the policy comes down fast enough SophosMtr will not have started by the time mtr plugin is restarted
    # This is only an issue with versions of base before we started using boost process
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/plugins/mtr/log/mtr.log  ProcessImpl <> The PID -1 does not exist or is not a child of the calling process.

    Check for Management Agent Failing To Send Message To MTR And Check Recovery

    Check All Product Logs Do Not Contain Error
    Check All Product Logs Do Not Contain Critical

    Check Current Release Installed Correctly

    ${BaseDevVersion} =     Get Version Number From Ini File   ${InstalledBaseVersionFile}
    ${MtrDevVersion} =      Get Version Number From Ini File   ${InstalledMDRPluginVersionFile}

    Should Not Be Equal As Strings  ${BaseReleaseVersion}  ${BaseDevVersion}
    Should Not Be Equal As Strings  ${MtrReleaseVersion}  ${MtrDevVersion}

We Can Downgrade From Master To A Release Without Unexpected Errors
    [Tags]   INSTALLER  THIN_INSTALLER  UNINSTALL  UPDATE_SCHEDULER  SULDOWNLOADER  OSTIA

    Start Local Cloud Server  --initial-alc-policy  ${BaseAndMtrVUTPolicy}

    Log File  /etc/hosts
    Configure And Run Thininstaller Using Real Warehouse Policy  0  ${BaseAndMtrVUTPolicy}
    Wait For Initial Update To Fail


    Send ALC Policy And Prepare For Upgrade  ${BaseAndMtrVUTPolicy}
    Trigger Update Now
        # waiting for 2nd because the 1st is a guaranteed failure
        Wait Until Keyword Succeeds
        ...   200 secs
        ...   10 secs
        ...   Check MCS Envelope Contains Event Success On N Event Sent  2

    Check Current Release Installed Correctly
    ${BaseDevVersion} =     Set Variable   Get Version Number From Ini File   ${InstalledBaseVersionFile}
    ${MtrDevVersion} =      Set Variable   Get Version Number From Ini File   ${InstalledMDRPluginVersionFile}


    Send ALC Policy And Prepare For Upgrade  ${BaseAndMtrReleasePolicy}
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  2 secs
    ...  Check Policy Written Match File  ALC-1_policy.xml  ${BaseAndMtrReleasePolicy}

    Mark Watchdog Log
    Mark Managementagent Log

    Trigger Update Now


    #TODO LINUXDAR-1196 uncomment when we next release
#    Wait Until Keyword Succeeds
#    ...   200 secs
#    ...   10 secs
#    ...   Check MCS Envelope Contains Event Success On N Event Sent  3

    #TODO LINUXDAR-1196 remove when we next release
    Wait Until Keyword Succeeds
    ...   200 secs
    ...   10 secs
    ...   Check MCS Envelope Contains Event On N Event Sent  3

    # If mtr is installed for the first time, this will appear
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/logs/base/wdctl.log  wdctlActions <> Plugin "mtr" not in registry
    # We have a race condition when stopping and starting mtr plugin, so sometimes this appears
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/logs/base/watchdog.log  ProcessMonitoringImpl <> /opt/sophos-spl/plugins/mtr/bin/mtr exited when not expected
    # If the policy comes down fast enough SophosMtr will not have started by the time mtr plugin is restarted
    # This is only an issue with versions of base before we started using boost process
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/plugins/mtr/log/mtr.log  ProcessImpl <> The PID -1 does not exist or is not a child of the calling process.


    #TODO LINUXDAR-1196 remove when we next release
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/plugins/mtr/log/mtr.log      mtr <> Plugin Api could not be instantiated: No incoming data
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/logs/base/watchdog.log       ProcessMonitoringImpl <> /opt/sophos-spl/plugins/mtr/bin/mtr died with 43
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/logs/base/suldownloader.log  suldownloaderdata <> Installation failed
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/logs/base/wdctl.log          Failed to start mtr: Timeout out connecting to watchdog: No incoming data

    Check for Management Agent Failing To Send Message To MTR And Check Recovery

    Check All Product Logs Do Not Contain Error
    Check All Product Logs Do Not Contain Critical

    Check EAP Release Installed Correctly

    ${BaseReleaseVersion} =     Get Version Number From Ini File   ${InstalledBaseVersionFile}
    ${MtrReleaseVersion} =      Get Version Number From Ini File   ${InstalledMDRPluginVersionFile}

    Should Not Be Equal As Strings  ${BaseReleaseVersion}  ${BaseDevVersion}
    Should Not Be Equal As Strings  ${MtrReleaseVersion}  ${MtrDevVersion}

Verify Upgrading Will Remove Files Which Are No Longer Required
    [Tags]      INSTALLER  UPDATE_SCHEDULER  SULDOWNLOADER  OSTIA
    [Timeout]   10 minutes

    Start Local Cloud Server  --initial-alc-policy  ${BaseAndMtrWithFakeLibs}

    Should Not Exist    ${SOPHOS_INSTALL}

    Log File  /etc/hosts
    Configure And Run Thininstaller Using Real Warehouse Policy  0  ${BaseAndMtrWithFakeLibs}  real=True
    Wait For Initial Update To Fail

    Send ALC Policy And Prepare For Upgrade  ${BaseAndMtrWithFakeLibs}
    Trigger Update Now
    # waiting for 2nd because the 1st is a guaranteed failure
    Wait Until Keyword Succeeds
    ...   200 secs
    ...   10 secs
    ...   Check MCS Envelope Contains Event Success On N Event Sent  2

    Check Files Before Upgrade

    Send ALC Policy And Prepare For Upgrade  ${BaseAndMtrVUTPolicy}
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  2 secs
    ...  Check Policy Written Match File  ALC-1_policy.xml  ${BaseAndMtrVUTPolicy}

    Trigger Update Now

    Wait Until Keyword Succeeds
    ...  320 secs
    ...  5 secs
    ...  Check Files After Upgrade



Verify Upgrading Will Not Remove Files Which Are Outside Of The Product Realm
    [Tags]      INSTALLER  UPDATE_SCHEDULER  SULDOWNLOADER  OSTIA
    [Timeout]   10 minutes

    Start Local Cloud Server  --initial-alc-policy  ${BaseAndMtrWithFakeLibs}

    Should Not Exist    ${SOPHOS_INSTALL}

    Log File  /etc/hosts
    Configure And Run Thininstaller Using Real Warehouse Policy  0  ${BaseAndMtrWithFakeLibs}  real=True
    Wait For Initial Update To Fail

    Send ALC Policy And Prepare For Upgrade  ${BaseAndMtrWithFakeLibs}
    Trigger Update Now
    # waiting for 2nd because the 1st is a guaranteed failure
    Wait Until Keyword Succeeds
    ...   200 secs
    ...   10 secs
    ...   Check MCS Envelope Contains Event Success On N Event Sent  2

    Send ALC Policy And Prepare For Upgrade  ${BaseAndMtrVUTPolicy}
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  2 secs
    ...  Check Policy Written Match File  ALC-1_policy.xml  ${BaseAndMtrVUTPolicy}

    # Swap old manifest files around, this will make the cleanup process mark files for delete which should not be
    # deleted, because the files are outside of the components realm

    Move File   ${SOPHOS_INSTALL}/base/update/ServerProtectionLinux-Base/manifest.dat  /tmp/base-manifest.dat
    Move File  ${SOPHOS_INSTALL}/base/update/ServerProtectionLinux-Plugin-MDR/manifest.dat  /tmp/MDR-manifest.dat

    Move File  /tmp/MDR-manifest.dat    ${SOPHOS_INSTALL}/base/update/ServerProtectionLinux-Base/manifest.dat
    Move File  /tmp/base-manifest.dat   ${SOPHOS_INSTALL}/base/update/ServerProtectionLinux-Plugin-MDR/manifest.dat

    Trigger Update Now

    Wait Until Keyword Succeeds
    ...   200 secs
    ...   10 secs
    ...   Check MCS Envelope Contains Event Success On N Event Sent  3

    # ensure that the list of files to remove contains files which are outside of the components realm
    ${BASE_REMOVE_FILE_CONTENT} =  Get File  ${SOPHOS_INSTALL}/tmp/ServerProtectionLinux-Base/removedFiles_manifest.dat
    Should Contain  ${BASE_REMOVE_FILE_CONTENT}  plugins/mtr

    ${MTR_REMOVE_FILE_CONTENT} =  Get File  ${SOPHOS_INSTALL}/tmp/ServerProtectionLinux-Plugin-MDR/removedFiles_manifest.dat
    Should Contain  ${MTR_REMOVE_FILE_CONTENT}  base

    # ensure that the cleanup process is prevented from deleting files which are not stored in the component realm
    # note files listed in the components filestodelete.dat file, will be deleted by that compoent, so these files need to be
    # filtered out.

    Check Files Have Not Been Removed  ${SOPHOS_INSTALL}  ${base_removed_files_manifest}  plugins/mtr  ${mtr_files_to_delete}
    Check Files Have Not Been Removed  ${SOPHOS_INSTALL}  ${mtr_removed_files_manifest}  base   ${base_files_to_delete}


Version Copy Versions All Changed Files When Upgrading
    [Tags]      INSTALLER  THIN_INSTALLER  UNINSTALL  UPDATE_SCHEDULER  SULDOWNLOADER  OSTIA
    [Documentation]  LINUXDAR-771 - check versioned copy works as expected.

    Start Local Cloud Server  --initial-alc-policy  ${BaseAndMtrReleasePolicy}

    Log File  /etc/hosts

    # Wrapped in a wait to keep trying for a bit if ostia is intermittent
    Wait Until Keyword Succeeds
    ...  15 mins
    ...  10 secs
    ...  Configure And Run Thininstaller Using Real Warehouse Policy  0  ${BaseAndMtrReleasePolicy}  real=True

    Wait For Initial Update To Fail
    Send ALC Policy And Prepare For Upgrade  ${BaseAndMtrReleasePolicy}
    Trigger Update Now
    # waiting for 2nd because the 1st is a guaranteed failure
    Wait Until Keyword Succeeds
    ...   200 secs
    ...   5 secs
    ...   Check MCS Envelope Contains Event Success On N Event Sent  2

    Check EAP Release Installed Correctly
    ${BaseReleaseVersion}=  Get Version Number From Ini File   ${InstalledBaseVersionFile}
    ${MtrReleaseVersion}=  Get Version Number From Ini File   ${InstalledMDRPluginVersionFile}

    ${BaseManifestPath}=  Set Variable  ${SOPHOS_INSTALL}/base/update/ServerProtectionLinux-Base/manifest.dat
    ${MTRPluginManifestPath}=  Set Variable  ${SOPHOS_INSTALL}/base/update/ServerProtectionLinux-Plugin-MDR/manifest.dat

    ${BeforeManifestBase}=  Get File  ${BaseManifestPath}
    ${BeforeManifestPluginMdr}=  Get File  ${MTRPluginManifestPath}

    Send ALC Policy And Prepare For Upgrade  ${BaseAndMtrVUTPolicy}
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  2 secs
    ...  Check Policy Written Match File  ALC-1_policy.xml  ${BaseAndMtrVUTPolicy}

    Mark Watchdog Log
    Mark Managementagent Log

    Trigger Update Now
    Wait Until Keyword Succeeds
    ...   200 secs
    ...   10 secs
    ...   Check MCS Envelope Contains Event Success On N Event Sent  3

    # If mtr is installed for the first time, this will appear
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/logs/base/wdctl.log  wdctlActions <> Plugin "mtr" not in registry
    # If the policy comes down fast enough SophosMtr will not have started by the time mtr plugin is restarted
    # This is only an issue with versions of base before we started using boost process
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/plugins/mtr/log/mtr.log  ProcessImpl <> The PID -1 does not exist or is not a child of the calling process.

    Check for Management Agent Failing To Send Message To MTR And Check Recovery

    Check All Product Logs Do Not Contain Error
    Check All Product Logs Do Not Contain Critical

    Check Current Release Installed Correctly

    ${BaseDevVersion} =  Get Version Number From Ini File   ${InstalledBaseVersionFile}
    ${MtrDevVersion} =  Get Version Number From Ini File   ${InstalledMDRPluginVersionFile}
    Should Not Be Equal As Strings  ${BaseReleaseVersion}  ${BaseDevVersion}
    Should Not Be Equal As Strings  ${MtrReleaseVersion}  ${MtrDevVersion}

    Check Files In Versioned Copy Manifests Have Correct Symlink Versioning

    ${AfterManifestBase}=  Get File  ${BaseManifestPath}
    ${AfterManifestPluginMdr}=  Get File  ${MTRPluginManifestPath}

    ${ChangedBase}=  Get File  ${SOPHOS_INSTALL}/tmp/ServerProtectionLinux-Base/changedFiles_manifest.dat
    ${AddedBase}=  Get File  ${SOPHOS_INSTALL}/tmp/ServerProtectionLinux-Base/addedFiles_manifest.dat
    ${combinedBaseChanges}=  Catenate  SEPARATOR=\n  ${ChangedBase}  ${AddedBase}
    ${ChangedPluginMdr}=  Get File  ${SOPHOS_INSTALL}/tmp/ServerProtectionLinux-Plugin-MDR/changedFiles_manifest.dat
    ${AddedPluginMdr}=  Get File  ${SOPHOS_INSTALL}/tmp/ServerProtectionLinux-Plugin-MDR/addedFiles_manifest.dat
    ${combinedPluginMdrChanges}=  Catenate  SEPARATOR=\n  ${ChangedPluginMdr}  ${AddedPluginMdr}

    Compare Before And After Manifests With Changed Files Manifest  ${BeforeManifestBase}       ${AfterManifestBase}        ${combinedBaseChanges}
    Compare Before And After Manifests With Changed Files Manifest  ${BeforeManifestPluginMdr}  ${AfterManifestPluginMdr}   ${combinedPluginMdrChanges}


Install Base And EDR From Ostia
    [Tags]  INSTALLER  THIN_INSTALLER  UNINSTALL  UPDATE_SCHEDULER  SULDOWNLOADER  OSTIA  EDR_PLUGIN
    Start Local Cloud Server  --initial-alc-policy  ${BaseAndEdrVUTPolicy}
    Log File  /etc/hosts
    Configure And Run Thininstaller Using Real Warehouse Policy  0  ${BaseAndEdrVUTPolicy}
    # Initial update fails beacuse installing with thinstaller the certs arent setup correctly registering with fake cloud
    Wait For Initial Update To Fail

    Override LogConf File as Global Level  DEBUG

    Send ALC Policy And Prepare For Upgrade  ${BaseAndEdrVUTPolicy}
    Trigger Update Now

    # Waiting for 2nd because the 1st is a guaranteed failure
    Wait Until Keyword Succeeds
    ...   200 secs
    ...   10 secs
    ...   Check MCS Envelope Contains Event Success On N Event Sent  2

    Should Exist  ${EDR_DIR}
    Wait Until EDR Running
    Wait Until OSQuery Running

    #Ensure MTR has not been installed.
    Check SophosMTR Executable Not Running
    Should Not Exist  ${MTR_DIR}


Install Base And MTR Then Migrate To EDR
    [Tags]   INSTALLER  THIN_INSTALLER  UNINSTALL  UPDATE_SCHEDULER  SULDOWNLOADER  OSTIA   EDR_PLUGIN
    Start Local Cloud Server  --initial-alc-policy  ${BaseAndMtrReleasePolicy}
    Log File  /etc/hosts
    Configure And Run Thininstaller Using Real Warehouse Policy  0  ${BaseAndMtrReleasePolicy}  real=True
    Wait For Initial Update To Fail

    # Install MTR
    Send ALC Policy And Prepare For Upgrade  ${BaseAndMtrReleasePolicy}
    Trigger Update Now
    Wait Until Keyword Succeeds
    ...  200 secs
    ...  5 secs
    ...  Should Exist  ${MTR_DIR}

    Check EDR Executable Not Running
    Should Not Exist  ${EDR_DIR}

    # Install EDR
    Send ALC Policy And Prepare For Upgrade  ${BaseAndEdrVUTPolicy}
    Trigger Update Now
    Wait Until Keyword Succeeds
    ...  200 secs
    ...  5 secs
    ...  should exist  ${EDR_DIR}

    Wait Until EDR Running
    Wait Until OSQuery Running
    Check SophosMTR Executable Not Running
    # MTR is uninstalled after EDR is installed
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  5 secs
    ...  Should Not Exist  ${MTR_DIR}


EDR Disables Auditd After Install With Auditd Running Default Behaviour
    [Tags]   INSTALLER  THIN_INSTALLER  UNINSTALL  UPDATE_SCHEDULER  SULDOWNLOADER  OSTIA  EDR_PLUGIN

    Install And Enable AuditD If Required

    Check AuditD Executable Running

    Start Local Cloud Server  --initial-alc-policy  ${BaseAndEdrVUTPolicy}
    Log File  /etc/hosts
    Configure And Run Thininstaller Using Real Warehouse Policy  0  ${BaseAndEdrVUTPolicy}
    Wait For Initial Update To Fail

    Override LogConf File as Global Level  DEBUG

    Send ALC Policy And Prepare For Upgrade  ${BaseAndEdrVUTPolicy}
    Trigger Update Now

    # Waiting for 2nd because the 1st is a guaranteed failure
    Wait Until Keyword Succeeds
    ...   200 secs
    ...   10 secs
    ...   Check MCS Envelope Contains Event Success On N Event Sent  2

    Should Exist  ${EDR_DIR}
    Wait Until EDR Running
    Wait Until OSQuery Running

    ${EDR_CONFIG_CONTENT}=  Get File  ${EDR_DIR}/etc/plugin.conf
    Should Contain  ${EDR_CONFIG_CONTENT}   disable_auditd=1

    Check AuditD Executable Not Running
    Check AuditD Service Disabled
    Check EDR Log Shows AuditD Has Been Disabled


EDR Disables Auditd After Install With Auditd Running With Disable Flag
    [Tags]  INSTALLER  THIN_INSTALLER  UNINSTALL  UPDATE_SCHEDULER  SULDOWNLOADER  OSTIA  EDR_PLUGIN

    Install And Enable AuditD If Required
    Check AuditD Executable Running

    Start Local Cloud Server  --initial-alc-policy  ${BaseAndEdrVUTPolicy}
    Log File  /etc/hosts
    Configure And Run Thininstaller Using Real Warehouse Policy  0  ${BaseAndEdrVUTPolicy}  args=--disable-auditd
    Wait For Initial Update To Fail

    Override LogConf File as Global Level  DEBUG

    Send ALC Policy And Prepare For Upgrade  ${BaseAndEdrVUTPolicy}
    Trigger Update Now

    # Waiting for 2nd because the 1st is a guaranteed failure
    Wait Until Keyword Succeeds
    ...   200 secs
    ...   10 secs
    ...   Check MCS Envelope Contains Event Success On N Event Sent  2

    Should Exist  ${EDR_DIR}
    Wait Until EDR Running
    Wait Until OSQuery Running

    ${INSTALL_OPTIONS_CONTENT}=  Get File  ${SOPHOS_INSTALL}/base/etc/install_options
    Should Contain  ${INSTALL_OPTIONS_CONTENT}   --disable-auditd
    ${EDR_CONFIG_CONTENT}=  Get File  ${EDR_DIR}/etc/plugin.conf
    Should Contain  ${EDR_CONFIG_CONTENT}   disable_auditd=1

    Check EDR Log Shows AuditD Has Been Disabled
    Check AuditD Executable Not Running

EDR Does Not Disable Auditd After Install With Auditd Running With Do Not Disable Flag
    [Tags]  INSTALLER  THIN_INSTALLER  UNINSTALL  UPDATE_SCHEDULER  SULDOWNLOADER  OSTIA  EDR_PLUGIN

    Install And Enable AuditD If Required
    Check AuditD Executable Running

    Start Local Cloud Server  --initial-alc-policy  ${BaseAndEdrVUTPolicy}
    Log File  /etc/hosts
    Configure And Run Thininstaller Using Real Warehouse Policy  0  ${BaseAndEdrVUTPolicy}  args=--do-not-disable-auditd
    Wait For Initial Update To Fail

    Override LogConf File as Global Level  DEBUG

    Send ALC Policy And Prepare For Upgrade  ${BaseAndEdrVUTPolicy}
    Trigger Update Now

    # Waiting for 2nd because the 1st is a guaranteed failure
    Wait Until Keyword Succeeds
    ...   200 secs
    ...   10 secs
    ...   Check MCS Envelope Contains Event Success On N Event Sent  2

    Should Exist  ${EDR_DIR}
    Wait Until EDR Running
    Wait Until OSQuery Running

    ${INSTALL_OPTIONS_CONTENT}=  Get File  ${SOPHOS_INSTALL}/base/etc/install_options
    Should Contain  ${INSTALL_OPTIONS_CONTENT}   --do-not-disable-auditd
    ${EDR_CONFIG_CONTENT}=  Get File  ${EDR_DIR}/etc/plugin.conf
    Should Contain  ${EDR_CONFIG_CONTENT}   disable_auditd=0

    Check EDR Log Shows AuditD Has Not Been Disabled
    Check AuditD Executable Running


EDR Does Disable Auditd After Manual Change To Config
    [Tags]  INSTALLER  THIN_INSTALLER  UNINSTALL  UPDATE_SCHEDULER  SULDOWNLOADER  OSTIA  EDR_PLUGIN

    Install And Enable AuditD If Required
    Check AuditD Executable Running

    Start Local Cloud Server  --initial-alc-policy  ${BaseAndEdrVUTPolicy}
    Log File  /etc/hosts
    Configure And Run Thininstaller Using Real Warehouse Policy  0  ${BaseAndEdrVUTPolicy}  args=--do-not-disable-auditd
    Wait For Initial Update To Fail

    Override LogConf File as Global Level  DEBUG

    Send ALC Policy And Prepare For Upgrade  ${BaseAndEdrVUTPolicy}
    Trigger Update Now

    # Waiting for 2nd because the 1st is a guaranteed failure
    Wait Until Keyword Succeeds
    ...   200 secs
    ...   10 secs
    ...   Check MCS Envelope Contains Event Success On N Event Sent  2

    Should Exist  ${EDR_DIR}
    Wait Until EDR Running
    Wait Until OSQuery Running

    ${INSTALL_OPTIONS_CONTENT}=  Get File  ${SOPHOS_INSTALL}/base/etc/install_options
    Should Contain  ${INSTALL_OPTIONS_CONTENT}   --do-not-disable-auditd
    ${EDR_CONFIG_CONTENT}=  Get File  ${EDR_DIR}/etc/plugin.conf
    Should Contain  ${EDR_CONFIG_CONTENT}   disable_auditd=0
    Check EDR Log Shows AuditD Has Not Been Disabled
    Check AuditD Executable Running

    Run Shell Process  echo disable_auditd=1 > ${EDR_DIR}/etc/plugin.conf   OnError=failed to set EDR config to disable auditd
    Run Shell Process  systemctl restart sophos-spl   OnError=failed to restart sophos-spl

    ${EDR_CONFIG_CONTENT}=  Get File  ${EDR_DIR}/etc/plugin.conf
    Should Contain  ${EDR_CONFIG_CONTENT}   disable_auditd=1

     Wait Until Keyword Succeeds
    ...   10 secs
    ...   1 secs
    ...   Check EDR Log Shows AuditD Has Been Disabled

    Check AuditD Executable Not Running


Thin Installer Creates Options File With Many Args
    [Tags]  INSTALLER  THIN_INSTALLER  UNINSTALL  UPDATE_SCHEDULER  SULDOWNLOADER  OSTIA

    Start Local Cloud Server  --initial-alc-policy  ${BaseAndEdrVUTPolicy}
    Configure And Run Thininstaller Using Real Warehouse Policy  0  ${BaseAndEdrVUTPolicy}  args=--a --b --c

    ${options}=  Get File  ${SOPHOS_INSTALL}/base/etc/install_options
    Should Contain  ${options}  --a
    Should Contain  ${options}  --b
    Should Contain  ${options}  --c


*** Keywords ***
Check AuditD Executable Running
    ${result} =    Run Process  pgrep  ^auditd
    Should Be Equal As Integers    ${result.rc}    0       msg="stdout:${result.stdout}\n err: ${result.stderr}"

Check AuditD Executable Not Running
    ${result} =    Run Process  pgrep  ^auditd
    Should Not Be Equal As Integers    ${result.rc}    0     msg="stdout:${result.stdout}\n err: ${result.stderr}"

Check AuditD Service Disabled
    ${result} =    Run Process  systemctl  is-enabled  auditd
    log  ${result.stdout}
    log  ${result.stderr}
    log  ${result.rc}
    Should Not Be Equal As Integers    ${result.rc}    0

Check EDR Log Shows AuditD Has Been Disabled
    ${EDR_LOG_CONTENT}=  Get File  ${EDR_DIR}/log/edr.log
    Should Contain  ${EDR_LOG_CONTENT}   EDR configuration set to disable AuditD
    Should Contain  ${EDR_LOG_CONTENT}   Successfully stopped service: auditd
    Should Contain  ${EDR_LOG_CONTENT}   Successfully disabled service: auditd

Check EDR Log Shows AuditD Has Not Been Disabled
    ${EDR_LOG_CONTENT}=  Get File  ${EDR_DIR}/log/edr.log
    Should Not Contain  ${EDR_LOG_CONTENT}   Successfully disabled service: auditd
    Should Contain  ${EDR_LOG_CONTENT}   EDR configuration set to not disable AuditD
    Should Contain  ${EDR_LOG_CONTENT}   AuditD is running, it will not be possible to obtain event data.

Check for Management Agent Failing To Send Message To MTR And Check Recovery
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/logs/base/sophosspl/sophos_managementagent.log  managementagent <> Failure on sending message to mtr. Reason: No incoming data
    ${EvaluationBool} =  Does Management Agent Log Contain    managementagent <> Failure on sending message to mtr. Reason: No incoming data
    Run Keyword If
    ...  ${EvaluationBool} == ${True}
    ...  Check For MTR Recovery

Check For MTR Recovery
    Remove File  ${MDR_PLUGIN_PATH}/var/policy/mtr.xml
    Check MDR Plugin Running
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  5 secs
    ...  Check Marked Watchdog Log Contains   watchdog <> Starting mtr
    Wait Until Keyword Succeeds
    ...  5 secs
    ...  1 secs
    ...  Check Marked Managementagent Log Contains   managementagent <> Policy ${SOPHOS_INSTALL}/base/mcs/policy/MDR_policy.xml applied to 1 plugins
    Check File Exists  ${MDR_PLUGIN_PATH}/var/policy/mtr.xml

Replace Versig Update Cert With Eng Certificate
    Copy File   ${SUPPORT_FILES}/sophos_certs/ps_rootca.crt  ${SOPHOS_INSTALL}/base/update/certs/ps_rootca.crt


Check Installed Version Files Against Expected
    [Arguments]  ${WarehousePath}
    ${WH_BASE} =  Get File  ${WarehousePath}/${WarehouseBaseVersionFileExtension}
    ${WH_MDR_PLUGIN} =  Get File  ${WarehousePath}/${WarehouseMDRPluginVersionFileExtension}
    ${WH_MDR_SUITE} =  Get File  ${WarehousePath}/${WarehouseMDRSuiteVersionFileExtension}

    ${INSTALLED_BASE} =  Get File  ${InstalledBaseVersionFile}
    ${INSTALLED_MDR_PLUGIN} =  Get File  ${InstalledMDRPluginVersionFile}
    ${INSTALLED_MDR_SUITE} =  Get File  ${InstalledMDRSuiteVersionFile}

    Should Be Equal As Strings  ${WH_BASE}  ${INSTALLED_BASE}
    Should Be Equal As Strings  ${WH_MDR_PLUGIN}  ${INSTALLED_MDR_PLUGIN}
    Should Be Equal As Strings  ${WH_MDR_SUITE}  ${INSTALLED_MDR_SUITE}

Redirect Sulconfig To Use Local Warehouse
    Replace Sophos URLS to Localhost  config_path=${SulConfigPath}
    Replace Username And Password In Sulconfig  config_path=${SulConfigPath}

Setup Warehouse For Latest Release
    Stop Update Server
    ${LocalWarehouse} =  Add Temporary Directory  ReleaseWarehouse

    ${PathToBase} =  Get Sspl Base Sdds Version 1
#    ${PathToBase} =  Get Folder With Installer
    ${PathToMDRSuite} =  Get Sspl Mdr Component Suite 1

    Setup Warehouse With Args  ${LocalWarehouse}  ${PathToBase}  ${PathToMDRSuite}
    [Return]  ${LocalWarehouse}

Setup Warehouse For 0 5 Release
    Stop Update Server
    ${LocalWarehouse} =  Add Temporary Directory  Warehouse05

    ${PathToBaseRelease_0_5} =  Get Sspl Base Sdds Version 0 5
    Remove Directory  ${LocalWarehouse}/TestInstallFiles/${BASE_RIGID_NAME}  recursive=${TRUE}
    Copy Directory     ${PathToBaseRelease_0_5}  ${LocalWarehouse}/TestInstallFiles/${BASE_RIGID_NAME}
    ${PathToExamplePluginRelease_0_5} =  Get Sspl Example Plugin Sdds Version 0 5
    Remove Directory  ${LocalWarehouse}/TestInstallFiles/${EXAMPLE_PLUGIN_RIGID_NAME}  recursive=${TRUE}
    Copy Directory     ${PathToExamplePluginRelease_0_5}  ${LocalWarehouse}/TestInstallFiles/${EXAMPLE_PLUGIN_RIGID_NAME}

    Clear Warehouse Config
    Add Component Warehouse Config   ${BASE_RIGID_NAME}   ${LocalWarehouse}/TestInstallFiles/    ${LocalWarehouse}/temp_warehouse/   ${BASE_RIGID_NAME}  ${BASE_RIGID_NAME}
    Add Component Warehouse Config   ${EXAMPLE_PLUGIN_RIGID_NAME}  ${LocalWarehouse}/TestInstallFiles/    ${LocalWarehouse}/temp_warehouse/   ${EXAMPLE_PLUGIN_RIGID_NAME}  ${BASE_RIGID_NAME}

    Generate Warehouse

    Start Update Server    1233    ${LocalWarehouse}/temp_warehouse/customer_files/
    Start Update Server    1234    ${LocalWarehouse}/temp_warehouse/warehouses/sophosmain/
    Can Curl Url    https://localhost:1234/catalogue/sdds.live.xml
    Can Curl Url    https://localhost:1233

Setup Warehouse For Master
    Stop Update Server
    ${LocalWarehouse} =  Add Temporary Directory  MasterWarehouse
    ${PathToBase} =  Get Folder With Installer
    ${PathToMDRSuite} =  Get SSPL MDR Component Suite

    Setup Warehouse With Args  ${LocalWarehouse}  ${PathToBase}  ${PathToMDRSuite}
    [Return]  ${LocalWarehouse}

Setup Warehouse With Args
    [Arguments]  ${warehousedir}  ${PathToBase}  ${PathToMDRSuite}
    Clear Warehouse Config

    Remove Directory  ${warehousedir}/TestInstallFiles/${BASE_RIGID_NAME}  recursive=${TRUE}
    Copy Directory     ${PathToBase}  ${warehousedir}/TestInstallFiles/${BASE_RIGID_NAME}
    Copy File   ${SUPPORT_FILES}/sophos_certs/rootca.crt  ${warehousedir}/TestInstallFiles/${BASE_RIGID_NAME}/files/base/update/certs/rootca.crt
    Copy File   ${SUPPORT_FILES}/sophos_certs/ps_rootca.crt  ${warehousedir}/TestInstallFiles/${BASE_RIGID_NAME}/files/base/update/certs/ps_rootca.crt

    Copy MDR Component Suite To   ${warehousedir}/TestInstallFiles   mdr_component_suite=${PathToMDRSuite}

    Clear Warehouse Config
    Add Component Warehouse Config   ${BASE_RIGID_NAME}   ${warehousedir}/TestInstallFiles/    ${warehousedir}/temp_warehouse/   ${BASE_RIGID_NAME}  Warehouse1
    Add Component Suite Warehouse Config   ${PathToMDRSuite.mdr_suite.rigid_name}  ${warehousedir}/TestInstallFiles/    ${warehousedir}/temp_warehouse/   Warehouse1
    Add Component Warehouse Config   ${PathToMDRSuite.mdr_plugin.rigid_name}  ${warehousedir}/TestInstallFiles/    ${warehousedir}/temp_warehouse/  ${PathToMDRSuite.mdr_suite.rigid_name}  Warehouse1
    Add Component Warehouse Config   ${PathToMDRSuite.dbos.rigid_name}  ${warehousedir}/TestInstallFiles/    ${warehousedir}/temp_warehouse/  ${PathToMDRSuite.mdr_suite.rigid_name}  Warehouse1
    Add Component Warehouse Config   ${PathToMDRSuite.osquery.rigid_name}  ${warehousedir}/TestInstallFiles/    ${warehousedir}/temp_warehouse/  ${PathToMDRSuite.mdr_suite.rigid_name}  Warehouse1

    Generate Warehouse

    Start Update Server    1233    ${warehousedir}/temp_warehouse/customer_files/
    Start Update Server    1234    ${warehousedir}/temp_warehouse/warehouses/sophosmain/
    Can Curl Url    https://localhost:1234/catalogue/sdds.live.xml
    Can Curl Url    https://localhost:1233

Check Old MCS Router Running
    ${pid} =  Check MCS Router Process Running  require_running=True
    [return]  ${pid}

Check Current Release Installed Correctly
    Check Mcs Router Running
    Check MDR Plugin Installed
    Check Installed Correctly

Check EAP Release Installed Correctly
    Check MCS Router Running
    Check MDR Plugin Installed
    Check Installed Correctly

Check Installed Correctly
    Should Exist    ${SOPHOS_INSTALL}

    Check Expected Base Processes Are Running
    Check Correct MCS Password And ID For Local Cloud Saved

    ${result}=  Run Process  stat  -c  "%A"  /opt
    ${ExpectedPerms}=  Set Variable  "drwxr-xr-x"
    Should Be Equal As Strings  ${result.stdout}  ${ExpectedPerms}

Check Files Before Upgrade
    # This is a selection of files from Base product, based on the version initialy installed
    File Should Exist   ${SOPHOS_INSTALL}/base/lib64/also_a_fake_lib.so
    File Should Exist   ${SOPHOS_INSTALL}/base/lib64/also_a_fake_lib.so.5
    File Should Exist   ${SOPHOS_INSTALL}/base/lib64/also_a_fake_lib.so.5.86
    File Should Exist   ${SOPHOS_INSTALL}/base/lib64/also_a_fake_lib.so.5.86.999
    File Should Exist   ${SOPHOS_INSTALL}/base/lib64/also_a_fake_lib.so.5.86.999.0
    File Should Exist   ${SOPHOS_INSTALL}/base/lib64/fake_lib.so
    File Should Exist   ${SOPHOS_INSTALL}/base/lib64/fake_lib.so.1
    File Should Exist   ${SOPHOS_INSTALL}/base/lib64/fake_lib.so.1.66
    File Should Exist   ${SOPHOS_INSTALL}/base/lib64/fake_lib.so.1.66.999
    File Should Exist   ${SOPHOS_INSTALL}/base/lib64/fake_lib.so.1.66.999.0
    File Should Exist   ${SOPHOS_INSTALL}/base/lib64/faker_lib.so
    File Should Exist   ${SOPHOS_INSTALL}/base/lib64/faker_lib.so.2
    File Should Exist   ${SOPHOS_INSTALL}/base/lib64/faker_lib.so.2.23
    File Should Exist   ${SOPHOS_INSTALL}/base/lib64/faker_lib.so.2.23.999
    File Should Exist   ${SOPHOS_INSTALL}/base/lib64/faker_lib.so.2.23.999.0

    File Should Exist   ${SOPHOS_INSTALL}/plugins/mtr/lib64/also_a_fake_lib.so
    File Should Exist   ${SOPHOS_INSTALL}/plugins/mtr/lib64/also_a_fake_lib.so.5
    File Should Exist   ${SOPHOS_INSTALL}/plugins/mtr/lib64/also_a_fake_lib.so.5.86
    File Should Exist   ${SOPHOS_INSTALL}/plugins/mtr/lib64/also_a_fake_lib.so.5.86.999
    File Should Exist   ${SOPHOS_INSTALL}/plugins/mtr/lib64/also_a_fake_lib.so.5.86.999.0
    File Should Exist   ${SOPHOS_INSTALL}/plugins/mtr/lib64/fake_lib.so
    File Should Exist   ${SOPHOS_INSTALL}/plugins/mtr/lib64/fake_lib.so.1
    File Should Exist   ${SOPHOS_INSTALL}/plugins/mtr/lib64/fake_lib.so.1.66
    File Should Exist   ${SOPHOS_INSTALL}/plugins/mtr/lib64/fake_lib.so.1.66.999
    File Should Exist   ${SOPHOS_INSTALL}/plugins/mtr/lib64/fake_lib.so.1.66.999.0
    File Should Exist   ${SOPHOS_INSTALL}/plugins/mtr/lib64/faker_lib.so
    File Should Exist   ${SOPHOS_INSTALL}/plugins/mtr/lib64/faker_lib.so.2
    File Should Exist   ${SOPHOS_INSTALL}/plugins/mtr/lib64/faker_lib.so.2.23
    File Should Exist   ${SOPHOS_INSTALL}/plugins/mtr/lib64/faker_lib.so.2.23.999
    File Should Exist   ${SOPHOS_INSTALL}/plugins/mtr/lib64/faker_lib.so.2.23.999.0

Check Files After Upgrade
    # This is a selection of removed files from Base product, based on the version initialy installed
    File Should Not Exist   ${SOPHOS_INSTALL}/base/lib64/also_a_fake_lib.so
    File Should Not Exist   ${SOPHOS_INSTALL}/base/lib64/also_a_fake_lib.so.5
    File Should Not Exist   ${SOPHOS_INSTALL}/base/lib64/also_a_fake_lib.so.5.86
    File Should Not Exist   ${SOPHOS_INSTALL}/base/lib64/also_a_fake_lib.so.5.86.999
    File Should Not Exist   ${SOPHOS_INSTALL}/base/lib64/also_a_fake_lib.so.5.86.999.0
    File Should Not Exist   ${SOPHOS_INSTALL}/base/lib64/fake_lib.so
    File Should Not Exist   ${SOPHOS_INSTALL}/base/lib64/fake_lib.so.1
    File Should Not Exist   ${SOPHOS_INSTALL}/base/lib64/fake_lib.so.1.66
    File Should Not Exist   ${SOPHOS_INSTALL}/base/lib64/fake_lib.so.1.66.999
    File Should Not Exist   ${SOPHOS_INSTALL}/base/lib64/fake_lib.so.1.66.999.0
    File Should Not Exist   ${SOPHOS_INSTALL}/base/lib64/faker_lib.so
    File Should Not Exist   ${SOPHOS_INSTALL}/base/lib64/faker_lib.so.2
    File Should Not Exist   ${SOPHOS_INSTALL}/base/lib64/faker_lib.so.2.23
    File Should Not Exist   ${SOPHOS_INSTALL}/base/lib64/faker_lib.so.2.23.999
    File Should Not Exist   ${SOPHOS_INSTALL}/base/lib64/faker_lib.so.2.23.999.0

    File Should Not Exist   ${SOPHOS_INSTALL}/plugins/mtr/lib64/also_a_fake_lib.so
    File Should Not Exist   ${SOPHOS_INSTALL}/plugins/mtr/lib64/also_a_fake_lib.so.5
    File Should Not Exist   ${SOPHOS_INSTALL}/plugins/mtr/lib64/also_a_fake_lib.so.5.86
    File Should Not Exist   ${SOPHOS_INSTALL}/plugins/mtr/lib64/also_a_fake_lib.so.5.86.999
    File Should Not Exist   ${SOPHOS_INSTALL}/plugins/mtr/lib64/also_a_fake_lib.so.5.86.999.0
    File Should Not Exist   ${SOPHOS_INSTALL}/plugins/mtr/lib64/fake_lib.so
    File Should Not Exist   ${SOPHOS_INSTALL}/plugins/mtr/lib64/fake_lib.so.1
    File Should Not Exist   ${SOPHOS_INSTALL}/plugins/mtr/lib64/fake_lib.so.1.66
    File Should Not Exist   ${SOPHOS_INSTALL}/plugins/mtr/lib64/fake_lib.so.1.66.999
    File Should Not Exist   ${SOPHOS_INSTALL}/plugins/mtr/lib64/fake_lib.so.1.66.999.0
    File Should Not Exist   ${SOPHOS_INSTALL}/plugins/mtr/lib64/faker_lib.so
    File Should Not Exist   ${SOPHOS_INSTALL}/plugins/mtr/lib64/faker_lib.so.2
    File Should Not Exist   ${SOPHOS_INSTALL}/plugins/mtr/lib64/faker_lib.so.2.23
    File Should Not Exist   ${SOPHOS_INSTALL}/plugins/mtr/lib64/faker_lib.so.2.23.999
    File Should Not Exist   ${SOPHOS_INSTALL}/plugins/mtr/lib64/faker_lib.so.2.23.999.0

    File Should Exist   ${SOPHOS_INSTALL}/tmp/ServerProtectionLinux-Base/addedFiles_manifest.dat
    File Should Exist   ${SOPHOS_INSTALL}/tmp/ServerProtectionLinux-Base/changedFiles_manifest.dat
    File Should Exist   ${SOPHOS_INSTALL}/tmp/ServerProtectionLinux-Base/removedFiles_manifest.dat
    File Should Exist   ${SOPHOS_INSTALL}/base/update/ServerProtectionLinux-Base/manifest.dat

    File Should Exist   ${SOPHOS_INSTALL}/tmp/ServerProtectionLinux-Plugin-MDR/addedFiles_manifest.dat
    File Should Exist   ${SOPHOS_INSTALL}/tmp/ServerProtectionLinux-Plugin-MDR/changedFiles_manifest.dat
    File Should Exist   ${SOPHOS_INSTALL}/tmp/ServerProtectionLinux-Plugin-MDR/removedFiles_manifest.dat
    File Should Exist   ${SOPHOS_INSTALL}/base/update/ServerProtectionLinux-Plugin-MDR/manifest.dat

    File Should Exist   ${SOPHOS_INSTALL}/base/update/var/update_config.json
    File Should Exist   ${SOPHOS_INSTALL}/base/update/ServerProtectionLinux-Base/manifest.dat
