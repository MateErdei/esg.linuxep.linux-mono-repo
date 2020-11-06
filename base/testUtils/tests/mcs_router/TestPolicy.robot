*** Settings ***
Library   OperatingSystem

Resource  McsRouterResources.robot

Suite Setup       Run Keywords
...               Setup MCS Tests  AND
...               Start Local Cloud Server  --verifycookies
Suite Teardown    Run Keywords
...               Stop Local Cloud Server  AND
...               Uninstall SSPL Unless Cleanup Disabled

Default Tags  MCS  FAKE_CLOUD  MCS_ROUTER  REGISTRATION

*** Test Case ***
# Note that our local cloud server does not support fragmented policy.
# Cloud may have a local cloud server that does support fragmented polices and
# it may be worth bringing this in when LNG requires this functionality.

Default Command Poll Interval Changed By MCS Policy
    Register With Local Cloud Server
    Check Correct MCS Password And ID For Local Cloud Saved
    Start MCSRouter
    Check Cloud Server Log For Command Poll
    Set MCS Policy Command Poll    10
    Check Cloud Server Log For Command Poll    4

    # Check for default of 10 seconds with tolerance of 2 seconds, ignore first two command polls
    Compare Default Command Poll Interval With Log    10    2    2
    Check Temp Policy Folder Doesnt Contain Policies

Default Policies Written to File
    [Tags]  MCS  FAKE_CLOUD  MCS_ROUTER  REGISTRATION
    Register With Local Cloud Server
    Check Correct MCS Password And ID For Local Cloud Saved
    Start MCSRouter
    Check Default Policies Exist
    Check Temp Policy Folder Doesnt Contain Policies
    Dump MCSRouter Log

Modified ALC Policy Is Written to File
    Register With Local Cloud Server
    Check Correct MCS Password And ID For Local Cloud Saved
    Start MCSRouter
    Check Default Policies Exist
    Send Fake ALC Policy
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  5 secs
    ...  Check Fake ALC Policy Written
    Check Temp Policy Folder Doesnt Contain Policies

Only One Policy Is Distributed When Receiving Multiple ALC Policies On Command Poll
    Register With Local Cloud Server
    Check Correct MCS Password And ID For Local Cloud Saved
    Start MCSRouter
    Check Default Policies Exist

    Mark Mcsrouter Log

    Remove File   ${SOPHOS_INSTALL}/base/mcs/policy/ALC-1_policy.xml
    ${FirstALCPolicy}=  Set Variable  ${SUPPORT_FILES}/CentralXml/ALC_policy_direct_just_base.xml
    Send Policy File  alc  ${FirstALCPolicy}
    ${SecondALCPolicy}=  Set Variable  ${SUPPORT_FILES}/CentralXml/ALC_policy_direct.xml
    Send Policy File  alc  ${SecondALCPolicy}
    ${ThirdALCPolicy}=  Set Variable  ${SUPPORT_FILES}/CentralXml/ALC_policy_direct_base_and_example_plugin.xml
    Send Policy File  alc  ${ThirdALCPolicy}
    ${FourthALCPolicy}=  Set Variable  ${SUPPORT_FILES}/CentralXml/ALC_policy_scheduled_update.xml
    Send Policy File  alc  ${FourthALCPolicy}
    ${FinalALCPolicy}=  Set Variable  ${SUPPORT_FILES}/CentralXml/ALC_policy_with_cache.xml
    Send Policy File  alc  ${FinalALCPolicy}

    Wait Until Keyword Succeeds
    ...  60 secs
    ...  5 secs
    ...  File Should Exist  ${SOPHOS_INSTALL}/base/mcs/policy/ALC-1_policy.xml

    # Check that only one policy is written to the MCS policy folder and that it is the last one received
    Check Policy Written Match File  ALC-1_policy.xml  ${FinalALCpolicy}
    Check Marked Mcsrouter Log Contains String N Times  Distribute new policy: ALC-1_policy.xml  1
    Check Temp Policy Folder Doesnt Contain Policies

    # Check that we have received all the ALC policies that were queued
    ${FirstALCPolicyContent}=  Get File  ${FirstALCPolicy}
    ${SecondALCPolicyContent}=  Get File  ${SecondALCPolicy}
    ${ThirdALCPolicyContent}=  Get File  ${ThirdALCPolicy}
    ${FourthALCPolicyContent}=  Get File  ${FourthALCPolicy}
    ${FinalALCPolicyContent}=  Get File  ${FinalALCPolicy}

    Check Mcsenvelope Log Contains In Order  ${FirstALCPolicyContent}  ${SecondALCPolicyContent}
    ...                                      ${ThirdALCPolicyContent}  ${FourthALCPolicyContent}
    ...                                      ${FinalALCPolicyContent}


SAV Policy with Chinese characters is written to File
    [Documentation]  Demonstrate error found in LINUXEP-6757
    Copy File  ./tests/mcs_router/installfiles/sav.json        ${SOPHOS_INSTALL}/base/pluginRegistry
    Register With Local Cloud Server
    Check Correct MCS Password And ID For Local Cloud Saved
    Start MCSRouter
    Check Default Policies Exist
    ${chinesepolicy}=  Set Variable  ${SUPPORT_FILES}/CentralXml/SAV_policy_with_chinese.xml
    Send Policy File  sav  ${chinesepolicy}
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  5 secs
    ...  Check Policy Written Match File  SAV-2_policy.xml  ${chinesepolicy}
    Check Temp Policy Folder Doesnt Contain Policies

