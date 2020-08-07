*** Settings ***
Resource  McsRouterResources.robot
Library     ${LIBS_DIRECTORY}/PushServerUtils.py

Suite Setup      Run Keywords  Setup MCS Tests  AND  Start MCS Push Server
Suite Teardown   Run Keywords  Uninstall SSPL Unless Cleanup Disabled  AND  Server Close

Test Setup       Start Local Cloud Server  --initial-mcs-policy  ${SUPPORT_FILES}/CentralXml/MCS_Push_Policy_PushFallbackPoll.xml
Test Teardown    Run Keywords
...              Stop Local Cloud Server    AND
...              Dump Log   ./tmp/proxy_server.log  AND
...              MCSRouter Default Test Teardown  AND
...			     Stop System Watchdog  AND
...              Stop Proxy Servers  AND
...              Stop Proxy If Running  AND
...              Remove Environment Variable    https_proxy

Default Tags  MCS  FAKE_CLOUD  MCS_ROUTER

*** Variables ***
${push_server_address}  localhost:4443

*** Test Case ***
Disable direct option stops direct connection
    [Documentation]  Derived from CLOUD.PROXY.012_useDirect_option_stops_direct_connection.sh
    Register With Local Cloud Server
    Start MCSRouter
    Send Mcs Policy With New Message Relay   <messageRelay priority='0' port='3345' address='localhost' id='eee'/>
    Send Mcs Policy With Direct Disabled

    Wait Until Keyword Succeeds
    ...  20 secs
    ...  2 secs
    ...  Check MCS Policy Config Contains    useDirect=false

    Wait Until Keyword Succeeds
    ...  30 secs
    ...  5 secs
    ...  Check MCSRouter Log Contains   No proxies/message relays set to communicate
    Check MCSRouter Log Contains   with Central - useDirect is False
    Check MCSRouter Log Contains   Failed to connect to Central

Proxy Creds And Message Relay Information Used On Registration
    [Documentation]  Derived from  CLOUD.MCS.007_mcs_handles_message_relay_policy.sh
    [Tags]  MCS  FAKE_CLOUD  MCS_ROUTER  REGISTRATION
    Start Proxy Server With Basic Auth    3000    username    password
    Register With Local Cloud Server With Proxy Creds And Message Relay  username  password  localhost:3000,0,1
    ${contents} =  Get File  ${SOPHOS_INSTALL}/base/etc/sophosspl/mcs_policy.config
    Should Contain  ${contents}  mcs_policy_proxy_credentials=CC
    Should Contain  ${contents}  message_relay_address1=localhost
    Should Contain  ${contents}  message_relay_port1=3000
    Should Contain  ${contents}  message_relay_priority1=0
    Should Contain  ${contents}  message_relay_id1=1
    Should Not Contain  ${contents}  username
    Should Not Contain  ${contents}  password
    Log To Console  ${contents}
    Check Register Central Log Contains  Successfully connected to localhost:4443 via localhost:3000
    Start MCSRouter
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  5 secs
    ...  Check MCSRouter Log Contains  Successfully connected to localhost:4443 via localhost:3000

Transient errors keeps same proxies
    [Documentation]  Derived from  CLOUD.MCS.008_one_transient_error_keeps_same_proxy.sh
    Start Simple Proxy Server    3000

    Register With Local Cloud Server
    Check Correct MCS Password And ID For Local Cloud Saved
    Send Mcs Policy With New Message Relay   <messageRelay priority='0' port='3000' address='localhost' id='no_auth_proxy'/>
    Start MCSRouter
    Wait New MCS Policy Downloaded

    Wait Until Keyword Succeeds
    ...  30 secs
    ...  5 secs
    ...  Check MCSRouter Log Contains  Successfully connected to localhost:4443 via localhost:3000
    Mark Mcsrouter Log

    # This will trigger a back-off and then re-try proxies
    Send Command From Fake Cloud    error
    Wait Until Keyword Succeeds
    ...  60 secs
    ...  10 secs
    ...  Check Marked Mcsrouter Log Contains   HTTP Service Unavailable (503): HTTP Service Unavailable

    Wait Until Keyword Succeeds
    ...  60 secs
    ...  10 secs
    ...  Check Marked MCSRouter Log Contains  Successfully connected to localhost:4443 via localhost:3000
    Mark Mcsrouter Log

    # This will trigger a back-off and then re-try proxies
    Send Command From Fake Cloud    error
    Wait Until Keyword Succeeds
    ...  60 secs
    ...  10 secs
    ...  Check Marked Mcsrouter Log Contains   HTTP Service Unavailable (503): HTTP Service Unavailable

    Wait Until Keyword Succeeds
    ...  60 secs
    ...  10 secs
    ...  Check Marked MCSRouter Log Contains  Successfully connected to localhost:4443 via localhost:3000


