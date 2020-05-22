*** Settings ***
Documentation    Suite description

Library    OperatingSystem
Library    Process
Resource  ../GeneralTeardownResource.robot
Library     ${LIBS_DIRECTORY}/TeardownTools.py
Library     ${LIBS_DIRECTORY}/FaultInjectionTools.py
Library     ${LIBS_DIRECTORY}/OSUtils.py
Resource  ../installer/InstallerResources.robot

Suite Setup  ZMQ Suite Setup

Suite Teardown  ZMQ Suite Teardown

Test Teardown   ZMQ Test Teardown

Default Tags  FAULTINJECTION

*** Variables ***
${ZMQCheckerSourceDir} =  ${LIBS_DIRECTORY}/../SystemProductTestOutput/zmqchecker
${ZMQCheckerDir} =  /tmp/zmqchecker

${TESTUSER} =  sophos-spl-user
${IPCPATH} =   /tmp/zmqtestdir
${TESTIPCCUSTOM} =   ${IPCPATH}/test.ipc
${TESTIPCTMP} =   /var/tmp/test.ipc
${REQUEST} =  req


*** Test Cases ***
Test IPC Path With Enough Permission To Create Channel Successfully Communicates
    ${handle} =  Start Process  sudo  -u  ${TESTUSER}  ${ZMQCheckerDir}/zmqchecker  rep  ${TESTIPCTMP}  alias=replierprocess
    Wait Until Keyword Succeeds
    ...  7s
    ...  1s
    ...  IPC File Should Exists  ${TESTIPCTMP}
    Run ZMQ Checker Executable As Low Priviledged User  req  ${TESTIPCTMP}  0  world
    ${result} =    Wait For Process    replierprocess  timeout=15 s  on_timeout=terminate
    Log  ${result.stdout}
    Log  ${result.stderr}
    Should Contain   ${result.stdout}  hello

Test IPC Path Replier Without Enough Permission To Create Channel Will Throw ZMQWrapperException Failed To Bind
    #start replier as root
    Create Directory  ${IPCPATH}
    ${handle} =  Start Process  sudo  -u  ${TESTUSER}  ${ZMQCheckerDir}/zmqchecker  rep  ${TESTIPCCUSTOM}  alias=replierprocess
    Wait Until Keyword Succeeds
    ...  7s
    ...  1s
    ...  Ipc File Should Not Exists  ${TESTIPCCUSTOM}

    # replier expected to throw as it does not have permission to create the socket in the directory that is owned by root.
    # ZeroMQWrapperException -> Failed to bind to ipc:...
    ${result} =   Wait For Process   replierprocess  timeout=5 s  on_timeout=terminate
    Log  ${result.stdout}
    Log  ${result.stderr}
    Should Contain   ${result.stdout}   Failed to bind to ipc:///tmp/zmqtestdir/test.ipc
    Should Be Equal As Integers    ${result.rc}    2  msg=not the expected result.


Test IPC Path Requester Without Enough Permission To Create Channel Will Throw ZMQWrapperException Failed To Receive Message

    #start replier as root
    Create Directory  ${IPCPATH}
    ${handle} =  Start Process   ${ZMQCheckerDir}/zmqchecker  rep  ${TESTIPCCUSTOM}  alias=replierprocess
    Wait Until Keyword Succeeds
    ...  7s
    ...  1s
    ...  Ipc File Should Exists  ${TESTIPCCUSTOM}

    ${handle} =  Start Process  sudo  -u  ${TESTUSER}  ${ZMQCheckerDir}/zmqchecker  req  ${TESTIPCCUSTOM}  alias=requesterprocess

    # requester expected to throw "Failed to receive message component"
    # This because it will have no permission to create a channel in a directory owned by root
    # but it will not fail on the method to write due to specific implementation of the zmq socket.
    # The problem will be detect only on reading the answer
    ${result_requester} =    Wait For Process    requesterprocess  timeout=10 s  on_timeout=terminate
    Log  ${result_requester.stdout}
    Log  ${result_requester.stderr}
    Should Contain   ${result_requester.stdout}   Failed to receive message component


    ${result_replier} =    Wait For Process    replierprocess  timeout=1 s  on_timeout=terminate
    Log  ${result_replier.stdout}
    Log  ${result_replier.stderr}
    Should Not Contain   ${result_replier.stdout}   hello

    Should Not Be Equal As Integers    ${result_requester.rc}    0  msg=not the expected result.

