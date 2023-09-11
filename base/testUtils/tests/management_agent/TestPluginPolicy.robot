*** Settings ***
Documentation     Test the Management Agent

Library           Process
Library           OperatingSystem
Library           Collections

Library     ${COMMON_TEST_LIBS}/LogUtils.py
Library     ${LIBS_DIRECTORY}/FakePluginWrapper.py

Resource    ManagementAgentResources.robot
Test Teardown   MA Fake Plugin Teardown

Force Tags    TAP_PARALLEL1

*** Test Cases ***
Verify Management Agent Sets Plugin Policy When Moved Into Policy Folder
    [Tags]    MANAGEMENT_AGENT  SMOKE
    # Test will invoke applyNewPolicy when file is moved into place

    # make sure no previous policy xml file exists.
    Remove Policy Xml Files

    Setup Plugin Registry
    Start Management Agent

    Start Plugin

    # Write policy file.
    ${policyFileName} =    Set Variable    ${SOPHOS_INSTALL}/base/mcs/policy/SAV-1_policy.xml
    ${policyTmpName} =    Set Variable   ${SOPHOS_INSTALL}/base/mcs/policy/TMP-SAV-1_policy.xml
    ${policyContents} =    Set Variable   'policy1'
    Create File     ${policyTmpName}   ${policyContents}
    Move File       ${policyTmpName}   ${policyFileName}

    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  Get Plugin Policy

    ${pluginPolicy} =    Get Plugin Policy


    Should Be True   """${pluginPolicy}""" == """${policyContents}"""


Verify Management Agent Send Plugin Policy When Requested By Plugin
    [Tags]    MANAGEMENT_AGENT
    # make sure no previous status xml file exists.
    Remove Policy Xml Files

    Setup Plugin Registry
    Start Management Agent

    Start Plugin

    #Policy will not be sent to the plugin if it is created directly in policy folder

    # Write policy file.
    ${policyFileName} =    Set Variable    ${SOPHOS_INSTALL}/base/mcs/policy/SAV-1_policy.xml
    ${policyContents} =    Set Variable   policyRequested1
    Create File     ${policyFileName}   ${policyContents}

    ${response} =    Send Plugin Request Policy
    ${expectedResponse} =   Set Variable    ACK

    Should Be True   """${response}""" == """${expectedResponse}"""

    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  Get Plugin Policy

    ${pluginPolicy} =    Get Plugin Policy

    Should Be True   """${pluginPolicy}""" == """${policyContents}"""

    #########
     # Update policy file.
    ${policyFileName} =    Set Variable    ${SOPHOS_INSTALL}/base/mcs/policy/SAV-1_policy.xml
    ${newPolicyContents} =    Set Variable   policyRequested2
    Create File     ${policyFileName}   ${newPolicyContents}

    # prove plugin policy is still the original policy
    Sleep  2
    ${pluginPolicy} =    Get Plugin Policy

    Should Be True   """${pluginPolicy}""" == """${policyContents}"""

    # prove plugin can get new policy
    ${response} =    Send Plugin Request Policy
    ${expectedResponse} =   Set Variable    ACK

    Should Be True   """${response}""" == """${expectedResponse}"""

    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  Check Log Contains String N Times   ${SOPHOS_INSTALL}/logs/base/sophosspl/sophos_managementagent.log    Management Agent Log   Policy /opt/sophos-spl/base/mcs/policy/SAV-1_policy.xml applied to 1 plugins   2

    ${pluginPolicy} =    Get Plugin Policy

    Should Be True   """${pluginPolicy}""" == """${newPolicyContents}"""


Verify Management Agent Sets Plugin Policy When Moved Into Internal Policy Folder
    [Tags]    MANAGEMENT_AGENT  SMOKE
    # Test will invoke applyNewPolicy when file is moved into place

    # make sure no previous policy xml file exists.
    Remove Policy Xml Files

    Setup Plugin Registry
    Start Management Agent

    Start Plugin

    # Write policy file.
    ${policyFileName} =    Set Variable    ${SOPHOS_INSTALL}/base/mcs/internal_policy/internal_SAV.json
    ${policyTmpName} =    Set Variable   ${SOPHOS_INSTALL}/base/mcs/policy/TMP-SAV-1_policy.xml
    ${policyContents} =    Set Variable   'policy1'
    Create File     ${policyTmpName}   ${policyContents}
    Move File       ${policyTmpName}   ${policyFileName}

    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  Get Plugin Policy

    ${pluginPolicy} =    Get Plugin Policy


    Should Be True   """${pluginPolicy}""" == """${policyContents}"""


Verify Management Agent Send Internal Plugin Policy When Requested By Plugin
    [Tags]    MANAGEMENT_AGENT
    # make sure no previous status xml file exists.
    Remove Policy Xml Files

    Setup Plugin Registry
    Start Management Agent

    Start Plugin

    #Policy will not be sent to the plugin if it is created directly in policy folder

    # Write policy file.
    ${policyFileName} =    Set Variable    ${SOPHOS_INSTALL}/base/mcs/internal_policy/internal_SAV.json
    ${policyContents} =    Set Variable   policyRequested1
    Create File     ${policyFileName}   ${policyContents}

    ${response} =    Send Plugin Request Policy
    ${expectedResponse} =   Set Variable    ACK

    Should Be True   """${response}""" == """${expectedResponse}"""

    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  Get Plugin Policy

    ${pluginPolicy} =    Get Plugin Policy

    Should Be True   """${pluginPolicy}""" == """${policyContents}"""

    #########
     # Update policy file.
    ${newPolicyContents} =    Set Variable   policyRequested2
    Create File     ${policyFileName}   ${newPolicyContents}

    # prove plugin policy is still the original policy
    Sleep  2
    ${pluginPolicy} =    Get Plugin Policy

    Should Be True   """${pluginPolicy}""" == """${policyContents}"""

    # prove plugin can get new policy
    ${response} =    Send Plugin Request Policy
    ${expectedResponse} =   Set Variable    ACK

    Should Be True   """${response}""" == """${expectedResponse}"""

    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  Check Log Contains String N Times   ${SOPHOS_INSTALL}/logs/base/sophosspl/sophos_managementagent.log    Management Agent Log   Policy /opt/sophos-spl/base/mcs/internal_policy/internal_SAV.json applied to 1 plugins   2

    ${pluginPolicy} =    Get Plugin Policy

    Should Be True   """${pluginPolicy}""" == """${newPolicyContents}"""

*** Keywords ***
MA Fake Plugin Teardown
    Stop Plugin
    Stop Management Agent
    General Test Teardown

Remove Policy Xml Files
    @{policyfiles} = 	List Files In Directory 	${SOPHOS_INSTALL}/base/mcs/policy

    FOR    ${item}     IN      @{policyfiles}
    Remove File   ${SOPHOS_INSTALL}/base/mcs/policy/${item}
    END




