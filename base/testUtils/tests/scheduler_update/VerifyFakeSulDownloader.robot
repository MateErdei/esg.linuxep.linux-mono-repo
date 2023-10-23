*** Settings ***
Documentation    Helper tests to prove that FakeSulDownloader can be used. It is used in effect in the UpdateSchedulerPlugin suite.

Suite Setup     Require Fresh Install

Test Setup      Stop SPPL Services
Test Teardown   Fake SulDownloader TearDown

Library    Process
Library    OperatingSystem
Library    ${LIBS_DIRECTORY}/FakeSulDownloader.py

Resource    ${COMMON_TEST_ROBOT}/GeneralTeardownResource.robot
Resource    ${COMMON_TEST_ROBOT}/InstallerResources.robot

Force Tags  SULDOWNLOADER  TAP_PARALLEL1

*** Test Cases ***
FakeSulDownloader Setup Done File
    Setup Done File
    ${result} =    Run Process    ${SUL_DOWNLOADER}
    Log    "stdout = ${result.stdout}"
    Log    "stderr = ${result.stderr}"
    File Should Exist  ${SOPHOS_INSTALL}/tmp/done


FakeSulDownloader CopyFile
    Setup Copy File  content=Test  sleeptime=1
    ${result} =    Run Process    ${SUL_DOWNLOADER}
    Log    "stdout = ${result.stdout}"
    Log    "stderr = ${result.stderr}"

    File Should Exist  ${SOPHOS_INSTALL}/base/update/var/updatescheduler/update_report.json
    ${content}=  Get File  ${SOPHOS_INSTALL}/base/update/var/updatescheduler/update_report.json
    Should Be Equal  ${content}  Test


FakeSulDownloader Base and Plugin Upgraded
    @{features}=  Create List   CORE
    Setup Base and Plugin Upgraded  ${features}
    ${result} =    Run Process    ${SUL_DOWNLOADER}
    Log    "stdout = ${result.stdout}"
    Log    "stderr = ${result.stderr}"

    File Should Exist  ${SOPHOS_INSTALL}/base/update/var/updatescheduler/update_report.json
    ${content}=  Get File  ${SOPHOS_INSTALL}/base/update/var/updatescheduler/update_report.json
    Log    ${content}
    Should Contain  ${content}  SUCCESS
    Should Contain  ${content}  ServerProtectionLinux-Base
    Should Contain  ${content}  ServerProtectionLinux-Plugin
    Should Contain  ${content}  UPGRADED


FakeSulDownloader Base and Plugin UpToDate
    @{features}=  Create List   CORE
    Setup Base and Plugin Sync and UpToDate  ${features}
    ${result} =    Run Process    ${SUL_DOWNLOADER}
    Log    "stdout = ${result.stdout}"
    Log    "stderr = ${result.stderr}"

    File Should Exist  ${SOPHOS_INSTALL}/base/update/var/updatescheduler/update_report.json
    ${content}=  Get File  ${SOPHOS_INSTALL}/base/update/var/updatescheduler/update_report.json
    Log    ${content}
    Should Contain  ${content}  SUCCESS
    Should Contain  ${content}  ServerProtectionLinux-Base
    Should Contain  ${content}  ServerProtectionLinux-Plugin
    Should Contain  ${content}  UPTODATE

FakeSulDownloader Plugin Install Failed
    Setup Plugin Install Failed
    ${result} =    Run Process    ${SUL_DOWNLOADER}
    ${content}=  Get File  ${SOPHOS_INSTALL}/base/update/var/updatescheduler/update_report.json
    Log    ${content}
    Should Contain  ${content}  ServerProtectionLinux-Base
    Should Contain  ${content}  ServerProtectionLinux-Plugin
    Should Contain  ${content}  INSTALLFAILED


FakeSulDownloader StartTime respected
    @{features}=  Create List   CORE
    Setup Base and Plugin Sync and UpToDate  ${features}  startTime=2
    ${result} =    Run Process    ${SUL_DOWNLOADER}
    ${content}=  Get File  ${SOPHOS_INSTALL}/base/update/var/updatescheduler/update_report.json
    Log    ${content}
    Should Contain  ${content}  20180822
    File Should Exist  ${SOPHOS_INSTALL}/tmp/fakesul.log


*** Keywords ***
List With Links BaseBin
    ${result} =    Run Process    ls  -l  ${SOPHOS_INSTALL}/base/bin
    Log  ${result.stdout}



Fake SulDownloader TearDown
    General Test Teardown
    Run Keyword If Test Failed  Log File  ${SOPHOS_INSTALL}/tmp/fakesul.log
    Run Keyword If Test Failed  Log File  ${SOPHOS_INSTALL}/tmp/fakesul.py
    Run Keyword If Test Failed  List With Links BaseBin
    Run Keyword If Test Failed  Log File  ${SOPHOS_INSTALL}/base/bin/FakeSulDownloader.py

Restore Normal Suldowloader
    Run Process  unlink  ${SUL_DOWNLOADER}
    Run Process  ln -sf  ${SUL_DOWNLOADER}.0   ${SUL_DOWNLOADER}

Stop SPPL Services
    Run Process  /bin/systemctl  stop  sophos-spl
