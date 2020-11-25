*** Settings ***
Documentation    Helper tests to prove that FakeSulDownloader can be used. It is used in effect in the UpdateSchedulerPlugin suite.

Suite Setup     Require Fresh Install

Test Setup      Stop SPPL Services
Test Teardown   Fake SulDownloader TearDown

Library    Process
Library    OperatingSystem
Library    ${LIBS_DIRECTORY}/FakeSulDownloader.py
Resource  ../installer/InstallerResources.robot
Resource  ../GeneralTeardownResource.robot

Default Tags  SULDOWNLOADER

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
    Setup Base and Plugin Upgraded
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
    Setup Base and Plugin Sync and UpToDate
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
    Setup Base and Plugin Sync and UpToDate  startTime=2
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




Stop SPPL Services
    Run Process  /bin/systemctl  stop  sophos-spl
