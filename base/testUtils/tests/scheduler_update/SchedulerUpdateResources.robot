*** Settings ***

Library    Process
Library    OperatingSystem
Library    String
Library    ${LIBS_DIRECTORY}/FakeSulDownloader.py
Library    ${LIBS_DIRECTORY}/UpdateSchedulerHelper.py
Library    ${LIBS_DIRECTORY}/CentralUtils.py
Library    ${LIBS_DIRECTORY}/WarehouseGenerator.py
Library    ${LIBS_DIRECTORY}/UpdateServer.py
Library    ${LIBS_DIRECTORY}/WarehouseUtils.py

Resource  ../installer/InstallerResources.robot
Resource  ../watchdog/LogControlResources.robot
Resource  ../management_agent/ManagementAgentResources.robot
Resource  ../GeneralTeardownResource.robot

*** Variables ***
${UPDATE_SCHEDULER_BINARY_NAME}  UpdateScheduler
${tmpdir}                       tmp/SDT
${statusPath}  ${SOPHOS_INSTALL}/base/mcs/status/ALC_status.xml
${SULDOWNLOADER_LOG_PATH}           ${SOPHOS_INSTALL}/logs/base/suldownloader.log


*** Keywords ***

Send Policy To UpdateScheduler
    [Arguments]   ${originalPolicyName}  &{kwargs}
    Register Current Sul Downloader Config Time
    Simulate Send Policy   ${originalPolicyName}  &{kwargs}
    Wait For New Sul Downloader Config File Created
    Log File   ${UPDATE_CONFIG}

Send Policy With Cache
    [Arguments]    &{kwargs}
    Remove File  ${UPDATECACHE_CERT_PATH}
    Send Policy To UpdateScheduler  ALC_policy_with_cache.xml  &{kwargs}
    Wait Until Keyword Succeeds
    ...  60 secs
    ...  3 secs
    ...  Check CacheCertificates Is Created Correctly

Send Policy With No Cache And No Proxy
    Send Policy To UpdateScheduler  ALC_policy_direct_just_base.xml

Send Mock Flags Policy
    [Arguments]    ${fileContents}={"livequery.network-tables.available": true, "scheduled_queries.next": true, "sdds3.enabled": true}
    Create File    /tmp/flags.json    ${fileContents}

    Copy File    /tmp/flags.json  /opt/NotARealFile
    Run Process  chown  sophos-spl-local:sophos-spl-group  /opt/NotARealFile
    Run Process  chmod  640  /opt/NotARealFile
    Move File    /opt/NotARealFile  ${SOPHOS_INSTALL}/base/mcs/policy/flags.json

Stop Update Scheduler
    ${wdctl} =   Set Variable  ${SOPHOS_INSTALL}/bin/wdctl
    ${result} =    Run Process    ${wdctl}    stop  updatescheduler
    Should Be Equal As Integers  ${result.rc}  0  msg="Failed to stop update scheduler ${result.stdout} ${result.stderr}"
    Sleep  1 second

Start Update Scheduler
    ${wdctl} =   Set Variable  ${SOPHOS_INSTALL}/bin/wdctl
    ${result} =    Run Process    ${wdctl}    start  updatescheduler
    Should Be Equal As Integers  ${result.rc}  0  msg="Failed to start update scheduler ${result.stdout} ${result.stderr}"
    Sleep  1 second

Stop Management Agent Via WDCTL
    ${wdctl} =   Set Variable  ${SOPHOS_INSTALL}/bin/wdctl
    ${result} =    Run Process    ${wdctl}    stop  managementagent
    Should Be Equal As Integers  ${result.rc}  0  msg="Failed to stop management agent ${result.stdout} ${result.stderr}"
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  Check Managementagent Not Running

Start Management Agent Via WDCTL
    ${wdctl} =   Set Variable  ${SOPHOS_INSTALL}/bin/wdctl
    ${result} =    Run Process    ${wdctl}    start  managementagent
    Should Be Equal As Integers  ${result.rc}  0  msg="Failed to start management agent ${result.stdout} ${result.stderr}"
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  Check Management Agent Running And Ready

Restart Update Scheduler
    Stop Update Scheduler
    Start Update Scheduler