Test IPC Path Requester Without Enough Permissions to Connect To The Channel Will Throw ZMQWrapperException Failed To Receive Message

    #start replier as root
    ${handle} =  Start Process  ${ZMQCheckerDir}/zmqchecker  rep  ${TESTIPCTMP}  alias=replierprocess
    #ipc is created with user without write permission and having read and execute permissions.
    Wait Until Keyword Succeeds
    ...  7s
    ...  1s
    ...  IPC File Should Exists  ${TESTIPCTMP}

    # requester will fail to bind to ipc:///tmp/zmqtestdir and throw ZeroMQWrapperException -> "Failed to receive message component"
    # Because  it does not have permission to connect to a server socket established by replier running as root.
    Run ZMQ Checker Executable As Low Priviledged User  req  ${TESTIPCTMP}  2  Failed to receive message component

    #replier will wait
    # and then recieve sigterm and exit gracefully
    ${result} =    Wait For Process    replierprocess  timeout=1 s  on_timeout=terminate
    Log  ${result.stdout}
    Log  ${result.stderr}

Test A Bad Client That Fails To Read Its Response From Replier Does Not Interfere With The service Of other Clients
    ${handle1} =  Start Process  sudo  -u  ${TESTUSER}  ${ZMQCheckerDir}/zmqchecker  rep  ${TESTIPCTMP}  continue  alias=replierprocess
    Wait Until Keyword Succeeds
    ...  7s
    ...  1s
    ...  IPC File Should Exists  ${TESTIPCTMP}
    #This requester will not read its reply
    ${handle2} =  Start Process  sudo  -u  ${TESTUSER}  ${ZMQCheckerDir}/zmqchecker  req-noread  ${TESTIPCTMP}  alias=requesterprocess_1
    ${handle1} =  Start Process  sudo  -u  ${TESTUSER}  ${ZMQCheckerDir}/zmqchecker  req  ${TESTIPCTMP}  alias=requesterprocess_2
    #should both successfully send


    ${result_requester2} =    Wait For Process    requesterprocess_2  timeout=10 s  on_timeout=terminate
    Log  ${result_requester2.stdout}
    Log  ${result_requester2.stderr}
    Should Contain   ${result_requester2.stdout}   world

    ${result_requester1} =    Wait For Process    requesterprocess_1  timeout=1 s  on_timeout=terminate
    Log  ${result_requester1.stdout}
    Log  ${result_requester1.stderr}
    Should Not Contain   ${result_requester1.stdout}  world


    ${result_replier} =    Wait For Process    replierprocess  timeout=1 s  on_timeout=terminate
    Log  ${result_replier.stdout}
    Log  ${result_replier.stderr}
    Should Contain   ${result_replier.stdout}   hello

    Should Be Equal As Integers    ${result_requester1.rc}    0  msg=not the expected result, actual result= ${result_requester1.rc}.
    Should Be Equal As Integers    ${result_requester2.rc}    0  msg=not the expected result, actual result= ${result_requester2.rc}.


Test IPC Requester Does Not Exists Or Does Not Respond Replier Will Continue To Service Other Clients
    #Create a requester process and replier process that consumes and never acknowledge

    ${handle} =  Start Process  sudo  -u  ${TESTUSER}  ${ZMQCheckerDir}/zmqchecker  rep  ${TESTIPCTMP}  alias=replierprocess
    Wait Until Keyword Succeeds
    ...  7s
    ...  1s
    ...  IPC File Should Exists  ${TESTIPCTMP}
    Wait For Process    replierprocess  timeout=30 s  on_timeout=continue

    ${result} =   Terminate Process  ${handle}
    Log  ${result.stdout}
    Log  ${result.stderr}
    Should Not Contain    Log  ${result.stdout}  exception


