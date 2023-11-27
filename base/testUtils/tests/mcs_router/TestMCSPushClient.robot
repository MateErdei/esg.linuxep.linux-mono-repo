*** Settings ***

Library     String
Library     ${COMMON_TEST_LIBS}/PushServerUtils.py
Library     ${COMMON_TEST_LIBS}/LogUtils.py

Resource    ${COMMON_TEST_ROBOT}/ManagementAgentResources.robot
Resource    ${COMMON_TEST_ROBOT}/McsPushClientResources.robot
Resource    ${COMMON_TEST_ROBOT}/McsRouterResources.robot
Resource    ${COMMON_TEST_ROBOT}/UpgradeResources.robot


Suite Setup      Setup MCS Tests
Suite Teardown   Uninstall SSPL Unless Cleanup Disabled

Test Teardown    Test Teardown

Force Tags  FAKE_CLOUD  MCS  MCS_ROUTER   TAP_PARALLEL1

*** Test Case ***

MCSRouter Can Start And Receive Messages From The Push Client
    Start MCS Push Server
    Install Register And Wait First MCS Policy With MCS Policy  ${SUPPORT_FILES}/CentralXml/MCS_Push_Policy_PushFallbackPoll.xml

    Push Client started and connects to Push Server when the MCS Client receives MCS Policy
    Send Message To Push Server And Expect It In MCSRouter Log   Single Message

MCS Push Client handles server breaking connection
    Start MCS Push Server
    Install Register And Wait First MCS Policy With MCS Policy  ${SUPPORT_FILES}/CentralXml/MCS_Push_Policy_PushFallbackPoll.xml

    Push Client started and connects to Push Server when the MCS Client receives MCS Policy
    Send Message To Push Server And Expect It In MCSRouter Log   Single Message
    Instruct Push Server To Close Connections
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  Check MCSRouter Log Contains In Order
            # We catch the stop iteration exception, meaning the connection has broken
    ...     Push client lost connection to server : sseclient raised StopIteration exception
            # Try to reconnect
    ...     Trying to re-connect to Push Server
            # Successfully connect
    ...     Push client successfully connected to localhost:4443 directly

MCSRouter Can Start and Receive Update Now Action From Push Client
    Start MCS Push Server
    Install Register And Wait First MCS Policy With MCS Policy  ${SUPPORT_FILES}/CentralXml/MCS_Push_Policy_PushFallbackPoll.xml

    Check Connected To Fake Cloud

    Push Client started and connects to Push Server when the MCS Client receives MCS Policy
    Send Message To Push Server From File   ${SUPPORT_FILES}/CentralXml/ALC_full_update_now_command.xml
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  Check Mcsrouter Log Contains   Received command from Push Server

    Wait Until Keyword Succeeds
    ...  5 secs
    ...  1 secs
    ...  File Should Exist  ${MCS_DIR}/action/ALC_action_FakeTime.xml

    Check File Content  <?xml version='1.0'?><action type="sophos.mgt.action.ALCForceUpdate"/>  ${MCS_DIR}/action/

MCSRouter Can Start and Receive Live Query From Push Client
    [Teardown]  EDR Push Client Teardown
    Start MCS Push Server
    Install Register And Wait First MCS Policy With MCS Policy  ${SUPPORT_FILES}/CentralXml/MCS_Push_Policy_PushFallbackPoll.xml
    Register EDR Plugin
    Check Connected To Fake Cloud
    Push Client started and connects to Push Server when the MCS Client receives MCS Policy
    Send Message To Push Server From File   ${SUPPORT_FILES}/CentralXml/LQ_select_users_command.xml
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  Check Mcsrouter Log Contains   Received command from Push Server

    Wait Until Keyword Succeeds
    ...  5 secs
    ...  1 secs
    ...  File Should Exist  ${MCS_DIR}/action/LiveQuery_ABC_request_2020-06-09T15:30:08.000Z_1591726608.json

    Check File Content  {"type": "sophos.mgt.action.RunLiveQuery", "name": "users", "query": "SELECT * from users"}  ${MCS_DIR}/action/

