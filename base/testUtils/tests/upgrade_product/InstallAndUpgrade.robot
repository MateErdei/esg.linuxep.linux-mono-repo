*** Settings ***
Suite Setup      Suite Setup
Suite Teardown   Suite Teardown

Test Setup       Test Setup
Test Teardown    Test Teardown

Library     ${LIBS_DIRECTORY}/WarehouseGenerator.py
Library     ${LIBS_DIRECTORY}/ThinInstallerUtils.py
Library     ${LIBS_DIRECTORY}/LogUtils.py
Library     ${LIBS_DIRECTORY}/FullInstallerUtils.py
Library     ${LIBS_DIRECTORY}/MCSRouter.py
Library     ${LIBS_DIRECTORY}/UpdateSchedulerHelper.py
Library     ${LIBS_DIRECTORY}/TemporaryDirectoryManager.py
Library     ${LIBS_DIRECTORY}/OSUtils.py
Library     ${LIBS_DIRECTORY}/WarehouseUtils.py
Library     ${LIBS_DIRECTORY}/TeardownTools.py
Library     ${LIBS_DIRECTORY}/UpgradeUtils.py
Library     Process
Library     OperatingSystem
Library     String

Resource    ../event_journaler/EventJournalerResources.robot
Resource    ../mcs_router/McsRouterResources.robot
Resource    ../installer/InstallerResources.robot
Resource    ../thin_installer/ThinInstallerResources.robot
Resource    ../example_plugin/ExamplePluginResources.robot
Resource    ../av_plugin/AVResources.robot
Resource    ../mdr_plugin/MDRResources.robot
Resource    ../edr_plugin/EDRResources.robot
Resource    ../scheduler_update/SchedulerUpdateResources.robot
Resource    ../GeneralTeardownResource.robot
Resource    UpgradeResources.robot

*** Variables ***
${BaseAndMtrReleasePolicy}                  ${GeneratedWarehousePolicies}/base_and_mtr_VUT-1.xml
${BaseAndAVReleasePolicy}                  ${GeneratedWarehousePolicies}/base_and_av_VUT-1.xml
${BaseEdrAndMtrReleasePolicy}               ${GeneratedWarehousePolicies}/base_edr_and_mtr_VUT-1.xml
${BaseEdrAndMtrAndAVReleasePolicy}          ${GeneratedWarehousePolicies}/base_edr_and_mtr_and_av_VUT-1.xml
${BaseAndMtrAndAvReleasePolicy}             ${GeneratedWarehousePolicies}/base_and_mtr_and_av_VUT-1.xml
${BaseAndEDROldWHFormat}                    ${GeneratedWarehousePolicies}/base_edr_old_wh_format.xml
${BaseAndMtrVUTPolicy}                      ${GeneratedWarehousePolicies}/base_and_mtr_VUT.xml
${BaseAndAVVUTPolicy}                      ${GeneratedWarehousePolicies}/base_and_av_VUT.xml
${BaseEdrAndMtrAndAVVUTPolicy}              ${GeneratedWarehousePolicies}/base_edr_and_mtr_and_av_VUT.xml
${BaseAndMtrAndEdrVUTPolicy}                ${GeneratedWarehousePolicies}/base_edr_and_mtr.xml
${BaseAndMtrWithFakeLibs}                   ${GeneratedWarehousePolicies}/base_and_mtr_0_6_0.xml
${BaseAndEdrVUTPolicy}                      ${GeneratedWarehousePolicies}/base_and_edr_VUT.xml
${BaseOnlyVUTPolicy}                        ${GeneratedWarehousePolicies}/base_only_VUT.xml
${BaseOnlyVUT_weekly_schedule_Policy}       ${GeneratedWarehousePolicies}/base_only_weeklyScheduleVUT.xml
${BaseOnlyVUT_Without_SDU_Policy}           ${GeneratedWarehousePolicies}/base_only_VUT_without_SDU_Feature.xml
${BaseAndMtrAndAvVUTPolicy}                 ${GeneratedWarehousePolicies}/base_and_mtr_and_av_VUT.xml

${LocalWarehouseDir}                        ./local_warehouses
${Testpolicy}                               ${SUPPORT_FILES}/CentralXml/RealWarehousePolicies/GeneratedAlcPolicies/base_and_mtr_VUT.xml
${SULDownloaderLog}                         ${SOPHOS_INSTALL}/logs/base/suldownloader.log
${SULDownloaderLogDowngrade}                ${SOPHOS_INSTALL}/logs/base/downgrade-backup/suldownloader.log
${UpdateSchedulerLog}                       ${SOPHOS_INSTALL}/logs/base/sophosspl/updatescheduler.log
${Sophos_Scheduled_Query_Pack}              ${SOPHOS_INSTALL}/plugins/edr/etc/osquery.conf.d/sophos-scheduled-query-pack.conf
${status_file}                              ${SOPHOS_INSTALL}/base/mcs/status/ALC_status.xml

