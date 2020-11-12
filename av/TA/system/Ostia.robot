*** Settings ***
Documentation    Updating from Ostia

Library         ../Libs/WarehouseUtils.py
Library         ../Libs/LogUtils.py
Library         ../Libs/OSUtils.py
Library         ../Libs/PathManager.py
Library         OperatingSystem
Library         String

Resource        ../shared/GlobalSetup.robot
Resource        ../shared/AVResources.robot

Suite Setup     No Operation
Suite Teardown  Global Teardown Tasks

Test Setup      No Operation
Test Teardown   Ostia Cleanup

*** Keywords ***

Ostia Cleanup
    Run Keyword If Test Failed  Display All SSPL Files Installed
    Run Keyword If Test Failed  dump_all_mcs_events
    Run Keyword If Test Failed  dump_watchdog_log
    Run Keyword If Test Failed  dump_managementagent_log
    Run Keyword If Test Failed  dump_suldownloader_log
    Run Keyword If Test Failed   Run Keyword And Ignore Error  Log File   ${AV_LOG_PATH}
    Run Keyword And Ignore Error  Remove File    ${AV_LOG_PATH}
    Run Keyword And Ignore Error  Uninstall All

Install Local SSL Server Cert To System
    ${LOCAL_HTTPS_CERT_PATH} =  PathManager.get_local_https_cert_path
    Copy File   ${LOCAL_HTTPS_CERT_PATH}/ca/root-ca.crt.pem    ${LOCAL_HTTPS_CERT_PATH}/ca/root-ca.crt
    Install System Ca Cert  ${LOCAL_HTTPS_CERT_PATH}/ca/root-ca.crt

Setup Ostia Warehouse Environment
    Generate Local Ssl Certs If They Dont Exist
    Install Local SSL Server Cert To System
    Install Ostia SSL Certs To System
    Setup Local Warehouses If Needed


Install Just Base
    Run Keyword And Ignore Error  Uninstall All
    Remove Directory   ${SOPHOS_INSTALL}   recursive=True
    Directory Should Not Exist  ${SOPHOS_INSTALL}
    Install Base For Component Tests

Create Policy For Updating From Ostia
    [Arguments]  ${policy_path}
    ${policy} =  WarehouseUtils.get warehouse policy from template  ${policy_path}
    [return]  ${policy}


Apply Policy to Base
    [Arguments]  ${policy_xml}
    Log  Got policy XML ${policy_xml}

    Create File  ${SOPHOS_INSTALL}/base/mcs/ALC-1_policy.xml  content=${policy_xml}
    Move File  ${SOPHOS_INSTALL}/base/mcs/ALC-1_policy.xml  ${SOPHOS_INSTALL}/base/mcs/policy/ALC-1_policy.xml

Trigger Update
    Log  Trigger Update
    ${actionFilename} =  Generate Random String
    ${full_action_filename} =  Set Variable  ALC_action_${actionFilename}.xml
    Copy File  ${RESOURCES_PATH}/alc_action/ALC_update_now.xml  ${SOPHOS_INSTALL}/base/mcs/${full_action_filename}
    Move File  ${SOPHOS_INSTALL}/base/mcs/${full_action_filename}  ${SOPHOS_INSTALL}/base/mcs/action/${full_action_filename}


Verify AV installed
    Log  Verify AV installed
    Wait Until Created  ${COMPONENT_ROOT_PATH}  2m
    Directory Should Exist  ${COMPONENT_ROOT_PATH}

    Check AV Plugin Installed With Base
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  3 secs
    ...  Check sophos_threat_detector Running

Install Ostia SSL Certs To System
    Install System Ca Cert  ${RESOURCES_PATH}/sophos_certs/OstiaCA.crt

Revert System CA Certs
    Cleanup System Ca Certs


Configure Debug logging
    Override LogConf File as Global Level  DEBUG

Override LogConf File as Global Level
    [Arguments]  ${logLevel}  ${key}=VERBOSITY
    ${LOGGERCONF} =  get_logger_conf_path
    Create File  ${LOGGERCONF}  [global]\n${key} = ${logLevel}\n

Stop Management Agent
    ${result} =    Run Process    ${SOPHOS_INSTALL}/bin/wdctl   stop   managementagent
    Should Be Equal As Integers    ${result.rc}    0

Start Management Agent
    ${result} =    Run Process    ${SOPHOS_INSTALL}/bin/wdctl   start   managementagent
    Should Be Equal As Integers    ${result.rc}    0

Check Managment Agent Is Not Running
    ${result} =    Run Process  pgrep  sophos_managementagent
    Should Not Be Equal As Integers    ${result.rc}    0   msg="stdout:${result.stdout}\n${SPACE}err: ${result.stderr}"

Wait For Management Agent To Stop
    Wait Until Keyword Succeeds
    ...  5 secs
    ...  1 secs
    ...  Check Managment Agent Is Not Running

Restart Management Agent
    Stop Management Agent
    Wait For Management Agent To Stop
    mark managementagent log
    Start Management Agent

*** Test Cases ***

Update from Ostia
    [Tags]  OSTIA
    [Timeout]  30min
    Setup Ostia Warehouse Environment

    Install Just Base

    Configure Debug logging

    restart management agent

    Wait Until Keyword Succeeds
    ...  10 secs
    ...  2 secs
    ...  check_marked_managementagent_log_contains  managementagent <> Management Agent running.

    ${policy_name} =  Set Variable  base_and_av_VUT.xml

    ${policy} =  Create Policy For Updating From Ostia  ${RESOURCES_PATH}/alc_policy/template/${policy_name}
    Apply Policy to Base  ${policy}
    install_upgrade_certs_for_policy  ${RESOURCES_PATH}/alc_policy/template/${policy_name}

    Wait Until Keyword Succeeds
    ...  20 secs
    ...  2 secs
    ...  check_marked_managementagent_log_contains_regex  Policy .*ALC-1_policy\.xml applied to 1 plugins

    Trigger Update

    Wait Until Keyword Succeeds
    ...  20 secs
    ...  3 secs
    ...  check_marked_managementagent_log_contains_regex  Action .*ALC_action.*\.xml sent to 1 plugins

    Wait Until Keyword Succeeds
    ...   60 secs
    ...   10 secs
    ...   check_suldownloader_log_contains   suldownloaderdata <> SUL Last Result: 0

    Wait Until Keyword Succeeds
    ...   60 secs
    ...   10 secs
    ...   check_suldownloader_log_contains   suldownloaderdata <> Product will be downloaded: ServerProtectionLinux-Plugin-AV

    Wait Until Keyword Succeeds
    ...   10 mins
    ...   15 secs
    ...   check_suldownloader_log_contains   suldownloaderdata <> Update success

    Verify AV installed

    dump_suldownloader_log
