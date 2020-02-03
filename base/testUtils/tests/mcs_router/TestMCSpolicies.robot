*** Settings ***
Documentation    Port tests related to mcs policies and fake cloud that were originally present for SAV. [LINUXEP-7787]

Library   String

Resource  McsRouterResources.robot


Suite Setup      Setup MCS Tests
Suite Teardown   Uninstall SSPL Unless Cleanup Disabled


Test Teardown    Run Keywords
...              Dump Log   ./tmp/proxy_server.log  AND
...              Stop Local Cloud Server    AND
...              MCSRouter Default Test Teardown  AND
...			     Stop System Watchdog   AND
...              Override LogConf File as Global Level  INFO

Default Tags  FAKE_CLOUD  MCS  MCS_ROUTER

*** Test Case ***
MCS policy Can Control Server URL
    [Documentation]  This test is connected to CLOUD.MCS.004_update_URL.sh
    Install Register And Wait First MCS Policy

    Update MCS Policy And Send   localhost:4443  localhost:4444
    Wait New MCS Policy Downloaded

    Wait EndPoint Report Error To Connect To Central

    Restart MCSRouter And Clear Logs

    Wait EndPoint Report Error To Connect To Central


MCS Policy Can Control URL used
    [Documentation]  This test is connected to CLOUD.MCS.006_alternative_URL_used.sh
    Install Register And Wait First MCS Policy

    Update MCS Policy And Send   localhost:4443  localhost:4444
    Wait New MCS Policy Downloaded

    Wait EndPoint Report Error To Connect to Central

    Stop Local Cloud Server

    Wait EndPoint Report Broken Connection To Central

    Start Local Cloud Server   --port  4444

    Verify That MCS Connection To Central Is Re-established   4444


# When Central detects two end-points using the same machineid and token it informs one of the end-points to re-register
# by returning a 401 on the EndPoint interaction with Central.
# After receiving the 401, the EndPoint must re-register
Central Can Control EndPoint to Re-Register
    [Documentation]  This test is connected to CLOUD.MCS.005_reregister_with_new_token.sh
    Install Register And Wait First MCS Policy

    Send Cmd To Fake Cloud  controller/reregisterNext

    Wait EndPoint Report It Has Re-registered
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  Check Cloud Server Log Contains Pattern    .*Register with.*PolicyRegToken.*
    Wait Until Keyword Succeeds
    ...  1 min
    ...  15 secs
    ...  Check Mcsrouter Log Contains    Endpoint re-registered


MCS Policy Can Remove and Replace Token
    [Documentation]  This test is connected to CLOUD.MCS.013_policy_can_remove_token.sh
    Override LogConf File as Global Level  DEBUG
    Install Register And Wait First MCS Policy

    Check MCS Policy Config Contains  PolicyRegToken
    Set MCS Policy Registration Token Empty

    Wait New MCS Policy Downloaded
    Check MCS Policy Config Does Not Contain  PolicyRegToken

    Update MCS Policy And Send   <registrationToken/>   <registrationToken>YouNeedANewToken</registrationToken>

    Wait New MCS Policy Downloaded
    Wait Until Keyword Succeeds
    ...  1 min
    ...  15 secs
    ...  Check Mcsrouter Log Contains    MCS policy registrationToken = YouNeedANewToken


Malformed MCS Policy Without Closing Tag Is Rejected By The EndPoint
    Install Register And Wait First MCS Policy
    Start Proxy Server With Basic Auth    3000    username   password

    #Change Fake Central Policy to provide a policy with a missing closing tag which contains a valid proxy 1 entry
    Break And Send Mcs Policy  <messageRelays/>  <messageRelays> without closure

    # Can not use the Wait New MCS Policy Downloaded as mcsrouter will not save an invalid policy to disk
    # Wait New MCS Policy Downloaded
    Ensure Log Contains Policy Rejected
    Run Keyword and Expect Error  *does not contain*   Check MCSRouter Log Contains  Successfully connected to localhost:4443 via localhost:3000

    #Change Fake Central Policy to Policy With Proxy 1
    Send Mcs Policy With Proxy   CCD4E57ZjW+t5XPiMSJH1TurG3MfWCN3DpjJRINMwqNaWl+3zzlVIdyVmifCHUwcmaX6+YTSyyBM8SslIIGV5rUw

    Wait New MCS Policy Downloaded
    Ensure Log Contains Connection Via Proxy


Malformed MCS Policy Is Rejected By The EndPoint
    Install Register And Wait First MCS Policy

    Break And Send Mcs Policy  <messageRelays/>  <messageRelays><stuff><messageRelays/><stuff/>

    Ensure Log Contains Policy Rejected


Malformed MCS Policy Missing Policy Node Is Rejected By The Endpoint
    Install Register And Wait First MCS Policy

    Send Blank Xml Policy

    Wait Until Keyword Succeeds
        ...  30 secs
        ...  5 secs
        ...  Check MCSRouter Log Contains  MCS Policy doesn't have one policy node


Malformed MCS Policy Missing Compliance Node Is Rejected By The Endpoint
    Install Register And Wait First MCS Policy

    Break And Send Mcs Policy  <csc:Comp RevID="aaaaaaaaac21f8d7510d562c56e34507711bdce48bfdfd3846965f130fef142a" policyType="25"/>  ${SPACE}

    Wait Until Keyword Succeeds
        ...  30 secs
        ...  5 secs
        ...  Check MCSRouter Log Contains  MCS Policy didn't contain one compliance node


Malformed MCS Policy Missing Policy Type Tag Is Rejected By The Endpoint
    Install Register And Wait First MCS Policy

    Break And Send Mcs Policy  policyType="25"  ${SPACE}

    Wait Until Keyword Succeeds
        ...  30 secs
        ...  5 secs
        ...  Check MCSRouter Log Contains  MCS policy did not contain policy type


Malformed MCS Policy Missing RevID is Rejected By The Endpoint
    Install Register And Wait First MCS Policy

    Break And Send Mcs Policy  RevID="aaaaaaaaac21f8d7510d562c56e34507711bdce48bfdfd3846965f130fef142a"  ${SPACE}

    Wait Until Keyword Succeeds
        ...  30 secs
        ...  5 secs
        ...  Check MCSRouter Log Contains   MCS policy did not contain revID