*** Test Cases ***

We Can Install From A Ballista Warehouse
    [Tags]  MANUAL

    # SETTINGS
    # ---------------------------------------------------------------------------------------
    #  warehouse creds
    #  SET THESE
    ${username} =  Set Variable  username
    ${password} =  Set Variable  password

    # ALC Policy Types
    ${Base} =  Set Variable         ${GeneratedWarehousePolicies}/base_only_VUT.xml
    ${BaseMtr} =  Set Variable      ${GeneratedWarehousePolicies}/base_and_mtr_VUT.xml
    ${BaseEdr} =  Set Variable      ${GeneratedWarehousePolicies}/base_and_edr_VUT.xml
    ${BaseEdrMTr} =  Set Variable   ${GeneratedWarehousePolicies}/base_edr_and_mtr.xml
    # CHANGE THIS ONE
    ${policy_template} =  Set Variable  ${BaseEdrMTr}
    # Make sure the policy template you picked is appropriate for the warehouse you want to test.
    # If you can't find one, make one.
    # ---------------------------------------------------------------------------------------
    # SETTINGS

    ${ballista_policy} =  Create Ballista Config From Template Policy  ${policy_template}  ${username}  ${password}
    Log File  ${ballista_policy}

    Start Local Cloud Server  --initial-alc-policy  ${ballista_policy}

    Log File  /etc/hosts
    Configure And Run Thininstaller Using Real Warehouse Policy  0  ${ballista_policy}

    Wait Until Keyword Succeeds
    ...   200 secs
    ...   10 secs
    ...   Check MCS Envelope Contains Event Success On N Event Sent  1
    Check Suldownloader Log Contains  Successfully connected to: Sophos at https://dci.sophosupd.com/cloudupdate

    Check All Product Logs Do Not Contain Error
    Check All Product Logs Do Not Contain Critical

#    Force A failure If you want to check for anything with the teardown logs
#    Fail

