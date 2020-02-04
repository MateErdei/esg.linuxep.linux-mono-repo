*** Settings ***
Documentation  Thin Installer tests which require a fake warehouse serving everest-base

# Thin installer can take a little while to run, e.g. 4 mins, which is close to the default test timeout of 5 mins
# so make it longer to allow for this.
Test Timeout    15 minutes

Test Setup   SetupServers
Test Teardown    Teardown

Suite Setup   Local Suite Setup
Suite Teardown   Local Suite Teardown

Library     ${libs_directory}/WarehouseGenerator.py
Library     ${libs_directory}/WarehouseUtils.py
Library     ${libs_directory}/UpdateServer.py
Library     ${libs_directory}/ThinInstallerUtils.py
Library     ${libs_directory}/LogUtils.py
Library     ${libs_directory}/FullInstallerUtils.py
Library     ${libs_directory}/MCSRouter.py
Library     ${libs_directory}/TemporaryDirectoryManager.py
Library     ${libs_directory}/CentralUtils.py
Library     Process
Library     OperatingSystem
Library     String
Resource    ../mcs_router/McsRouterResources.robot
Resource    ../installer/InstallerResources.robot
Resource    ../upgrade_product/UpgradeResources.robot
Resource    ../GeneralTeardownResource.robot

Default Tags  THIN_INSTALLER  OSTIA
*** Variables ***
${PROXY_LOG}  ./tmp/proxy_server.log
${MCS_CONFIG_FILE}  ${SOPHOS_INSTALL}/base/etc/mcs.config
${CUSTOM_DIR_BASE}  /CustomPath
${BaseVUTPolicy}   ${GeneratedWarehousePolicies}/base_only_VUT.xml

*** Keywords ***
Local Suite Setup
    Regenerate Certificates
    Copy File  ${SUPPORT_FILES}/CloudAutomation/root-ca.crt.pem  /tmp/root-ca.crt.pem
    Setup Ostia Warehouse Environment

Local Suite Teardown
    Teardown Ostia Warehouse Environment
    Remove File  /tmp/root-ca.crt.pem

SetupServers
    Ensure Fake Cloud Certs Generated
    Set Environment Variable  CORRUPTINSTALL  no
    Require Uninstalled
    Uninstall SAV
    Get Thininstaller
    Set Local CA Environment Variable
    Start Local Cloud Server  --initial-alc-policy  ${BaseVUTPolicy}

Teardown
    Run Keyword If Test Failed  Dump Thininstaller Log
    Log File  ${PROXY_LOG}
    General Test Teardown
    Stop Update Server
    Stop Proxy Servers
    Stop Proxy If Running
    Stop Local Cloud Server
    Teardown Reset Original Path
    Uninstall SAV
    Run Keyword If Test Failed    Dump Thininstaller Log
    Run Keyword And Ignore Error  Move File  /etc/hosts.bk  /etc/hosts
    Cleanup Files
    Require Uninstalled
    Reset Sophos Install Environment Variable
    Remove Directory  ${CUSTOM_DIR_BASE}  recursive=$true
    Cleanup Temporary Folders
    Cleanup Systemd Files


Check Proxy Log Contains
    [Arguments]  ${pattern}  ${fail_message}
    ${ret} =  Grep File  ${PROXY_LOG}  ${pattern}
    Should Contain  ${ret}  ${pattern}  ${fail_message}

Check MCS Config Contains
    [Arguments]  ${pattern}  ${fail_message}
    ${ret} =  Grep File  ${MCS_CONFIG_FILE}  ${pattern}
    Should Contain  ${ret}  ${pattern}  ${fail_message}

Find IP Address With Distance
    [Arguments]  ${dist}
    ${result} =  Run Process  ip  addr
    ${ipaddresses} =  Get Regexp Matches  ${result.stdout}  inet (10[^/]*)  1
    ${head} =  Get Regexp Matches  ${ipaddresses[0]}  (10\.[^\.]*\.[^\.]*\.)[^\.]*  1
    ${tail} =  Get Regexp Matches  ${ipaddresses[0]}  10\.[^\.]*\.[^\.]*\.([^\.]*)  1
    ${tail_xored} =  Evaluate  ${tail[0]} ^ (2 ** ${dist})
    [return]  ${head[0]}${tail_xored}