Setup Update Scheduler Environment
    Require Fresh Install
    Set Test Variable  ${statusPath}  ${SOPHOS_INSTALL}/base/mcs/status/ALC_status.xml
    Send Mock Flags Policy
    Disable mcsrouter
    setup mcs config with JWT token

Setup Update Scheduler Environment and Stop All Services
    Setup Update Scheduler Environment
    Stop Update Scheduler
    Stop Management Agent Via WDCTL

setup mcs config with JWT token
    create file    ${SOPHOS_INSTALL}/base/etc/sophosspl/mcs.config    jwt_token=stuff
    Run Process  chmod  640  ${SOPHOS_INSTALL}/base/etc/sophosspl/mcs.config
    Run Process  chown  sophos-spl-local:sophos-spl-group  ${SOPHOS_INSTALL}/base/etc/sophosspl/mcs.config

Setup Current Update Scheduler Environment
    Require Fresh Install
    Set Test Variable  ${statusPath}  ${SOPHOS_INSTALL}/base/mcs/status/ALC_status.xml
    Send Mock Flags Policy
    Disable mcsrouter
    setup mcs config with JWT token
    Create Empty Config File To Stop First Update On First Policy Received
    Set Log Level For Component And Reset and Return Previous Log  sophos_managementagent   DEBUG
    Set Log Level For Component And Reset and Return Previous Log  updatescheduler   DEBUG
    Send Policy With Cache

Setup Current Update Scheduler Environment Without Policy
    Require Fresh Install
    Set Test Variable  ${statusPath}  ${SOPHOS_INSTALL}/base/mcs/status/ALC_status.xml
    Send Mock Flags Policy
    Disable mcsrouter
    setup mcs config with JWT token

Setup Current Update Scheduler Environment Without Flags
    Require Fresh Install
    Set Test Variable  ${statusPath}  ${SOPHOS_INSTALL}/base/mcs/status/ALC_status.xml
    Disable mcsrouter
    setup mcs config with JWT token

Check Status and Events Are Created
    [Arguments]   ${waitTime}=10 secs  ${attemptsTime}=1 secs

   Check Status Is Created    ${waitTime}  ${attemptsTime}

    @{words} =  Split String    ${waitTime}
    ${eventPath} =  Check Event File Generated  ${words}[0]
    LogFile  ${eventPath}
    LogFile  ${statusPath}
    [Return]  ${eventPath}

Check Status Is Created
    [Arguments]   ${waitTime}=10 secs  ${attemptsTime}=1 secs

    Wait Until Keyword Succeeds
    ...  ${waitTime}
    ...  ${attemptsTime}
    ...  File Should Exist  ${statusPath}
    LogFile  ${statusPath}
    [Return]  ${statusPath}

Simulate Update Now
    Empty Directory  ${SOPHOS_INSTALL}/base/mcs/event
    Copy File   ${SUPPORT_FILES}/CentralXml/ALC_update_now.xml  ${SOPHOS_INSTALL}/tmp
    ${result} =  Run Process  chown sophos-spl-user:sophos-spl-group ${SOPHOS_INSTALL}/tmp/ALC_update_now.xml    shell=True
    Should Be Equal As Integers    ${result.rc}    0  Failed to replace permission to file. Reason: ${result.stderr}
    Move File   ${SOPHOS_INSTALL}/tmp/ALC_update_now.xml  ${SOPHOS_INSTALL}/base/mcs/action/ALC_action_1.xml

Check Event Report Success
    [Arguments]   ${eventPath}
    ${fileContent}  Get File  ${eventPath}
    Should Contain  ${fileContent}  <number>0</number>  msg="Event does not correspond to a success"

Check Event Report Install Failed
    [Arguments]   ${eventPath}
    ${fileContent}  Get File  ${eventPath}
    Should Contain  ${fileContent}  <number>103</number>  msg="Event does not correspond to an install failed (103 error number)"

Check Status Report Contain
    [Arguments]  ${expectedContent}  ${ErrorMessage}="Status does not contain expected content"
    ${fileContent}  Get File  ${statusPath}
    Should Contain  ${fileContent}  ${expectedContent}  msg="${ErrorMessage}. Looked for: {${expectedContent}} inside Status xml."