MCSRouter Can Start and Receive Wakeup Command From Push Client
    Start MCS Push Server
    Install Register And Wait First MCS Policy With MCS Policy  ${SUPPORT_FILES}/CentralXml/MCS_Push_Policy_PushFallbackPoll.xml

    Check Connected To Fake Cloud

    Push Client started and connects to Push Server when the MCS Client receives MCS Policy
    Send Message To Push Server And Expect It In MCSRouter Log   <action type="sophos.mgt.action.GetCommands"></action>
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  Check Mcsrouter Log Contains  Received command from Push Server
    Send Message To Push Server And Expect It In MCSRouter Log   <action type="sophos.mgt.action.GetCommands"></action>
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  Check Mcsrouter Log Contains  Received command from Push Server

    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  Check MCSRouter Log Contains In Order  Received Wakeup from Push Server
    ...                                             Checking for commands for
    ...                                             Received Wakeup from Push Server
    ...                                             Checking for commands for

MCSRouter Safely Logs Invalid XML Action From Push Client And Recovers
    Start MCS Push Server
    Install Register And Wait First MCS Policy With MCS Policy  ${SUPPORT_FILES}/CentralXml/MCS_Push_Policy_PushFallbackPoll.xml

    Check Connected To Fake Cloud

    Push Client started and connects to Push Server when the MCS Client receives MCS Policy
    Send Message To Push Server And Expect It In MCSRouter Log   <action type="sophos.mgt.action.Geands"></act>
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  Check Mcsrouter Log Contains  Failed to parse commands
    Send Message To Push Server From File   ${SUPPORT_FILES}/CentralXml/ALC_full_update_now_command.xml
    Wait Until Keyword Succeeds
    ...  5 secs
    ...  1 secs
    ...  File Should Exist  ${MCS_DIR}/action/ALC_action_FakeTime.xml

MCSRouter Safely Logs Empty Action From Push Client And Recovers
    Start MCS Push Server
    Install Register And Wait First MCS Policy With MCS Policy  ${SUPPORT_FILES}/CentralXml/MCS_Push_Policy_PushFallbackPoll.xml

    Check Connected To Fake Cloud

    Push Client started and connects to Push Server when the MCS Client receives MCS Policy
    Send Message To Push Server And Expect It In MCSRouter Log   ""
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  Check Mcsrouter Log Contains  Failed to parse commands
    Send Message To Push Server From File   ${SUPPORT_FILES}/CentralXml/ALC_full_update_now_command.xml
    Wait Until Keyword Succeeds
    ...  5 secs
    ...  1 secs
    ...  File Should Exist  ${MCS_DIR}/action/ALC_action_FakeTime.xml

Push Client informs MCS Client if the connection with Push Server is disconnected and logged to MCSRouter
    Start MCS Push Server
    Configure Push Server To Ping Interval  300
    Install Register And Wait First MCS Policy With MCS Policy  ${SUPPORT_FILES}/CentralXml/MCS_Push_Policy_PushFallbackPoll.xml

    Push Client started and connects to Push Server when the MCS Client receives MCS Policy
    Send Message To Push Server And Expect It In MCSRouter Log   Single Message

    Wait Until Keyword Succeeds
    ...     30s
    ...     3s
    ...     Check MCS Router Log Contains   Push Server service reported: No Ping from Server

Push Client stops connection to Push server when instructed by the MCS Client
    Start MCS Push Server
    Install Register And Wait First MCS Policy With MCS Policy  ${SUPPORT_FILES}/CentralXml/MCS_Push_Policy_PushFallbackPoll.xml

    Push Client started and connects to Push Server when the MCS Client receives MCS Policy
    Send Message To Push Server And Expect It In MCSRouter Log   <action>validxml</action>

    ${mcs_policy} =    Get File  ${SUPPORT_FILES}/CentralXml/MCS_Push_Policy_PushFallbackPoll.xml
    ${changed_ping} =  Replace String  ${mcs_policy}  <pushPingTimeout>12</pushPingTimeout>    <pushPingTimeout>20</pushPingTimeout>
    Send Policy   mcs  ${changed_ping}

    Wait Until Keyword Succeeds
    ...   60s
    ...   2s
    ...   Check MCS Router Log Contains     ${changed_ping}

    Wait Until Keyword Succeeds
    ...   40s
    ...   3s
    ...   Check MCS Router Log Contains In Order
    ...          MCS push client stopped
    ...          Established MCS Push Connection