We Can Upgrade From Dogfood to Develop Without Unexpected Errors
    [Timeout]  10 minutes
    [Tags]  INSTALLER  THIN_INSTALLER  UNINSTALL  UPDATE_SCHEDULER  SULDOWNLOADER  OSTIA

    Start Local Cloud Server  --initial-alc-policy  ${BaseAndMtrAndAvReleasePolicy}

    Log File  /etc/hosts
    Configure And Run Thininstaller Using Real Warehouse Policy  0  ${BaseAndMtrAndAvReleasePolicy}
    Override Local LogConf File Using Content  [suldownloader]\nVERBOSITY = DEBUG\n

    Wait Until Keyword Succeeds
    ...   300 secs
    ...   10 secs
    ...   Check MCS Envelope Contains Event Success On N Event Sent  1

    Run Shell Process   /opt/sophos-spl/bin/wdctl stop av     OnError=Failed to stop av
    Override LogConf File as Global Level  DEBUG
    Run Shell Process   /opt/sophos-spl/bin/wdctl start av    OnError=Failed to start av

    Check Log Contains String N times   ${SOPHOS_INSTALL}/logs/base/suldownloader.log  suldownloader_log   Update success  1

    Check EAP Release With AV Installed Correctly
    ${BaseReleaseVersion} =     Get Version Number From Ini File   ${InstalledBaseVersionFile}
    ${MtrReleaseVersion} =      Get Version Number From Ini File   ${InstalledMDRPluginVersionFile}
    ${AVReleaseVersion} =      Get Version Number From Ini File   ${InstalledAVPluginVersionFile}
    #TODO LINUXDAR-3183 enable these checks when event journaler is in the dogfood warehouse
    #${EJReleaseVersion} =      Get Version Number From Ini File  ${EVENTJOURNALER_DIR}/VERSION.ini

    Send ALC Policy And Prepare For Upgrade  ${BaseAndMtrAndAvVUTPolicy}
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  2 secs
    ...  Check Policy Written Match File  ALC-1_policy.xml  ${BaseAndMtrAndAvVUTPolicy}
    Wait until threat detector running

    Mark Watchdog Log
    Mark Managementagent Log
    Trigger Update Now

    Wait Until Keyword Succeeds
    ...   300 secs
    ...   10 secs
    ...   Check Log Contains String At Least N times    ${SOPHOS_INSTALL}/logs/base/suldownloader.log   suldownloader_log   Update success  2

    # Make sure the second update performs an upgrade.
    Check Log Contains String N times    ${SOPHOS_INSTALL}/logs/base/suldownloader.log   suldownloader_log   Product Report for product downloaded: ServerProtectionLinux-Base-component Upgraded  1
    Check Log Contains String N times    ${SOPHOS_INSTALL}/logs/base/suldownloader.log   suldownloader_log   Product Report for product downloaded: ServerProtectionLinux-Plugin-liveresponse Upgraded  1
    Check Log Contains String N times    ${SOPHOS_INSTALL}/logs/base/suldownloader.log   suldownloader_log   Product Report for product downloaded: ServerProtectionLinux-Plugin-MDR Upgraded  1
    Check Log Contains String N times    ${SOPHOS_INSTALL}/logs/base/suldownloader.log   suldownloader_log   Product Report for product downloaded: ServerProtectionLinux-Plugin-AV Upgraded  1

    #confirm that the warehouse flags supplement is installed when upgrading
    File Exists With Permissions  ${SOPHOS_INSTALL}/base/etc/sophosspl/flags-warehouse.json  root  sophos-spl-group  -rw-r-----

    Check Mtr Reconnects To Management Agent After Upgrade
    Check for Management Agent Failing To Send Message To MTR And Check Recovery

    # If the policy comes down fast enough SophosMtr will not have started by the time mtr plugin is restarted
    # This is only an issue with versions of base before we started using boost process
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/plugins/mtr/log/mtr.log  ProcessImpl <> The PID -1 does not exist or is not a child of the calling process.
    #  This is raised when PluginAPI has been changed so that it is no longer compatible until upgrade has completed.
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/plugins/mtr/log/mtr.log  mtr <> Policy is invalid: RevID not found
    #TODO LINUXDAR-2972 remove when this defect is fixed
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/logs/base/sophosspl/mcsrouter.log  root <> Atomic write failed with message: [Errno 13] Permission denied: '/opt/sophos-spl/tmp/policy/flags.json'
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/logs/base/sophosspl/mcsrouter.log  root <> utf8 write failed with message: [Errno 13] Permission denied: '/opt/sophos-spl/tmp/policy/flags.json'
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/logs/base/sophosspl/mcsrouter.log  root <> Atomic write failed with message: [Errno 2] No such file or directory: '/opt/sophos-spl/tmp/policy/flags.json'
    #not an error should be a WARN instead, but it's happening on the EAP version so it's too late to change it now
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/plugins/av/log/sophos_threat_detector/sophos_threat_detector.log  ThreatScanner <> Failed to read customerID - using default value
    #TODO LINUXDAR-3191 remove when this defect is closed
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/plugins/av/log/av.log  av <> Failed to get SAV policy at startup (No Policy Available)
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/plugins/av/log/av.log  av <> Failed to get ALC policy at startup (No Policy Available)
    #this is expected because we are restarting the avplugin to enable debug logs, we need to make sure it occurs only once though
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/plugins/av/log/av.log  ScanProcessMonitor <> Exiting sophos_threat_detector with code: 15
    #TODO LINUXDAR-3187 remove when this defect is fixed
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/logs/base/watchdog.log    ProcessMonitoringImpl <> /opt/sophos-spl/plugins/av/sbin/sophos_threat_detector_launcher died with 15
    #TODO LINUXDAR-3188 remove when this defect is closed
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/plugins/av/log/av.log  UnixSocket <> Failed to write Process Control Request to socket. Exception caught: Environment interruption
    Run Keyword And Expect Error  *
    ...     Check Log Contains String N  times ${SOPHOS_INSTALL}/plugins/av/log/av.log  av.log  Exiting sophos_threat_detector with code: 15  2

    Check All Product Logs Do Not Contain Error
    Check All Product Logs Do Not Contain Critical

    Check Current Release With AV Installed Correctly

    ${BaseDevVersion} =     Get Version Number From Ini File   ${InstalledBaseVersionFile}
    ${MtrDevVersion} =      Get Version Number From Ini File   ${InstalledMDRPluginVersionFile}
    ${AVDevVersion} =       Get Version Number From Ini File   ${InstalledAVPluginVersionFile}
    #TODO LINUXDAR-3183 enable these checks when event journaler is in the dogfood warehouse
    #${EJDevVersion} =       Get Version Number From Ini File  ${EVENTJOURNALER_DIR}/VERSION.ini

    Should Not Be Equal As Strings  ${BaseReleaseVersion}  ${BaseDevVersion}
    Should Not Be Equal As Strings  ${MtrReleaseVersion}  ${MtrDevVersion}
    Should Not Be Equal As Strings  ${AVReleaseVersion}  ${AVDevVersion}
    #TODO LINUXDAR-3183 enable these checks when event journaler is in the dogfood warehouse
    #Should Not Be Equal As Strings  ${EJReleaseVersion}  ${EJDevVersion}
    Check Event Journaler Executable Running
    Check Update Reports Have Been Processed

