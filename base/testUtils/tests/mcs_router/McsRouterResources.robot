*** Settings ***
Documentation    Shared keywords for MCS Router tests

Library     OperatingSystem

Library     ${LIBS_DIRECTORY}/MCSRouter.py
Library     ${LIBS_DIRECTORY}/LogUtils.py
Library     ${LIBS_DIRECTORY}/OSUtils.py
Library     ${LIBS_DIRECTORY}/UpdateServer.py
Library     ${LIBS_DIRECTORY}/CentralUtils.py

Resource  ../installer/InstallerResources.robot
Resource  ../watchdog/WatchdogResources.robot
Resource  ../GeneralTeardownResource.robot
Resource  ../GeneralUtilsResources.robot

*** Variables ***
${MCS_ROUTER_LOG}           ${SOPHOS_INSTALL}/logs/base/sophosspl/mcsrouter.log

*** Keywords ***
Setup MCS Tests
    Require Fresh Install
    Set Local CA Environment Variable
    Stop System Watchdog

    Check For Existing MCSRouter
    Cleanup MCSRouter Directories
    Cleanup Local Cloud Server Logs
    Create File  ${SOPHOS_INSTALL}/base/etc/sophosspl/flags-warehouse.json  {}

Clean McsRouter Log File
    ${result} =  Run Process   >   ${SOPHOS_INSTALL}/logs/base/sophosspl/mcsrouter.log   shell=True
    Should Be Equal As Integers  ${result.rc}   0   msg=Failed to truncate mcsrouter log file

Wait For MCS Router To Be Running
    Wait Until Keyword Succeeds
    ...  30
    ...  1
    ...  Check MCS Router Running

Cleanup Certificates
    Run Process    make   cleanCerts    cwd=${SUPPORT_FILES}/CloudAutomation/
    Remove Directory  ../everest-base/modules/mcsrouter/base  recursive=True


MCSRouter Test Teardown
    [Timeout]  2 minutes
    Run Keyword If Test Failed    Log File    /etc/hosts
    Run Keyword If Test Failed    Dump Cloud Server Log
    Run Keyword If Test Failed    Dump MCS Policy Config
    Run Keyword If Test Failed    Dump MCS Config
    Run Keyword If Test Failed    Dump MCS Router Dir Contents

    Run Keyword If Test Failed    Display All tmp Files Present
    General Test Teardown


MCSRouter Default Test Teardown
    Check MCSRouter Does Not Contain Critical Exceptions
    MCSRouter Test Teardown
    Stop MCSRouter If Running
    #There are two ways of starting proxy servers so both required
    Stop Proxy Servers
    Stop Proxy If Running

    Cleanup Proxy Logs

    Cleanup Local Cloud Server Logs
    Cleanup Register Central Log
    Deregister From Central

    Cleanup MCSRouter Directories
    Create File  ${SOPHOS_INSTALL}/base/etc/sophosspl/flags-warehouse.json  {}
    Remove File  ${MCS_POLICY_CONFIG}
    Remove File  ${MCS_CONFIG}
    # ensure no other msc router is running even those not created by this test.
    Kill Mcsrouter
    Unset_CA_Environment_Variable


Restart MCSRouter And Clear Logs
    Stop Mcsrouter If Running
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  Check MCS Router Not Running
    Remove File  ${SOPHOS_INSTALL}/logs/base/sophosspl/mcsrouter.log
    Remove File  ${SOPHOS_INSTALL}/logs/base/sophosspl/mcs_envelope.log
    ${mcsrouter_mark} =  Mark Log Size    ${MCS_ROUTER_LOG}
    Start MCSRouter
    Wait For MCS Router To Be Running
    [Return]  ${mcsrouter_mark}

Regenerate Certificates
    ${result} =  Run Process    make    cleanCerts    cwd=${SUPPORT_FILES}/CloudAutomation  stderr=STDOUT  env:OPENSSL_CONF=../https/openssl.cnf  env:RANDFILE=.rnd
    Should Be Equal As Integers 	${result.rc} 	0
    Log 	cleanCerts output: ${result.stdout}

    Generate Local Fake Cloud Certificates

Generate Local Fake Cloud Certificates
    #Note RANDFILE env variable is required to create the certificates on amazon
    ${result} =  Run Process    make    all           cwd=${SUPPORT_FILES}/CloudAutomation  stderr=STDOUT  env:OPENSSL_CONF=../https/openssl.cnf  env:RANDFILE=.rnd
    Log 	all output: ${result.stdout}

    # You need execute permission on all folders up to the certificate file to use it in mcsrouter (lower priv user)
    # This makes the necessary folder on jenkins and amazon have the correct permissions. Ubuntu has a+x on home folders
    # by default.
    Run Keyword And Ignore Error  Run Process    chmod  a+x  /home/jenkins  #RHEL and CentOS
    Run Keyword And Ignore Error  Run Process    chmod  a+x  /root          #Amazon

    File Should Exist  ${SUPPORT_FILES}/CloudAutomation/server-private.pem
    File Should Exist  ${SUPPORT_FILES}/CloudAutomation/root-ca.crt.pem