Push Client Is Stopped When MCSRouter Stops
    Start MCS Push Server
    Install Register And Wait First MCS Policy With MCS Policy  ${SUPPORT_FILES}/CentralXml/MCS_Push_Policy_PushFallbackPoll.xml

    Push Client started and connects to Push Server when the MCS Client receives MCS Policy
    Send Message To Push Server And Expect It In MCSRouter Log   Single Message

    Stop Mcsrouter If Running
    Wait Until Keyword Succeeds
    ...   10s
    ...   2s
    ...   Check MCS Router Log Contains     MCS push client stopped


Verify Push Client Will Attempt To Connect On Every Command Poll When Push Server Is Configured
    [Documentation]  From https://wiki.sophos.net/display/PP/MCS+Push+Endpoint+Requirements
    ...  Attempt to connect at each server poll
    #commandPollingDelay default="5"
    Override LogConf File as Global Level  DEBUG

    Install Register And Wait First MCS Policy With MCS Policy  ${SUPPORT_FILES}/CentralXml/MCS_Push_Policy_PushFallbackPoll.xml

    # expect and check for attempt to connect push_client in every mcs_client poll @ every 5 seconds
    # push client connection failures must not trigger back-off
    Wait Until Keyword Succeeds
    ...   6s
    ...   1s
    ...   Run Keywords
    ...   Check Log Contains String N times   ${MCS_ROUTER_LOG}   mcs_router  Failed to start MCS Push Client service  1  AND
    ...   Check Mcs Envelope Log Contains Regex String N Times   GET \/commands\/applications\/MCS;(\\w;*)+\/endpoint\/   1

    Wait Until Keyword Succeeds
    ...   30s
    ...   2s
    ...   Run Keywords
    ...   Check Log Contains String N times   ${MCS_ROUTER_LOG}   mcs_router  Trying to re-connect to Push Server  1  AND
    ...   Check Mcs Envelope Log Contains Regex String N Times   GET \/commands\/applications\/MCS;(\\w;*)+\/endpoint\/   2

    Wait Until Keyword Succeeds
    ...   30s
    ...   2s
    ...   Run Keywords
    ...   Check Log Contains String N times   ${MCS_ROUTER_LOG}   mcs_router  Trying to re-connect to Push Server  2   AND
    ...   Check Mcs Envelope Log Contains Regex String N Times   GET \/commands\/applications\/MCS;(\\w;*)+\/endpoint\/   3

Verify When Push Client Is Connected MCS Router Will Command Poll The Server Using The Push Fall Back Command Poll Interval
    [Documentation]  From https://wiki.sophos.net/display/PP/MCS+Push+Endpoint+Requirements
    ...  Poll the server using the pushFallbackPollInterval interval

    Install Register And Wait First MCS Policy With MCS Policy  ${SUPPORT_FILES}/CentralXml/MCS_Push_Policy_PushFallbackPoll.xml

    Wait Until Keyword Succeeds
    ...    6s
    ...    1s
    ...    Run Keywords
    ...    Check Log Contains String N times   ${MCS_ROUTER_LOG}   mcs_router  Failed to start MCS Push Client service  1  AND
    ...    Check Mcs Envelope Log Contains Regex String N Times   GET \/commands\/applications\/MCS;(\\w;*)+\/endpoint\/   1

    Wait Until Keyword Succeeds
    ...    6s
    ...    1s
    ...    Run Keywords
    ...    Check Log Contains String N times   ${MCS_ROUTER_LOG}   mcs_router  Trying to re-connect to Push Server  1  AND
    ...    Check Mcs Envelope Log Contains Regex String N Times   GET \/commands\/applications\/MCS;(\\w;*)+\/endpoint\/   2

    Start MCS Push Server
    # Ping interval needs to be less, than the ping timeout set in the policy
    Push Client started and connects to Push Server when the MCS Client receives MCS Policy
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  Run Keywords
    ...  Check Mcsrouter Log Contains   Set command poll interval to 20   AND
    ...  Check Mcs Envelope Log Contains Regex String N Times   GET \/commands\/applications\/MCS;(\\w;*)+\/endpoint\/   3

    Sleep  15s
    #checking there is still only three matching lines after 15 seconds
    Check Mcs Envelope Log Contains Regex String N Times   GET \/commands\/applications\/MCS;(\\w;*)+\/endpoint\/   3

    Wait Until Keyword Succeeds
    ...    10s
    ...    1s
    ...    Check Mcs Envelope Log Contains Regex String N Times   GET \/commands\/applications\/MCS;(\\w;*)+\/endpoint\/   4


