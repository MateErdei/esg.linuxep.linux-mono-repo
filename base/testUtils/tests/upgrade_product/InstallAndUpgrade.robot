*** Settings ***
Suite Setup      Suite Setup
Suite Teardown   Suite Teardown

Test Setup       Test Setup
Test Teardown    Test Teardown

Test Timeout  10 mins
Force Tags  LOAD7
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
Resource    ../runtimedetections_plugin/RuntimeDetectionsResources.robot
Resource    ../scheduler_update/SchedulerUpdateResources.robot
Resource    ../GeneralTeardownResource.robot
Resource    UpgradeResources.robot
Resource    ../management_agent/ManagementAgentResources.robot

*** Variables ***
${BaseEdrAndMtrAndAVDogfoodPolicy}          ${GeneratedWarehousePolicies}/base_edr_and_mtr_and_av_VUT-1.xml
${BaseEdrAndMtrAndAVReleasePolicy}          ${GeneratedWarehousePolicies}/base_edr_and_mtr_and_av_Release.xml
${BaseAndMtrVUTPolicy}                      ${GeneratedWarehousePolicies}/base_and_mtr_VUT.xml
${BaseEdrAndMtrAndAVVUTPolicy}              ${GeneratedWarehousePolicies}/base_edr_and_mtr_and_av_VUT.xml
${BaseOnlyVUT_weekly_schedule_Policy}       ${GeneratedWarehousePolicies}/base_only_weeklyScheduleVUT.xml

${LocalWarehouseDir}                        ./local_warehouses
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


Check Installed Version In Status Message Is Correctly Reported Based On Version Ini File
    [Tags]  INSTALLER  THIN_INSTALLER  UPDATE_SCHEDULER  SULDOWNLOADER  OSTIA
    # Setup for test.
    Start Local Cloud Server  --initial-alc-policy  ${BaseAndMtrVUTPolicy}

    Configure And Run Thininstaller Using Real Warehouse Policy  0  ${BaseAndMtrVUTPolicy}
    Wait Until Keyword Succeeds
    ...   60 secs
    ...   5 secs
    ...   File Should Exist  ${status_file}

    Wait Until Keyword Succeeds
    ...   180 secs
    ...   10 secs
    ...   Check SulDownloader Log Contains String N Times   Update success   1

    # Trigger update to make sure everything has settle down for test, i.e. we do not want files to be updated
    # when the actual test update is being performed.
    Trigger Update Now
    Wait Until Keyword Succeeds
    ...   120 secs
    ...   10 secs
    ...   Check SulDownloader Log Contains String N Times   Update success   2

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
    ...   Check SulDownloader Log Contains String N Times   Update success   3

    # Check content of ALC_status.xml file contsins the expected installedVersion values.
    Check Status File Component Installed Version Is Correct  ServerProtectionLinux-Base-component  1.0.123.123  ${status_file}
    Check Status File Component Installed Version Is Correct  ServerProtectionLinux-Plugin-liveresponse  ${EMPTY}  ${status_file}
    Check Status File Component Installed Version Is Correct  ServerProtectionLinux-Plugin-MDR  1.0.0.234  ${status_file}

*** Keywords ***

Cleanup Warehouse And Test Teardown
    Restore Product Warehouse  develop
    Test Teardown

Simulate Previous Scheduled Update Success
    # Trigger updating via Central Update Scheduler which will create the previous_update_config.json file only when
    # update scheduler process expected current reports rather than unprocessed old reports
    Run Process   cp  ${UPDATE_CONFIG}   ${SOPHOS_INSTALL}/base/update/var/updatescheduler/previous_update_config.json
    Run Process   chown  sophos-spl-updatescheduler:sophos-spl-group   ${SOPHOS_INSTALL}/base/update/var/updatescheduler/previous_update_config.json

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

Component Version has changed
    [Arguments]  ${oldVersion}  ${InstalledVersionFile}
    ${NewDevVersion} =  Get Version Number From Ini File   ${InstalledVersionFile}
    Should Not Be Equal As Strings  ${oldVersion}  ${NewDevVersion}


Get Pid Of Process
    [Arguments]  ${process_name}
    ${result} =    Run Process  pidof  ${process_name}
    Should Be Equal As Integers    ${result.rc}    0   msg=${result.stderr}
    [Return]  ${result.stdout}


Runtime Detections Plugin Is Installed And Running