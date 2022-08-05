*** Settings ***
Test Setup      Setup Thininstaller Test
Test Teardown   Thininstaller Test Teardown

Suite Setup  sdds3 suite setup with fakewarehouse with real base
Suite Teardown   sdds3 suite fake warehouse Teardown

Library     ${LIBS_DIRECTORY}/UpdateServer.py
Library     ${LIBS_DIRECTORY}/ThinInstallerUtils.py
Library     ${LIBS_DIRECTORY}/OSUtils.py
Library     ${LIBS_DIRECTORY}/LogUtils.py
Library     ${LIBS_DIRECTORY}/FullInstallerUtils.py
Library     ${LIBS_DIRECTORY}/TemporaryDirectoryManager.py
Library     ${LIBS_DIRECTORY}/MCSRouter.py
Library     ${LIBS_DIRECTORY}/CentralUtils.py
Library     Process
Library     DateTime
Library     OperatingSystem

Resource  ../installer/InstallerResources.robot
Resource  ../GeneralTeardownResource.robot
Resource  ../scheduler_update/SchedulerUpdateResources.robot
Resource    ../upgrade_product/UpgradeResources.robot
Resource    ../mcs_router/McsRouterResources.robot
Resource  ThinInstallerResources.robot

Default Tags  THIN_INSTALLER

*** Variables ***
${PROXY_LOG}  ./tmp/proxy_server.log
${BaseVUTPolicy}                    ${SUPPORT_FILES}/CentralXml/ALC_policy_direct_just_base.xml
${MCS_CONFIG_FILE}  ${SOPHOS_INSTALL}/base/etc/mcs.config