Check All Sophos-spl Services Running
    Check Watchdog Running
    Check Management Agent Running
    Check Update Scheduler Running
    Check MCS Router Process Running

Check For Single Process Running
    [Arguments]  ${ProcessName}
    ${result}=   Run Process  ps  -ef
    ${lines}=  Get Lines Matching Pattern  ${result.stdout}  *${ProcessName}*
    Should Contain X Times  ${lines}  ${ProcessName}  1  Zero or more than one process with name: ${ProcessName}

Check Root Directory Permissions Are Not Changed
    ${result}=  Run Process  stat  -c  "%A"  /
    ${ExpectedPerms}=  Set Variable  "dr[w-]xr-xr-x"
    Should Match Regexp  ${result.stdout}  ${ExpectedPerms}

Check All Relevant Logs Contain Install Path
    [Arguments]   ${Install_Path}
    Check Log Contains  ${Install_Path}  ${Install_Path}/sophos-spl/logs/base/register_central.log  Register Central
    Check Log Contains  ${Install_Path}  ${Install_Path}/sophos-spl/logs/base/watchdog.log  Watchdog
    Check Log Contains  ${Install_Path}  ${Install_Path}/sophos-spl/logs/base/wdctl.log  WDCTL
    Check Log Contains  ${Install_Path}  ${Install_Path}/sophos-spl/logs/base/sophosspl/sophos_managementagent.log  Management Agent
    Check Log Contains  ${Install_Path}  ${Install_Path}/sophos-spl/logs/base/sophosspl/mcsrouter.log  MCS Router

Create Initial Installation
    Require Fresh Install
    Set Local CA Environment Variable


*** Test Case ***
Thin Installer Reregisters Existing Install
    Create Initial Installation
    # MCS Password and ID should not be saved
    Run Keyword And Expect Error  No MCS Config - registration failed.  Check Correct MCS Password And ID For Local Cloud Saved
    Configure And Run Thininstaller Using Real Warehouse Policy  0  ${BaseVUTPolicy}
    Check Thininstaller Log Contains  Attempting to register existing installation with Sophos Central
    Check Thininstaller Log Contains  Now managed by Sophos Central
    Check Correct MCS Password And ID For Local Cloud Saved
    Check Thininstaller Log Does Not Contain  ERROR
    Check Root Directory Permissions Are Not Changed

Thin Installer Repairs Broken Existing Installation
    # Install to default location and break it
    Create Initial Installation
    Should Exist  ${REGISTER_CENTRAL}
    Remove File  ${REGISTER_CENTRAL}
    Should Not Exist  ${REGISTER_CENTRAL}
    
    Configure And Run Thininstaller Using Real Warehouse Policy  0  ${BaseVUTPolicy}

    Check Thininstaller Log Contains  Found existing installation here: /opt/sophos-spl
    Check Thininstaller Log Does Not Contain  ERROR
    Should Exist  ${REGISTER_CENTRAL}
    remove_thininstaller_log
    Check Root Directory Permissions Are Not Changed

Thin Installer Fails When No Path In Systemd File
    # Install to default location and break it
    Create Initial Installation

    Log File  /lib/systemd/system/sophos-spl.service

    ${result}=  Run Process  sed  -i  s/SOPHOS_INSTALL.*/SOPHbroken/  /lib/systemd/system/sophos-spl.service
    Should Be Equal As Integers    ${result.rc}    0

    Log File  /lib/systemd/system/sophos-spl.service

    ${result}=  Run Process  systemctl  daemon-reload

    Log File  /lib/systemd/system/sophos-spl.service

    Configure And Run Thininstaller Using Real Warehouse Policy  20  ${BaseVUTPolicy}
    Check Thininstaller Log Contains  An existing installation of Sophos Managed Threat Response for Linux was found but could not find the installed path.
    Check Thininstaller Log Does Not Contain  ERROR
    remove_thininstaller_log
    Check Root Directory Permissions Are Not Changed
    
