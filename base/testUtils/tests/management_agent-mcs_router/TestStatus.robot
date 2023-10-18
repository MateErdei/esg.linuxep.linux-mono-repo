*** Settings ***
Library    OperatingSystem
Library    ${LIBS_DIRECTORY}/MCSRouter.py
Library    ${LIBS_DIRECTORY}/FakePluginWrapper.py
Library    ${COMMON_TEST_LIBS}/LogUtils.py

Resource    ../management_agent/ManagementAgentResources.robot
Resource    ../mcs_router/McsRouterResources.robot
Resource    ../installer/InstallerResources.robot

*** Variables ***
${SophosManagementLog}      ${SOPHOS_INSTALL}/logs/base/sophosspl/sophos_managementagent.log
${MCS_ROUTER_LOG}           ${SOPHOS_INSTALL}/logs/base/sophosspl/mcsrouter.log

*** Test Case ***
Verify Status Sent To Management Agent Will Be Passed To MCS And Received In Fake Cloud
    [Tags]  MANAGEMENT_AGENT  MCS  FAKE_CLOUD  MCS_ROUTER
    Register With Local Cloud Server
    Check Correct MCS Password And ID For Local Cloud Saved
    Start MCSRouter

    Remove Status Xml Files

    Setup Plugin Registry
    Start Management Agent

    Start Plugin

    ${statusContent}    Evaluate    str('status contents')
    Send Plugin Status   ${statusContent}

    Check Status File           ${statusContent}
    Check Status Cache File     ${statusContent}

    Wait Until Keyword Succeeds
    ...  1 min
    ...  5 secs
    ...  Check Cloud Server Log Contains    ${statusContent}   1

    #Negative test to confirm these two processes are never registered as plugins LINUXDAR-1637
    Check Log Does Not Contain    Registered plugin mcsrouter   ${SophosManagementLog}    Sophos Management Agent
    Check Log Does Not Contain    Registered plugin managementagent   ${SophosManagementLog}    Sophos Management Agent

Verify Health Status Sent To Cloud Only If Changed
    [Tags]  MANAGEMENT_AGENT  MCS  FAKE_CLOUD  MCS_ROUTER
    Register With Local Cloud Server
    Check Correct MCS Password And ID For Local Cloud Saved

    Create File  ${SOPHOS_INSTALL}/base/etc/logger.conf.local  [mcs_router]\nVERBOSITY=DEBUG\n
    ${mcsrouter_mark} =  Mark Log Size    ${MCS_ROUTER_LOG}
    Start MCSRouter

    Remove Status Xml Files

    Setup Plugin Registry
    Start Management Agent

    Start Plugin

    Wait For Log Contains From Mark  ${mcsrouter_mark}  Adapter SHS changed status  timeout=${120}

    Wait Until Keyword Succeeds
    ...  1 min
    ...  5 secs
    ...  Check Cloud Server Log Contains  Endpoint health status set to

    ${mcsrouter_mark}=  Restart MCSRouter And Clear Logs

    Wait For Log Contains From Mark  ${mcsrouter_mark}  Adapter SHS hasn't changed status  timeout=${120}
