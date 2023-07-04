*** Settings ***
Suite Setup      Upgrade Resources Suite Setup
Suite Teardown   Upgrade Resources Suite Teardown

Test Setup       Upgrade Resources Test Setup
Test Teardown    Run Keywords
...                Remove Environment Variable  http_proxy    AND
...                Remove Environment Variable  https_proxy  AND
...                Stop Proxy If Running    AND
...                Stop Proxy Servers   AND
...                Clean up fake warehouse  AND
...                Remove Environment Variable  COMMAND  AND
...                Remove Environment Variable  EXITCODE  AND
...                Upgrade Resources SDDS3 Test Teardown

Library    DateTime
Library     ${LIBS_DIRECTORY}/FakeSDDS3UpdateCacheUtils.py
Library     ${LIBS_DIRECTORY}/PolicyUtils.py

Resource    ../scheduler_update/SchedulerUpdateResources.robot
Resource    ../installer/InstallerResources.robot
Resource    ../update/SDDS3Resources.robot
Resource    ../upgrade_product/UpgradeResources.robot
Resource    SulDownloaderResources.robot

Default Tags  SULDOWNLOADER
Force Tags  LOAD6

*** Variables ***
${sdds3_server_output}                      /tmp/sdds3_server.log
${UpdateSchedulerLog}                       ${SOPHOS_INSTALL}/logs/base/sophosspl/updatescheduler.log

*** Test Cases ***
ALC Policy With ESM Is Received By Update Scheduler
    ${content} =    Populate Esm Fixed Version    LTS 2023.1.1    f4d41a16-b751-4195-a7b2-1f109d49469d
    Create File    /tmp/tmpALC.xml    ${content}
    LOG    ${content}

    ${update_mark} =  mark_log_size    ${UpdateSchedulerLog}

    Start Local Cloud Server    --initial-alc-policy  /tmp/tmpALC.xml
    ${handle}=  Start Local SDDS3 Server With Empty Repo
    Set Suite Variable    ${GL_handle}    ${handle}
    Require Fresh Install
    Create File    ${MCS_DIR}/certs/ca_env_override_flag
    Create Local SDDS3 Override

    Register With Local Cloud Server

    Wait Until Keyword Succeeds
    ...    10s
    ...    1s
    ...    Log File    ${UPDATE_CONFIG}


    wait_for_log_contains_from_mark  ${update_mark}  Using FixedVersion f4d41a16-b751-4195-a7b2-1f109d49469d with token LTS 2023.1.1


*** Keywords ***
Check MCS Envelope Contains Event with Update cache
    [Arguments]  ${Event_Number}
    ${string}=  Check Log And Return Nth Occurrence Between Strings   <event><appId>ALC</appId>  </event>  ${SOPHOS_INSTALL}/logs/base/sophosspl/mcs_envelope.log  ${Event_Number}
    Should contain   ${string}   updateSource&gt;4092822d-0925-4deb-9146-fbc8532f8c55&lt

File Should Contain
    [Arguments]  ${file_path}  ${expected_contents}
    ${contents}=  Get File   ${file_path}
    Should Contain  ${contents}   ${expected_contents}