Thin Installer Installs Base And Services Start
    Should Not Exist    ${SOPHOS_INSTALL}

    Check MCS Router Not Running
    ${result} =  Run Process    pgrep  -f  ${MANAGEMENT_AGENT}
    Should Not Be Equal As Integers  ${result.rc}  0  Management Agent running before installation

    Configure And Run Thininstaller Using Real Warehouse Policy  0  ${BaseVUTPolicy}  mcs_ca=/tmp/root-ca.crt.pem

    Should Exist    ${SOPHOS_INSTALL}

    ${result} =  Run Process    pgrep  -f  ${MANAGEMENT_AGENT}
    Should Be Equal As Integers  ${result.rc}  0  Management Agent not running after installation
    Check MCS Router Running
    Check Correct MCS Password And ID For Local Cloud Saved

    Check Thininstaller Log Does Not Contain  ERROR

    ${result}=  Run Process  stat  -c  "%A"  /opt
    ${ExpectedPerms}=  Set Variable  "drwxr-xr-x"
    Should Be Equal As Strings  ${result.stdout}  ${ExpectedPerms}

Thin Installer Installs To Custom Location And Services Start
    [Tags]    THIN_INSTALLER  CUSTOM_LOCATION  OSTIA
    Install Local SSL Server Cert To System
    Should Not Exist    ${SOPHOS_INSTALL}
    ${Install_Path}=  Set Variable  ${CUSTOM_DIR_BASE}/with-sub/with-sub2

    Set Sophos Install Environment Variable  ${Install_Path}/sophos-spl
    Configure And Run Thininstaller Using Real Warehouse Policy  0  ${BaseVUTPolicy}  args=--instdir=${Install_Path}

    Should Exist  ${Install_Path}
    Should Not Exist  /opt/sophos-spl

    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  Check All Sophos-spl Services Running

    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  Check All Relevant Logs Contain Install Path  ${Install_Path}

    Check Thininstaller Log Does Not Contain  ERROR
    Check Root Directory Permissions Are Not Changed

Thin Installer installs to a custom location with and deletes sophos-spl directory on uninstall
    [Tags]    THIN_INSTALLER  CUSTOM_LOCATION  OSTIA
    ${Install_Path}=  Set Variable  ${CUSTOM_DIR_BASE}/dir1/dir2/

    Set Sophos Install Environment Variable  ${Install_Path}/sophos-spl
    Configure And Run Thininstaller Using Real Warehouse Policy  0  ${BaseVUTPolicy}  args=--instdir=${Install_Path}

    Directory Should Exist  ${Install_Path}/sophos-spl

    Require Uninstalled

    Directory Should Exist  ${Install_Path}
    Directory Should Not Exist  ${Install_Path}/sophos-spl

    Check Thininstaller Log Does Not Contain  ERROR
    Check Root Directory Permissions Are Not Changed

Thin Installer Installs With A Path Length Of 50 Chars
    [Tags]     THIN_INSTALLER  CUSTOM_LOCATION  OSTIA
    ${Install_Path}=  Set Variable  ${CUSTOM_DIR_BASE}/34567892023456789302345678940234567895

    Set Sophos Install Environment Variable  ${Install_Path}/sophos-spl
    Configure And Run Thininstaller Using Real Warehouse Policy  0  ${BaseVUTPolicy}  mcs_ca=/tmp/root-ca.crt.pem  args=--instdir=${Install_Path}

    Should Exist  ${Install_Path}
    Should Not Exist  /opt/sophos-spl

    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  Check All Sophos-spl Services Running

    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  Check All Relevant Logs Contain Install Path  ${Install_Path}

    Check Thininstaller Log Does Not Contain  ERROR
    Check Root Directory Permissions Are Not Changed