The Connection Will Transfer Between Proxies When One Fails
    [Documentation]  Derived from  CLOUD.MCS.009 and CLOUD.MCS.010
    ${Proxy_Port_One} =  Set Variable  3331
    ${Proxy_Port_Two} =  Set Variable  7771
    ${Proxy_Name_One} =  Set Variable  deadbeef-feedbeef
    ${Proxy_Name_Two} =  Set Variable  feedbeef-deadbeef
    Start Simple Proxy Server    ${Proxy_Port_One}
    Start Simple Proxy Server    ${Proxy_Port_Two}
    Register With Local Cloud Server
    Check Correct MCS Password And ID For Local Cloud Saved
    Send Mcs Policy With New Message Relay   <messageRelay priority='0' port='${Proxy_Port_One}' address='localhost' id='${Proxy_Name_One}'/><messageRelay priority='0' port='${Proxy_Port_Two}' address='localhost' id='${Proxy_Name_Two}'/>

    Start MCSRouter

    Check Cloud Server Log For Command Poll
    Check MCS Policy Config Contains  message_relay_address1=localhost
    Check MCSRouter Log Contains  Successfully connected to localhost:4443 via localhost:${Proxy_Port_One}
    Check Mcsenvelope Log Contains In Order  PUT /statuses/endpoint  ${Proxy_Name_One}

    Stop Proxy Server On Port  3331
    Mark Mcsrouter Log
    Check Cloud Server Log For Command Poll  2
    Check Marked MCSRouter Log Contains  Failed connection with message relay via localhost:${Proxy_Port_One}
    Check Marked MCSRouter Log Contains  Successfully connected to localhost:4443 via localhost:${Proxy_Port_Two}

    Check Cloud Server Log For Command Poll  3
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  5 secs
    ...  Check Mcsenvelope Log Contains In Order  PUT /statuses/endpoint  ${Proxy_Name_One}  PUT /statuses/endpoint  ${Proxy_Name_Two}

Mcs Router reregisters after only getting one 401 error
    [Documentation]  Derived from  CLOUD.MCS.011
    # Check that mcs_router will immediately re-register after getting a single 401 error, do de-dup works
    # See LINUXEP-7953
    ${Proxy_Port_One} =  Set Variable  3332
    ${Proxy_Port_Two} =  Set Variable  7772
    ${Proxy_Name_One} =  Set Variable  deadbeef-feedbeef
    ${Proxy_Name_Two} =  Set Variable  feedbeef-deadbeef
    Start Simple Proxy Server    ${Proxy_Port_One}
    Start Simple Proxy Server    ${Proxy_Port_Two}
    Register With Local Cloud Server
    Check Correct MCS Password And ID For Local Cloud Saved
    Send Mcs Policy With New Message Relay   <messageRelay priority='0' port='${Proxy_Port_One}' address='localhost' id='${Proxy_Name_One}'/><messageRelay priority='0' port='${Proxy_Port_Two}' address='localhost' id='${Proxy_Name_Two}'/>

    Start MCSRouter
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  2 secs
    ...  Check MCS Policy Config Contains  message_relay_address1=localhost
    Mark Mcsrouter Log
    Send Command From Fake Cloud    controller/reregisterNext

    Wait Until Keyword Succeeds
    ...  30 secs
    ...  5 secs
    ...  Check Marked Mcsrouter Log Contains String N Times  UNAUTHORIZED from server 401  1

    Check MCSRouter Log Contains In Order  Successfully connected to localhost:4443 via localhost:${Proxy_Port_One}
    ...                                    UNAUTHORIZED from server 401 WWW-Authenticate=Basic realm="register"

    Wait Until Keyword Succeeds
    ...  40 secs
    ...  10 secs
    ...  Check Marked MCSRouter Log Contains  Re-registering with MCS


