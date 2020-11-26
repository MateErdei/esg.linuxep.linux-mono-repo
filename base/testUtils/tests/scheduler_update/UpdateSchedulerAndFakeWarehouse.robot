*** Settings ***
Documentation    Check UpdateScheduler Plugin using real SulDownloader and the Fake Warehouse to prove it works.
Default Tags   UPDATE_SCHEDULER  OSTIA


Test Setup      Setup For Test
Test Teardown   Teardown For Test

Suite Setup   Setup Ostia Warehouse Environment
Suite Teardown   Teardown Ostia Warehouse Environment

Library    Process
Library    OperatingSystem
Library    ${LIBS_DIRECTORY}/UpdateSchedulerHelper.py
Library    ${LIBS_DIRECTORY}/UpdateServer.py
Library    ${LIBS_DIRECTORY}/WarehouseGenerator.py
Library    ${LIBS_DIRECTORY}/LogUtils.py
Library    ${LIBS_DIRECTORY}/FullInstallerUtils.py
Library    ${LIBS_DIRECTORY}/SulDownloader.py

Resource  SchedulerUpdateResources.robot
Resource  ../watchdog/LogControlResources.robot
Resource  ../upgrade_product/UpgradeResources.robot

*** Variables ***
${logpath}              ${SOPHOS_INSTALL}/logs/base/suldownloader.log
${BasePolicy}           ${SUPPORT_FILES}/CentralXml/RealWarehousePolicies/GeneratedAlcPolicies/base_only_VUT.xml

*** Test Cases ***
UpdateScheduler Update Against Ostia
    Set Log Level For Component And Reset and Return Previous Log  suldownloader  DEBUG

    Simulate Send Policy And Run Update  ${BasePolicy}

    ${eventPath} =  Check Status and Events Are Created  waitTime=120 secs
    Log File  /opt/sophos-spl/logs/base/suldownloader.log
    Check Event Report Success  ${eventPath}

    File Should Exist    ${SOPHOS_INSTALL}/base/update/var/updatescheduler/previous_update_config.json

    Wait Until Keyword Succeeds
    ...  60 secs
    ...  5 secs
    ...  Check Log Contains String N Times   ${logpath}   SULDownloader Log   suldownloaderdata <> Installing product: ServerProtectionLinux-Base-component version:   1

UpdateScheduler Does Not Create A Config For An Invalid Policy With No Username
    Register Current Sul Downloader Config Time
    Simulate Send Policy   ALC_policy_invalid.xml

    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  Check Log Contains   Invalid policy: Username is empty    ${SOPHOS_INSTALL}/logs/base/sophosspl/updatescheduler.log   Update Scheduler Log

    ${File}=  Get File   ${UPDATE_CONFIG}
    Should not Contain   ${File}  UserPassword="NotARealPassword"

    Check Update Scheduler Running

UpdateScheduler Status No Longer Contains Deprecated Fields
    [Documentation]  Checks fix from LINUXEP-8108
    Set Log Level For Component And Reset and Return Previous Log  suldownloader  DEBUG

    Simulate Send Policy And Run Update  ${BasePolicy}
    ${eventPath} =  Check Status and Events Are Created  waitTime=120 secs
    ${StatusContent} =  Get File  ${SOPHOS_INSTALL}/base/mcs/status/ALC_status.xml

    Should Not Contain  ${StatusContent}  <lastBootTime>
    Should Not Contain  ${StatusContent}  <lastStartedTime>
    Should Not Contain  ${StatusContent}  <lastSyncTime>
    Should Not Contain  ${StatusContent}  <lastInstallStartedTime>
    Should Not Contain  ${StatusContent}  <lastFinishedTime>
    Should Not Contain  ${StatusContent}  <lastResult>

    Check Event Report Success  ${eventPath}

    File Should Exist    ${SOPHOS_INSTALL}/base/update/var/updatescheduler/previous_update_config.json


UpdateScheduler Report Failure On Versig Error
    [Tags]  UPDATE_SCHEDULER
    [Documentation]  Reproduces the error reported in LINUXEP-8012
    #
    #  This test shows that after an update that fails on versig verification.
    #  The same error should be reported while the warehouse does not change
    Set Log Level For Component And Reset and Return Previous Log  suldownloader  DEBUG

    # Normal operation
    Send Policy With No Cache And No Proxy

    Copy File   ${SUPPORT_FILES}/sophos_certs/ps_rootca.crt  ${UPDATE_ROOTCERT_DIR}
    Copy File   ${SUPPORT_FILES}/sophos_certs/rootca.crt  ${UPDATE_ROOTCERT_DIR}

    Replace Sophos URLS to Localhost

    Remove Files In Directory  /opt/sophos-spl/base/mcs/event/
    Simulate Update Now
    Check Status and Events Are Created  waitTime=120 secs
    ${eventPath} =  Get Oldest File In Directory  /opt/sophos-spl/base/mcs/event/

    Log File  /opt/sophos-spl/logs/base/suldownloader.log

    Check Event Report Success  ${eventPath}

    Remove File    ${statusPath}
    Remove File    ${eventPath}

    # create warehouse that should fail versig verification and run another update
    Set Log Level For Component And Reset and Return Previous Log  suldownloader  DEBUG

    Regenerate Warehouse For Update Scheduler   CORRUPTINSTALL=yes

    Replace Sophos URLS to Localhost
    Simulate Update Now
    ${eventPath} =  Check Event File Generated  120

    Wait Until Keyword Succeeds
    ...  50 secs
    ...  5 secs
    ...  SulDownloader Should Report Verification Failed
    Remove File    ${logpath}

    ${eventContent} =   Get File   ${eventPath}
    Should Not Contain   ${eventContent}  <number>0</number>

    File Should Not Exist  ${statusPath}
    Remove File    ${eventPath}

    # run a second update, should report the same error.
    # no event file is created, as it is the same event to be reported.

    Simulate Update Now
    Wait Until Keyword Succeeds
    ...  50 secs
    ...  5 secs
    ...  SulDownloader Should Report Verification Failed

    # run a third time with a good warehouse and show it has been restored.
    Regenerate Warehouse For Update Scheduler

    Replace Sophos URLS to Localhost
    Simulate Update Now

    Wait Until Keyword Succeeds
    ...  50 secs
    ...  5 secs
    ...  SulDownloader Should Report Update Success

    ${eventPath} =  Check Event File Generated  120
    Check Event Report Success  ${eventPath}
    File Should Not Exist  ${statusPath}