Test IPC Replier Does Not Exists Or Does Not Respond Will Throw ZMQWrapperException Failed To Receive Message
    # this replier will not respond to the requests
    ${handle1} =  Start Process  sudo  -u  ${TESTUSER}  ${ZMQCheckerDir}/zmqchecker  rep-noreply  ${TESTIPCTMP}  alias=replierprocess
    Wait Until Keyword Succeeds
    ...  7s
    ...  1s
    ...  IPC File Should Exists  ${TESTIPCTMP}

    ${handle2} =  Start Process  sudo  -u  ${TESTUSER}  ${ZMQCheckerDir}/zmqchecker  req  ${TESTIPCTMP}  alias=requesterprocess

    #  Requester throws
    #  ZeroMQWrapperException Failed to receive message component
    ${result_requester} =  Wait For Process    requesterprocess  timeout=10 s  on_timeout=terminate
    Log  ${result_requester.stdout}
    Log  ${result_requester.stderr}
    Should Contain   ${result_requester.stdout}   Failed to receive message component

    ${result_replier} =    Wait For Process    replierprocess  timeout=1 s  on_timeout=terminate
    Log  ${result_replier.stdout}
    Log  ${result_replier.stderr}

    Should Be Equal As Integers    ${result_requester.rc}    2  msg=not the expected result, actual result= ${result_requester.rc}.

*** Keywords ***
Run ZMQ Checker Executable As Low Priviledged User
    [Arguments]  ${connectionType}  ${ipcPath}  ${expectedResult}  ${expectedOutput}
    ${result} =  Run Process  sudo  -u  ${TESTUSER}  ${ZMQCheckerDir}/zmqchecker  ${connectionType}  ${ipcPath}
    Log  ${result.stdout}
    Log  ${result.stderr}
    Should Contain  ${result.stdout}  ${expectedOutput}
    Should Be Equal As Integers   ${result.rc}    ${expectedResult}   msg=not the expected result.

Move Executable To Tmp
    #RHEL based system will not allow executable to be ran from path that include home directory
    #Move to tmp brfore running tests
    Copy Directory  ${ZMQCheckerSourceDir}  /tmp/

ZMQ Suite Setup
    Require Installed
    Move Executable To Tmp

ZMQ Suite Teardown
    Ensure Uninstalled
    Run Process  rm  -rf  ${ZMQCheckerDir}

Get Pid of zmqchecker
    ${result} =    Run Process  pidof  zmqchecker
    Should Be Equal As Integers    ${result.rc}    0   msg=${result.stderr}
    [Return]  ${result.stdout}

Describe information for given Pid
    [Arguments]  ${pid}
    ${result}=    Run Process  ls -l /proc/${pid}/fd  shell=True
    Log  ${result.stdout}

    ${result}=    Run Process  cat /proc/${pid}/status  shell=True
    Log  ${result.stdout}

    ${result}=    Run Process  cat /proc/${pid}/environ  shell=True
    Log  ${result.stdout}

    ${result}=    Run Process  ps -elfT | grep ${pid}  shell=True
    Log  ${result.stdout}


Report Status and Output of Replier
    # can be sudo
    ${pid}=   Process.Get Process Id  replierprocess
    Describe information for given Pid  ${pid}

    #ensure it is zmqchecker    
    ${pid}=  Get Pid of zmqchecker
    Describe information for given Pid  ${pid}

    ${result}=    Run Process  ldd  ${ZMQCheckerDir}/zmqchecker  shell=True
    Log  ${result.stdout} 
    
    # get average load time
    ${result}=    Run Process  uptime  shell=True
    Log  ${result.stdout}

    ${result} =    Wait For Process    replierprocess  timeout=15 s  on_timeout=terminate
    Log  ${result.stdout}
    Log  ${result.stderr}

    # secure may report the sudo sessions
    Dump Teardown Log   /var/log/dnf.log
    Dump Teardown Log   /var/log/secure

ZMQ Test Teardown
    Run Keyword If Test Failed   Display List Files Dash L In Directory   /tmp/
    Run Keyword If Test Failed   Display List Files Dash L In Directory   /var/tmp/
    Run Keyword If Test Failed   Report Status and Output of Replier
    General Test Teardown
    Run Process  pkill  -f  zmqchecker
    Run Process  rm  -f  ${TESTIPCCUSTOM}
    Run Process  rm  -f  ${TESTIPCTMP}
    Remove Directory  ${IPCPATH}