Check Status Report Does Not Contain
    [Arguments]  ${shouldNotBeInStatus}  ${ErrorMessage}="Status contains unexpected content"
    ${fileContent}  Get File  ${statusPath}
    Should Not Contain  ${fileContent}  ${shouldNotBeInStatus}  msg="${ErrorMessage}. Checked for: {${shouldNotBeInStatus}} inside Status xml."

Check CacheCertificates Is Created Correctly
    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  File Should Exist  ${UPDATECACHE_CERT_PATH}
    ${fileContent}  Get File  ${UPDATECACHE_CERT_PATH}
    Should Contain  ${fileContent}  MIIDxDCCAqygAwIBAgIQE  msg="Certificate content differs from expected"


Setup Warehouse For Base
    Generate Update Certs
    ${dist} =  Get Folder With Installer

    Copy Directory  ${dist}  ${tmpdir}/TestInstallFiles/ServerProtectionLinux-Base
    Create File   ${tmpdir}/TestInstallFiles/ServerProtectionLinux-Base/VERSION.ini   PRODUCT_NAME = Sophos Server Protection Linux - Base Component\nPRODUCT_VERSION = 9.9.9.999\nBUILD_DATE = 2020-11-09
    log file  ${tmpdir}/TestInstallFiles/ServerProtectionLinux-Base/VERSION.ini

    Add Component Warehouse Config   ServerProtectionLinux-Base   ${tmpdir}/TestInstallFiles/    ${tmpdir}/temp_warehouse/   ServerProtectionLinux-Base
    Generate Warehouse

    Start Update Server    1233    ${tmpdir}/temp_warehouse//customer_files/
    Start Update Server    1234    ${tmpdir}/temp_warehouse/warehouses/sophosmain/
    Can Curl Url    https://localhost:1234/catalogue/sdds.ServerProtectionLinux-Base.xml
    Can Curl Url    https://localhost:1233

Setup Warehouse For Mdr
    Generate Update Certs
    ${base_dist} =  Get Folder With Installer
    ${dist} =  get_sspl_mdr_plugin_sdds

    Remove Directory  ${tmpdir}/TestInstallFiles/  recursive=${TRUE}
    Remove Directory  ${tmpdir}/temp_warehouse/  recursive=${TRUE}

    Copy Directory     ${base_dist}  ${tmpdir}/TestInstallFiles/${BASE_RIGID_NAME}
    Copy Directory  ${dist}  ${tmpdir}/TestInstallFiles/ServerProtectionLinux-Plugin-MDR
    Create File   ${tmpdir}/TestInstallFiles/ServerProtectionLinux-Base/VERSION.ini   PRODUCT_NAME = Sophos Server Protection Linux - Base Component\nPRODUCT_VERSION = 9.9.9.999\nBUILD_DATE = 2020-11-09
    log file  ${tmpdir}/TestInstallFiles/ServerProtectionLinux-Base/VERSION.ini

    Add Component Warehouse Config   ${BASE_RIGID_NAME}   ${tmpdir}/TestInstallFiles/    ${tmpdir}/temp_warehouse/   ${BASE_RIGID_NAME}  Warehouse1
    Add Component Warehouse Config   ServerProtectionLinux-Plugin-MDR   ${tmpdir}/TestInstallFiles/    ${tmpdir}/temp_warehouse/   ServerProtectionLinux-Plugin-MDR    Warehouse1
    Generate Warehouse    MDR_FEATURES=MDR

    Start Update Server    1233    ${tmpdir}/temp_warehouse//customer_files/
    Start Update Server    1234    ${tmpdir}/temp_warehouse/warehouses/sophosmain/
    Can Curl Url    https://localhost:1234/catalogue/sdds.ServerProtectionLinux-Plugin-MDR.xml
    Can Curl Url    https://localhost:1233

