*** Settings ***
Library     Process
Library     Collections
Library     OperatingSystem

Library     ${libs_directory}/LogUtils.py
Library     ${libs_directory}/FullInstallerUtils.py
Library     ${libs_directory}/CapnPublisher.py
Library     ${libs_directory}/CapnSubscriber.py

Test Setup  Setup Event Processor Plugin Tmpdir
Test Teardown  Default Test Teardown

Resource  ../sul_downloader/SulDownloaderResources.robot
Resource  ../installer/InstallerResources.robot
Resource  ../management_agent/ManagementAgentResources.robot
Resource  EventProcessorResources.robot

Default Tags  EVENT_PLUGIN  PUB_SUB
*** Test Cases ***

Check Fake Pub Sub Work With Capn Serialisation For Credential Event
    [Tags]  PUB_SUB
    Require Fresh Install
    #Stop the mcs router process but leave the other processes running
    Run Process  ${SOPHOS_INSTALL}/bin/wdctl  stop  mcsrouter
    Start Publisher
    Start Subscriber
    Create And Send Authentication Fail With Current Time Plus Time Seconds  0
    Receive Authentication Fail With Historic Time

Check 10 Failed Login Attempts In 5 Minutes Creates and Sends An Event
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

    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  SAV Event File Exists And Contains Failed Authentication Info

Check 10 Failed Login Attempts In 5 Minutes Creates and Sends An Event With Network Address
    ${networkAddress} =  Set Variable  123.123.123.123
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
    Create And Send Authentication Fail Containing Network Address With Current Time Plus Time Seconds  ${networkAddress}
    Create And Send Authentication Fail Containing Network Address With Current Time Plus Time Seconds  ${networkAddress}
    Create And Send Authentication Fail Containing Network Address With Current Time Plus Time Seconds  ${networkAddress}
    Create And Send Authentication Fail Containing Network Address With Current Time Plus Time Seconds  ${networkAddress}
    Create And Send Authentication Fail Containing Network Address With Current Time Plus Time Seconds  ${networkAddress}

    Create And Send Authentication Fail Containing Network Address With Current Time Plus Time Seconds  ${networkAddress}
    Create And Send Authentication Fail Containing Network Address With Current Time Plus Time Seconds  ${networkAddress}
    Create And Send Authentication Fail Containing Network Address With Current Time Plus Time Seconds  ${networkAddress}
    Create And Send Authentication Fail Containing Network Address With Current Time Plus Time Seconds  ${networkAddress}
    Create And Send Authentication Fail Containing Network Address With Current Time Plus Time Seconds  ${networkAddress}

    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  SAV Event File Exists And Contains Failed Authentication Info With String  from ${networkAddress}



Check 20 Failed Login Attempts In Under 5 Minutes Creates and Sends 2 Events
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

    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  SAV Event File Exists And Contains Failed Authentication Info  2

Check 10 Failed Login Attempts In 10 Minutes followed by 10 Failed Logins In 10 Seconds Creates and Sends One Event
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
    Create And Send Authentication Fail With Current Time Plus Time Seconds  0
    Create And Send Authentication Fail With Current Time Plus Time Seconds  60
    Create And Send Authentication Fail With Current Time Plus Time Seconds  120
    Create And Send Authentication Fail With Current Time Plus Time Seconds  180
    Create And Send Authentication Fail With Current Time Plus Time Seconds  240
    Create And Send Authentication Fail With Current Time Plus Time Seconds  300
    Create And Send Authentication Fail With Current Time Plus Time Seconds  360
    Create And Send Authentication Fail With Current Time Plus Time Seconds  420
    Create And Send Authentication Fail With Current Time Plus Time Seconds  480
    Create And Send Authentication Fail With Current Time Plus Time Seconds  540
    Create And Send Authentication Fail With Current Time Plus Time Seconds  600

    Create And Send Authentication Fail With Current Time Plus Time Seconds  900
    Create And Send Authentication Fail With Current Time Plus Time Seconds  901
    Create And Send Authentication Fail With Current Time Plus Time Seconds  902
    Create And Send Authentication Fail With Current Time Plus Time Seconds  903
    Create And Send Authentication Fail With Current Time Plus Time Seconds  904
    Create And Send Authentication Fail With Current Time Plus Time Seconds  905
    Create And Send Authentication Fail With Current Time Plus Time Seconds  906
    Create And Send Authentication Fail With Current Time Plus Time Seconds  907
    Create And Send Authentication Fail With Current Time Plus Time Seconds  908
    Create And Send Authentication Fail With Current Time Plus Time Seconds  909

    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  SAV Event File Exists And Contains Failed Authentication Info


Check 10 Failed Login Attempts In 10 seconds followed by 10 Failed Logins In Ten Minutes Creates and Sends One Event
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

    Create And Send Authentication Fail With Current Time Plus Time Seconds  0
    Create And Send Authentication Fail With Current Time Plus Time Seconds  1
    Create And Send Authentication Fail With Current Time Plus Time Seconds  2
    Create And Send Authentication Fail With Current Time Plus Time Seconds  3
    Create And Send Authentication Fail With Current Time Plus Time Seconds  4
    Create And Send Authentication Fail With Current Time Plus Time Seconds  5
    Create And Send Authentication Fail With Current Time Plus Time Seconds  6
    Create And Send Authentication Fail With Current Time Plus Time Seconds  7
    Create And Send Authentication Fail With Current Time Plus Time Seconds  8
    Create And Send Authentication Fail With Current Time Plus Time Seconds  9

    Create And Send Authentication Fail With Current Time Plus Time Seconds  10
    Create And Send Authentication Fail With Current Time Plus Time Seconds  69
    Create And Send Authentication Fail With Current Time Plus Time Seconds  129
    Create And Send Authentication Fail With Current Time Plus Time Seconds  189
    Create And Send Authentication Fail With Current Time Plus Time Seconds  249
    Create And Send Authentication Fail With Current Time Plus Time Seconds  309
    Create And Send Authentication Fail With Current Time Plus Time Seconds  369
    Create And Send Authentication Fail With Current Time Plus Time Seconds  429
    Create And Send Authentication Fail With Current Time Plus Time Seconds  489
    Create And Send Authentication Fail With Current Time Plus Time Seconds  549
    Create And Send Authentication Fail With Current Time Plus Time Seconds  609


    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  SAV Event File Exists And Contains Failed Authentication Info



