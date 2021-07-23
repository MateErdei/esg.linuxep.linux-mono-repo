*** Settings ***
Resource  EventJournalerResources.robot
Suite Setup  Setup
Test Teardown   Event Journaler Teardown
Suite Teardown  Uninstall Base

*** Test Cases ***
Event Journaler Can Receive Events
    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  Should Exist   ${EVENT_JOURNALER_LOG_PATH}
    ${mark} =  Mark File  ${EVENT_JOURNALER_LOG_PATH}
    Publish Threat Event
    #{"details":{"filePath":"/tmp/dirty_file","sha256FileHash":"275a021bbfb6489e54d471899f7db9d1663fc695ec2fe2a2c4538aabf651fd0f"},"detectionName":{"short":"EICAR-AV-Test"},"items":{"1":{"path":"/tmp/dirty_file","primary":true,"sha256":"275a021bbfb6489e54d471899f7db9d1663fc695ec2fe2a2c4538aabf651fd0f","type":1}},"threatSource":1,"threatType":1,"time":
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  5 secs
    ...  Check Journal Contains Detection Event With Content   {"threatName":"EICAR-AV-Test","threatPath":"/home/admin/eicar.com"}

Event Journaler Restarts Subscriber After Socket Removed
    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  Should Exist   ${EVENT_JOURNALER_LOG_PATH}
    ${mark} =  Mark File  ${EVENT_JOURNALER_LOG_PATH}
    sleep  5
    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  Should Exist  ${SOPHOS_INSTALL}/var/ipc/events.ipc

    # Delete socket file, subscriber should exit and Journaler should restart it.
    Remove Subscriber Socket

    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  Check Marked Event Journaler Log Contains  The subscriber socket has been unexpectedly removed   ${mark}

    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  Check Marked Event Journaler Log Contains  Subscriber not running, restarting it   ${mark}

    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  Check Marked Event Journaler Log Contains  Subscriber restart called   ${mark}


Event Journaler Can Receive Events From Multiple Publishers
    [Teardown]  Custom Teardown For Tests With Publishers Running In Background
    ${mark} =  Mark File  ${EVENT_JOURNALER_LOG_PATH}
    Publish Threat Events In Background Process   "msg-from-pub1"  100
    Publish Threat Events In Background Process   "msg-from-pub2"  100
    Publish Threat Events In Background Process   "msg-from-pub3"  100

    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  Check Marked Event Journaler Log Contains  msg-from-pub1  ${mark}

    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  Check Marked Event Journaler Log Contains  msg-from-pub2  ${mark}

    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  Check Marked Event Journaler Log Contains  msg-from-pub2  ${mark}


Event Journaler Can Receive Many Events From Publisher
    [Teardown]  Custom Teardown For Tests With Publishers Running In Background
    ${mark} =  Mark File  ${EVENT_JOURNALER_LOG_PATH}
    Publish Threat Events In Background Process   "msg-from-pub1"  100
    Wait Until Marked Journaler Log Contains String X Times  msg-from-pub1  100  ${mark}


Event Journaler Can Be Stopped And Started
    Restart Event Journaler


*** Keywords ***
Setup
    Install Base For Component Tests
    Install Event Journaler Directly from SDDS
    Run Shell Process  chmod a+x ${EVENT_PUB_SUB_TOOL}  OnError=Failed to chmod EventPubSub binary tool   timeout=3s
    Run Shell Process  chmod a+x ${EVENT_READER_TOOL}  OnError=Failed to chmod JournalReader binary tool   timeout=3s

Custom Teardown For Tests With Publishers Running In Background
    Event Journaler Teardown
    Terminate All Processes