Setup Warehouse For Mdr 060
    Generate Update Certs
    ${base_dist} =  Get Folder With Installer
    ${dist} =  get_sspl_mdr_plugin_060_sdds

    Remove Directory  ${tmpdir}/TestInstallFiles/  recursive=${TRUE}
    Remove Directory  ${tmpdir}/temp_warehouse/  recursive=${TRUE}


    Copy Directory     ${base_dist}  ${tmpdir}/TestInstallFiles/${BASE_RIGID_NAME}
    Copy Directory  ${dist}  ${tmpdir}/TestInstallFiles/ServerProtectionLinux-Plugin-MDR
    Create File   ${tmpdir}/TestInstallFiles/ServerProtectionLinux-Base/VERSION.ini   PRODUCT_NAME = Sophos Server Protection Linux - Base Component\nPRODUCT_VERSION = 9.9.9.999\nBUILD_DATE = 2020-11-09
    log file  ${tmpdir}/TestInstallFiles/ServerProtectionLinux-Base/VERSION.ini

    Add Component Warehouse Config   ${BASE_RIGID_NAME}   ${tmpdir}/TestInstallFiles/    ${tmpdir}/temp_warehouse/   ${BASE_RIGID_NAME}  Warehouse1
    Add Component Warehouse Config   ServerProtectionLinux-Plugin-MDR   ${tmpdir}/TestInstallFiles/    ${tmpdir}/temp_warehouse/   ServerProtectionLinux-Plugin-MDR    Warehouse1
    Generate Warehouse    MDR_FEATURES=MDR

    Start Update Server    1233    ${tmpdir}/temp_warehouse//customer_files/
    Start Update Server    1234    ${tmpdir}/temp_warehouse/warehouses/sophosmain/
    Can Curl Url    https://localhost:1234/catalogue/sdds.ServerProtectionLinux-Plugin-MDR.xml
    Can Curl Url    https://localhost:1233



Setup Servers For Update Scheduler
    Setup Current Update Scheduler Environment
    Setup Warehouse For Base

Setup For Test With Warehouse Containing Base and MDR
    Setup Environment Before Warehouse Generation
    Setup Warehouse For MDR
    Setup Environment After Warehouse Generation

Setup Environment Before Warehouse Generation
    Setup Update Scheduler Environment
    Create Directory    ${tmpdir}

Setup Environment After Warehouse Generation
    [Arguments]  ${suldownloader_log_level}=DEBUG
    Set Log Level For Component Plus Subcomponent And Reset and Return Previous Log  updatescheduler   DEBUG    suldownloader=${suldownloader_log_level}
    Create Empty Config File To Stop First Update On First Policy Received
    Stop Update Scheduler

    Copy file  ${SUPPORT_FILES}/sophos_certs/ps_rootca.crt   ${SOPHOS_INSTALL}/base/update/rootcerts/
    Copy file  ${SUPPORT_FILES}/sophos_certs/rootca.crt   ${SOPHOS_INSTALL}/base/update/rootcerts/
    Copy File   ${SUPPORT_FILES}/https/ca/root-ca.crt.pem    ${SUPPORT_FILES}/https/ca/root-ca.crt
    Install System Ca Cert  ${SUPPORT_FILES}/https/ca/root-ca.crt

    Start Update Scheduler
    Regenerate Certificates
    Set Local CA Environment Variable


Setup For Test With Warehouse Containing Base
    Setup Environment Before Warehouse Generation
    Setup Warehouse For Base
    Setup Environment After Warehouse Generation


Log And Remove SulDownloader Log
    ${logpath} =  Set Variable  ${SOPHOS_INSTALL}/logs/base/suldownloader.log
    Run Keyword and Ignore Error   Log File  ${logpath}
    Remove File  ${logpath}

SulDownloader Reports Finished
    ${content} =  Get File   ${SOPHOS_INSTALL}/logs/base/suldownloader.log
    Should Contain  ${content}   Generating the report file

Wait for SulDownloader Report
    Wait Until Keyword Succeeds
    ...  60
    ...  1
    ...  SulDownloader Reports Finished

Send Policy With Host Redirection And Run Update And Return Event Path
    [Arguments]    ${Policy}  &{kwargs}
    Send Policy With Host Redirection And Run Update  ${Policy}  &{kwargs}
    ${eventPath} =  Check Status and Events Are Created  waitTime=20 secs
    [Return]  ${eventPath}