We Can Downgrade From Develop to Dogfood Without Unexpected Errors
    [Timeout]  10 minutes
    [Tags]   INSTALLER  THIN_INSTALLER  UNINSTALL  UPDATE_SCHEDULER  SULDOWNLOADER  OSTIA   BASE_DOWNGRADE

    Start Local Cloud Server  --initial-alc-policy  ${BaseEdrAndMtrAndAVVUTPolicy}

    Configure And Run Thininstaller Using Real Warehouse Policy  0  ${BaseEdrAndMtrAndAVVUTPolicy}

    Wait Until Keyword Succeeds
    ...   300 secs
    ...   10 secs
    ...   Check MCS Envelope Contains Event Success On N Event Sent  1

    Start Process  tail -f ${SOPHOS_INSTALL}/logs/base/suldownloader.log > /tmp/preserve-sul-downgrade  shell=true
    Check Log Contains String N times   ${SOPHOS_INSTALL}/logs/base/suldownloader.log   suldownloader_log   Update success  1

    Check Current Release Installed Correctly
    # products that should change version
    ${BaseDevVersion} =     Get Version Number From Ini File   ${InstalledBaseVersionFile}
    ${MtrDevVersion} =      Get Version Number From Ini File   ${InstalledMDRPluginVersionFile}
    ${EdrDevVersion} =      Get Version Number From Ini File   ${InstalledEDRPluginVersionFile}
    ${AVDevVersion} =      Get Version Number From Ini File   ${InstalledAVPluginVersionFile}
    #TODO LINUXDAR-3183 enable these checks when event journaler is in the dogfood warehouse
    #${EJDevVersion} =      Get Version Number From Ini File  ${EVENTJOURNALER_DIR}/VERSION.ini
    Directory Should Not Exist   ${SOPHOS_INSTALL}/logs/base/downgrade-backup

    # Products that should be uninstalled after downgrade
    Should Exist  ${InstalledLRPluginVersionFile}

    #the query pack should have been installed with EDR VUT
    Should Exist  ${Sophos_Scheduled_Query_Pack}
    ${osquery_pid_before_query_pack_removed} =  Get Edr OsQuery PID


    # Changing the policy here will result in an automatic update
    # Note when downgrading from a release with live response to a release without live response
    # results in a second update.
    Override LogConf File as Global Level  DEBUG
    Create File  ${SOPHOS_INSTALL}/base/mcs/action/testfile
    Should Exist  ${SOPHOS_INSTALL}/base/mcs/action/testfile
    Run Process  chown  -R  sophos-spl-local:sophos-spl-group  ${SOPHOS_INSTALL}/base/mcs/action/testfile

    Send ALC Policy And Prepare For Upgrade  ${BaseEdrAndMtrAndAVReleasePolicy}
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  2 secs
    ...  Check Policy Written Match File  ALC-1_policy.xml  ${BaseEdrAndMtrAndAVReleasePolicy}

    Mark Watchdog Log
    Mark Managementagent Log

    Wait Until Keyword Succeeds
    ...   300 secs
    ...   10 secs
    ...   Directory Should Exist   ${SOPHOS_INSTALL}/logs/base/downgrade-backup

    Should Not Exist  ${SOPHOS_INSTALL}/base/mcs/action/testfile
    Check Log Contains  Preparing ServerProtectionLinux-Base-component for downgrade  ${SULDownloaderLogDowngrade}  backedup suldownloader log
    #TODO LINUXDAR-2881 remove sleep when this defect is fixed in downgrade version- provide a time space if suldownloader is kicked off again by policy change.
    Sleep  10
    Trigger Update Now

    Wait Until Keyword Succeeds
    ...  200 secs
    ...  10 secs
    ...  Check Log Contains String At Least N Times   ${SULDownloaderLog}  Update Log  Update success  1

    Check for Management Agent Failing To Send Message To MTR And Check Recovery
    # If the policy comes down fast enough SophosMtr will not have started by the time mtr plugin is restarted
    # This is only an issue with versions of base before we started using boost process
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/plugins/mtr/log/mtr.log  ProcessImpl <> The PID -1 does not exist or is not a child of the calling process.
    #  This is raised when PluginAPI has been changed so that it is no longer compatible until upgrade has completed.
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/plugins/mtr/log/mtr.log  mtr <> Policy is invalid: RevID not found
     #TODO LINUXDAR-2881 remove when this defect is fixed in downgrade version
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/logs/base/suldownloader.log  suldownloaderdata <> Failed to process input settings
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/logs/base/suldownloader.log  suldownloaderdata <> Failed to process json message
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/logs/base/sophosspl/updatescheduler.log  updatescheduler <> Update Service (sophos-spl-update.service) failed
    #TODO LINUXDAR-2972 remove when this defect is fixed
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/logs/base/sophosspl/mcsrouter.log  root <> Atomic write failed with message: [Errno 13] Permission denied: '/opt/sophos-spl/tmp/policy/flags.json'
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/logs/base/sophosspl/mcsrouter.log  root <> utf8 write failed with message: [Errno 13] Permission denied: '/opt/sophos-spl/tmp/policy/flags.json'
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/logs/base/sophosspl/mcsrouter.log  mcsrouter.utils.plugin_registry <> Failed to load plugin file: /opt/sophos-spl/base/pluginRegistry/edr.json
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/logs/base/sophosspl/mcsrouter.log  mcsrouter.utils.plugin_registry <> [Errno 13] Permission denied: '/opt/sophos-spl/base/pluginRegistry/edr.json'
    #not an error should be a WARN instead, but it's happening on the EAP version so it's too late to change it now
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/plugins/av/log/sophos_threat_detector/sophos_threat_detector.log  ThreatScanner <> Failed to read customerID - using default value
    #avp tries to pick up the policy after it starts, if the policy is not there it logs the error and then waits 10s for the policy to show up.
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/plugins/av/log/av.log  av <> Failed to get SAV policy at startup (No Policy Available)

    Check All Product Logs Do Not Contain Error
    Check All Product Logs Do Not Contain Critical

    Log File  /tmp/preserve-sul-downgrade
    Check EAP Release With AV Installed Correctly

    ${BaseReleaseVersion} =     Get Version Number From Ini File   ${InstalledBaseVersionFile}
    ${MtrReleaseVersion} =      Get Version Number From Ini File   ${InstalledMDRPluginVersionFile}
    ${EdrReleaseVersion} =      Get Version Number From Ini File   ${InstalledEDRPluginVersionFile}
    ${AVReleaseVersion} =       Get Version Number From Ini File   ${InstalledAVPluginVersionFile}
    #TODO LINUXDAR-3183 enable these checks when event journaler is in the dogfood warehouse
    #${EJReleaseVersion} =           Get Version Number From Ini File  ${EVENTJOURNALER_DIR}/VERSION.ini
    Should Not Be Equal As Strings  ${BaseReleaseVersion}  ${BaseDevVersion}
    Should Not Be Equal As Strings  ${MtrReleaseVersion}  ${MtrDevVersion}
    Should Not Be Equal As Strings  ${EdrReleaseVersion}  ${EdrDevVersion}
    Should Not Be Equal As Strings  ${AVReleaseVersion}  ${AVDevVersion}
    #TODO LINUXDAR-3183 enable these checks when event journaler is in the dogfood warehouse
    #Should Not Be Equal As Strings  ${EJReleaseVersion}  ${EJDevVersion}

    ${osquery_pid_after_query_pack_removed} =  Get Edr OsQuery PID
    Should Not Be Equal As Integers  ${osquery_pid_after_query_pack_removed}  ${osquery_pid_before_query_pack_removed}

    # Upgrade back to master to check we can upgrade from a downgraded product
    Send ALC Policy And Prepare For Upgrade  ${BaseEdrAndMtrAndAVVUTPolicy}
    Trigger Update Now

    Wait Until Keyword Succeeds
    ...  150 secs
    ...  10 secs
    ...  Component Version has changed  ${BaseReleaseVersion}    ${InstalledBaseVersionFile}

    Wait Until Keyword Succeeds
    ...  200 secs
    ...  5 secs
    ...  Component Version has changed  ${MtrReleaseVersion}    ${InstalledMDRPluginVersionFile}

    Wait Until Keyword Succeeds
    ...  200 secs
    ...  5 secs
    ...  Component Version has changed  ${EdrReleaseVersion}    ${InstalledEDRPluginVersionFile}

    #wait for AV plugin to be running before attempting uninstall
    Wait Until Keyword Succeeds
    ...  200 secs
    ...  5 secs
    ...  Component Version has changed  ${AVReleaseVersion}    ${InstalledAVPluginVersionFile}

    #the query pack should have been re-installed
    Wait Until Keyword Succeeds
    ...  20 secs
    ...  5 secs
    ...  file should exist  ${Sophos_Scheduled_Query_Pack}
    ${osquery_pid_after_query_pack_restored} =  Get Edr OsQuery PID
    Should Not Be Equal As Integers  ${osquery_pid_after_query_pack_restored}  ${osquery_pid_after_query_pack_removed}

