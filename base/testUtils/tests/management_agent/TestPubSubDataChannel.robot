*** Settings ***
Documentation     Test the Management Agent Pub Sub Data Channel

Library           Process
Library           OperatingSystem
Library           Collections

Library     ${LIBS_DIRECTORY}/FakeMultiSubscriber.py
Library     ${LIBS_DIRECTORY}/FakeMultiPublisher.py

Test Teardown   Test Teardown For Suite

Resource    ManagementAgentResources.robot
Resource    ../installer/InstallerResources.robot
Resource  ../GeneralTeardownResource.robot

Test Timeout    4 minutes

Default Tags    MANAGEMENT_AGENT  PUB_SUB

*** Test Cases ***
Verify Can Send Message Via Management Agent Pub Sub Data Channel
    Start Management Agent

    Add Subscriber   ChannelTest1   Subscriber1
    Add Publishers   1
    Send Data  1   ChannelTest1   HelloWorld

    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  Check Log Contains  Sub Subscriber1 received message: [b'ChannelTest1', b'HelloWorld']  ./tmp/fake_multi_subscriber.log  Fake Multi-Subscriber Log


Verify Can Send Message Via Management Agent Pub Sub Data Channel And Check That Data Sent Through Pub Sub Is Logged
    Start Management Agent

     Wait Until Keyword Succeeds
    ...  5 secs
    ...  1 secs
    ...  Check Log Contains  sophos_managementagent configured for level: DEBUG  ${BASE_LOGS_DIR}/sophosspl/sophos_managementagent.log   Management Agent Log

    Add Subscriber   ChannelTest1   Subscriber1
    Add Publishers   1
    Send Data  1   ChannelTest1   HelloWorld

    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  Check Log Contains  Sub Subscriber1 received message: [b'ChannelTest1', b'HelloWorld']  ./tmp/fake_multi_subscriber.log  Fake Multi-Subscriber Log

    Stop Management Agent

    #When a subscriber is created a publisher is set up and a message sent to check that
    #the subscriber is connected to the pub sub system. The publisher is then destroyed
    ${SubscriberStartup}=  Create List   Subscribe Subscriber1_StartUp  # Initialise publisher
    ...                                  Subscriber1_StartUp Approx Wait Time On Socket Confirmation =
    ...                                  Unsubscribe Subscriber1_StartUp

    ...                                  ChannelTest1 Check-Connected  # Test connection

    ...                                  Subscribe Subscriber1_ShutDown  # Destroy publisher
    ...                                  Subscriber1_ShutDown Approx Wait Time On Socket Confirmation =
    ...                                  Unsubscribe Subscriber1_ShutDown


    ${publisherStartup}=  Create List   Subscribe publisher_0_StartUp  # Initialise publisher
    ...                                 publisher_0_StartUp Approx Wait Time On Socket Confirmation =
    ...                                 Unsubscribe publisher_0_StartUp

    ${MessageSent}=  Create List  Subscribe ChannelTest1
    ...                           ChannelTest1 HelloWorld
    ...                           TERMINATE

    Wait Until Keyword Succeeds
    ...  10 seconds
    ...  1 seconds
    ...  log_contains_in_order  /opt/sophos-spl/logs/base/sophosspl/sophos_managementagent.log  Management Agent  ${SubscriberStartup}

    Wait Until Keyword Succeeds
    ...  10 seconds
    ...  1 seconds
    ...  log_contains_in_order  /opt/sophos-spl/logs/base/sophosspl/sophos_managementagent.log  Management Agent  ${publisherStartup}

    Wait Until Keyword Succeeds
    ...  10 seconds
    ...  1 seconds
    ...  log_contains_in_order  /opt/sophos-spl/logs/base/sophosspl/sophos_managementagent.log  Management Agent  ${MessageSent}


