*** Settings ***
Suite Setup  Setup Update Tests
Test Setup      Setup Legacy Thininstaller Test
Test Teardown   Teardown

Library     ${LIBS_DIRECTORY}/WarehouseGenerator.py
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

*** Keywords ***

Setup Legacy Thininstaller Test
    Start Local Cloud Server
    Require Uninstalled
    Set Environment Variable  CORRUPTINSTALL  no
    Get Legacy Thininstaller
    Create Default Credentials File
    Build Default Creds Thininstaller From Sections

Setup Warehouse
    [Arguments]    ${customer_file_protocol}=--tls1_2   ${warehouse_protocol}=--tls1_2
    Clear Warehouse Config
    Generate Install File In Directory     ./tmp/TestInstallFiles/${BASE_RIGID_NAME}
    Add Component Warehouse Config   ${BASE_RIGID_NAME}   ./tmp/TestInstallFiles/    ./tmp/temp_warehouse/
    Generate Warehouse
    Start Update Server    1233    ./tmp/temp_warehouse/customer_files/   ${customer_file_protocol}
    Start Update Server    1234    ./tmp/temp_warehouse/warehouses/sophosmain/   ${warehouse_protocol}



Teardown
    General Test Teardown
    Stop Update Server
    Stop Proxy Servers
    Run Keyword If Test Failed   Dump Cloud Server Log
    Stop Local Cloud Server
    Cleanup Local Cloud Server Logs
    Teardown Reset Original Path
    Run Keyword If Test Failed    Dump Thininstaller Log
    Remove Thininstaller Log
    Cleanup Files
    Require Uninstalled
    Remove Environment Variable  SOPHOS_INSTALL
    Remove Environment Variable  INSTALL_OPTIONS_FILE
    Cleanup Temporary Folders

*** Variables ***
${PROXY_LOG}  ./tmp/proxy_server.log
${MCS_CONFIG_FILE}  ${SOPHOS_INSTALL}/base/etc/mcs.config
${BaseVUTPolicy}                    ${SUPPORT_FILES}/CentralXml/ALC_policy_direct_just_base.xml

*** Test Case ***
Thin Installer Falls Back From Bad Env Proxy To Direct
    # NB we use the warehouse URL as the MCSUrl here as the thin installer just does a get over HTTPS that's all we need
    # the url to respond against
    Setup warehouse
    Run Default Thininstaller   expected_return_code=0  override_location=https://localhost:1233  proxy=http://notanaddress.sophos.com  force_certs_dir=${SUPPORT_FILES}/sophos_certs
    Check Thininstaller Log Contains  INSTALLER EXECUTED
    Check Thininstaller Log Contains  WARN: Could not connect using proxy

Thin Installer Registering With Message Relays Is Not Impacted By Env Proxy
    Setup warehouse
    Start Message Relay

    Create Default Credentials File  message_relays=dummyhost1:10000,1,2;localhost:20000,2,4
    Build Default Creds Thininstaller From Sections
    Run Default Thininstaller   expected_return_code=0  override_location=https://localhost:1233  proxy=http://notanaddress.sophos.com  force_certs_dir=${SUPPORT_FILES}/sophos_certs

    #Check Thininstaller Log Contains  INFO - Product successfully registered via proxy: localhost:20000 # log for sdds3
    Check Thininstaller Log Contains  Successfully verified connection to Sophos Central via Message Relay: localhost:20000

