*** Settings ***
Library     Process
Library     Collections
Library     OperatingSystem

Library     ${LIBS_DIRECTORY}/LogUtils.py

Resource    ../example_plugin/ExamplePluginResources.robot
Resource  ../GeneralTeardownResource.robot


Test Teardown  ManagementAgent Default Test Teardown

*** Test Cases ***

Management Agent Passes SAV Policy And Action To Example Plugin And Plugin Reports Virus
    [Tags]  MANAGEMENT_AGENT  EXAMPLE_PLUGIN
    Setup Example Plugin Tmpdir
    ${SDDS} =  Copy Example Plugin sdds
    ${ScanPath} =  Run Process  readlink  -f  ${tmpdir}/scanpath
    Copy File  ./tests/management_agent-example_plugin/RegistryFile/Example.json  ${SDDS}/files/base/pluginRegistry/Example.json
    Run Process  sed  -i  s+PATH_TO_SCAN+${ScanPath.stdout}+g  ${SDDS}/files/base/pluginRegistry/Example.json
    Require Fresh Install
    #Stop the mcs router process but leave the other processes running
    Run Process  ${SOPHOS_INSTALL}/bin/wdctl  stop  mcsrouter
    Install Plugin Example From  ${SDDS}
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  Check Example Plugin Running

    # On access needs setting enabled for the scanner to be active.
    Atomic Write SAV Policy On Access Scan Enabled

    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  Check On Access Scan Set In SAV Status  true

    Create File        ${tmpdir}/scanpath/newvirus.txt   "this contain VIRUS threat"
    File Should Exist  ${tmpdir}/scanpath/newvirus.txt
    Run Process  chown  sophos-spl-user:sophos-spl-group  ${tmpdir}/scanpath/newvirus.txt

    Atomic Write SAV Action Scan Now

    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  Directory Should Be Empty  /opt/sophos-spl/base/mcs/action

    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  SAV Event File Exists And Contains Virus Info

    LogUtils.Dump Log          ${tmpdir}/pluginout.log
    Remove Directory  ${SDDS}  True

Plugin Uninstalls Cleanly And Management Agent Notices
    [Tags]  MANAGEMENT_AGENT  EXAMPLE_PLUGIN  UNINSTALL
    Setup Example Plugin Tmpdir
    Require Fresh Install
    #Stop the mcs router process but leave the other processes running
    Run Process  ${SOPHOS_INSTALL}/bin/wdctl  stop  mcsrouter
    Install Plugin Example
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  Check Example Plugin Running

    Run Process     ${SOPHOS_INSTALL}/plugins/Example/bin/uninstall.sh
    Check Example Plugin Not Running

    Directory Should Not Exist  ${SOPHOS_INSTALL}/plugins/Example
    File Should Not Exist  ${SOPHOS_INSTALL}/base/pluginRegistry/Example.json
    File Should Not Exist  ${SOPHOS_INSTALL}/base/update/var/installedproducts/ServerProtectionLinux-Plugin-Example.sh

    Atomic Write SAV Action Scan Now

    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  Directory Should Be Empty  /opt/sophos-spl/base/mcs/action

    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  Management Agent Log Contains  Example has been uninstalled.



*** Keywords ***

Atomic Write SAV Policy On Access Scan Enabled
    Copy File  ${SUPPORT_FILES}/CentralXml/SAVPolicyOnAccessEnabled.xml  /opt/sophos-spl/base/mcs/tmp/SAV_policy.xml
    Run Process  chown  sophos-spl-user:sophos-spl-group  /opt/sophos-spl/base/mcs/tmp/SAV_policy.xml
    Move File  /opt/sophos-spl/base/mcs/tmp/SAV_policy.xml  /opt/sophos-spl/base/mcs/policy/SAV_policy.xml

Atomic Write SAV Action Scan Now
    Copy File  ${SUPPORT_FILES}/CentralXml/ScanNowAction.xml  /opt/sophos-spl/base/mcs/tmp/SAV_action_dummytime.xml
    Run Process  chown  sophos-spl-user:sophos-spl-group  /opt/sophos-spl/base/mcs/tmp/SAV_action_dummytime.xml
    Move File  /opt/sophos-spl/base/mcs/tmp/SAV_action_dummytime.xml  /opt/sophos-spl/base/mcs/action/SAV_action_dummytime.xml

Check On Access Scan Set In SAV Status
    [Arguments]  ${ODSTrue}
    File Should Exist  /opt/sophos-spl/base/mcs/status/SAV_status.xml
    ${SavStatusContents} =  Get File  /opt/sophos-spl/base/mcs/status/SAV_status.xml
    Should Contain  ${SavStatusContents}  <on-access>${ODSTrue}</on-access>

SAV Event File Exists And Contains Virus Info
    Directory Should Not Be Empty  /opt/sophos-spl/base/mcs/event/
    ${NumberOfFiles} =  Count Files In Directory  /opt/sophos-spl/base/mcs/event/
    Should Be Equal As Integers  ${NumberOfFiles}  1  Only expected a single SAV event file after scan
    ${EventFiles} =  List Files In Directory  /opt/sophos-spl/base/mcs/event/
    Should Contain  ${EventFiles[0]}  SAV_event-
    ${EventContents} =  Get File  /opt/sophos-spl/base/mcs/event/${EventFiles[0]}
    Should Contain  ${EventContents}  description="Virus File found in
    Should Contain  ${EventContents}  <item file="newvirus.txt"

Uninstall And Terminate All Processes
    Run Keyword If Test Failed  LogUtils.Dump Log  ${tmpdir}/update_server.log
    Run Keyword If Test Failed  Dump Warehouse Log
    Remove Directory  ${tmpdir}  recursive=true
    Uninstall SSPL
    Clean Up Warehouse Temp Dir
    Terminate All Processes  kill=True

Management Agent Log Contains
    [Arguments]  ${TextToFind}
    ${management_agent_log} =  Get File  ${SOPHOS_INSTALL}/logs/base/sophosspl/sophos_managementagent.log
    Should Contain  ${management_agent_log}  ${TextToFind}


ManagementAgent Default Test Teardown
    General Test Teardown
    Uninstall And Terminate All Processes