Ensure Supplement Updates Only Perform A Supplement Update
    ## This can't run against real (remote) warehouses, since it modifies the warehouses to prevent product updates from working
    [Tags]  MANUAL
    [TearDown]  Cleanup Warehouse And Test Teardown
    [Timeout]    20 minutes

    ## This policy has a weekly schedule for 3am yesterday
    Start Local Cloud Server  --initial-alc-policy  ${BaseOnlyVUT_weekly_schedule_Policy}

    Log File  /etc/hosts
    Log File  ${BaseOnlyVUT_weekly_schedule_Policy}
    Configure And Run Thininstaller Using Real Warehouse Policy  0  ${BaseOnlyVUT_weekly_schedule_Policy}

    Wait Until Keyword Succeeds
    ...   200 secs
    ...   10 secs
    ...   Check MCS Envelope Contains Event Success On N Event Sent  1

    ## Running on a vagrant box - force local build in
#    Run Process  bash  +x  /vagrant/everest-base/output/SDDS-COMPONENT/install.sh
#    Run Process   rm  -rf  /opt/sophos-spl/base/VERSION*
#    Copy File  /opt/sophos-spl/base/update/cache/primary/ServerProtectionLinux-Base-component/VERSION.ini  /opt/sophos-spl/base/VERSION.ini

    ## Need to remove the log so that we can see if the later update is definitely supplement only in the logs
    Log File  ${SULDownloaderLog}
    Remove File  ${SULDownloaderLog}

    Disable Product Warehouse to ensure we only perform a supplement update  develop

    ## First update happens between 5-10 minutes after starting UpdateScheduler
    Sleep  5 minutes
    Wait Until Keyword Succeeds
    ...  5 minutes
    ...  30 secs
    ...  Check Log Contains In Order  ${SULDownloaderLog}
         ...  Doing supplement-only update
         ...  Update success


    Check Log Does Not Contain  Doing product and supplement update   ${SULDownloaderLog}  SulDownloaderLog
    Check Log Does Not Contain  Forcing product update due previous update failure or change in configuration   ${SULDownloaderLog}  SulDownloaderLog