Thin Installer Attempts Install And Register Through Message Relays
    [Teardown]  Teardown With Temporary Directory Clean And Stopping Message Relays
    Setup For Test With Warehouse Containing Product
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
    Run Default Thininstaller  expected_return_code=0  override_location=https://localhost:1233  force_certs_dir=${SUPPORT_FILES}/sophos_certs

    # Check current proxy file is written with correct content and permissions.
    # Once MCS gets the BaseVUTPolicy policy the current_proxy file will be set to {} as there are no MRs in the policy
    Wait Until Keyword Succeeds
    ...  5 secs
    ...  1 secs
    ...  Check Current Proxy Is Created With Correct Content And Permissions  localhost:20000

    # Check the MCS Capabilities check is performed with the Message Relays in the right order
    Check Thininstaller Log Contains    Message Relays: dummyhost1:10000,1,2;localhost:20000,2,4
    # Thininstaller orders only by priority, localhost is only one with low priority
    Log File  /etc/hosts
    Check Thininstaller Log Contains In Order
    ...  Checking we can connect to Sophos Central (at https://localhost:4443/mcs via dummyhost1:10000)
    ...  Checking we can connect to Sophos Central (at https://localhost:4443/mcs via localhost:20000)\nDEBUG: Set CURLOPT_PROXYAUTH to CURLAUTH_ANY\nDEBUG: Set CURLOPT_PROXY to: localhost:20000\nDEBUG: Successfully got [No error] from Sophos Central
     #enable these lines when using sdds3 proxy
#    ...  DEBUG - Performing request: https://localhost:4443/mcs/register\nDEBUG - cURL Info:   Trying 127.0.0.1:10000...\n\nDEBUG - cURL Info: connect to 127.0.0.1 port 10000 failed: Connection refused
#    ...  INFO - Product successfully registered via proxy: localhost:20000
#    ...  DEBUG - Performing request: https://localhost:4443/mcs/authenticate/endpoint/ThisIsAnMCSID+1001/role/endpoint\nDEBUG - cURL Info:   Trying 127.0.0.1:20000...\n\nDEBUG - cURL Info: Connected to localhost (127.0.0.1) port 20000 (#0)

    # Check the message relays made their way through to the registration command in the full installer
    Check Register Central Log Contains In Order
    ...  Trying connection via message relay dummyhost1:10000
    ...  Successfully connected to localhost:4443 via localhost:20000

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

    Check Thininstaller Log Does Not Contain  ERROR
    Check Root Directory Permissions Are Not Changed

Thin Installer Digest Proxy
    [Teardown]  Teardown With Temporary Directory Clean
    Setup For Test With Warehouse Containing Product
    Check MCS Router Not Running
    ${result} =  Run Process    pgrep  -f  ${MANAGEMENT_AGENT}
    Should Not Be Equal As Integers  ${result.rc}  0  Management Agent running before installation

    Start Proxy Server With Digest Auth  10000  username  password

    Run Default Thininstaller  expected_return_code=0  override_location=https://localhost:1233  force_certs_dir=${SUPPORT_FILES}/sophos_certs  proxy=http://username:password@localhost:10000

    #The customer and warehouse domains are localhost:1233 and localhost:1234
    Check Proxy Log Contains  "CONNECT localhost:4443 HTTP/1.1" 200  Proxy Log does not show connection to Fake Cloud
    Check Proxy Log Contains  "CONNECT localhost:1233 HTTP/1.1" 200  Proxy Log does not show connection to Fake Warehouse
    Check Proxy Log Contains  "CONNECT localhost:1234 HTTP/1.1" 200  Proxy Log does not show connection to Fake Warehouse
    Check MCS Config Contains  proxy=http://username:password@localhost:10000  MCS Config does not have proxy present

    Check Thininstaller Log Does Not Contain  ERROR

    Check Thininstaller Log Contains  DEBUG: Checking we can connect to Sophos Central (at https://localhost:4443/mcs via http://username:password@localhost:10000)\nDEBUG: Set CURLOPT_PROXYAUTH to CURLAUTH_ANY\nDEBUG: Set CURLOPT_PROXY to: http://username:password@localhost:10000\nDEBUG: Successfully got [No error] from Sophos Central
    Check Root Directory Permissions Are Not Changed

    Wait Until Keyword Succeeds
    ...  60 secs
    ...  2 secs
    ...  Check SulDownloader Log Contains   Try connection: Sophos at http://dci.sophosupd.com/update via proxy: http://localhost:10000

    Check Log Does Not Contain  Try connection: Sophos at http://dci.sophosupd.com/update via proxy: http://localhost:10000\n\n  ${SOPHOS_INSTALL}/logs/base/suldownloader.log  suldownloader log

Thin Installer Environment Proxy
    [Teardown]  Teardown With Temporary Directory Clean
    Setup For Test With Warehouse Containing Product
    Should Not Exist    ${SOPHOS_INSTALL}
    Check MCS Router Not Running
    ${result} =  Run Process    pgrep  -f  ${MANAGEMENT_AGENT}
    Should Not Be Equal As Integers  ${result.rc}  0  Management Agent running before installation

    Start Simple Proxy Server  10000
    Run Default Thininstaller  expected_return_code=0  override_location=https://localhost:1233  force_certs_dir=${SUPPORT_FILES}/sophos_certs  proxy=http://localhost:10000

    Log File  /opt/sophos-spl/base/etc/mcs.config

    #The customer and warehouse domains are localhost:1233 and localhost:1234
    Check Proxy Log Contains  "CONNECT localhost:4443 HTTP/1.1" 200  Proxy Log does not show connection to Fake Cloud
    Check Proxy Log Contains  "CONNECT localhost:1233 HTTP/1.1" 200  Proxy Log does not show connection to Fake Warehouse
    Check Proxy Log Contains  "CONNECT localhost:1234 HTTP/1.1" 200  Proxy Log does not show connection to Fake Warehouse
    Check MCS Config Contains  proxy=http://localhost:10000  MCS Config does not have proxy present

    Check Thininstaller Log Does Not Contain  ERROR
    Check Thininstaller Log Contains  DEBUG: Checking we can connect to Sophos Central (at https://localhost:4443/mcs via http://localhost:10000)\nDEBUG: Set CURLOPT_PROXYAUTH to CURLAUTH_ANY\nDEBUG: Set CURLOPT_PROXY to: http://localhost:10000\nDEBUG: Successfully got [No error] from Sophos Central
    Check Root Directory Permissions Are Not Changed
    Wait Until Keyword Succeeds
    ...  60 secs
    ...  2 secs
    ...  Check SulDownloader Log Contains   Try connection: Sophos at http://dci.sophosupd.com/update via proxy: http://localhost:10000


Thin Installer Parses Update Caches Correctly
    [Teardown]  Teardown With Temporary Directory Clean
    Setup For Test With Warehouse Containing Product
    Create Default Credentials File  update_caches=localhost:10000,1,1;localhost:20000,2,2;localhost:9999,1,3
    Build Default Creds Thininstaller From Sections
    Run Default Thininstaller  expected_return_code=10  no_connection_address_override=True  force_certs_dir=${SUPPORT_FILES}/sophos_certs


    Check Thininstaller Log Contains    List of update caches to install from: localhost:10000,1,1;localhost:20000,2,2;localhost:9999,1,3
    Check Thininstaller Log Contains In Order
    ...  Listing warehouse with creds [9539d7d1f36a71bbac1259db9e868231] at [https://localhost:10000/sophos/customer]
    ...  Listing warehouse with creds [9539d7d1f36a71bbac1259db9e868231] at [https://localhost:9999/sophos/customer]
    ...  Listing warehouse with creds [9539d7d1f36a71bbac1259db9e868231] at [https://localhost:20000/sophos/customer]
    ...  Listing warehouse with creds [9539d7d1f36a71bbac1259db9e868231] at [http://dci.sophosupd.com/update]

    Check Thininstaller Log Does Not Contain  ERROR
    Check Root Directory Permissions Are Not Changed
