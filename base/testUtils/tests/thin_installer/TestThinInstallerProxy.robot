*** Settings ***
Test Setup      Setup Thininstaller Test
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
Setup With Large Group Creation
    Setup Group File With Large Group Creation

Teardown With Large Group Creation
    Teardown Group File With Large Group Creation
    Teardown

Setup Thininstaller Test
    Start Local Cloud Server
    Setup Thininstaller Test Without Local Cloud Server

Setup Thininstaller Test Without Local Cloud Server
    Require Uninstalled
    Set Environment Variable  CORRUPTINSTALL  no
    Get Thininstaller
    Create Default Credentials File
    Build Default Creds Thininstaller From Sections

Setup Legacy Thininstaller Test
    Start Local Cloud Server
    Setup Legacy Thininstaller Test Without Local Cloud Server

Setup Legacy Thininstaller Test Without Local Cloud Server
    Setup Update Tests

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

Teardown With Temporary Directory Clean
    Teardown
    Remove Directory   ${tmpdir}  recursive=True

Teardown With Temporary Directory Clean And Stopping Message Relays
    Teardown With Temporary Directory Clean
    Stop Proxy If Running

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
    Remove Directory  ${CUSTOM_TEMP_UNPACK_DIR}  recursive=True
    Remove Environment Variable  INSTALL_OPTIONS_FILE
    Cleanup Temporary Folders

Remove SAV files
    Remove Fake Savscan In Tmp
    Run Keyword And Ignore Error    Delete Fake Sweep Symlink    /usr/bin
    Run Keyword And Ignore Error    Delete Fake Sweep Symlink    /usr/local/bin/
    Run Keyword And Ignore Error    Delete Fake Sweep Symlink    /bin
    Run Keyword And Ignore Error    Delete Fake Sweep Symlink    /tmp

SAV Teardown
    Remove SAV files
    Teardown

Cert Test Teardown
    Teardown
    Install System Ca Cert  ${SUPPORT_FILES}/https/ca/root-ca.crt

Run ThinInstaller Instdir And Check It Fails
    [Arguments]    ${path}
    Log  ${path}
    Run Default Thininstaller With Args  19  --instdir=${path}
    Check Thininstaller Log Contains  The --instdir path provided contains invalid characters. Only alphanumeric and '/' '-' '_' '.' characters are accepted
    Remove Thininstaller Log

Run ThinInstaller With Bad Group Name And Check It Fails
    [Arguments]    ${groupName}
    Log  ${GroupName}
    Run Default Thininstaller With Args  24  --group=${groupName}
    Check Thininstaller Log Contains  Error: Group name contains one of the following invalid characters: < & > ' \" --- aborting install
    Remove Thininstaller Log

Create Fake Ldd Executable With Version As Argument And Add It To Path
    [Arguments]  ${version}
    ${FakeLddDirectory} =  Add Temporary Directory  FakeExecutable
    ${script} =     Catenate    SEPARATOR=\n
    ...    \#!/bin/bash
    ...    echo "ldd (Ubuntu GLIBC 99999-ubuntu1) ${version}"
    ...    \
    Create File    ${FakeLddDirectory}/ldd   content=${script}
    Run Process    chmod  +x  ${FakeLddDirectory}/ldd

    ${PATH} =  Get System Path
    ${result} =  Run Process  ${FakeLddDirectory}/ldd
    Log  ${result.stdout}

    [Return]  ${FakeLddDirectory}:${PATH}

Get System Path
    ${PATH} =  Get Environment Variable  PATH
    [Return]  ${PATH}

Thin Installer Calls Base Installer With Environment Variables For Product Argument
    [Arguments]  ${productArgs}

    Validate Env Passed To Base Installer  --products ${productArgs}   --customer-token ThisIsACustomerToken   ThisIsARegToken   https://localhost:4443/mcs

Thin Installer Calls Base Installer Without Environment Variables For Product Argument
    Validate Env Passed To Base Installer  ${EMPTY}  --customer-token ThisIsACustomerToken  ThisIsARegToken  https://localhost:4443/mcs

Run Thin Installer And Check Argument Is Saved To Install Options File
    [Arguments]  ${argument}
    ${install_location}=  get_default_install_script_path
    ${thin_installer_cmd}=  Create List    ${install_location}   ${argument}
    Remove Directory  ${CUSTOM_TEMP_UNPACK_DIR}  recursive=True
    Run Thin Installer  ${thin_installer_cmd}   expected_return_code=0  cleanup=False  temp_dir_to_unpack_to=${CUSTOM_TEMP_UNPACK_DIR}
    Should Exist  ${CUSTOM_TEMP_UNPACK_DIR}
    Should Exist  ${CUSTOM_TEMP_UNPACK_DIR}/install_options
    ${contents} =  Get File  ${CUSTOM_TEMP_UNPACK_DIR}/install_options
    Should Contain  ${contents}  ${argument}

*** Variables ***
${PROXY_LOG}  ./tmp/proxy_server.log
${MCS_CONFIG_FILE}  ${SOPHOS_INSTALL}/base/etc/mcs.config
${CUSTOM_TEMP_UNPACK_DIR} =  /tmp/temporary-unpack-dir
@{FORCE_ARGUMENT} =  --force
@{PRODUCT_MDR_ARGUMENT} =  --products\=mdr
${BaseVUTPolicy}                    ${SUPPORT_FILES}/CentralXml/ALC_policy_direct_just_base.xml

*** Test Case ***
Thin Installer Falls Back From Bad Env Proxy To Direct
    # NB we use the warehouse URL as the MCSUrl here as the thin installer just does a get over HTTPS that's all we need
    # the url to respond against
    [Setup]  Setup Legacy Thininstaller Test
    Setup warehouse
    Run Default Thininstaller   expected_return_code=0  override_location=https://localhost:1233  proxy=http://notanaddress.sophos.com  force_certs_dir=${SUPPORT_FILES}/sophos_certs
    Check Thininstaller Log Contains  INSTALLER EXECUTED
    Check Thininstaller Log Contains  WARN: Could not connect using proxy

Thin Installer Registering With Message Relays Is Not Impacted By Env Proxy
    [Setup]  Setup Legacy Thininstaller Test
    Setup warehouse
    Start Message Relay

    Create Default Credentials File  message_relays=dummyhost1:10000,1,2;localhost:20000,2,4
    Build Default Creds Thininstaller From Sections
    Run Default Thininstaller   expected_return_code=0  override_location=https://localhost:1233  proxy=http://notanaddress.sophos.com  force_certs_dir=${SUPPORT_FILES}/sophos_certs

    #Check Thininstaller Log Contains  INFO - Product successfully registered via proxy: localhost:20000 # log for sdds3
    Check Thininstaller Log Contains  Successfully verified connection to Sophos Central via Message Relay: localhost:20000

Thin Installer Attempts Install And Register Through Message Relays
    [Setup]    Setup Legacy Thininstaller Test Without Local Cloud Server
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
    Check Current Proxy Is Created With Correct Content And Permissions  localhost:20000

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
    [Setup]  Setup Legacy Thininstaller Test Without Local Cloud Server
    [Teardown]  Teardown With Temporary Directory Clean
    Start Local Cloud Server
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
    [Setup]  Setup Legacy Thininstaller Test Without Local Cloud Server
    [Teardown]  Teardown With Temporary Directory Clean
    Start Local Cloud Server
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
    [Setup]  Setup Legacy Thininstaller Test Without Local Cloud Server
    [Teardown]  Teardown With Temporary Directory Clean
    Start Local Cloud Server
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