Verify Can Send Message Via The Data Channel Proxy Before And After Management Agent Restart
    Start Management Agent

    Add Subscriber   ChannelTest1   Subscriber1
    Add Subscriber   ChannelTest2   Subscriber2
    Add Subscriber   ChannelTest3   Subscriber3
    Add Publishers   3
    Send Data  1   ChannelTest1   HelloWorld
    Send Data  2   ChannelTest2   HelloWorld
    Send Data  3   ChannelTest3   HelloWorld

    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  Check Log Contains  Sub Subscriber1 received message: [b'ChannelTest1', b'HelloWorld']  ./tmp/fake_multi_subscriber.log  Fake Multi-Subscriber Log

    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  Check Log Contains  Sub Subscriber2 received message: [b'ChannelTest2', b'HelloWorld']  ./tmp/fake_multi_subscriber.log  Fake Multi-Subscriber Log

    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  Check Log Contains  Sub Subscriber3 received message: [b'ChannelTest3', b'HelloWorld']  ./tmp/fake_multi_subscriber.log  Fake Multi-Subscriber Log

    Stop Management Agent

    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  Check Managementagent Not Running

    Send Data  1   ChannelTest1   GoodbyeCruelWorld
    Send Data  2   ChannelTest2   GoodbyeCruelWorld
    Send Data  3   ChannelTest3   GoodbyeCruelWorld

    Sleep  10  # Wait enough time for messages sending to be attempted.

    Check Log Does Not Contain  Sub Subscriber1 received message: ['ChannelTest1', 'GoodbyeCruelWorld']  ./tmp/fake_multi_subscriber.log  Fake Multi-Subscriber Log
    Check Log Does Not Contain  Sub Subscriber2 received message: ['ChannelTest2', 'GoodbyeCruelWorld']  ./tmp/fake_multi_subscriber.log  Fake Multi-Subscriber Log
    Check Log Does Not Contain  Sub Subscriber3 received message: ['ChannelTest3', 'GoodbyeCruelWorld']  ./tmp/fake_multi_subscriber.log  Fake Multi-Subscriber Log

    Start Management Agent

    Check Publishers Connected

    Send Data  1   ChannelTest1   HelloWorldAgain
    Send Data  2   ChannelTest2   HelloWorldAgain
    Send Data  3   ChannelTest3   HelloWorldAgain

    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  Check Log Contains  Sub Subscriber1 received message: [b'ChannelTest1', b'HelloWorldAgain']  ./tmp/fake_multi_subscriber.log  Fake Multi-Subscriber Log

    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  Check Log Contains  Sub Subscriber2 received message: [b'ChannelTest2', b'HelloWorldAgain']  ./tmp/fake_multi_subscriber.log  Fake Multi-Subscriber Log

    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  Check Log Contains  Sub Subscriber3 received message: [b'ChannelTest3', b'HelloWorldAgain']  ./tmp/fake_multi_subscriber.log  Fake Multi-Subscriber Log


Verify Can Send Multiple Messages Via Data Channel Proxy
    Start Management Agent

    Add Subscriber   ChannelTest1   Subscriber1
    Add Publishers   1

    @{DataItems}=   Create List    HelloWord1   HelloWord2   HelloWord3   HelloWord4

    Send Data Over Data Channel   ChannelTest1  @{DataItems}

    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  Check Log Contains Data   ChannelTest1  Subscriber1  @{DataItems}


Verify Can Send Multiple Message On One One Publisher Via Data Channel Proxy To Multiple Subscribers
    Start Management Agent

    ${channelName}=  Set Variable  ChannelTest1
    Add Subscriber   ${channelName}   Subscriber1
    Add Subscriber   ${channelName}   Subscriber2
    Add Subscriber   ${channelName}   Subscriber3
    Add Publishers   1

    @{DataItems}=   Create List    HelloWord1   HelloWord2   HelloWord3   HelloWord4

    Send Data Over Data Channel   ${channelName}  @{DataItems}

    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  Check Log Contains Data   ${channelName}  Subscriber1  @{DataItems}

    Wait Until Keyword Succeeds
    ...  2 secs
    ...  1 secs
    ...  Check Log Contains Data   ${channelName}  Subscriber2  @{DataItems}

    Wait Until Keyword Succeeds
    ...  2 secs
    ...  1 secs
    ...  Check Log Contains Data   ${channelName}  Subscriber3  @{DataItems}