Test That Only Subscriptions Appear As Subscriptions In ALC Status File
    [Tags]  INSTALLER  THIN_INSTALLER  UNINSTALL  UPDATE_SCHEDULER  SULDOWNLOADER  OSTIA

    Start Local Cloud Server  --initial-alc-policy  ${BaseAndMtrVUTPolicy}

    Configure And Run Thininstaller Using Real Warehouse Policy  0  ${BaseAndMtrVUTPolicy}

    Wait Until Keyword Succeeds
    ...   200 secs
    ...   10 secs
    ...   Check MCS Envelope Contains Event Success On N Event Sent  1

    Only Subscriptions In Policy Are In Alc Status Subscriptions

We Can Upgrade AV From Dogfood To VUT Without Unexpected Errors
    [Timeout]  10 minutes
    [Tags]  INSTALLER  THIN_INSTALLER  UNINSTALL  UPDATE_SCHEDULER  SULDOWNLOADER  OSTIA

    Start Local Cloud Server  --initial-alc-policy  ${BaseAndAVReleasePolicy}

    Log File  /etc/hosts
    Configure And Run Thininstaller Using Real Warehouse Policy  0  ${BaseAndAVReleasePolicy}

    Wait Until Keyword Succeeds
    ...   300 secs
    ...   10 secs
    ...   Check MCS Envelope Contains Event Success On N Event Sent  1

    Run Shell Process   /opt/sophos-spl/bin/wdctl stop av     OnError=Failed to stop av
    Override LogConf File as Global Level  DEBUG
    Run Shell Process   /opt/sophos-spl/bin/wdctl start av    OnError=Failed to start av

    Check Log Contains String N times   ${SOPHOS_INSTALL}/logs/base/suldownloader.log  suldownloader_log   Update success  1

    Check AV Plugin Installed
    ${BaseReleaseVersion} =      Get Version Number From Ini File   ${InstalledBaseVersionFile}
    ${AVReleaseVersion} =      Get Version Number From Ini File   ${InstalledAVPluginVersionFile}

    Send ALC Policy And Prepare For Upgrade  ${BaseAndAVVUTPolicy}

    Wait Until Keyword Succeeds
    ...  30 secs
    ...  2 secs
    ...  Check Policy Written Match File  ALC-1_policy.xml  ${BaseAndAVVUTPolicy}
    Wait until threat detector running

    Mark Watchdog Log
    Mark Managementagent Log

    Wait Until Keyword Succeeds
    ...   300 secs
    ...   10 secs
    ...   Check Log Contains String At Least N times    ${SOPHOS_INSTALL}/logs/base/suldownloader.log   suldownloader_log   Update success  2

    # Make sure the second update performs an upgrade.
    Check Log Contains String N times    ${SOPHOS_INSTALL}/logs/base/suldownloader.log   suldownloader_log   Product Report for product downloaded: ServerProtectionLinux-Base-component Upgraded  1
    Check Log Contains String N times    ${SOPHOS_INSTALL}/logs/base/suldownloader.log   suldownloader_log   Product Report for product downloaded: ServerProtectionLinux-Plugin-liveresponse Upgraded  1
    Check Log Contains String N times    ${SOPHOS_INSTALL}/logs/base/suldownloader.log   suldownloader_log   Product Report for product downloaded: ServerProtectionLinux-Plugin-AV Upgraded  1

    Check AV Plugin Installed

    #not an error should be a WARN instead, but it's happening on the EAP version so it's too late to change it now
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/plugins/av/log/sophos_threat_detector/sophos_threat_detector.log  ThreatScanner <> Failed to read customerID - using default value
    #TODO LINUXDAR-2972 remove when this defect is fixed
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/logs/base/sophosspl/mcsrouter.log  root <> Atomic write failed with message: [Errno 13] Permission denied: '/opt/sophos-spl/tmp/policy/flags.json'
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/logs/base/sophosspl/mcsrouter.log  root <> utf8 write failed with message: [Errno 13] Permission denied: '/opt/sophos-spl/tmp/policy/flags.json'
    #avp tries to pick up the policy after it starts, if the policy is not there it logs the error and then waits 10s for the policy to show up.
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/plugins/av/log/av.log  av <> Failed to get SAV policy at startup (No Policy Available)
    #this is expected because we are restarting the avplugin to enable debug logs, we need to make sure it occurs only once though
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/plugins/av/log/av.log  ScanProcessMonitor <> Exiting sophos_threat_detector with code: 15
    Run Keyword And Expect Error  *
    ...     Check Log Contains String N  times ${SOPHOS_INSTALL}/plugins/av/log/av.log  av.log  Exiting sophos_threat_detector with code: 15  2
    #TODO LINUXDAR-3188 remove when this defect is closed
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/plugins/av/log/av.log  UnixSocket <> Failed to write Process Control Request to socket. Exception caught: Environment interruption
    Check All Product Logs Do Not Contain Error
    Check All Product Logs Do Not Contain Critical

    ${BaseDevVersion} =     Get Version Number From Ini File   ${InstalledBaseVersionFile}
    ${AVDevVersion} =   Get Version Number From Ini File   ${InstalledAVPluginVersionFile}

    Should Not Be Equal As Strings  ${BaseReleaseVersion}  ${BaseDevVersion}
    Should Not Be Equal As Strings  ${AVReleaseVersion}  ${AVDevVersion}

    Check AV Plugin Permissions
    Check AV Plugin Can Scan Files

