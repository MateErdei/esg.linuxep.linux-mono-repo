*** Settings ***
Documentation    Suite description

Resource  ../installer/InstallerResources.robot
Resource  AVResources.robot
Library     ${LIBS_DIRECTORY}/LogUtils.py
Resource  ../mcs_router/McsRouterResources.robot

Suite Setup     Run keywords
...             Setup For Fake Cloud  AND
...             Start Local Cloud Server  AND
...             Register With Fake Cloud

Suite Teardown  Run Keywords
...             Require Uninstalled  AND
...             Stop Local Cloud Server

Test Teardown  Run Keywords
...            Run Keyword If Test Failed  Dump Teardown Log  /tmp/av_install.log  AND
...            Remove File  /tmp/av_install.log  AND
...            General Test Teardown

Force Tags  LOAD4
Default Tags   AV_PLUGIN


*** Test Cases ***
Test av health is green right after install
    [Timeout]  10 minutes
    Wait Until Keyword Succeeds
    ...  45 secs
    ...  5 secs
    ...  Check Log Contains String N Times   ${SOPHOS_INSTALL}/logs/base/sophosspl/sophos_managementagent.log    Management Agent Log   Starting service health checks  1

    Install AV Plugin Directly
    Wait Until Keyword Succeeds
    ...  40 secs
    ...  10 secs
    ...  Check Log Contains String N Times   ${SOPHOS_INSTALL}/logs/base/watchdog.log   watchdog Log    Starting /opt/sophos-spl/plugins/av/sbin/sophos_threat_detector_launcher  2
    ${contents}=  Get File   ${AV_LOG_FILE}
    Should Not Contain  ${contents}   Health found previous Sophos Threat Detector process no longer running:

    ${EXPECTED_CONTENT}=  Set Variable  <detail name="Sophos Linux AntiVirus" value="0" />

    ${STATUS_CONTENT}=  Get File  /opt/sophos-spl/base/mcs/status/SHS_status.xml

    Should Contain   ${STATUS_CONTENT}  ${EXPECTED_CONTENT}

*** Keywords ***
Setup For Fake Cloud
    Regenerate Certificates
    Require Fresh Install
    Set Local CA Environment Variable
    Override LogConf File as Global Level  DEBUG
    Check For Existing MCSRouter
    Cleanup MCSRouter Directories
    Cleanup Local Cloud Server Logs