Verify Messages Sent From One Channel Are not Received On Another
    Start Management Agent

    ${channelName1}=  Set Variable  ChannelTest1
    ${channelName2}=  Set Variable  ChannelTest2
    Add Subscriber   ${channelName1}   Subscriber1
    Add Subscriber   ${channelName2}   Subscriber2
    Add Subscriber   ${channelName1}   Subscriber3
    Add Subscriber   ${channelName2}   Subscriber4
    Add Publishers   1

    @{DataItems1}=   Create List    Hello1   Hello2   Hello3   Hello4
    @{DataItems2}=   Create List    World1   World2   World3   World4

    Send Data Over Data Channel   ${channelName1}  @{DataItems1}
    Send Data Over Data Channel   ${channelName2}  @{DataItems2}

    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  Check Log Contains Data   ${channelName1}  Subscriber1  @{DataItems1}

    Wait Until Keyword Succeeds
    ...  2 secs
    ...  1 secs
    ...  Check Log Contains Data   ${channelName2}  Subscriber2  @{DataItems2}

    Wait Until Keyword Succeeds
    ...  2 secs
    ...  1 secs
    ...  Check Log Contains Data   ${channelName1}  Subscriber3  @{DataItems1}

    Wait Until Keyword Succeeds
    ...  2 secs
    ...  1 secs
    ...  Check Log Contains Data   ${channelName2}  Subscriber4  @{DataItems2}

    Check Log Does Not Contain Data   ${channelName2}  Subscriber1  @{DataItems2}
    Check Log Does Not Contain Data   ${channelName1}  Subscriber2  @{DataItems1}
    Check Log Does Not Contain Data   ${channelName2}  Subscriber3  @{DataItems2}
    Check Log Does Not Contain Data   ${channelName1}  Subscriber4  @{DataItems1}

Verify Messages Sent From Multiple Publishers On Same Channel Concurrently Arrive To Subscriber
    [Teardown]  Test Teardown For Verify Messages Sent From Multiple Publishers On Same Channel Concurrently Arrive To Subscriber

    Start Management Agent

    Clean Up Multi Subscriber

    ${channelName1}=  Set Variable  MultiPubSingleSubChannel
    ${subscriberName}=  Set Variable  MultiPubSingleSubscriber
    Add Subscriber   ${channelName1}   ${subscriberName}

    ${NumberOfPublishers} =  Convert To Integer  10
    ${DataItems} =  Convert To Integer  100
    Send On Multiple Publishers  ${NumberOfPublishers}  ${DataItems}  ${channelName1}

    Wait Until Keyword Succeeds
    ...  20 secs
    ...  4 secs
    ...  Check Log Contains Data From Multisend  ./tmp  ${subscriberName}  ${NumberOfPublishers}  ${DataItems}  ${channelName1}


Verify Messages Sent From Multiple Publishers On Same Channel Concurrently Arrive To Multiple Concurrent Subscribers
    Start Management Agent

    ${channelName1}=  Set Variable  ChannelTest1
    ${NumberOfSubscribers} =  Convert To Integer  15
    Start Multiple Subscribers  ${NumberOfSubscribers}  ${channelName1}

    ${NumberOfPublishers} =  Convert To Integer  15
    ${DataItems} =  Convert To Integer  100
    Send On Multiple Publishers  ${NumberOfPublishers}  ${DataItems}  ${channelName1}

    Wait Until Keyword Succeeds
    ...  15 secs
    ...  5 secs
    ...  Check Multiple Logs Contains Data From Multisend  ./tmp  ${NumberOfPublishers}  ${NumberOfSubscribers}  ${DataItems}  ${channelName1}  ${channelName1}