Send Policy With Host Redirection And Run Update
    [Arguments]    ${Policy}   &{kwargs}
    Remove File   ${statusPath}
    Log And Remove SulDownloader Log
    Send Policy To UpdateScheduler  ${Policy}  &{kwargs}
    Replace Sophos URLS to Localhost
    Simulate Update Now
    Wait Until Keyword Succeeds
    ...  200 secs
    ...  10 secs
    ...  SulDownloader Reports Finished

Simulate Send Policy And Run Update And Return Event Path
    [Arguments]  ${Policy}  &{kwargs}
    Simulate Send Policy And Run Update  ${Policy}  &{kwargs}
    ${eventPath} =  Check Status and Events Are Created  waitTime=20 secs
    [Return]  ${eventPath}


Simulate Send Policy And Run Update
    [Arguments]  ${Policy}  &{kwargs}
    Remove File   ${statusPath}
    Prepare Installation For Upgrade Using Policy  ${Policy}
    Send Policy To UpdateScheduler  ${Policy}  &{kwargs}
    Simulate Update Now
    Wait Until Keyword Succeeds
    ...  300 secs
    ...  5 secs
    ...  SulDownloader Reports Finished

Simulate Send Policy And Simulate Update Now
    [Arguments]  ${Policy}  &{kwargs}
    Remove File   ${statusPath}
    Prepare Installation For Upgrade Using Policy  ${Policy}
    Send Policy To UpdateScheduler  ${Policy}  &{kwargs}
    Simulate Update Now

Check Update Success
    [Arguments]    ${eventPath}
    Check Event Report Success  ${eventPath}
    Log File   ${eventPath}

Send Policy With Host Redirection And Run Update And Check Success
    [Arguments]    &{kwargs}
    ${eventPath} =  Send Policy With Host Redirection And Run Update And Return Event Path  &{kwargs}
    Check Update Success  ${eventPath}
    [Return]  ${eventPath}

Send Policy With Host Redirection And Run Update And Check Success Of Oldest Event
    [Arguments]    &{kwargs}
    Send Policy With Host Redirection And Run Update And Return Event Path  ALC_policy_local_warehouse_mdr.xml  &{kwargs}
    ${eventPath} =  Get Oldest File In Directory  /opt/sophos-spl/base/mcs/event/
    Check Update Success  ${eventPath}
    [Return]  ${eventPath}

Get Oldest File In Directory
    [Arguments]    ${directory}
    @{files} = 	List Files In Directory  ${directory}
    ${earliestModifiedFile} =   Get From List   ${files}    0
    ${time1} =  OperatingSystem.Get Modified Time       ${directory}/${earliestModifiedFile}    epoch
    FOR    ${file}    IN    @{files}
        ${time}    Get Modified Time   ${directory}/${file}    epoch
        ${earliestModifiedFile} =   Set Variable If    ${time1} > ${time}    ${directory}/${file}    ${earliestModifiedFile}
        ${time1} =    Set Variable If    ${time1} > ${time}    ${time}    ${time1}
    END
    [Return]  ${directory}/${earliestModifiedFile}

Simulate Send Policy And Run Update And Check Success
    [Arguments]  ${Policy}  &{kwargs}
    ${eventPath} =  Simulate Send Policy And Run Update And Return Event Path  ${Policy}  &{kwargs}
    Check Update Success  ${eventPath}
    [Return]  ${eventPath}

Check ALC Status Sent To Central Contains MDR Subscription
    Check Status Report Contain  rigidName\=\"ServerProtectionLinux-Plugin-MDR\"


Regenerate Warehouse For Update Scheduler
    [Arguments]   &{kwargs}
    Stop Update Server
    Remove Directory    ${tmpdir}/temp_warehouse   Recursive=True
    Clear Warehouse Config
    Add Component Warehouse Config   ServerProtectionLinux-Base   ${tmpdir}/TestInstallFiles/    ${tmpdir}/temp_warehouse/   ServerProtectionLinux-Base
    Generate Warehouse  &{kwargs}

    Start Update Server    1233    ${tmpdir}/temp_warehouse//customer_files/
    Start Update Server    1234    ${tmpdir}/temp_warehouse/warehouses/sophosmain/
    Can Curl Url    https://localhost:1234/catalogue/sdds.ServerProtectionLinux-Base.xml
    Can Curl Url    https://localhost:1233