MCSRouter Fails To Apply An MCS Policy With Non Ascii Characters Cleanly
    [Documentation]  Demonstrate error found in LINUXDAR-594
    Override LogConf File as Global Level  DEBUG
    Register With Local Cloud Server
    Check Correct MCS Password And ID For Local Cloud Saved
    Send Mcs Policy With Non Ascii Characters
    Start MCSRouter
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  Check Mcsrouter Log Contains   Failed to apply MCS policy:
    Check Mcsrouter Log Does Not Contain   CRITICAL
    Check Mcsrouter Log Does Not Contain   Caught exception at top-level
    Check Mcsrouter Log Contains   thisIsTheRevIdOfANonAsciiMcsPolicy

Policy Proxy Credentials Deobfuscated And Used
    Start Proxy Server With Basic Auth    3000    username   password
    Register With Local Cloud Server
    Check Correct MCS Password And ID For Local Cloud Saved
    Start MCSRouter
    Send Mcs Policy With Proxy   CCD4E57ZjW+t5XPiMSJH1TurG3MfWCN3DpjJRINMwqNaWl+3zzlVIdyVmifCHUwcmaX6+YTSyyBM8SslIIGV5rUw
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  5 secs
    ...  Check MCSRouter Log Contains  Successfully connected to localhost:4443 via localhost:3000

Two Cookies Sent From Central
    Register With Local Cloud Server
    Check Correct MCS Password And ID For Local Cloud Saved
    Start MCSRouter
    Check Default Policies Exist
    Check Cloud Server Log Contains  Cookies are good!  1
    Check Log Does Not Contain  Received mismatched cookie from MCS  ./tmp/cloudServer.log  Cloud Server Log


MCS Resets Token If Empty Token Is Sent In MCS Policy
    ${User_Agent_File}=  Set Variable  ./tmp/UserAgent
    Register With Local Cloud Server
    Check Correct MCS Password And ID For Local Cloud Saved
    Start MCSRouter
    Check Cloud Server Log For Command Poll

    Store Default Mcs Policy

    #Check user agent and policy config has a reg token
    Send Cmd To Fake Cloud   controller/userAgent  ${User_Agent_File}
    ${User_Agent_Info}=  Get File  ${User_Agent_File}
    Should Contain  ${User_Agent_Info}  regToken
    Check MCS Policy Config Contains  PolicyRegToken

    Set MCS Policy Registration Token Empty

    Check Cloud Server Log For Command Poll  2
    #Check that the policy has been updated to not contain reg token
    Check MCS Policy Config Does Not Contain    PolicyRegToken

    #Wait until next command poll as user agent info is updated in the header of the command poll
    Check Cloud Server Log For Command Poll  3
    #Check user agent no longer has a reg token.
    Send Cmd To Fake Cloud   controller/userAgent  ${User_Agent_File}
    ${User_Agent_Info}=  Get File  ${User_Agent_File}
    Should Not Contain  ${User_Agent_Info}  regToken

    Set Default MCS Policy

    Check Cloud Server Log For Command Poll  4
    Check MCS Policy Config Contains  PolicyRegToken

    #Wait until next command poll as user agent info is updated in the header of the command poll
    Check Cloud Server Log For Command Poll  5
    Send Cmd To Fake Cloud   controller/userAgent  ${User_Agent_File}
    ${User_Agent_Info}=  Get File  ${User_Agent_File}
    Should Contain  ${User_Agent_Info}  regToken

    Check Temp Policy Folder Doesnt Contain Policies

MCS Resets Token If Empty Token Is Sent In MCS Policy And Remains Empty On Restart
    ${User_Agent_File}=  Set Variable  ./tmp/UserAgent
    Register With Local Cloud Server
    Check Correct MCS Password And ID For Local Cloud Saved
    Start MCSRouter
    Check Cloud Server Log For Command Poll
    Store Default Mcs Policy

    #Check user agent and policy config has a reg token
    Send Cmd To Fake Cloud   controller/userAgent  ${User_Agent_File}
    ${User_Agent_Info}=  Get File  ${User_Agent_File}
    Check MCS Policy Config Contains  PolicyRegToken
    Set MCS Policy Registration Token Empty

    Check Cloud Server Log For Command Poll  2
    #Check that the policy has been updated to not contain reg token
    Check MCS Policy Config Does Not Contain  PolicyRegToken

    #Wait until next command poll as user agent info is updated in the header of the command poll
    Check Cloud Server Log For Command Poll  3
    #Check user agent no longer has a reg token.
    Send Cmd To Fake Cloud   controller/userAgent  ${User_Agent_File}
    ${User_Agent_Info}=  Get File  ${User_Agent_File}
    Should Not Contain  ${User_Agent_Info}  regToken

    Stop MCSRouter If Running

    Start MCSRouter

    Check Cloud Server Log For Command Poll  4
    #Check user agent no longer has a reg token.
    Send Cmd To Fake Cloud   controller/userAgent  ${User_Agent_File}
    ${User_Agent_Info}=  Get File  ${User_Agent_File}
    Should Not Contain  ${User_Agent_Info}  regToken
    Check MCS Policy Config Does Not Contain  PolicyRegToken

Test MCS Receives LiveQuery Policy
        Register With Local Cloud Server
        Check Correct MCS Password And ID For Local Cloud Saved
        Create File  ${SOPHOS_INSTALL}/base/pluginRegistry/LiveQueryPlugin.json  {"policyAppIds": ["LiveQuery"]}
        Start MCSRouter
        Wait Until Keyword Succeeds
        ...  10s
        ...  1s
        ...  File Should Exist  ${SOPHOS_INSTALL}/base/mcs/policy/LiveQuery_policy.xml



*** Keywords ***
Check Cloud Server Log For Command Poll
    [Arguments]    ${occurrence}=1
    Wait Until Keyword Succeeds
    ...  1 min
    ...  5 secs
    ...  Check Cloud Server Log Contains    GET - /mcs/commands/applications    ${occurrence}