Policy proxy overrides local proxy
    [Tags]  FAKE_CLOUD  MCS  MCS_ROUTER   TAP_TESTS
    [Documentation]  Derived from CLOUD.PROXY.004_policy_proxy.sh
    ${Proxy_Port_One} =  Set Variable  3333
    ${Proxy_Port_Two} =  Set Variable  7773
    ${Proxy_Name_One} =  Set Variable  deadbeef-feedbeef
    ${Proxy_Name_Two} =  Set Variable  feedbeef-deadbeef
    Start Simple Proxy Server    ${Proxy_Port_One}
    Start Simple Proxy Server    ${Proxy_Port_Two}
    Set Environment Variable  https_proxy   http://localhost:${Proxy_Port_Two}
    Register With Local Cloud Server
    Start MCSRouter
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  5 secs
    ...  Check MCSRouter Log Contains   Successfully connected to localhost:4443 via localhost:${Proxy_Port_Two}
    Check MCSRouter Log Contains   Push client successfully connected to ${push_server_address} via localhost:${Proxy_Port_Two}

    Check MCSRouter Log Does Not Contain   Successfully connected to localhost:4443 via localhost:${Proxy_Port_One}
    Check MCSRouter Log Does Not Contain   Push client successfully connected to ${push_server_address} directly

    Send Mcs Policy With New Message Relay   <messageRelay priority='0' port='${Proxy_Port_One}' address='localhost' id='${Proxy_Name_One}'/>

    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  Check MCS Policy Config Contains    message_relay_address1=localhost
    Check Log Contains    message_relay_port1=3333     ${SOPHOS_INSTALL}/base/etc/sophosspl/mcs_policy.config     Mcs_policy_config

    Wait Until Keyword Succeeds
    ...  30 secs
    ...  5 secs
    ...  Check MCSRouter Log Contains  Successfully connected to localhost:4443 via localhost:${Proxy_Port_One}
    Check MCSRouter Log Contains   Push client successfully connected to ${push_server_address} via localhost:${Proxy_Port_One}


Fallback to direct connection when policy proxy fails
    [Tags]  FAKE_CLOUD  MCS  MCS_ROUTER   TAP_TESTS
    [Documentation]  Derived from CLOUD.PROXY.005_fallback_to_direct_after_policy_proxy_fails.sh
    ${Proxy_Port_One} =  Set Variable  3346
    ${Proxy_Name_One} =  Set Variable  deadbeef-feedbeef
    Start Simple Proxy Server    ${Proxy_Port_One}
    Remove File   ${SOPHOS_INSTALL}/base/etc/sophosspl/mcs_policy.config
    Register With Local Cloud Server
    Start MCSRouter
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  5 secs
    ...  Check MCS Router Running

    # give enough time for the mcsrouter to fetch the initial policies
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  5 secs
    ...  File Should Exist   ${SOPHOS_INSTALL}/base/mcs/policy/ALC-1_policy.xml

    Wait Until Keyword Succeeds
    ...  30 secs
    ...  5 secs
    ...  File Should Exist   ${SOPHOS_INSTALL}/base/etc/sophosspl/mcs_policy.config

    Send Mcs Policy With New Message Relay   <messageRelay priority='0' port='${Proxy_Port_One}' address='localhost' id='${Proxy_Name_One}'/>
    Mark Mcsrouter Log
    Wait Until Keyword Succeeds
    ...  50 secs
    ...  5 secs
    ...  Check MCS Policy Config Contains    message_relay_address1=localhost
    Wait Until Keyword Succeeds
    ...  50 secs
    ...  5 secs
    ...  Check Marked MCSRouter Log Contains   Successfully connected to localhost:4443 via localhost:${Proxy_Port_One}
    Check Marked MCSRouter Log Contains   Push client successfully connected to ${push_server_address} via localhost:${Proxy_Port_One}

    Stop Proxy Server On Port  3346

    Wait Until Keyword Succeeds
    ...  50 secs
    ...  5 secs
    ...  Check Marked MCSRouter Log Contains   Successfully directly connected to localhost:4443
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  2 secs
    ...  Check Marked MCSRouter Log Contains   Push client successfully connected to ${push_server_address} directly