Check MCS Router Running
    [Arguments]    ${logs_location}=${SOPHOS_INSTALL}
    ${pid} =  Check MCS Router Process Running  require_running=True

    Wait Until Created   ${logs_location}/logs/base/sophosspl/mcsrouter.log  timeout=5 secs
    [return]  ${pid}

Check MCS Router Not Running
    Check MCS Router Process Running   require_running=False

Check Cloud Server Log For EDR Response
    [Arguments]    ${app_id}  ${correlation_id}  ${occurrence}=1
    Wait Until Keyword Succeeds
    ...  1 min
    ...  5 secs
    ...  Check Cloud Server Log Contains    POST - /mcs/v2/responses/device/ThisIsADeviceID+1001/app_id/${app_id}/correlation_id/${correlation_id}    ${occurrence}

Check Cloud Server Log For EDR Response Body
    [Arguments]    ${app_id}  ${correlation_id}  ${expected_body}
    Wait Until Keyword Succeeds
    ...  1 min
    ...  5 secs
    ...  Cloud Server Log Should Contain  ${app_id} response (${correlation_id}) = ${expected_body}

Check Cloud Server Log For Scheduled Query
    [Arguments]    ${feed_id}  ${device_id}=example-device-id  ${occurrence}=1
    Wait Until Keyword Succeeds
    ...  1 min
    ...  5 secs
    ...  Check Cloud Server Log Contains    POST - /mcs/v2/data_feed/device/${device_id}/feed_id/${feed_id}    ${occurrence}

Check Cloud Server Log For Scheduled Query Body
    [Arguments]    ${feed_id}    ${expected_body}
    Wait Until Keyword Succeeds
    ...  1 min
    ...  5 secs
    ...  Cloud Server Log Should Contain  ${feed_id} datafeed = ${expected_body}

Check Cloud Server Log for Command Poll
    [Arguments]    ${occurrence}=1
    Wait Until Keyword Succeeds
        ...  1 min
        ...  5 secs
        ...  Check Cloud Server Log Contains    GET - /mcs/commands/applications    ${occurrence}


Check Policies Exist
    [Arguments]    @{args}
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  5 secs
    ...  Check Policy Files Exists
    ...      @{args}


Check Default Policies Exist
    Check Policies Exist
    ...      ALC-1_policy.xml
    ...      MCS-25_policy.xml


Check MCS Policy Config Contains
    [Arguments]   ${StringToContain}
    ${MCS_Policy_Conf}=  Set Variable  ${MCS_POLICY_CONFIG}
    Check Log Contains  ${StringToContain}  ${MCS_Policy_Conf}  MCS Policy Config


Check MCS Policy Config Does Not Contain
    [Arguments]   ${StringToContain}
    ${MCS_Policy_Conf}=  Set Variable  ${MCS_POLICY_CONFIG}
    Check Log Does Not Contain  ${StringToContain}  ${MCS_Policy_Conf}  MCS Policy Config


Wait EndPoint Report Error To Connect To Central
    Wait Until Keyword Succeeds
    ...  1 min
    ...  15 secs
    ...  Check mcsrouter Log Contains    Trying connection directly to localhost:4444
   Check Mcsrouter Log Contains  Failed direct connection to localhost:4444


Verify That MCS Connection To Central Is Re-established
    [Arguments]  ${Port}=4443
    Wait Until Keyword Succeeds
    ...  1 min
    ...  15 secs
    ...  Check Mcsrouter Log Contains    Successfully directly connected to localhost:${Port}


Wait EndPoint Report Broken Connection To Central
    Wait Until Keyword Succeeds
    ...  1 min
    ...  5 secs
    ...  Check Mcsrouter Log Contains    Failed direct connection to localhost:4443


Wait EndPoint Report It Has Re-registered
    Wait Until Keyword Succeeds
    ...  1 min
    ...  5 secs
    ...  Check Mcsrouter Log Contains    Re-registering with MCS
    Check Mcsrouter Log Contains    Lost authentication with server


Ensure Log Contains Connection Via Proxy
    Wait Until Keyword Succeeds
    ...  60 secs
    ...  10 secs
    ...  Check MCSRouter Log Contains  Successfully connected to localhost:4443 via localhost:3000


Ensure Log Contains Policy Rejected
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  5 secs
    ...  Check MCSRouter Log Contains  Failed to parse MCS policy