Log Settings Files
    Run Keyword And Ignore Error   Log File  ${tmpdir}/temp_warehouse/ServerProtectionLinux-Base/sdds/SDDS-Import.xml
    Run Keyword And Ignore Error   Log File  ${tmpdir}/temp_warehouse/ServerProtectionLinux-Plugin-EventProcessor/sdds/SDDS-Import.xml
    Run Keyword And Ignore Error   Log File  ${tmpdir}/temp_warehouse/ServerProtectionLinux-Plugin-AuditPlugin/sdds/SDDS-Import.xml
    Run Keyword And Ignore Error   Log File  ${SOPHOS_INSTALL}/base/mcs/policy/ALC-1_policy.xml
    Run Keyword And Ignore Error   Log File  ${statusPath}

Log SystemCtl Update Status
    ${result}=  Run Process   systemctl  status  sophos-spl-update
    Log  ${result.stdout}
    Log  ${result.stderr}


Teardown Servers For Update Scheduler
    General Test Teardown
    Log SystemCtl Update Status
    Stop Update Server
    Run Process    make   cleanCerts    cwd=${SUPPORT_FILES}/CloudAutomation/
    Run Keyword If Test Failed    Display All tmp Files Present
    Run Keyword If Test Failed    Log File  /etc/hosts
    Run Keyword And Ignore Error  Move File  /etc/hosts.bk  /etc/hosts
    Run Keyword If Test Failed    Log Settings Files
    Run Keyword If Test Failed    Run Keyword and Ignore Error  Log File  ${UPDATE_CONFIG}
    Run Keyword If Test Failed    Log Report Files
    Run Keyword If Test Failed    Run Keyword and Ignore Error  Log File  ${SOPHOS_INSTALL}/tmp/fakesul.log
    Remove Directory   ${tmpdir}  recursive=True


Check MDR Installed
    Directory Should Exist   ${SOPHOS_INSTALL}/plugins/mtr/bin
    File Should Exist   ${SOPHOS_INSTALL}/plugins/mtr/bin/mtr
    Directory Should Exist   ${SOPHOS_INSTALL}/plugins/mtr/dbos/data
    Directory Should Exist   ${SOPHOS_INSTALL}/plugins/mtr/var/policy
    File Should Exist   ${SOPHOS_INSTALL}/plugins/mtr/dbos/SophosMTR
    File Should Exist   ${SOPHOS_INSTALL}/plugins/mtr/dbos/data/certificate.crt
    Check MDR Plugin Installed

Check Sensors Not Installed
    Directory Should Not Exist   ${SOPHOS_INSTALL}/plugins/AuditPlugin/bin
    Directory Should Not Exist   ${SOPHOS_INSTALL}/plugins/EventProcessor/bin

Teardown For Test
    Run Keyword If Test Failed   Log Settings Files
    Teardown Servers For Update Scheduler
    Remove Directory   ${tmpdir}    recursive=True
    Run Process  unlink  ${SUL_DOWNLOADER}
    Run Process  ln -sf  ${SUL_DOWNLOADER}.0   ${SUL_DOWNLOADER}


Check Event Source
    [Arguments]   ${eventPath}   ${expectedSource}
    ${fileContent}  Get File  ${eventPath}
    Should Contain  ${fileContent}  ${expectedSource}  msg="Event does not report Source: ${expectedSource}"

Check Update Scheduler Run as Sophos-spl-user
    ${Pid} =   Run Process   pidof UpdateScheduler   shell=yes
    ${UpdateSchedulerProcess} =  Run Process   ps -o user\= -p ${Pid.stdout}    shell=yes
    Should Contain   ${UpdateSchedulerProcess.stdout}   sophos-spl-updatescheduler   msg="Failed to detect process ownership of UpdateScheduler. Expected it to run as sophos-spl-updatescheduler. ${UpdateSchedulerProcess.stdout}. Err: ${UpdateSchedulerProcess.stderr}"


