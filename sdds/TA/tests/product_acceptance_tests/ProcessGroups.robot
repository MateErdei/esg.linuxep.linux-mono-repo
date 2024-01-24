*** Settings ***
Library    ${COMMON_TEST_LIBS}/CentralUtils.py
Library    ${COMMON_TEST_LIBS}/LogUtils.py
Library    ${COMMON_TEST_LIBS}/OnFail.py
Library    ${COMMON_TEST_LIBS}/UserAndGroupReconfigurationUtils.py
Library    ${COMMON_TEST_LIBS}/LiveResponseUtils.py
Library    ${COMMON_TEST_LIBS}/WebsocketWrapper.py
Library    ${COMMON_TEST_LIBS}/OSUtils.py
Library    ProcessGroupCheck.py

Resource    ${COMMON_TEST_ROBOT}/AVResources.robot
Resource    ${COMMON_TEST_ROBOT}/ResponseActionsResources.robot
Resource    ${COMMON_TEST_ROBOT}/TelemetryResources.robot
Resource    ${COMMON_TEST_ROBOT}/UpgradeResources.robot
Resource    ${COMMON_TEST_ROBOT}/WatchdogResources.robot
Resource    ${COMMON_TEST_ROBOT}/LiveResponseResources.robot
Resource    ${COMMON_TEST_ROBOT}/DiagnoseResources.robot
Resource    ProductAcceptanceTestsResources.robot

Suite Setup      Upgrade Resources Suite Setup

Test Setup       Require Uninstalled
Test Teardown    Upgrade Resources SDDS3 Test Teardown

Force Tags    TAP_PARALLEL2    REGRESSION

*** Test Cases ***
Check All Sophos Processes Use Sophos Spl Group
    [Tags]    THININSTALLER    RTD_CHECKED
    Start Process Group Check
    Run All Sophos Processes
    Finish Process Group Check


*** Keywords ***
Run All Sophos Processes
    [Timeout]    10 minutes
    Start Local Cloud Server
    ${handle} =    Start Local SDDS3 Server
    Configure And Run SDDS3 Thininstaller    0    https://localhost:8080    https://localhost:8080

    Override LogConf File as Global Level    DEBUG
    Run Process    systemctl restart sophos-spl    shell=${True}

    # Wait for persistent processes to be running
    Wait for All Processes To Be Running

    # Then make sure non-persistent processes have been run at least once

    # Telemetry executable
    Register Cleanup    Cleanup Telemetry Server
    Prepare To Run Telemetry Executable
    ${time} =  Get Current Time
    ${scheduled_time_in_past} =  Expected Scheduled Time  ${time}  -5
    Set Scheduled Time In Scheduler Status File   ${scheduled_time_in_past}
    Restart Telemetry Scheduler
    Wait For Telemetry Executable To Have Run

    # Command-line scanner
    Check Avscanner Can Detect Eicar

    # SulDownloader
    ${sul_mark} =    Mark Log Size    ${SULDOWNLOADER_LOG_PATH}
    Trigger Update Now
    Wait For Log Contains From Mark    ${sul_mark}    Update success    ${150}

    # Action Runner
    HttpsServer.Start Https Server    ${CERT_PATH}    443    tlsv1_2
    ${response_mark} =    Mark Log Size    ${RESPONSE_ACTIONS_LOG_PATH}
    Register Cleanup    Remove Directory    /tmp/folder/    recursive=${TRUE}
    Create And Send Download File From Fake Cloud
    Wait For Log Contains From Mark    ${response_mark}    Action correlation-id has succeeded
    Stop Https Server

    # Liveterminal
    Register Cleanup    Uninstall Lt Server Certificates
    Install Lt Server Certificates

    Start Websocket Server
    ${websocket_server_log} =    get_log_path
    Register Cleanup    Stop Websocket Server
    Register Cleanup    LogUtils.Dump Log  ${websocket_server_log}

    ${random_path} =    generate_random_path
    ${liveResponse} =    Create Live Response Action Fake Cloud  ${websocket_server_url}/${random_path}  ${Thumbprint}
    ${correlation_id} =    Get Correlation Id
    Run Live Response    ${liveResponse}    ${correlation_id}
    wait_for_check_that_command_prompt_has_started    /${random_path}

    Send Message With Newline    ${SOPHOS_INSTALL}/base/bin/SulDownloader ${SOPHOS_INSTALL}/base/update/var/updatescheduler/update_config.json ${SOPHOS_INSTALL}/base/update/var/updatescheduler/update_report.json    ${random_path}
    ${path} =    Set Variable    /tmp/test_${correlation_id}.txt
    Send Message With Newline    touch ${path}    ${random_path}

    Wait Until Created    ${path}
    # The permissions differ between -rw-r--r-- and -rw------- on some platforms
    Check Owner Matches    root    ${path}
    Check Group Matches    sophos-spl-group    ${path}
    Remove File   ${path}
    Send Message With Newline    exit    ${random_path}


    # sophos_diagnose
    ${remote_diagnose_log_mark} =  Mark Log Size    ${SOPHOS_INSTALL}/logs/base/sophosspl/remote_diagnose.log
    Simulate SDU Action Now
    Wait For Log Contains From Mark    ${remote_diagnose_log_mark}    Diagnose finished    ${180}