Policy authentication with digest
    [Tags]  FAKE_CLOUD  MCS  MCS_ROUTER   TAP_TESTS
    [Documentation]  Derived from CLOUD.PROXY.013_policy_authentication.sh
    ${username} =  Set Variable  username
    ${password} =  Set Variable  password
    Start Proxy Server With Digest Auth    3000  ${username}  ${password}
    Register With Local Cloud Server
    Start MCSRouter
    Send Mcs Policy With Proxy   CCD4E57ZjW+t5XPiMSJH1TurG3MfWCN3DpjJRINMwqNaWl+3zzlVIdyVmifCHUwcmaX6+YTSyyBM8SslIIGV5rUw
    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  Check MCS Policy Config Contains    CCD4E57ZjW+t5XPiMSJH1TurG3MfWCN3DpjJRINMwqNaWl+3zzlVIdyVmifCHUwcmaX6+YTSyyBM8SslIIGV5rUw
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  5 secs
    ...  Check MCSRouter Log Contains   Successfully connected to localhost:4443 via localhost:3000

    Check MCSRouter Does Not Retry Direct Connection
    Wait Until Keyword Succeeds
        ...  30 secs
        ...  5 secs
        ...  Check MCSRouter Log Contains   Established MCS Push Connection
    Check MCSRouter Log Contains   Push client successfully connected to ${push_server_address} via localhost:3000


Attempt to Connect Via Digest Proxy with Wrong Credentials Produce Correct Error Line
    ${username} =  Set Variable  username
    ${password} =  Set Variable  differentpassword
    Start Proxy Server With Digest Auth    3000  ${username}  ${password}
    Register With Local Cloud Server
    Start MCSRouter
    # this will try a connection that is invalid
    Send Mcs Policy With Proxy   CCD4E57ZjW+t5XPiMSJH1TurG3MfWCN3DpjJRINMwqNaWl+3zzlVIdyVmifCHUwcmaX6+YTSyyBM8SslIIGV5rUw
    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  Check MCS Policy Config Contains    CCD4E57ZjW+t5XPiMSJH1TurG3MfWCN3DpjJRINMwqNaWl+3zzlVIdyVmifCHUwcmaX6+YTSyyBM8SslIIGV5rUw
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  5 secs
    ...  Check MCS Router Log Contains In Order
             ...   Trying connection via proxy localhost:3000
             ...   Trying connection directly to localhost:4443
             ...   Successfully directly connected to localhost:4443
    Check MCSRouter Log Does Not Contain  Successfully connected to localhost:4443 via localhost:3000
    Wait Until Keyword Succeeds
        ...  30 secs
        ...  5 secs
        ...  Check MCSRouter Log Contains   Established MCS Push Connection
    Check MCS Router Log Contains In Order
             ...   Trying push connection to localhost:4443 via localhost:3000
             ...   Failed to connect to localhost:4443 via localhost:3000
             ...   Trying push connection directly to localhost:4443
             ...   Push client successfully connected to localhost:4443 directly


Policy authentication with 3DES obfuscation
    [Documentation]  Derived from CLOUD.PROXY.014
    ${username} =  Set Variable  foo
    ${password} =  Set Variable  bar

    Start Proxy Server With Digest Auth    3000  ${username}  ${password}
    Register With Local Cloud Server
    Start MCSRouter
    Send Mcs Policy With Proxy   BwiLF3k1Y9l3mePu13c7Tk4b
    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  Check MCS Policy Config Contains    BwiLF3k1Y9l3mePu13c7Tk4b
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  5 secs
    ...  Check MCSRouter Log Contains   Successfully connected to localhost:4443 via localhost:3000