Update Scheduler Ignores Non Report Files When Processing Reports

    ${config} =    Create JSON Config    install_path=${tmpdir}/sspl
    Create File    ${tmpdir}/NotAReportFile.json    content=${config}
    Copy File   ${tmpdir}/NotAReportFile.json   ${SOPHOS_INSTALL}/base/update/var/config.json

    Create File    ${tmpdir}/RandomName.jpeg    content=${config}
    Copy File   ${tmpdir}/RandomName.jpeg   ${SOPHOS_INSTALL}/base/update/var/RandomName.jpeg

    Set Log Level For Component And Reset And Return Previous Log  suldownloader  DEBUG

    Simulate Send Policy And Run Update  ${BasePolicy}
    ${eventPath} =  Check Status and Events Are Created  waitTime=120 secs

    Log File  /opt/sophos-spl/logs/base/suldownloader.log

    Check Event Report Success  ${eventPath}
    Check Log Does Not Contain   Failed to process file: /opt/sophos-spl/base/update/var/config.json  ${SOPHOS_INSTALL}/logs/base/sophosspl/updatescheduler.log   Update Scheduler Log

UpdateScheduler Can Detect SulDownloader Service Error
    # After install replace Sul Downloader with a fake version for test, then run SulDownloader in the same way UpdateScheduler does
    # Check if the expected return code is given.

    # Test is designed to prove the contract for the check will work across all platforms.

    Replace Sul Downloader With Fake Broken Version
    Run Process   /bin/systemctl  start  sophos-spl-update.service
    ${result} =    Run Process   /bin/systemctl  is-failed  sophos-spl-update.service
    Should Be Equal As Integers  ${result.rc}  0  msg="Failed to detect sophos-spl-update.service error"

UpdateScheduler Can Detect SulDownloader Service Runs Without Error
    # After install directly run SulDownloader in the same way UpdateScheduler does
    # Check if the expected return code is given.

    # Test is designed to prove the contract for the check will work across all platforms.

    Run Process   /bin/systemctl  start  sophos-spl-update.service
    ${result} =    Run Process   /bin/systemctl  is-failed  sophos-spl-update.service
    Should Be Equal As Integers  ${result.rc}  1  msg="Failed to detect sophos-spl-update.service error"

UpdateScheduler Can Detect SulDownloader Service Runs Without Error After Error Reported
    # After install replace Sul Downloader with a fake version for test, then run SulDownloader in the same way UpdateScheduler does
    # Check if the expected return code is given.

    # Test is designed to prove the contract for the check will work across all platforms.

    Copy File  ${SOPHOS_INSTALL}/base/bin/SulDownloader.0  ${SOPHOS_INSTALL}/base/bin/SulDownloader.0.bak

    Replace Sul Downloader With Fake Broken Version
    ${startresult} =  Run Process   /bin/systemctl  start  sophos-spl-update.service
    ${result} =    Run Process   /bin/systemctl  is-failed  sophos-spl-update.service
    Should Be Equal As Integers  ${result.rc}  0  msg="Detected error in sophos-spl-update.service error. stdout: ${result.stdout} stderr: ${result.stderr}. Start stdout: ${startresult.stdout}. stderr: ${startresult.stderr}"

    Replace Original Sul Downloader

    ${startresult} =  Run Process   /bin/systemctl  start  sophos-spl-update.service
    ${result} =    Run Process   /bin/systemctl  is-failed  sophos-spl-update.service
    Should Be Equal As Integers  ${result.rc}  1  msg="Failed to detect sophos-spl-update.service error. stdout: ${result.stdout} stderr: ${result.stderr}. Start stdout: ${startresult.stdout}. stderr: ${startresult.stderr}"

*** Keywords ***

SulDownloader Should Report Verification Failed
    ${sullog} =   Get File  ${logpath}
    Should Contain   ${sullog}   failed signature verification

SulDownloader Should Report Update Success
    ${sullog} =   Get File  ${logpath}
    Should Contain   ${sullog}   Update success

Display Permissions of SulDownloader Files
    ${result} =    Run Process   ls -lZ /opt/sophos-spl/base/bin/ /opt/sophos-spl/base/update/var     shell=yes
    Log   ${result.stdout}

Setup For Test
    Setup Servers For Update Scheduler
    Empty Directory  ${SOPHOS_INSTALL}/base/mcs/event

Teardown For Test
    Run Keyword If Test Failed   Display Permissions of SulDownloader Files
    Teardown Servers For Update Scheduler

Replace Original Sul Downloader
    Remove File  ${SOPHOS_INSTALL}/base/bin/SulDownloader.0
    Copy File  ${SOPHOS_INSTALL}/base/bin/SulDownloader.0.bak  ${SOPHOS_INSTALL}/base/bin/SulDownloader.0