Thin Installer Attempts Install And Register Through Message Relays
    Start Message Relay
    Should Not Exist    ${SOPHOS_INSTALL}

    Check MCS Router Not Running
    ${result} =  Run Process    pgrep  -f  ${MANAGEMENT_AGENT}
    Should Not Be Equal As Integers  ${result.rc}  0  Management Agent running before installation

    # Create Dummy Hosts with certain distances (assume IP address is on eng (starts with 10))
    ${dist1} =  Find IP Address With Distance  1
    ${dist3} =  Find IP Address With Distance  3
    ${dist7} =  Find IP Address With Distance  7
    Copy File  /etc/hosts  /etc/hosts.bk
    Append To File  /etc/hosts  ${dist1} dummyhost1\n${dist3} dummyhost3\n${dist7} dummyhost7\n

    Install Local SSL Server Cert To System

    # Add Message Relays to Thin Installer
    Configure And Run Thininstaller Using Real Warehouse Policy  0  ${BaseVUTPolicy}  mcs_ca=/tmp/root-ca.crt.pem  message_relays=dummyhost3:10000,1,1;dummyhost1:20000,1,2;localhost:20000,2,4;dummyhost7:9999,1,3

    # Check the MCS Capabilities check is performed with the Message Relays in the right order
    Check Thininstaller Log Contains    Message Relays: dummyhost3:10000,1,1;dummyhost1:20000,1,2;localhost:20000,2,4;dummyhost7:9999,1,3
    # Thininstaller orders only by priority, localhost is only one with low priority
    Log File  /etc/hosts
    Check Thininstaller Log Contains In Order
    ...  Checking we can connect to Sophos Central (at https://localhost:4443/mcs via dummyhost3:10000)
    ...  Checking we can connect to Sophos Central (at https://localhost:4443/mcs via dummyhost1:20000)
    ...  Checking we can connect to Sophos Central (at https://localhost:4443/mcs via dummyhost7:9999)
    ...  Checking we can connect to Sophos Central (at https://localhost:4443/mcs via localhost:20000)\nSuccessfully got [No error] from Sophos Central

    Should Exist    ${SOPHOS_INSTALL}
    ${result} =  Run Process    pgrep  -f  ${MANAGEMENT_AGENT}
    Should Be Equal As Integers  ${result.rc}  0  Management Agent not running after installation
    Check MCS Router Running

    # Check the message relays made their way through to the registration command in the full installer
    # Message relays ordered by distance and priority
    Check Register Central Log Contains In Order
    ...  Trying connection via message relay dummyhost1:20000
    ...  Trying connection via message relay dummyhost3:10000
    ...  Trying connection via message relay dummyhost7:9999
    ...  Successfully connected to localhost:4443 via localhost:20000

    # Check the message relays made their way through to the MCS Router
    File Should Exist  ${SUPPORT_FILES}/CloudAutomation/root-ca.crt.pem
    Wait Until Keyword Succeeds
        ...  65 secs
        ...  5 secs
        ...  Check MCS Router Log Contains  Successfully connected to localhost:4443 via localhost:20000

    Check Thininstaller Log Does Not Contain  ERROR
    Check Root Directory Permissions Are Not Changed

Thin Installer Environment Proxy
    Should Not Exist    ${SOPHOS_INSTALL}
    Check MCS Router Not Running
    ${result} =  Run Process    pgrep  -f  ${MANAGEMENT_AGENT}
    Should Not Be Equal As Integers  ${result.rc}  0  Management Agent running before installation


    # Installation should fail before starting the proxy
    Configure And Run Thininstaller Using Real Warehouse Policy  3  ${BaseVUTPolicy}  proxy=http://localhost:10000


    Start Simple Proxy Server  10000
    Configure And Run Thininstaller Using Real Warehouse Policy  0  ${BaseVUTPolicy}  proxy=http://localhost:10000

    ${customer_file_address} =  Get Customer File Domain For Policy  ${BaseVUTPolicy}
    ${warehouse_address} =  Get Warehouse Domain For Policy  ${BaseVUTPolicy}

    Check Proxy Log Contains  "CONNECT localhost:4443 HTTP/1.1" 200  Proxy Log does not show connection to Fake Cloud
    Check Proxy Log Contains  "CONNECT ${customer_file_address} HTTP/1.1" 200  Proxy Log does not show connection to Fake Warehouse
    Check Proxy Log Contains  "CONNECT ${warehouse_address} HTTP/1.1" 200  Proxy Log does not show connection to Fake Warehouse
    Check MCS Config Contains  proxy=http://localhost:10000  MCS Config does not have proxy present

    Check Thininstaller Log Does Not Contain  ERROR
    Check Root Directory Permissions Are Not Changed

Thin Installer Digest Proxy
    Should Not Exist    ${SOPHOS_INSTALL}
    Check MCS Router Not Running
    ${result} =  Run Process    pgrep  -f  ${MANAGEMENT_AGENT}
    Should Not Be Equal As Integers  ${result.rc}  0  Management Agent running before installation


    # Installation should fail before starting the proxy
    Configure And Run Thininstaller Using Real Warehouse Policy  3  ${BaseVUTPolicy}  proxy=http://username:password@localhost:10000


    Start Proxy Server With Digest Auth  10000  username  password
    Configure And Run Thininstaller Using Real Warehouse Policy  0  ${BaseVUTPolicy}  proxy=http://username:password@localhost:10000

    ${customer_file_address} =  Get Customer File Domain For Policy  ${BaseVUTPolicy}
    ${warehouse_address} =  Get Warehouse Domain For Policy  ${BaseVUTPolicy}

    Check Proxy Log Contains  "CONNECT localhost:4443 HTTP/1.1" 200  Proxy Log does not show connection to Fake Cloud
    Check Proxy Log Contains  "CONNECT ${customer_file_address} HTTP/1.1" 200  Proxy Log does not show connection to Fake Warehouse
    Check Proxy Log Contains  "CONNECT ${warehouse_address} HTTP/1.1" 200  Proxy Log does not show connection to Fake Warehouse
    Check MCS Config Contains  proxy=http://username:password@localhost:10000  MCS Config does not have proxy present

    Check Thininstaller Log Does Not Contain  ERROR
    Check Root Directory Permissions Are Not Changed

Thin Installer Basic Proxy
    Should Not Exist    ${SOPHOS_INSTALL}
    Check MCS Router Not Running
    ${result} =  Run Process    pgrep  -f  ${MANAGEMENT_AGENT}
    Should Not Be Equal As Integers  ${result.rc}  0  Management Agent running before installation

    # Installation should fail before starting the proxy
    Configure And Run Thininstaller Using Real Warehouse Policy  3  ${BaseVUTPolicy}  proxy=http://username:password@localhost:10000

    Start Proxy Server With Basic Auth  10000  username  password
    Configure And Run Thininstaller Using Real Warehouse Policy  0  ${BaseVUTPolicy}  proxy=http://username:password@localhost:10000

    ${customer_file_address} =  Get Customer File Domain For Policy  ${BaseVUTPolicy}
    ${warehouse_address} =  Get Warehouse Domain For Policy  ${BaseVUTPolicy}

    Check Proxy Log Contains  "CONNECT localhost:4443 HTTP/1.1" 200  Proxy Log does not show connection to Fake Cloud
    Check Proxy Log Contains  "CONNECT ${customer_file_address} HTTP/1.1" 200  Proxy Log does not show connection to Fake Warehouse
    Check Proxy Log Contains  "CONNECT ${warehouse_address} HTTP/1.1" 200  Proxy Log does not show connection to Fake Warehouse
    Check MCS Config Contains  proxy=http://username:password@localhost:10000  MCS Config does not have proxy present

    Check Thininstaller Log Does Not Contain  ERROR
    Check Root Directory Permissions Are Not Changed

Thin Installer Parses Update Caches Correctly
    # Expect installer to fail, we only care about log contents
    Configure And Run Thininstaller Using Real Warehouse Policy  10  ${BaseVUTPolicy}  bad_url=${True}  update_caches=localhost:10000,1,1;localhost:20000,2,2;localhost:9999,1,3
    Check Thininstaller Log Contains    List of update caches to install from: localhost:10000,1,1;localhost:20000,2,2;localhost:9999,1,3
    Check Thininstaller Log Contains In Order
    ...  Listing warehouse with creds [c071bb1cc186531a72e200467a61b321] at [https://localhost:10000/sophos/customer]
    ...  Listing warehouse with creds [c071bb1cc186531a72e200467a61b321] at [https://localhost:9999/sophos/customer]
    ...  Listing warehouse with creds [c071bb1cc186531a72e200467a61b321] at [https://localhost:20000/sophos/customer]
    ...  Listing warehouse with creds [c071bb1cc186531a72e200467a61b321] at [http://dci.sophosupd.com/update]

    Check Thininstaller Log Does Not Contain  ERROR
    Check Root Directory Permissions Are Not Changed


Thin Installer Force Works
    # Install to default location and break it
    Create Initial Installation

    # Remove install directory
    Should Exist  ${REGISTER_CENTRAL}
    Remove Directory  /opt/sophos-spl  recursive=True
    Should Not Exist  ${REGISTER_CENTRAL}

    # Force an installation
    Configure And Run Thininstaller Using Real Warehouse Policy  0  ${BaseVUTPolicy}  args=--force

    Check Thininstaller Log Does Not Contain  ERROR
    Should Exist  ${REGISTER_CENTRAL}

    remove_thininstaller_log
    Check Root Directory Permissions Are Not Changed