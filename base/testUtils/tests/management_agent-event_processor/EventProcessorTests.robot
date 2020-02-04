*** Settings ***
Library     Process
Library     Collections
Library     OperatingSystem

Library     ${LIBS_DIRECTORY}/LogUtils.py
Library     ${LIBS_DIRECTORY}/FullInstallerUtils.py
Library     ${LIBS_DIRECTORY}/CapnPublisher.py
Library     ${LIBS_DIRECTORY}/CapnSubscriber.py

Test Setup  Setup Event Processor Plugin Tmpdir
Test Teardown  Default Test Teardown

Resource  ../sul_downloader/SulDownloaderResources.robot
Resource  ../installer/InstallerResources.robot
Resource  ../management_agent/ManagementAgentResources.robot
Resource  EventProcessorResources.robot

Default Tags  EVENT_PLUGIN
*** Test Cases ***

SSPL Check RigidName is correct for EventProcessor
    ${SDDS} =  Get Event Processor Plugin SDDS
    Check Sdds Import Matches Rigid Name  ${EVENT_PROCESSOR_RIGID_NAME}  ${SDDS}

Event Processor Installs And Starts
    [Tags]  EVENT_PLUGIN  INSTALLER  WDCTL
    ${SDDS} =  Copy Event Processor Plugin Sdds
    Require Fresh Install
    #Stop the mcs router process but leave the other processes running
    Run Process  ${SOPHOS_INSTALL}/bin/wdctl  stop  mcsrouter
    Install Event Processor Plugin From  ${SDDS}
    Check Event Processor Installed