# This keyword ensures that the product installs, registers with fake cloud and receives the MCS policy
Install Register And Wait First MCS Policy
    Start Local Cloud Server
    Register With Local Cloud Server
    Check Correct MCS Password And ID For Local Cloud Saved
    Start MCSRouter
    Wait New MCS Policy Downloaded

Check MCS Envelope Contains Event Success On N Event Sent
    [Arguments]  ${Event_Number}
    ${string}=  Check Log And Return Nth Occurrence Between Strings   <event><appId>ALC</appId>  </event>  ${SOPHOS_INSTALL}/logs/base/sophosspl/mcs_envelope.log  ${Event_Number}
    Should contain   ${string}   &lt;number&gt;0&lt;/number&gt;

Check MCS Envelope Contains Event Fail On N Event Sent
    [Arguments]  ${Event_Number}
    ${string}=  Check Log And Return Nth Occurrence Between Strings   <event><appId>ALC</appId>  </event>  ${SOPHOS_INSTALL}/logs/base/sophosspl/mcs_envelope.log  ${Event_Number}
    Should Not Contain   ${string}   &lt;number&gt;0&lt;/number&gt;
Start 401 Server
    [Arguments]    ${port}=4443
    ${handle} =  Start Process  python3  ${SUPPORT_FILES}/CloudAutomation/401server.py  ${port}
    [Return]    ${handle}


Stop 401 Server
    [Arguments]    ${handle}
    ${result} =  Terminate Process  ${handle}
    Log  "401 server output: " ${result.stdout} ${result.stderr}


Log Content of MCS Push Server Log
    ${LogContent}=  MCS Push Server Log
    Log  ${LogContent}

Push Server Teardown with MCS Fake Server
    MCSRouter Test Teardown
    Stop Local Cloud Server
    Cleanup Local Cloud Server Logs
    Push Server Test Teardown   runGeneral=False


Push Server Test Teardown
    [Arguments]  ${runGeneral}=True
    Shutdown MCS Push Server
    Run Keyword If Test Failed    Log Content of MCS Push Server Log
    Run Keyword If  ${runGeneral}  General Test Teardown

Install Register And Wait First MCS Policy With MCS Policy
    [Arguments]  ${mcs_policy}
    Override LogConf File as Global Level  DEBUG
    Remove File  ${SOPHOS_INSTALL}/logs/base/sophosspl/mcsrouter.log
    Remove File  ${SOPHOS_INSTALL}/logs/base/sophosspl/mcs_envelope.log
    Start Local Cloud Server  --initial-mcs-policy  ${mcs_policy}
    Register With Local Cloud Server
    Check Correct MCS Password And ID For Local Cloud Saved
    Start MCSRouter
    Wait For MCS Router To Be Running
    Wait New MCS Policy Downloaded

JWT Token Is Updated In MCS Config
    # Whenever the token is requested and received, it is also stored in mcs.config
    Wait Until Keyword Succeeds
    ...  15s
    ...  2s
    ...  File Should Contain  ${MCS_CONFIG}  jwt_token=
    File Should Contain  ${MCS_CONFIG}  tenant_id=
    File Should Contain  ${MCS_CONFIG}  device_id=

Restart MCS Router With Debug Logging
    Stop Mcsrouter If Running
    Wait Until Keyword Succeeds
    ...  15s
    ...  2s
    ...  Check MCS Router Not Running
    Mark MCSRouter Log
    Create File         ${SOPHOS_INSTALL}/base/etc/logger.conf.local   [mcs_router]\nVERBOSITY=DEBUG\n
    Start MCSRouter
    Wait Until Keyword Succeeds
    ...  15s
    ...  2s
    ...  Check MCS Router Running

Register With Fake Cloud
    Register With Fake Cloud Without Starting MCSRouter
    Start MCSRouter

Register With Fake Cloud Without Starting MCSRouter
    Should Exist  ${REGISTER_CENTRAL}
    Register With Local Cloud Server
    Check Correct MCS Password And ID For Local Cloud Saved
    Check Correct MCS Password And ID For Local Cloud Saved

Check Current Proxy Is Created With Correct Content And Permissions
    [Arguments]  ${content}
    ${currentProxyFilePath} =  Set Variable  ${SOPHOS_INSTALL}/base/etc/sophosspl/current_proxy
    ${currentProxyContents} =  Get File  ${currentProxyFilePath}
    Should Contain  ${currentProxyContents}  ${content}
    Ensure Owner and Group Matches  ${currentProxyFilePath}  sophos-spl-local  sophos-spl-group

Get MCSRouter PID
    ${r} =  Run Process  pgrep  -f  mcsrouter
    Should Be Equal As Strings  ${r.rc}  0
    [Return]  ${r.stdout}

Get Device ID From Config
    ${device_id}  get_value_from_ini_file  device_id  ${MCS_CONFIG}
    [Return]  ${device_id}