Policy Basic Auth Proxy Credentials Deobfuscated And Used
    [Documentation]  Derived from CLOUD.PROXY.007
    Start Proxy Server With Basic Auth    3000    username   password
    Set Environment Variable  https_proxy   http://username:password@localhost:3000
    Register With Local Cloud Server
    Check Correct MCS Password And ID For Local Cloud Saved
    Start MCSRouter

    Wait Until Keyword Succeeds
    ...  30 secs
    ...  5 secs
    ...  Check MCSRouter Log Contains  Successfully connected to localhost:4443 via localhost:3000
    Check MCSRouter Log Contains  Push client successfully connected to ${push_server_address} via localhost:3000

    Check MCSRouter Log Does Not Contain  Successfully directly connected to localhost:4443
    Check MCSRouter Log Does Not Contain  Push client successfully connected to ${push_server_address} directly


Status Sent After Message Relay Changed
    [Documentation]  Derived from  CLOUD.MCS.012_status_sent_after_message_relay_changed.sh
    ${Proxy_Port_One} =  Set Variable  3333
    ${Proxy_Port_Two} =  Set Variable  7773
    ${Proxy_Name_One} =  Set Variable  deadbeef-feedbeef
    ${Proxy_Name_Two} =  Set Variable  feedbeef-deadbeef
    ${MCS_Envelope_Log} =  Set Variable  ${SOPHOS_INSTALL}/logs/sophosspl/mcs_envelope.log
    Start Simple Proxy Server    ${Proxy_Port_One}
    Start Simple Proxy Server    ${Proxy_Port_Two}

    Register With Local Cloud Server

    Send Mcs Policy With New Message Relay   <messageRelay priority='0' port='${Proxy_Port_One}' address='localhost' id='${Proxy_Name_One}'/><messageRelay priority='0' port='${Proxy_Port_Two}' address='localhost' id='${Proxy_Name_Two}'/>

    Start MCSRouter

    Wait Until Keyword Succeeds
    ...  10 secs
    ...  2 secs
    ...  Check MCS Router Running

    Wait Until Keyword Succeeds
    ...  30 secs
    ...  5 secs
    ...  Check MCSRouter Log Contains   Successfully connected to localhost:4443 via localhost:${Proxy_Port_One}
    Wait Until Keyword Succeeds
    ...  5 secs
    ...  1 secs
    ...  Check MCSRouter Log Contains   Push client successfully connected to ${push_server_address} via localhost:${Proxy_Port_One}

    Check MCSRouter Log Does Not Contain   Successfully connected to localhost:4443 via localhost:${Proxy_Port_Two}
    Check MCSRouter Log Does Not Contain   Push client successfully connected to ${push_server_address} via localhost:${Proxy_Port_Two}

    Check Log Contains    message_relay_address1=localhost          ${SOPHOS_INSTALL}/base/etc/sophosspl/mcs_policy.config     Mcs_policy_config
    Check Log Contains    message_relay_port1=${Proxy_Port_One}     ${SOPHOS_INSTALL}/base/etc/sophosspl/mcs_policy.config     Mcs_policy_config
    Check Log Contains    message_relay_address2=localhost          ${SOPHOS_INSTALL}/base/etc/sophosspl/mcs_policy.config     Mcs_policy_config
    Check Log Contains    message_relay_port2=${Proxy_Port_Two}     ${SOPHOS_INSTALL}/base/etc/sophosspl/mcs_policy.config     Mcs_policy_config

    # Check status associated with first proxy connection is received in fake cloud before stopping proxy
    # to avoid potential race condition where a status is sent twice when the connection fails during send.
    Wait Until Keyword Succeeds
    ...  40 secs
    ...  5 secs
    ...  Check Cloud Server Log For MCS Status   1

    Stop Proxy Server On Port  ${Proxy_Port_One}

    Wait Until Keyword Succeeds
    ...  30 secs
    ...  5 secs
    ...  Check MCSRouter Log Contains  Successfully connected to localhost:4443 via localhost:${Proxy_Port_Two}
    Check MCSRouter Log Contains  Push client successfully connected to ${push_server_address} via localhost:${Proxy_Port_Two}


    Wait Until Keyword Succeeds
    ...  40 secs
    ...  5 secs
    ...  Check Cloud Server Log For MCS Status   2

    ${MCS_Status} =  Check Cloud Server Log For MCS Status And Return It   2
    Should Contain  ${MCS_Status}  messageRelay endpointId=&quot;${Proxy_Name_Two}&quot;


