*** Settings ***
Library     OperatingSystem

Library    ${COMMON_TEST_LIBS}/CentralUtils.py
Library    ${COMMON_TEST_LIBS}/LogUtils.py
Library    ${COMMON_TEST_LIBS}/MCSRouter.py
Library    ${COMMON_TEST_LIBS}/OnFail.py
Library    ${COMMON_TEST_LIBS}/UserAndGroupReconfigurationUtils.py

Resource    ${COMMON_TEST_ROBOT}/AVResources.robot
Resource    ${COMMON_TEST_ROBOT}/ProductAcceptanceTestsResources.robot
Resource    ${COMMON_TEST_ROBOT}/ResponseActionsResources.robot
Resource    ${COMMON_TEST_ROBOT}/TelemetryResources.robot
Resource    ${COMMON_TEST_ROBOT}/UpgradeResources.robot
Resource    ${COMMON_TEST_ROBOT}/WatchdogResources.robot

Suite Setup      Upgrade Resources Suite Setup

Test Setup       Require Uninstalled
Test Teardown    Upgrade Resources SDDS3 Test Teardown

Force Tags  TAP_PARALLEL2    SMOKE

*** Variables ***
${STRACE}   /usr/bin/strace

*** Test Cases ***
Verify That There Are No Existing Configs to Openssl
    File Should Exist  ${STRACE}
	# Install VUT product
    Start Local Cloud Server
    ${handle} =    Start Local SDDS3 Server
    Set Suite Variable    ${GL_handle}    ${handle}
    ${sul_mark} =    mark_log_size    ${SULDOWNLOADER_LOG_PATH}
    ${rtd_mark} =    mark_log_size    ${SOPHOS_INSTALL}/plugins/runtimedetections/log/runtimedetections.log

    Configure And Run SDDS3 Thininstaller    ${0}    https://localhost:8080    https://localhost:8080
    Override LogConf File as Global Level    DEBUG
    wait_for_log_contains_from_mark    ${sul_mark}    Update success    ${150}
    Run Keyword Unless
    ...  ${KERNEL_VERSION_TOO_OLD_FOR_RTD}
    ...  wait_for_log_contains_from_mark    ${rtd_mark}    Analytics started processing telemetry    20

    Run Process   systemctl stop sophos-spl   shell=yes

    Wait Until Keyword Succeeds
    ...  10 secs
    ...  0.5 secs
    ...  Check Watchdog Not Running

    # Configure watchdog to run through strace:
    ${serviceDir} =    Get Service Folder
    ${result} =    Run Process    sed    -i    s@\"/opt/sophos-spl/base/bin/sophos_watchdog\"@${STRACE} -o /tmp/strace.log -e trace\=open,openat -t -f ${SOPHOS_INSTALL}/base/bin/sophos_watchdog@g    ${serviceDir}/sophos-spl.service
    Log File    ${serviceDir}/sophos-spl.service
    Should Be Equal As Integers    ${result.rc}    ${0}

    Run Process    systemctl    daemon-reload

    Register Cleanup    Remove File  /tmp/strace.log
    ${systemctlResult} =    Run Process    systemctl start sophos-spl    shell=yes
    Should Be Equal As Integers    ${systemctlResult.rc}    ${0}   ${systemctlResult.stderr}
    Wait for All Processes To Be Running

    Register Cleanup    Cleanup Telemetry Server
    Prepare To Run Telemetry Executable
    Run Telemetry Executable     ${EXE_CONFIG_FILE}     ${SUCCESS}

    check avscanner can detect eicar

    ${sul_mark} =    mark_log_size  ${SULDOWNLOADER_LOG_PATH}
    Trigger Update Now
    wait_for_log_contains_from_mark  ${sul_mark}    Update success    ${200}

    HttpsServer.Start Https Server  ${CERT_PATH}  443  tlsv1_2
    ${response_mark} =    mark_log_size    ${RESPONSE_ACTIONS_LOG_PATH}
    Register Cleanup    Remove Directory    /tmp/folder/  recursive=${TRUE}
    Send_download_file_from_fake_cloud
    wait_for_log_contains_from_mark    ${response_mark}    Action correlation-id has succeeded    25
    Stop Https Server

    Run Process    systemctl stop sophos-spl    shell=yes

    check_strace_log_for_openssl_mentions  /tmp/strace.log