Verify MCSRouter Disconnect Push Client When It Loses Its own Connection To MCS Server And Return To Standard Command Poll Until Push Server Reconnection
    [Documentation]  From https://wiki.sophos.net/display/PP/MCS+Push+Endpoint+Requirements
    ...  If there are errors, disconnect the push server. Since the poll connection is used to send responses there is little to gain in keeping the push server connected

    Start MCS Push Server
    Install Register And Wait First MCS Policy With MCS Policy  ${SUPPORT_FILES}/CentralXml/MCS_Push_Policy_PushFallbackPoll.xml

    Push Client started and connects to Push Server when the MCS Client receives MCS Policy
    Mark Mcsrouter Log

    Shutdown MCS Push Server
    #    Wait EndPoint Report Broken Connection To Central
    Wait Until Keyword Succeeds
    ...  20 secs
    ...  5 secs
    ...  Check Marked Mcsrouter Log Contains   Failed to establish a new connection: [Errno 111] Connection refused'))

    Wait Until Keyword Succeeds
    ...   30s
    ...   3s
    ...   Check MCS Router Log Contains In Order
    ...         Requesting MCS Push client to stop
    ...         MCS push client stopped

    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  Check MCS Router Log Contains In Order
    ...        Set command poll interval to 20
    ...        Set command poll interval to 5

    Start MCS Push Server

    Wait Until Keyword Succeeds
    ...  15 secs
    ...  1 secs
    ...  Check MCS Router Log Contains In Order
    ...        Set command poll interval to 20
    ...        Set command poll interval to 5
    ...        Set command poll interval to 20

Verify When Push Client Loses Its Connection To Push Server MCS Router Will Trigger An Immediate Command Poll
    [Documentation]  From https://wiki.sophos.net/display/PP/MCS+Push+Endpoint+Requirements
    ...  If PS fails with errors, disconnect PS connection and resume to normal server polling
    ...  Trigger an immediate server poll. This will avoid a potentially long poll wait and attempt to reconnect immediately
    Override LogConf File as Global Level  DEBUG
    Start MCS Push Server
    Install Register And Wait First MCS Policy With MCS Policy  ${SUPPORT_FILES}/CentralXml/MCS_Push_Policy_PushFallbackPoll.xml

    Push Client started and connects to Push Server when the MCS Client receives MCS Policy
    Send Message To Push Server And Expect It In MCSRouter Log   Single Message

    Wait Until Keyword Succeeds
    ...  15 secs
    ...  2 secs
    ...  Check Mcsrouter Log Contains   Set command poll interval to 20

    Shutdown MCS Push Server

    Wait Until Keyword Succeeds
    ...   30s
    ...   3s
    ...   Check MCS Router Log Contains In Order
    ...          Push Server service reported: Push client lost connection to server
    ...          Requesting MCS Push client to stop
    ...          MCS push client stopped
    ...          Checking for commands for ['MCS', 'ALC', 'CORE', 'SDU', 'SHS', 'APPSPROXY', 'AGENT']
    ...          Trying to re-connect to Push Server

    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  Check MCS Router Log Contains In Order
    ...        Set command poll interval to 20
    ...        Set command poll interval to 5