*** Test Case ***
SDDS3 Thin Installer Falls Back From Bad Env Proxy To Direct
    Run Default Thininstaller   expected_return_code=0  proxy=http://notanaddress.sophos.com   force_certs_dir=${SDDS3_DEVCERTS}
    Check Thininstaller Log Contains  Successfully installed product
    Check Thininstaller Log Contains  Checking we can connect to Sophos Central (at https://localhost:4443/mcs via http://notanaddress.sophos.com)
    Check Thininstaller Log Contains  Failed to connect to Sophos Central at https://localhost:4443/mcs (cURL error is [Couldn't resolve proxy name])

SDDS3 Thin Installer Registering With Message Relays Is Not Impacted By Env Proxy
    Start Message Relay

    Create Default Credentials File  message_relays=dummyhost1:10000,1,2;localhost:20000,2,4
    Build Default Creds Thininstaller From Sections
    Run Default Thininstaller   expected_return_code=0    proxy=http://notanaddress.sophos.com   force_certs_dir=${SDDS3_DEVCERTS}

    Check Thininstaller Log Contains  INFO - Product successfully registered via proxy: localhost:20000
    Check suldownloader log contains   Trying to update via proxy localhost:20000 to https://localhost:8080
    check_suldownloader_log_should_not_contain  Connecting to update source directly

SDDS3 Thin Installer Attempts Install And Register Through Message Relays
    [Teardown]  Teardown With Temporary Directory Clean And Stopping Message Relays
    Start Message Relay
    Should Not Exist    ${SOPHOS_INSTALL}
    Stop Local Cloud Server
    Start Local Cloud Server   --initial-mcs-policy    ${SUPPORT_FILES}/CentralXml/FakeCloudMCS_policy_Message_Relay.xml   --initial-alc-policy    ${BaseVUTPolicy}

    # Create Dummy Host
    #there will be no proxy on port 10000
    ${dist1} =  Set Variable  127.0.0.1

    Copy File  /etc/hosts  /etc/hosts.bk
    Append To File  /etc/hosts  ${dist1} dummyhost1

    Install Local SSL Server Cert To System

    # Add Message Relays to Thin Installer
    Create Default Credentials File  message_relays=dummyhost1:10000,1,2;localhost:20000,2,4
    Build Default Creds Thininstaller From Sections
    Run Default Thininstaller  expected_return_code=0  override_location=https://localhost:1233   force_certs_dir=${SDDS3_DEVCERTS}

    # Check current proxy file is written with correct content and permissions.
    # Once MCS gets the BaseVUTPolicy policy the current_proxy file will be set to {} as there are no MRs in the policy
    Check Current Proxy Is Created With Correct Content And Permissions  localhost:20000

    # Check the MCS Capabilities check is performed with the Message Relays in the right order
    Check Thininstaller Log Contains    Message Relays: dummyhost1:10000,1,2;localhost:20000,2,4
    # Thininstaller orders only by priority, localhost is only one with low priority
    Log File  /etc/hosts
    Check Thininstaller Log Contains In Order
    ...  Checking we can connect to Sophos Central (at https://localhost:4443/mcs via dummyhost1:10000)
    ...  Checking we can connect to Sophos Central (at https://localhost:4443/mcs via localhost:20000)\nDEBUG: Set CURLOPT_PROXYAUTH to CURLAUTH_ANY\nDEBUG: Set CURLOPT_PROXY to: localhost:20000\nDEBUG: Successfully got [No error] from Sophos Central
    ...  DEBUG - Performing request: https://localhost:4443/mcs/register\nDEBUG - cURL Info:   Trying 127.0.0.1:10000...\n\nDEBUG - cURL Info: connect to 127.0.0.1 port 10000 failed: Connection refused
    ...  INFO - Product successfully registered via proxy: localhost:20000
    ...  DEBUG - Performing request: https://localhost:4443/mcs/authenticate/endpoint/ThisIsAnMCSID+1001/role/endpoint\nDEBUG - cURL Info:   Trying 127.0.0.1:20000...\n\nDEBUG - cURL Info: Connected to localhost (127.0.0.1) port 20000 (#0)

    Should Exist    ${SOPHOS_INSTALL}
    ${result} =  Run Process    pgrep  -f  ${MANAGEMENT_AGENT}
    Should Be Equal As Integers  ${result.rc}  0  Management Agent not running after installation
    Check MCS Router Running

    # Check the message relays made their way through to the MCS Router
    File Should Exist  ${SUPPORT_FILES}/CloudAutomation/root-ca.crt.pem
    Wait Until Keyword Succeeds
    ...  65 secs
    ...  5 secs
    ...  Check MCS Router Log Contains  Successfully connected to localhost:4443 via localhost:20000

    # Also to prove MCS is working correctly check that we get an ALC policy
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  2 secs
    ...  Check Policy Written Match File  ALC-1_policy.xml  ${BaseVUTPolicy}

    Check suldownloader log contains   Trying to update via proxy localhost:20000 to https://localhost:8080
    check_suldownloader_log_should_not_contain  Connecting to update source directly

    Check Thininstaller Log Does Not Contain  ERROR
    Check Root Directory Permissions Are Not Changed

SDDS3 Thin Installer Digest Proxy
    [Setup]  Setup Thininstaller Test Without Local Cloud Server
    [Teardown]  Teardown With Temporary Directory Clean
    Start Local Cloud Server
    Check MCS Router Not Running
    ${result} =  Run Process    pgrep  -f  ${MANAGEMENT_AGENT}
    Should Not Be Equal As Integers  ${result.rc}  0  Management Agent running before installation

    Start Proxy Server With Digest Auth  10000  username  password

    Run Default Thininstaller  expected_return_code=0   proxy=http://username:password@localhost:10000  force_certs_dir=${SDDS3_DEVCERTS}

    #The customer and warehouse domains are localhost:1233 and localhost:1234
    Check Proxy Log Contains  "CONNECT localhost:4443 HTTP/1.1" 200  Proxy Log does not show connection to Fake Cloud
    Check Proxy Log Contains  "CONNECT localhost:8080 HTTP/1.1" 200  Proxy Log does not show connection to Fake Warehouse
    Check MCS Config Contains  proxy=http://username:password@localhost:10000  MCS Config does not have proxy present

    Check Thininstaller Log Does Not Contain  ERROR

    Check Thininstaller Log Contains  DEBUG: Checking we can connect to Sophos Central (at https://localhost:4443/mcs via http://username:password@localhost:10000)\nDEBUG: Set CURLOPT_PROXYAUTH to CURLAUTH_ANY\nDEBUG: Set CURLOPT_PROXY to: http://username:password@localhost:10000\nDEBUG: Successfully got [No error] from Sophos Central
    Check Root Directory Permissions Are Not Changed

    Wait Until Keyword Succeeds
    ...  60 secs
    ...  2 secs
    ...  Check SulDownloader Log Contains   Trying to update via proxy http://username:password@localhost:10000 to https://localhost:8080

    check_suldownloader_log_should_not_contain  Connecting to update source directly

SDDS3 Thin Installer Environment Proxy
    [Setup]  Setup Thininstaller Test Without Local Cloud Server
    [Teardown]  Teardown With Temporary Directory Clean
    Start Local Cloud Server
    Should Not Exist    ${SOPHOS_INSTALL}
    Check MCS Router Not Running
    ${result} =  Run Process    pgrep  -f  ${MANAGEMENT_AGENT}
    Should Not Be Equal As Integers  ${result.rc}  0  Management Agent running before installation

    Start Simple Proxy Server  10000
    Run Default Thininstaller  expected_return_code=0   proxy=http://localhost:10000  force_certs_dir=${SDDS3_DEVCERTS}

    #The customer and warehouse domains are localhost:1233 and localhost:1234
    Check Proxy Log Contains  "CONNECT localhost:4443 HTTP/1.1" 200  Proxy Log does not show connection to Fake Cloud
    Check Proxy Log Contains  "CONNECT localhost:8080 HTTP/1.1" 200  Proxy Log does not show connection to Fake Warehouse
    Check MCS Config Contains  proxy=http://localhost:10000  MCS Config does not have proxy present

    Check Thininstaller Log Does Not Contain  ERROR
    Check Thininstaller Log Contains  DEBUG: Checking we can connect to Sophos Central (at https://localhost:4443/mcs via http://localhost:10000)\nDEBUG: Set CURLOPT_PROXYAUTH to CURLAUTH_ANY\nDEBUG: Set CURLOPT_PROXY to: http://localhost:10000\nDEBUG: Successfully got [No error] from Sophos Central
    Check Root Directory Permissions Are Not Changed
    Wait Until Keyword Succeeds
    ...  60 secs
    ...  2 secs
    ...  Check SulDownloader Log Contains   Trying to update via proxy http://localhost:10000 to https://localhost:8080

    check_suldownloader_log_should_not_contain  Connecting to update source directly