Verify Messages Sent From Multiple Publishers On Different Channels Concurrently Arrive To Multiple Concurrent Subscribers On Different Channels
    Start Management Agent

    ${channelName1}=  Set Variable  ChannelTest1
    ${channelName2}=  Set Variable  ChannelTest2
    ${channelName3}=  Set Variable  ChannelTest3
    ${NumberOfSubscribers} =  Convert To Integer  7
    Start Multiple Subscribers  ${NumberOfSubscribers}  ${channelName1}
    Start Multiple Subscribers  ${NumberOfSubscribers}  ${channelName2}
    Start Multiple Subscribers  ${NumberOfSubscribers}  ${channelName3}

    ${NumberOfPublishers} =  Convert To Integer  7
    ${DataItems} =  Convert To Integer  100
    Send On Multiple Publishers  ${NumberOfPublishers}  ${DataItems}  ${channelName1}
    Send On Multiple Publishers  ${NumberOfPublishers}  ${DataItems}  ${channelName2}
    Send On Multiple Publishers  ${NumberOfPublishers}  ${DataItems}  ${channelName3}

    Wait Until Keyword Succeeds
    ...  15 secs
    ...  5 secs
    ...  Check Multiple Logs Contains Data From Multisend  ./tmp  ${NumberOfPublishers}  ${NumberOfSubscribers}  ${DataItems}  ${channelName1}  ${channelName1}

    Check Multiple Logs Do Not Contain Data From Multisend  ./tmp  ${NumberOfPublishers}  ${NumberOfSubscribers}  ${channelName1}  ${channelName2}
    Check Multiple Logs Do Not Contain Data From Multisend  ./tmp  ${NumberOfPublishers}  ${NumberOfSubscribers}  ${channelName1}  ${channelName3}

    Wait Until Keyword Succeeds
    ...  15 secs
    ...  5 secs
    ...  Check Multiple Logs Contains Data From Multisend  ./tmp  ${NumberOfPublishers}  ${NumberOfSubscribers}  ${DataItems}  ${channelName2}  ${channelName2}

    Check Multiple Logs Do Not Contain Data From Multisend  ./tmp  ${NumberOfPublishers}  ${NumberOfSubscribers}  ${channelName2}  ${channelName1}
    Check Multiple Logs Do Not Contain Data From Multisend  ./tmp  ${NumberOfPublishers}  ${NumberOfSubscribers}  ${channelName2}  ${channelName3}


    Wait Until Keyword Succeeds
    ...  15 secs
    ...  5 secs
    ...  Check Multiple Logs Contains Data From Multisend  ./tmp  ${NumberOfPublishers}  ${NumberOfSubscribers}  ${DataItems}  ${channelName3}  ${channelName3}

    Check Multiple Logs Do Not Contain Data From Multisend  ./tmp  ${NumberOfPublishers}  ${NumberOfSubscribers}  ${channelName3}  ${channelName1}
    Check Multiple Logs Do Not Contain Data From Multisend  ./tmp  ${NumberOfPublishers}  ${NumberOfSubscribers}  ${channelName3}  ${channelName2}