Verify When MCS receives a Command Response Message It Immediately Attempt To Send Regardless Of Any Error Or Backoff
    [Documentation]  From https://wiki.sophos.net/display/PP/MCS+Push+Endpoint+Requirements
    ...   On receipt of a command-response message,   Attempt to send it immediately regardless any errors or backoff
    ...   Command response can be any message to send to the MCS Server i.e ALC Status message.

    Start MCS Push Server

    Install Register And Wait First MCS Policy With MCS Policy  ${SUPPORT_FILES}/CentralXml/MCS_Push_Policy_PushFallbackPoll_300.xml

    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  Check Log Contains String N times   ${MCS_ROUTER_LOG}   mcs_router  Set command poll interval to 300  1

    Mark Mcsrouter Log

    ${expected_body} =  Send EDR Response     LiveQuery  f291664d-112a-328b-e3ed-f920012cdea1
    Check Cloud Server Log For EDR Response Body   LiveQuery  f291664d-112a-328b-e3ed-f920012cdea1  ${expected_body}
    Cloud Server Log Should Not Contain  Failed to decompress response body content

    # Check to ensure the command poll has not run again.
    Check Marked McsRouter Log Contains String N times    Set command poll interval to   0

Connection is lost between endpoint and proxy
    [Teardown]  Test Teardown With Env Proxy
    Start MCS Push Server
    Start Proxy Server With Basic Auth    1235   username   password
    Set Environment Variable  https_proxy   http://username:password@localhost:1235
    Start Local Cloud Server  --initial-mcs-policy  ${SUPPORT_FILES}/CentralXml/MCS_Push_Policy_PushFallbackPoll_300_no_direct.xml
    Register With Local Cloud Server
    Check Correct MCS Password And ID For Local Cloud Saved
    Override LogConf File as Global Level  INFO
    Restart MCSRouter And Clear Logs
    Wait New MCS Policy Downloaded
    Push Client started and connects to Push Server when the MCS Client receives MCS Policy Proxy
    Send Message To Push Server And Expect It In MCSRouter Log  <xml>First Message</xml>

    Stop Proxy Server On Port  1235
    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  Check Mcsrouter Log Contains   MCS push client stopped

    Mark Mcsrouter Log
    Start Proxy Server With Basic Auth    1235   username   password

    Wait Until Keyword Succeeds
    ...          30s
    ...          3s
    ...          Check marked Mcsrouter Log Contains   Push client successfully connected to localhost:4443 via localhost:1235

    Send Message To Push Server And Expect It In MCSRouter Log  <xml>Second Message</xml>

Try connection via proxy with bad authentication
    [Teardown]  Test Teardown With Env Proxy
    Start MCS Push Server
    Start Proxy Server With Basic Auth    1235   username   password
    Set Environment Variable  https_proxy   http://badusername:badpassword@localhost:1235
    Install Register And Wait First MCS Policy With MCS Policy  ${SUPPORT_FILES}/CentralXml/MCS_Push_Policy_PushFallbackPoll_300.xml

    Wait Until Keyword Succeeds
    ...  30 secs
    ...  5 secs
    ...  Check MCSRouter Log Contains  mcsrouter.mcs_push_client <> Failed to connect to localhost:4443 via localhost:1235: Tunnel connection failed: 407 Proxy Authentication Required
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  2 secs
    ...  Check MCSRouter Log Contains  Push client successfully connected to localhost:4443 directly

MCS Push Client Logs Successfull Connection Via Proxy

    Start Proxy Server With Basic Auth    1235   username   password
    Start MCS Push Server

    Start Local Cloud Server  --initial-mcs-policy  ${SUPPORT_FILES}/CentralXml/MCS_Push_Policy_PushFallbackPoll_300.xml
    Register With Local Cloud Server
    Check Correct MCS Password And ID For Local Cloud Saved

    Log File  /opt/sophos-spl/base/etc/mcs.config
    ${config} =     Catenate    SEPARATOR=\n
    ...    MCSToken=ThisIsARegToken
    ...    MCSURL=https://localhost:4443/mcs
    ...    proxy=http://username:password@localhost:1235

    Remove File  /opt/sophos-spl/base/etc/mcs.config
    Create File  /opt/sophos-spl/base/etc/mcs.config  content=${config}
    ${r} =  Run Process  chown  root:sophos-spl-group  /opt/sophos-spl/base/etc/mcs.config
    Log  ${r.stderr}
    should be equal as strings  0  ${r.rc}
    ${r} =  Run Process  chmod  640  /opt/sophos-spl/base/etc/mcs.config
    Log  ${r.stderr}
    should be equal as strings  0  ${r.rc}
    Log File  /opt/sophos-spl/base/etc/mcs.config

    Start MCSRouter

    Wait New MCS Policy Downloaded
    Log File  /opt/sophos-spl/base/etc/mcs.config

    Push Client started and connects to Push Server when the MCS Client receives MCS Policy  proxy

    Send Message To Push Server And Expect It In MCSRouter Log   Single Message