Connection Timeout When Connecting Via A Proxy
    [Documentation]  CLOUD.019_connection_timeout.sh
    ${Time_Out_Proxy_Port} =  Set Variable  5123
    Set Environment Variable  https_proxy   http://localhost:${Time_Out_Proxy_Port}

    Start Timeout Proxy Server    ${Time_Out_Proxy_Port}

    Register With Local Cloud Server

    Wait Until Keyword Succeeds
    ...  40 secs
    ...  4 secs
    ...  Check Register Central Log Contains  Failed connection with proxy via localhost:5123 to localhost:4443: timed out


Central Mcs Policy Should Control Proxy Used
    ${Proxy_Port_One} =  Set Variable  3333
    ${Proxy_Name_One} =  Set Variable  deadbeef-feedbeef

    Register With Local Cloud Server
    Send Mcs Policy With New Message Relay   <messageRelay priority='0' port='${Proxy_Port_One}' address='localhost' id='${Proxy_Name_One}'/>

    Start MCSRouter

    Wait Until Keyword Succeeds
    ...  10 secs
    ...  2 secs
    ...  Check MCS Router Running

    ${Failed_Connection_Message} =          Set Variable  Failed connection with message relay via localhost:3333 to localhost:4443
    ${Failed_Push_Connection_Message} =          Set Variable  Failed to connect to ${push_server_address} via localhost:3333
    ${Success_Direct_Connection_Message} =  Set Variable  Successfully directly connected to localhost:4443
    ${Success_Push_Direct_Connection_Message} =  Set Variable  Push client successfully connected to ${push_server_address} directly

    Wait Until Keyword Succeeds
    ...  5 secs
    ...  1 secs
    ...  Check MCSRouter Log Contains   ${Success_Direct_Connection_Message}
    Check MCSRouter Log Contains   ${Success_Push_Direct_Connection_Message}

    # Check log contains failed connection message only once
    ${MCS_Router_Log} =  Get File  ${SOPHOS_INSTALL}/logs/base/sophosspl/mcsrouter.log
    Should Contain X Times  ${MCS_Router_Log}  ${Failed_Connection_Message}          1  Mcs Router Log Doesn't Contain The Following Message Once: ${Failed_Connection_Message}
    Should Contain X Times  ${MCS_Router_Log}  ${Failed_Push_Connection_Message}          1  Mcs Router Log Doesn't Contain The Following Message Once: ${Failed_Push_Connection_Message}

    Send Default Mcs Policy

    Wait Until Keyword Succeeds
    ...  60 secs
    ...  5 secs
    ...  Check Cloud Server Log For MCS Status   2

    # New policy received removing proxy info check that there are no failed connections that occur after this
    ${MCS_Router_Log} =  Get File  ${SOPHOS_INSTALL}/logs/base/sophosspl/mcsrouter.log
    Should Contain X Times  ${MCS_Router_Log}  ${Failed_Connection_Message}          1  Mcs Router Log Doesn't Contain The Following Message Once: ${Failed_Connection_Message}
    Should Contain X Times  ${MCS_Router_Log}  ${Failed_Push_Connection_Message}          1  Mcs Router Log Doesn't Contain The Following Message Once: ${Failed_Push_Connection_Message}

*** Keywords ***

MCSRouter And Push Client Log Connection Via Proxy
    [Arguments]  ${mcs_server}  ${push_server}  ${proxy}
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  5 secs
    ...  Run Keywords
    ...  Check MCSRouter Log Contains   Successfully connected to ${mcs_server} via ${proxy}  AND
    ...  Check MCSRouter Log Contains   Push client successfully connected to ${push_server} via localhost:3000

MCSRouter tries direct connection after proxy
    Check MCS Router Log Contains In Order
    ...   Successfully connected to localhost:4443 via localhost:3000
    ...   Trying connection directly to localhost:4443
    ...   Successfully directly connected to localhost:4443


Check MCSRouter Does Not Retry Direct Connection
    Run Keyword And Expect Error
    ...   STARTS: Remainder of MCS Router log doesn't contain Trying connection
    ...   MCSRouter tries direct connection after proxy
