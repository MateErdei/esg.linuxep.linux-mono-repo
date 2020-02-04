*** Settings ***
Library     Process
Library     Collections
Library     OperatingSystem

Library     ${LIBS_DIRECTORY}/LogUtils.py
Library     ${LIBS_DIRECTORY}/FullInstallerUtils.py
Library     ${LIBS_DIRECTORY}/CapnPublisher.py
Library     ${LIBS_DIRECTORY}/CapnSubscriber.py

Resource  ../sul_downloader/SulDownloaderResources.robot
Resource  ../installer/InstallerResources.robot
Resource  ../management_agent/ManagementAgentResources.robot
Resource  ../watchdog/LogControlResources.robot
Resource  ../GeneralTeardownResource.robot

*** Variable ***
${EVENT_PROCESSOR_RIGID_NAME}   ServerProtectionLinux-Plugin-EventProcessor

*** Keywords ***

Install Event Processor Plugin From
    [Arguments]  ${event_processsor_plugin_sdds}
    Install Package Via Warehouse  ${event_processsor_plugin_sdds}    ${EVENT_PROCESSOR_RIGID_NAME}
    Set Log Level For Component And Reset and Return Previous Log  EventProcessor  DEBUG

Check Event Processor Running
    ${result} =    Run Process  pgrep  eventpro
    Should Be Equal As Integers    ${result.rc}    0

Event Processor Log Contains
    [Arguments]  ${TextToFind}
    File Should Exist  ${SOPHOS_INSTALL}/plugins/EventProcessor/log/EventProcessor.log
    ${fileContent}=  Get File  ${SOPHOS_INSTALL}/plugins/EventProcessor/log/EventProcessor.log
    Should Contain  ${fileContent}    ${TextToFind}

Check Event Processor Installed
    File Should Exist   ${SOPHOS_INSTALL}/plugins/EventProcessor/bin/eventprocessor
    Wait Until Keyword Succeeds
    ...  5 secs
    ...  1 secs
    ...  Check Event Processor Running
    Wait Until Keyword Succeeds
    ...  5 secs
    ...  1 secs
    ...  Event Processor Log Contains  EventProcessor <> Entering the main loop

Uninstall And Terminate All Processes
    Uninstall SSPL
    Terminate All Processes  kill=True

Management Agent Log Contains
    [Arguments]  ${TextToFind}
    ${management_agent_log} =  Get File  ${SOPHOS_INSTALL}/logs/base/sophosspl/sophos_managementagent.log
    Should Contain  ${management_agent_log}  ${TextToFind}

Default Test Teardown
    General Test Teardown
    Clean Up Warehouse Temp Dir
    Uninstall And Terminate All Processes
    Remove Directory   ${tmpdir}    recursive=True
    Remove Directory   ./tmp        recursive=True  #Pub/Sub logging is done here so needs cleaning up

Setup Event Processor Plugin Tmpdir
    Set Test Variable  ${tmpdir}            ./tmp/
    Remove Directory   ${tmpdir}    recursive=True
    Create Directory   ${tmpdir}
    Run Process  chown  sophos-spl-user:sophos-spl-group  ${tmpdir}
    Run Process  chown  -R  sophos-spl-user:sophos-spl-group  ${tmpdir}

SAV Event File Exists And Contains Failed Authentication Info
    [Arguments]  ${NumberOfEvents}=1
    Directory Should Not Be Empty  /opt/sophos-spl/base/mcs/event/
    ${NumberOfFiles} =  Count Files In Directory  /opt/sophos-spl/base/mcs/event/
    Should Be Equal As Integers  ${NumberOfFiles}  ${NumberOfEvents}  Only expected ${NumberOfEvents} SAV event file(s) after scan
    Should Be True  ${NumberOfFiles} >= ${NumberOfEvents}  Expected ${NumberOfEvents} SAV event file(s) after scan. Received ${NumberOfFiles}
    ${EventFiles} =  List Files In Directory  /opt/sophos-spl/base/mcs/event/
    Should Contain  ${EventFiles[0]}  SAV_event-
    ${EventContents} =  Get File  /opt/sophos-spl/base/mcs/event/${EventFiles[0]}
    Should Contain  ${EventContents}  description="10 Failed Logins Occurred"
    Should Contain  ${EventContents}  name="Suspected Attack: 10 Failed Logins Occurred In 5 Minutes"
    [Return]  ${EventContents}

SAV Event File Exists And Contains Failed Authentication Info With String
    [Arguments]  ${String}
    ${EventContents} =  SAV Event File Exists And Contains Failed Authentication Info
    Should Contain  ${EventContents}  ${String}


SAV Event File Exists And Contains Correct Contents
    [Arguments]  ${Description}  ${Name}  ${NumberOfEvents}=1
    Directory Should Not Be Empty  /opt/sophos-spl/base/mcs/event/
    ${NumberOfFiles} =  Count Files In Directory  /opt/sophos-spl/base/mcs/event/
    Should Be Equal As Integers  ${NumberOfFiles}  ${NumberOfEvents}  Only expected ${NumberOfEvents} SAV event file(s) after scan
    ${EventFiles} =  List Files In Directory  /opt/sophos-spl/base/mcs/event/
    Should Contain  ${EventFiles[0]}  SAV_event-
    ${EventContents} =  Get File  /opt/sophos-spl/base/mcs/event/${EventFiles[0]}
    Should Contain  ${EventContents}  description=${Description}
    Should Contain  ${EventContents}  name=${Name}

Install EventProcessor Plugin Directly
    ${SDDS} =  Get Event Processor Plugin SDDS
    ${result} =    Run Process  ${SDDS}/install.sh
    Log  ${result.stdout}
    Log  ${result.stderr}
    Set Log Level For Component And Reset and Return Previous Log  EventProcessor  DEBUG
    Check Event Processor Installed

Uninstall EventProcessor Plugin
    File Should Exist  ${SOPHOS_INSTALL}/plugins/EventProcessor/bin/eventprocessor
    ${result} =    Run Process  ${SOPHOS_INSTALL}/plugins/EventProcessor/bin/uninstall.sh
    Should Be Equal As Integers    ${result.rc}    0   msg=${result.stderr}

SAV Event File Exists And Contains Wget Execution Info
    [Arguments]  ${NumberOfEvents}=1
    Directory Should Not Be Empty  /opt/sophos-spl/base/mcs/event/
    ${NumberOfFiles} =  Count Files In Directory  /opt/sophos-spl/base/mcs/event/
    Should Be Equal As Integers  ${NumberOfFiles}  ${NumberOfEvents}  Only expected ${NumberOfEvents} SAV event file(s) after scan
    ${EventFiles} =  List Files In Directory  /opt/sophos-spl/base/mcs/event/
    Should Contain  ${EventFiles[0]}  SAV_event-
    ${EventContents} =  Get File  /opt/sophos-spl/base/mcs/event/${EventFiles[0]}
    Should Contain  ${EventContents}  description="Unsafe process was started"
    Should Contain  ${EventContents}  name="ATTENTION: unsafe process was started"

SAV Event File Is Not Created
    [Arguments]  ${how long to sleep for}=5seconds
    sleep  ${how long to sleep for}
    Directory Should Be Empty  /opt/sophos-spl/base/mcs/event/