Push Connection Does Not Hang Forever If Proxy Does Not Respond
    [Teardown]  Test Teardown With Push Server Redirect
    Backup Hosts File With Push Server Redirect
    Log File  /etc/hosts
    Start Proxy Server Which Hangs On Push Connection  1235
    Start MCS Push Server

    Start Local Cloud Server  --initial-mcs-policy  ${SUPPORT_FILES}/CentralXml/MCS_Push_Policy_PushFallbackPoll_300_push_redirect_server_address.xml
    Register With Local Cloud Server
    Check Correct MCS Password And ID For Local Cloud Saved

    Log File  /opt/sophos-spl/base/etc/mcs.config
    ${config} =     Catenate    SEPARATOR=\n
    ...    MCSToken=ThisIsARegToken
    ...    MCSURL=https://localhost:4443/mcs
    ...    proxy=http://username:password@localhost:1235
    Remove File  /opt/sophos-spl/base/etc/mcs.config
    Create File  /opt/sophos-spl/base/etc/mcs.config  content=${config}
    ${r} =  Run Process  chown  root:sophos-spl-group  /opt/sophos-spl/base/etc/mcs.config
    should be equal as strings  0  ${r.rc}
    ${r} =  Run Process  chmod  640  /opt/sophos-spl/base/etc/mcs.config
    should be equal as strings  0  ${r.rc}

    Start MCSRouter
    Wait New MCS Policy Downloaded

    Wait Until Keyword Succeeds
    ...  40 secs
    ...  5 secs
    ...  Check mcsrouter Log Contains    Failed to connect to pushredirect:4443 via localhost:1235: HTTPSConnectionPool(host='pushredirect', port=4443): Max retries exceeded with url: /mcs/v2/push/device/ThisIsADeviceID+1001 (Caused by ProxyError('Cannot connect to proxy.', TimeoutError('timed out')))
    Push Client started and connects to Push Server when the MCS Client receives MCS Policy  pushHost=pushredirect

    Send Message To Push Server And Expect It In MCSRouter Log   Single Message

Push Connection Closes Cleanly When MCSRouter Is Stopped
    Remove File  ./tmp/push_server_log.log
    Start MCS Push Server
    Install Register And Wait First MCS Policy With MCS Policy  ${SUPPORT_FILES}/CentralXml/MCS_Push_Policy_PushFallbackPoll.xml
    Push Client started and connects to Push Server when the MCS Client receives MCS Policy
    Send Message To Push Server And Expect It In MCSRouter Log   Single Message
    Stop Mcsrouter If Running

    Wait Until Keyword Succeeds
    ...  5s
    ...  1s
    ...  Check Log Contains  Connection closed with client   ./tmp/push_server_log.log  push server log

*** Keywords ***
Test Teardown
    Push Client Test Teardown
    Remove File  ${SOPHOS_INSTALL}/base/pluginRegistry/edr.json

Test Teardown With Push Server Redirect
    Test Teardown
    Restore Hosts File With Push Server Redirect

Test Teardown With Env Proxy
    Test Teardown
    Remove Environment Variable    https_proxy

EDR Push Client Teardown
    Test Teardown
    Deregister EDR Plugin

Register EDR Plugin
    Copy File  ${SUPPORT_FILES}/base_data/edr.json  ${SOPHOS_INSTALL}/base/pluginRegistry/edr.json

Deregister EDR Plugin
    Remove File  ${SOPHOS_INSTALL}/base/pluginRegistry/edr.json

Backup Hosts File With Push Server Redirect
    Move File  /etc/hosts  /etc/hosts.bak
    Copy File  /etc/hosts.bak  /etc/hosts
    Append To File  /etc/hosts  127.0.0.1\tpushredirect

Restore Hosts File With Push Server Redirect
    Remove File  /etc/hosts
    Move File  /etc/hosts.bak  /etc/hosts