Check Installed Version In Status Message Is Correctly Reported Based On Version Ini File
    [Tags]  INSTALLER  THIN_INSTALLER  UPDATE_SCHEDULER  SULDOWNLOADER  OSTIA
    # Setup for test.
    Start Local Cloud Server  --initial-alc-policy  ${BaseAndMtrVUTPolicy}

    Configure And Run Thininstaller Using Real Warehouse Policy  0  ${BaseAndMtrVUTPolicy}
    Wait Until Keyword Succeeds
    ...   60 secs
    ...   5 secs
    ...   File Should Exist  ${status_file}

    Override Local LogConf File for a component   DEBUG  global
    Run Process  systemctl  restart  sophos-spl

    # Tigger update to make sure everything has settle down foe test, i.e. we do not want files to be updated
    # when the actual test update is being performed.
    Trigger Update Now
    Wait Until Keyword Succeeds
    ...   240 secs
    ...   10 secs
    ...   Check Log Contains String N times   ${SOPHOS_INSTALL}/logs/base/suldownloader.log   suldownloader_log   Update success   2

    # Run test to simulate installedVersion content is correctly generated in status file.
    # First modify Base, live response and MTR version ini files
    # Base is set to an older version (make sure this is never newer than real installed version).
    #       Or installer will run.  This is to simulate new version failed to install.
    # Live reponse version.ini file will be removed, to simulate component never installed.
    # MTR is set to an older version (make sure this is never newer than real installed version).
    Remove File  ${SOPHOS_INSTALL}/base/VERSION.ini
    Create File   ${SOPHOS_INSTALL}/base/VERSION.ini   PRODUCT_VERSION = 1.0.123.123
    Run Process  chmod  640   ${SOPHOS_INSTALL}/base/VERSION.ini
    Run Process  chown  root:sophos-spl-group  ${SOPHOS_INSTALL}/base/VERSION.ini
    Remove File  ${SOPHOS_INSTALL}/plugins/liveresponse/VERSION.ini
    Remove File  ${SOPHOS_INSTALL}/plugins/mtr/VERSION.ini
    Create File  ${SOPHOS_INSTALL}/plugins/mtr/VERSION.ini   PRODUCT_VERSION = 1.0.0.234
    Remove File  ${SOPHOS_INSTALL}/plugins/mtr/dbos/data/PRODUCT_VERSION.ini
    Create File  ${SOPHOS_INSTALL}/plugins/mtr/dbos/data/PRODUCT_VERSION.ini  PRODUCT_VERSION = 1.0.0.345
    Remove File  ${SOPHOS_INSTALL}/plugins/mtr/osquery/VERSION.ini
    Create File  ${SOPHOS_INSTALL}/plugins/mtr/osquery/VERSION.ini  PRODUCT_VERSION = 1.0.0.456

    #  This update should not install any files.
    Trigger Update Now

    Wait Until Keyword Succeeds
    ...   240 secs
    ...   10 secs
    ...   Check Log Contains String N times   ${SOPHOS_INSTALL}/logs/base/suldownloader.log   suldownloader_log   Update success   3

    # Check content of ALC_status.xml file contsins the expected installedVersion values.
    Check Status File Component Installed Version Is Correct  ServerProtectionLinux-Base-component  1.0.123.123  ${status_file}
    Check Status File Component Installed Version Is Correct  ServerProtectionLinux-Plugin-liveresponse  ${EMPTY}  ${status_file}
    Check Status File Component Installed Version Is Correct  ServerProtectionLinux-MDR-Control-Component  1.0.0.234  ${status_file}
    Check Status File Component Installed Version Is Correct  ServerProtectionLinux-MDR-DBOS-Component  1.0.0.345  ${status_file}
    Check Status File Component Installed Version Is Correct  ServerProtectionLinux-MDR-osquery-Component  1.0.0.456  ${status_file}