Management Agent Contains
    [Arguments]  ${Contents}
    ${ManagementAgentLog} =    Get File  /opt/sophos-spl/logs/base/sophosspl/sophos_managementagent.log
    Should Contain  ${ManagementAgentLog}  ${Contents}

Configure Hosts File
    Copy File  /etc/hosts  /etc/hosts.bk
    Append To File  /etc/hosts  127.0.0.1 dci.sophosupd.net\n127.0.0.1 dci.sophosupd.com\n
    Append To File  /etc/hosts  127.0.0.1 d1.sophosupd.net\n127.0.0.1 d1.sophosupd.com\n
    Append To File  /etc/hosts  127.0.0.1 d2.sophosupd.net\n127.0.0.1 d2.sophosupd.com\n
    Append To File  /etc/hosts  127.0.0.1 d3.sophosupd.net\n127.0.0.1 d3.sophosupd.com
    Append To File  /etc/hosts  127.0.0.1 es-web.sophos.com\n



Check report was a product update
    [Arguments]  ${reportPath}
    ${contents} =    Get File  ${reportPath}
    Should Contain  ${contents}  "supplementOnlyUpdate": false

Check report was supplement-only update
    [Arguments]  ${reportPath}
    ${contents} =    Get File  ${reportPath}
    Should Contain  ${contents}  "supplementOnlyUpdate": true

Check Sul Downloader log contains
    [Arguments]  ${contents}
    Check Log Contains   ${contents}    ${SULDOWNLOADER_LOG_PATH}   ${SULDOWNLOADER_LOG_PATH}

Fail Update Install And Check Status Shows Good Install State
    Remove File    ${SOPHOS_INSTALL}/base/mcs/status/ALC_status.xml
    Setup Plugin Install Failed  startTime=2
    Send Policy To UpdateScheduler  ALC_policy_direct.xml
    Simulate Update Now
    Wait Until Keyword Succeeds
    ...  1 minutes
    ...  5 secs
    ...  Should Exist  ${SOPHOS_INSTALL}/base/mcs/status/ALC_status.xml

    ${StatusContent} =  Get File  ${SOPHOS_INSTALL}/base/mcs/status/ALC_status.xml
    Should Contain  ${StatusContent}  <downloadState><state>good</state></downloadState>
    Should Contain  ${StatusContent}  <installState><state>good</state><lastGood>
    Should Contain  ${StatusContent}  </lastGood></installState>
    Should Not Contain  ${StatusContent}  <installState><state>bad</state><failedSince>

Fail Update Install And Check Status Not ReGenerated
    Remove File    ${SOPHOS_INSTALL}/base/mcs/status/ALC_status.xml
    Setup Plugin Install Failed  startTime=2
    Send Policy To UpdateScheduler  ALC_policy_direct.xml
    Simulate Update Now
    Sleep  5
    Should Not Exist   ${SOPHOS_INSTALL}/base/mcs/status/ALC_status.xml

Fail Update Install And Check Status Shows Bad Install State
    Setup Plugin Install Failed  startTime=2
    Send Policy To UpdateScheduler  ALC_policy_direct.xml
    Simulate Update Now
    Wait Until Keyword Succeeds
    ...  1 minutes
    ...  5 secs
    ...  Check Status Contains Bad State


Check Status Contains Bad State
    ${StatusContent} =  Get File  ${SOPHOS_INSTALL}/base/mcs/status/ALC_status.xml
    Should Contain  ${StatusContent}  <downloadState><state>good</state></downloadState>
    Should Not Contain  ${StatusContent}  <installState><state>good</state><lastGood>
    Should Contain  ${StatusContent}  <installState><state>bad</state><failedSince>
    # not checking actual value of lastGood because this is a time stamp that will change.
    Should Contain  ${StatusContent}   </failedSince></installState>

Convert report to success
    [Arguments]  ${reportPath}
    ${contents} =    Get File  ${reportPath}
    ${contents} =  Replace String  ${contents}  "status": "CONNECTIONERROR",  "status": "SUCCESS",
    ${contents} =  Replace String  ${contents}  "errorDescription": "Failed to connect to warehouse",  "errorDescription": "",
    Create File   ${reportPath}  ${contents}