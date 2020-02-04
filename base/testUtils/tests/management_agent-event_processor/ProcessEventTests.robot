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

Default Tags  EVENT_PLUGIN  PUB_SUB
*** Test Cases ***

Check Fake Pub Sub Work With Capn Serialisation For Process Event
    Require Fresh Install
    #Stop the mcs router process but leave the other processes running
    Run Process  ${SOPHOS_INSTALL}/bin/wdctl  stop  mcsrouter
    Start Publisher
    Start Subscriber
    Create And Send Whoami Process Event
    Recieve Process Event With Historic Time

Check A Process Event Which Contains Wget Creates And Sends An Event
    ${SDDS} =  Copy Event Processor Plugin Sdds
    Require Fresh Install
    #Stop the mcs router process but leave the other processes running
    Run Process  ${SOPHOS_INSTALL}/bin/wdctl  stop  mcsrouter
    Install Event Processor Plugin From  ${SDDS}
    Check Event Processor Installed
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  Check Management Agent Running And Ready
    Start Publisher
    Create And Send Wget Process Event

    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  SAV Event File Exists And Contains Wget Execution Info

Check An End Process Event Which Contains Wget Does Not Create And Send An Event
    ${SDDS} =  Copy Event Processor Plugin Sdds
    Require Fresh Install
    #Stop the mcs router process but leave the other processes running
    Run Process  ${SOPHOS_INSTALL}/bin/wdctl  stop  mcsrouter
    Install Event Processor Plugin From  ${SDDS}
    Check Event Processor Installed
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  Check Management Agent Running And Ready
    Start Publisher
    Create And Send Wget Process Event  end
    Create And Send Authentication Fail With Current Time Plus Time Seconds
    Create And Send Authentication Fail With Current Time Plus Time Seconds
    Create And Send Authentication Fail With Current Time Plus Time Seconds
    Create And Send Authentication Fail With Current Time Plus Time Seconds
    Create And Send Authentication Fail With Current Time Plus Time Seconds

    Create And Send Authentication Fail With Current Time Plus Time Seconds
    Create And Send Authentication Fail With Current Time Plus Time Seconds
    Create And Send Authentication Fail With Current Time Plus Time Seconds
    Create And Send Authentication Fail With Current Time Plus Time Seconds
    Create And Send Authentication Fail With Current Time Plus Time Seconds

    #Send one end process event and then trigger a failed authentication event
    #Only the failed Auth event xml file should exist and gates the negative test case
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  SAV Event File Exists And Contains Failed Authentication Info


Check Two Process Events Which Contains Wget Creates And Sends An Event
    ${SDDS} =  Copy Event Processor Plugin Sdds
    Require Fresh Install
    #Stop the mcs router process but leave the other processes running
    Run Process  ${SOPHOS_INSTALL}/bin/wdctl  stop  mcsrouter
    Install Event Processor Plugin From  ${SDDS}
    Check Event Processor Installed
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  Check Management Agent Running And Ready
    Start Publisher

    Create And Send Wget Process Event
    Create And Send Wget Process Event

    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  SAV Event File Exists And Contains Wget Execution Info  2

Check A Process Event Which Does Not Contain Wget Does Not Create And Send An Event
    ${SDDS} =  Copy Event Processor Plugin Sdds
    Require Fresh Install
    #Stop the mcs router process but leave the other processes running
    Run Process  ${SOPHOS_INSTALL}/bin/wdctl  stop  mcsrouter
    Install Event Processor Plugin From  ${SDDS}
    Check Event Processor Installed
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  Check Management Agent Running And Ready
    Start Publisher
    Create And Send Whoami Process Event
    Create And Send Authentication Fail With Current Time Plus Time Seconds
    Create And Send Authentication Fail With Current Time Plus Time Seconds
    Create And Send Authentication Fail With Current Time Plus Time Seconds
    Create And Send Authentication Fail With Current Time Plus Time Seconds
    Create And Send Authentication Fail With Current Time Plus Time Seconds

    Create And Send Authentication Fail With Current Time Plus Time Seconds
    Create And Send Authentication Fail With Current Time Plus Time Seconds
    Create And Send Authentication Fail With Current Time Plus Time Seconds
    Create And Send Authentication Fail With Current Time Plus Time Seconds
    Create And Send Authentication Fail With Current Time Plus Time Seconds

    #Send one whoami start process event and then trigger a failed authentication event
    #Only the failed Auth event xml file should exist and gates the negative test case
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  SAV Event File Exists And Contains Failed Authentication Info