*** Keywords ***

Cleanup Warehouse And Test Teardown
    Restore Product Warehouse  develop
    Test Teardown

Simulate Previous Scheduled Update Success
    # Trigger updating via Central Update Scheduler which will create the previous_update_config.json file only when
    # update scheduler process expected current reports rather than unprocessed old reports
    Run Process   cp  ${UPDATE_CONFIG}   ${SOPHOS_INSTALL}/base/update/var/updatescheduler/previous_update_config.json
    Run Process   chown  sophos-spl-updatescheduler:sophos-spl-group   ${SOPHOS_INSTALL}/base/update/var/updatescheduler/previous_update_config.json

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
    Copy File   ${SUPPORT_FILES}/sophos_certs/ps_rootca.crt  ${UPDATE_ROOTCERT_DIR}/ps_rootca.crt


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
    Replace Sophos URLS to Localhost  config_path=${UPDATE_CONFIG}
    Replace Username And Password In Sulconfig  config_path=${UPDATE_CONFIG}

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


Check Current Release With AV Installed Correctly
    Check Mcs Router Running
    Check MDR Plugin Installed
    Check AV Plugin Installed
    Check Installed Correctly

Check EAP Release With AV Installed Correctly
    Check MCS Router Running
    Check MDR Plugin Installed
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  1 secs
    ...  MDR Plugin Log Contains  Run SophosMTR
    Check AV Plugin Installed
    Check Installed Correctly


Check Installed Correctly
    Should Exist    ${SOPHOS_INSTALL}

    Check Correct MCS Password And ID For Local Cloud Saved

    ${result}=  Run Process  stat  -c  "%A"  /opt
    ${ExpectedPerms}=  Set Variable  "drwxr-xr-x"
    Should Be Equal As Strings  ${result.stdout}  ${ExpectedPerms}
    ${version_number} =  Get Version Number From Ini File  ${InstalledBaseVersionFile}
    Check Expected Base Processes Except SDU Are Running

Component Version has changed
    [Arguments]  ${oldVersion}  ${InstalledVersionFile}
    ${NewDevVersion} =  Get Version Number From Ini File   ${InstalledVersionFile}
    Should Not Be Equal As Strings  ${oldVersion}  ${NewDevVersion}

Check Update Reports Have Been Processed
    Directory Should Exist  ${SOPHOS_INSTALL}/base/update/var/updatescheduler/processedReports
    ${files_in_processed_dir} =  List Files In Directory  ${SOPHOS_INSTALL}/base/update/var/updatescheduler/processedReports
    Log  ${files_in_processed_dir}
    ${filesInUpdateVar} =  List Files In Directory  ${SOPHOS_INSTALL}/base/update/var/updatescheduler
    Log  ${filesInUpdateVar}
    ${UpdateReportCount} =  Count Files In Directory  ${SOPHOS_INSTALL}/base/update/var/updatescheduler  update_report[0-9]*.json

    Wait Until Keyword Succeeds
    ...  30 secs
    ...  2 secs
    ...  Check N Update Reports Processed  ${UpdateReportCount}

    Should Contain  ${files_in_processed_dir}[0]  update_report
    Should Not Contain  ${files_in_processed_dir}[0]  update_report.json
    Should Contain  ${filesInUpdateVar}   ${files_in_processed_dir}[0]

Check N Update Reports Processed
    [Arguments]  ${update_report_count}
    ${processed_file_count} =  Count Files In Directory  ${SOPHOS_INSTALL}/base/update/var/updatescheduler/processedReports
    Should Be Equal As Integers  ${processed_file_count}  ${update_report_count}

Get Pid Of Process
    [Arguments]  ${process_name}
    ${result} =    Run Process  pidof  ${process_name}
    Should Be Equal As Integers    ${result.rc}    0   msg=${result.stderr}
    [Return]  ${result.stdout}

Check Mtr Reconnects To Management Agent After Upgrade
    Check Log Contains In Order
    ...  ${mdr_log_file}
    ...  mtr <> Plugin Finished.
    ...  mtr <> Entering the main loop
    ...  Received new policy
    ...  RevID of the new policy