Verify Messages Sent From Multiple Publishers On Different Channels Concurrently Arrive To Multiple Concurrent Subscribers On Multiple Different Channels
    Start Management Agent

    ${channelName1}=  Set Variable  ChannelTest1
    ${channelName2}=  Set Variable  ChannelTest2
    ${channelName3}=  Set Variable  ChannelTest3
    ${NumberOfSubscribers} =  Convert To Integer  7
    Start Multiple Subscribers  ${NumberOfSubscribers}  ${channelName1}_${channelName2}_${channelName3}
    Start Multiple Subscribers  ${NumberOfSubscribers}  ${channelName2}_${channelName3}
    Start Multiple Subscribers  ${NumberOfSubscribers}  ${channelName3}

    ${NumberOfPublishers} =  Convert To Integer  7
    ${DataItems} =  Convert To Integer  100
    Send On Multiple Publishers  ${NumberOfPublishers}  ${DataItems}  ${channelName1}
    Send On Multiple Publishers  ${NumberOfPublishers}  ${DataItems}  ${channelName2}
    Send On Multiple Publishers  ${NumberOfPublishers}  ${DataItems}  ${channelName3}

    Wait Until Keyword Succeeds
    ...  15 secs
    ...  5 secs
    ...  Check Multiple Logs Contains Data From Multisend  ./tmp  ${NumberOfPublishers}  ${NumberOfSubscribers}  ${DataItems}  ${channelName1}_${channelName2}_${channelName3}  ${channelName1}

    Wait Until Keyword Succeeds
    ...  15 secs
    ...  5 secs
    ...  Check Multiple Logs Contains Data From Multisend  ./tmp  ${NumberOfPublishers}  ${NumberOfSubscribers}  ${DataItems}  ${channelName1}_${channelName2}_${channelName3}  ${channelName2}

    Wait Until Keyword Succeeds
    ...  15 secs
    ...  5 secs
    ...  Check Multiple Logs Contains Data From Multisend  ./tmp  ${NumberOfPublishers}  ${NumberOfSubscribers}  ${DataItems}  ${channelName1}_${channelName2}_${channelName3}  ${channelName3}

    Wait Until Keyword Succeeds
    ...  15 secs
    ...  5 secs
    ...  Check Multiple Logs Contains Data From Multisend  ./tmp  ${NumberOfPublishers}  ${NumberOfSubscribers}  ${DataItems}  ${channelName2}_${channelName3}  ${channelName2}

    Wait Until Keyword Succeeds
    ...  15 secs
    ...  5 secs
    ...  Check Multiple Logs Contains Data From Multisend  ./tmp  ${NumberOfPublishers}  ${NumberOfSubscribers}  ${DataItems}  ${channelName2}_${channelName3}  ${channelName3}

    Check Multiple Logs Do Not Contain Data From Multisend  ./tmp  ${NumberOfPublishers}  ${NumberOfSubscribers}  ${channelName2}_${channelName3}  ${channelName1}

    Wait Until Keyword Succeeds
    ...  15 secs
    ...  5 secs
    ...  Check Multiple Logs Contains Data From Multisend  ./tmp  ${NumberOfPublishers}  ${NumberOfSubscribers}  ${DataItems}  ${channelName3}  ${channelName3}

    Check Multiple Logs Do Not Contain Data From Multisend  ./tmp  ${NumberOfPublishers}  ${NumberOfSubscribers}  ${channelName3}  ${channelName1}
    Check Multiple Logs Do Not Contain Data From Multisend  ./tmp  ${NumberOfPublishers}  ${NumberOfSubscribers}  ${channelName3}  ${channelName2}



*** Keywords ***

Send Data Over Data Channel
    [Arguments]    ${Channel_Name}   @{DataItems}
    FOR    ${item}     IN      @{DataItems}
    Send Data  1   ${ChannelName}   ${item}
    END

Check Log Contains Data
    [Arguments]    ${Channel_Name}  ${Subscriber}  @{DataItems}
    FOR    ${item}     IN      @{DataItems}
    Check Log Contains  Sub ${Subscriber} received message: [b'${ChannelName}', b'${item}']  ./tmp/fake_multi_subscriber.log  Fake Multi-Subscriber Log
    END

Check Log Does Not Contain Data
    [Arguments]    ${Channel_Name}  ${Subscriber}  @{DataItems}
    FOR    ${item}     IN      @{DataItems}
    Check Log Does Not Contain  Sub ${Subscriber} received message: [b'${ChannelName}', b'${item}']  ./tmp/fake_multi_subscriber.log  Fake Multi-Subscriber Log
    END

Test Teardown For Suite
    General Test Teardown
    Run Keyword If Test Failed  Dump Log  ./tmp/management.log
    Run Keyword If Test Failed  Dump Log  ./tmp/fake_multi_subscriber.log
    Stop Management Agent
    Clean Up Multi Subscriber
    Run Keyword If Test Failed  List Files In Directory  ${tmpdir}
    Remove Directory   ${tmpdir}    recursive=True
    Terminate All Processes  kill=True

Test Teardown For Verify Messages Sent From Multiple Publishers On Same Channel Concurrently Arrive To Subscriber
    Run Keyword If Test Failed
    ...     dump log   ./tmp/fake_multi_subscriber.log
    Test Teardown For Suite
