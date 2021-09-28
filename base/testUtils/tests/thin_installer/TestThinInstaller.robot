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
    Teardown With Temporary Directory Clean

Setup Warehouse
    [Arguments]    ${customer_file_protocol}=--tls1_2   ${warehouse_protocol}=--tls1_2
    Clear Warehouse Config
    Generate Install File In Directory     ./tmp/TestInstallFiles/${BASE_RIGID_NAME}
    Add Component Warehouse Config   ${BASE_RIGID_NAME}   ./tmp/TestInstallFiles/    ./tmp/temp_warehouse/
    Generate Warehouse
    Start Update Server    1233    ./tmp/temp_warehouse/customer_files/   ${customer_file_protocol}
    Start Update Server    1234    ./tmp/temp_warehouse/warehouses/sophosmain/   ${warehouse_protocol}

Setup Thininstaller Test
    Require Uninstalled
    Set Environment Variable  CORRUPTINSTALL  no
    Get Thininstaller
    Create Default Credentials File
    Build Default Creds Thininstaller From Sections


Teardown With Temporary Directory Clean
    Teardown
    Remove Directory   ${tmpdir}  recursive=True

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
    Remove Directory  ${CUSTOM_DIR_BASE}  recursive=True
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

    Validate Env Passed To Base Installer  --products ${productArgs}   --customer-token ThisIsACustomerToken   ThisIsARegToken   https://localhost:1233

Thin Installer Calls Base Installer Without Environment Variables For Product Argument
    Validate Env Passed To Base Installer  ${EMPTY}  --customer-token ThisIsACustomerToken  ThisIsARegToken  https://localhost:1233

Run Thin Installer And Check Argument Is Saved To Install Options File
    [Arguments]  ${argument}
    ${install_location}=  get_default_install_script_path
    ${thin_installer_cmd}=  Create List    ${install_location}   ${argument}
    Remove Directory  ${CUSTOM_TEMP_UNPACK_DIR}  recursive=True
    Run Thin Installer  ${thin_installer_cmd}   expected_return_code=3  cleanup=False  temp_dir_to_unpack_to=${CUSTOM_TEMP_UNPACK_DIR}
    Should Exist  ${CUSTOM_TEMP_UNPACK_DIR}
    Should Exist  ${CUSTOM_TEMP_UNPACK_DIR}/install_options
    ${contents} =  Get File  ${CUSTOM_TEMP_UNPACK_DIR}/install_options
    Should Contain  ${contents}  ${argument}

*** Variables ***
${PROXY_LOG}  ./tmp/proxy_server.log
${MCS_CONFIG_FILE}  ${SOPHOS_INSTALL}/base/etc/mcs.config
${CUSTOM_DIR_BASE} =  /CustomPath
${CUSTOM_TEMP_UNPACK_DIR} =  /tmp/temporary-unpack-dir
@{FORCE_ARGUMENT} =  --force
@{PRODUCT_MDR_ARGUMENT} =  --products\=mdr
${BaseVUTPolicy}                    ${GeneratedWarehousePolicies}/base_only_VUT.xml

*** Test Case ***
Thin Installer can download test file from warehouse and execute it
    [Tags]  SMOKE  THIN_INSTALLER
    Setup Warehouse
    Run Default Thininstaller    0    https://localhost:1233  force_certs_dir=${SUPPORT_FILES}/sophos_certs
    Check Thininstaller Log Contains    INSTALLER EXECUTED

Thin Installer fails to download test file from warehouse if certificate is not installed
    [Teardown]  Cert Test Teardown
    Cleanup System Ca Certs
    Setup Warehouse
    Run Default Thininstaller    10    https://localhost:1233

Thin Installer fails to install on system without enough memory
    Run Default Thininstaller With Fake Memory Amount    234
    Check Thininstaller Log Contains    This machine does not meet product requirements. The product requires at least 1GB of RAM

Thin Installer fails to install on system without enough storage
    Run Default Thininstaller With Fake Small Disk
    Check Thininstaller Log Contains    Not enough space in / to install Sophos Linux Protection. You can install elsewhere by re-running this installer with the --instdir argument

Thin Installer fails to install to the tmp folder
    ${Install_Path}=  Set Variable  /tmp
    Run Default Thininstaller With Args  19  --instdir=${Install_Path}
    Check Thininstaller Log Contains  The --instdir path provided is in the non-persistent /tmp folder. Please choose a location that is persistent

Thin Installer fails to install to a folder within the temp folder
    ${Install_Path}=  Set Variable  /tmp/dir2/dir2
    Run Default Thininstaller With Args  19  --instdir=${Install_Path}
    Check Thininstaller Log Contains  The --instdir path provided is in the non-persistent /tmp folder. Please choose a location that is persistent

Thin Installer Fails With Invalid Paths
    Run ThinInstaller Instdir And Check It Fails   /abc"def
    Run ThinInstaller Instdir And Check It Fails   /abc'def
    Run ThinInstaller Instdir And Check It Fails   /abc*def
    Run ThinInstaller Instdir And Check It Fails   /"abc\def"
    Run ThinInstaller Instdir And Check It Fails   /abc&def
    Run ThinInstaller Instdir And Check It Fails   /abc<def
    Run ThinInstaller Instdir And Check It Fails   /abc>def
    Run ThinInstaller Instdir And Check It Fails   /abc%def
    Run ThinInstaller Instdir And Check It Fails   /abc!def
    Run ThinInstaller Instdir And Check It Fails   /'abc£def'
    Run ThinInstaller Instdir And Check It Fails   /abc^def
    Run ThinInstaller Instdir And Check It Fails   /abc(def
    Run ThinInstaller Instdir And Check It Fails   /abc)def
    Run ThinInstaller Instdir And Check It Fails   /abc[def
    Run ThinInstaller Instdir And Check It Fails   /abc]def
    Run ThinInstaller Instdir And Check It Fails   /abc{def
    Run ThinInstaller Instdir And Check It Fails   /abc}def
    Run ThinInstaller Instdir And Check It Fails   /{}!"£%^^&^&*$"£$!
    Run ThinInstaller Instdir And Check It Fails   /abc:def
    Run ThinInstaller Instdir And Check It Fails   /abc;def
    Run ThinInstaller Instdir And Check It Fails   /abc def
    Run ThinInstaller Instdir And Check It Fails   /abc=def

Thin Installer Tells Us It Is Governed By A License
    Run Default Thininstaller    3
    Check Thininstaller Log Contains    This software is governed by the terms and conditions of a licence agreement with Sophos Limited

Thin Installer Does Not Tell Us About Which Sweep
    Run Default Thininstaller    3
    Check Thininstaller Log Does Not Contain    which: no sweep in

Thin Installer Detects Sweep And Cancels Installation
    [Tags]  SAV  THIN_INSTALLER
    [Teardown]  SAV Teardown
    Create Fake Savscan In Tmp
    Create Fake Sweep Symlink    /usr/bin
    Run Default Thininstaller    8
    Check Thininstaller Log Contains    Found an existing installation of SAV in /tmp/i/am/fake
    Delete Fake Sweep Symlink   /usr/bin

    Create Fake Sweep Symlink    /usr/local/bin/
    Run Default Thininstaller    8
    Check Thininstaller Log Contains    Found an existing installation of SAV in /tmp/i/am/fake
    Delete Fake Sweep Symlink    /usr/local/bin/

    Create Fake Sweep Symlink    /bin
    Run Default Thininstaller    8
    Check Thininstaller Log Contains    Found an existing installation of SAV in /tmp/i/am/fake
    Delete Fake Sweep Symlink    /bin

    ${path} =  Get System Path
    Create Fake Sweep Symlink   /tmp
    run thininstaller with non standard path    8  /tmp:/bin:${path}
    Check Thininstaller Log Contains     Found an existing installation of SAV in /tmp/i/am/fake
    Delete Fake Sweep Symlink    /tmp

Thin Installer Has Working Version Option
    Run Default Thininstaller With Args   0     --version
    Check Thininstaller Log Contains    Sophos Linux Protection Installer, version: 1.

Thin Installer Has Working Version Option With Other Arguments
    Run Default Thininstaller With Args   0     --version  --other
    Check Thininstaller Log Contains    Sophos Linux Protection Installer, version: 1.

Thin Installer Has Working Version Option Where Preceding Arguments Are Ignored
    Run Default Thininstaller With Args   0     --other  --version
    Check Thininstaller Log Contains    Sophos Linux Protection Installer, version: 1.

Check Installer Does Not Contain Todos Or Fixmes
    [Tags]   THIN_INSTALLER
    @{stringsToCheck}=  Create List   FIXME  TODO
    ${Result}=  Check If Unwanted Strings Are In ThinInstaller   ${stringsToCheck}
    Should Be Equal  ${Result}  ${False}   Error: Installer contains at least one of the following un-wanted values: ${stringsToCheck} which are not allowed in release

Thin Installer Fails When System Has Glibc Less Than Build Machine
    [Tags]  SMOKE  THIN_INSTALLER
    Setup Warehouse
    ${LowGlibcVersion} =  Set Variable  1.0
    ${buildGlibcVersion} =  Get Glibc Version From Thin Installer
    ${PATH} =  Create Fake Ldd Executable With Version As Argument And Add It To Path  ${LowGlibcVersion}
    Run Thininstaller With Non Standard Path  21  ${PATH}  https://localhost:1233
    Check Thininstaller Log Contains    Failed to install on unsupported system. Detected GLIBC version 1.0 < required ${buildGlibcVersion}

Thin Installer Succeeds When System Has Glibc Greater Than Build Machine
    Setup Warehouse
    ${HighGlibcVersion} =  Set Variable  999.999
    ${PATH} =  Create Fake Ldd Executable With Version As Argument And Add It To Path  ${HighGlibcVersion}
    Run Thininstaller With Non Standard Path  0  ${PATH}  https://localhost:1233  force_certs_dir=${SUPPORT_FILES}/sophos_certs
    Check Thininstaller Log Contains    INSTALLER EXECUTED

Thin Installer Succeeds When System Has Glibc Same As Build Machine
    Setup Warehouse
    ${buildGlibcVersion} =  Get Glibc Version From Thin Installer
    ${PATH} =  Create Fake Ldd Executable With Version As Argument And Add It To Path  ${buildGlibcVersion}
    Run Thininstaller With Non Standard Path  0  ${PATH}  https://localhost:1233  force_certs_dir=${SUPPORT_FILES}/sophos_certs
    Check Thininstaller Log Contains    INSTALLER EXECUTED

Thin Installer Falls Back From Bad Env Proxy To Direct
    Setup Warehouse
    # NB we use the warehouse URL as the MCSUrl here as the thin installer just does a get over HTTPS that's all we need
    # the url to respond against
    Run Default Thininstaller   expected_return_code=0  mcsurl=https://localhost:1233  override_location=https://localhost:1233  proxy=http://notanaddress.sophos.com  force_certs_dir=${SUPPORT_FILES}/sophos_certs
    Check Thininstaller Log Contains  INSTALLER EXECUTED
    Check Thininstaller Log Contains  WARN: Could not connect using proxy

Thin Installer Will Not Connect to Central If Connection Has TLS below TLSv1_2
    [Tags]  SMOKE  THIN_INSTALLER
    Setup Warehouse   --tls1_2   --tls1_2
    Start Local Cloud Server   --tls   tlsv1_1
    Cloud Server Log Should Contain      SSL version: _SSLMethod.PROTOCOL_TLSv1_1
    Run Default Thininstaller    3    https://localhost:4443
    Check Thininstaller Log Contains    Failed to connect to Sophos Central at https://localhost:4443 (cURL error is [SSL connect error]). Please check your firewall rules

Thin Installer SUL Library Will Not Connect to Warehouse If Connection Has TLS below TLSv1_2
    Setup Warehouse   --tls1_2   --tls1_1
    Start Local Cloud Server   --tls   tlsv1_2
    Run Default Thininstaller    10    https://localhost:4443
    Check Thininstaller Log Contains    Failed to download the base installer! (Error code = 46)

Thin Installer And SUL Library Will Successfully Connect With Server Running TLSv1_2
    [Tags]  SMOKE  THIN_INSTALLER
    Setup Warehouse   --tls1_2   --tls1_2
    Start Local Cloud Server   --tls   tlsv1_2
    Run Default Thininstaller    0    https://localhost:4443  force_certs_dir=${SUPPORT_FILES}/sophos_certs
    Check Thininstaller Log Contains    INSTALLER EXECUTED

Thin Installer With Space In Name Works
    Setup Warehouse
    Run Default Thininstaller With Different Name    SophosSetup (1).sh    0    https://localhost:1233   force_certs_dir=${SUPPORT_FILES}/sophos_certs
    Check Thininstaller Log Contains    INSTALLER EXECUTED

Thin Installer Fails When No Path In Systemd File
    # Install to default location and break it
    Create Initial Installation

    Log File  /lib/systemd/system/sophos-spl.service

    ${result}=  Run Process  sed  -i  s/SOPHOS_INSTALL.*/SOPHbroken/  /lib/systemd/system/sophos-spl.service
    Should Be Equal As Integers    ${result.rc}    0

    Log File  /lib/systemd/system/sophos-spl.service

    ${result}=  Run Process  systemctl  daemon-reload

    Log File  /lib/systemd/system/sophos-spl.service

    Build Default Creds Thininstaller From Sections
    Run Default Thininstaller  20

    Check Thininstaller Log Contains  An existing installation of Sophos Linux Protection was found but could not find the installed path.
    Check Thininstaller Log Does Not Contain  ERROR
    Check Root Directory Permissions Are Not Changed

Thin Installer Attempts Install And Register Through Message Relays
    [Teardown]  Teardown With Temporary Directory Clean
    Setup For Test With Warehouse Containing Product
    Start Message Relay
    Should Not Exist    ${SOPHOS_INSTALL}
    Stop Local Cloud Server
    Start Local Cloud Server   --initial-mcs-policy    ${SUPPORT_FILES}/CentralXml/FakeCloudMCS_policy_Message_Relay.xml   --initial-alc-policy    ${BaseVUTPolicy}

    Check MCS Router Not Running
    ${result} =  Run Process    pgrep  -f  ${MANAGEMENT_AGENT}
    Should Not Be Equal As Integers  ${result.rc}  0  Management Agent running before installation

    # Create Dummy Host
    #there will be no proxy on port 10000
    ${dist1} =  Set Variable  127.0.0.1

    Copy File  /etc/hosts  /etc/hosts.bk
    Append To File  /etc/hosts  ${dist1} dummyhost1

    Install Local SSL Server Cert To System

    # Add Message Relays to Thin Installer
    Create Default Credentials File  message_relays=dummyhost1:10000,1,2;localhost:20000,2,4
    Build Default Creds Thininstaller From Sections
    #Configure And Run TCredentialshininstaller Using Real Warehouse Policy  0  ${BaseVUTPolicy}  mcs_ca=/tmp/root-ca.crt.pem
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


    Should Exist    ${SOPHOS_INSTALL}
    ${result} =  Run Process    pgrep  -f  ${MANAGEMENT_AGENT}
    Should Be Equal As Integers  ${result.rc}  0  Management Agent not running after installation
    Check MCS Router Running

    # Check the message relays made their way through to the registration command in the full installer
    Check Register Central Log Contains In Order
    ...  Trying connection via message relay dummyhost1:10000
    ...  Successfully connected to localhost:4443 via localhost:20000

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
    Start Local Cloud Server

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
    ...  Check SulDownloader Log Contains   Try connection: Sophos at http://dci.sophosupd.com/update via proxy: localhost:10000

    Check Log Does Not Contain  Try connection: Sophos at http://dci.sophosupd.com/update via proxy: localhost:10000\n\n  ${SOPHOS_INSTALL}/logs/base/suldownloader.log  suldownloader log

Thin Installer Environment Proxy
    [Teardown]  Teardown With Temporary Directory Clean
    Setup For Test With Warehouse Containing Product
    Start Local Cloud Server

    Should Not Exist    ${SOPHOS_INSTALL}
    Check MCS Router Not Running
    ${result} =  Run Process    pgrep  -f  ${MANAGEMENT_AGENT}
    Should Not Be Equal As Integers  ${result.rc}  0  Management Agent running before installation

    Start Simple Proxy Server  10000
    Run Default Thininstaller  expected_return_code=0  override_location=https://localhost:1233  force_certs_dir=${SUPPORT_FILES}/sophos_certs  proxy=http://localhost:10000

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
    ...  Check SulDownloader Log Contains   Try connection: Sophos at http://dci.sophosupd.com/update via proxy: localhost:10000


Thin Installer Parses Update Caches Correctly
    # Expect installer to fail, we only care about log contents
    [Teardown]  Teardown With Temporary Directory Clean
    Setup For Test With Warehouse Containing Product
    Start Local Cloud Server

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

Thin Installer Registers Existing Installation
    [Tags]  THIN_INSTALLER  MCS_ROUTER
    [Teardown]  Teardown With Temporary Directory Clean
    Setup For Test With Warehouse Containing Base
    Start Local Cloud Server

    # Force an installation
    Run Default Thininstaller  expected_return_code=0  override_location=https://localhost:1233  force_certs_dir=${SUPPORT_FILES}/sophos_certs

    Check Thininstaller Log Does Not Contain  ERROR
    Check Cloud Server Log Contains  Register with ::ThisIsARegToken\n
    Check Cloud Server Log Does Not Contain  Register with ::ThisIsARegTokenFromTheDeploymentAPI

    remove_thininstaller_log


Thin Installer Does Not Pass Customer Token Argument To Register Central When No Product Selection Arguments Given
    [Tags]  THIN_INSTALLER  MCS_ROUTER
    [Teardown]  Teardown With Temporary Directory Clean
    Setup For Test With Warehouse Containing Base
    Start Local Cloud Server
    ${registerCentralArgFilePath} =  replace_register_central_with_script_that_echos_args

    # Force an installation
    Run Default Thininstaller  expected_return_code=0  override_location=https://localhost:1233  force_certs_dir=${SUPPORT_FILES}/sophos_certs

    Check Thininstaller Log Does Not Contain  ERROR
    ${registerCentralArgs} =  Get File  ${registerCentralArgFilePath}
    # Register central arguments should not contain --customer-token by default, as older installations may not
    # Be able to process the argument
    Should Be Equal As Strings  '${registerCentralArgs.strip()}'   'ThisIsARegToken https://localhost:4443/mcs'

    Remove Thininstaller Log

Thin Installer Registers Existing Installation With Product Args
    [Tags]  THIN_INSTALLER  MCS_ROUTER
    [Teardown]  Teardown With Temporary Directory Clean
    Setup For Test With Warehouse Containing Base
    Start Local Cloud Server

    # Force an installation
    Run Default Thininstaller  thininstaller_args=${PRODUCT_MDR_ARGUMENT}  expected_return_code=0  override_location=https://localhost:1233  force_certs_dir=${SUPPORT_FILES}/sophos_certs

    Check Thininstaller Log Does Not Contain  ERROR
    Check Cloud Server Log Contains  Register with ::ThisIsARegTokenFromTheDeploymentAPI
    Check Cloud Server Log Does Not Contain  Register with ::ThisIsARegToken\n

    Remove Thininstaller Log


Thin Installer Installs Product Successfully When A Large Number Of Users Are In One Group
    [Documentation]  Created for LINUXDAR-2249
    [Teardown]  Teardown With Large Group Creation
    Setup Group File With Large Group Creation
    Setup For Test With Warehouse Containing Product
    Start Local Cloud Server

    Should Not Exist    ${SOPHOS_INSTALL}

    Run Default Thininstaller  expected_return_code=0  override_location=https://localhost:1233  force_certs_dir=${SUPPORT_FILES}/sophos_certs

    Check Expected Base Processes Are Running

    Check Thininstaller Log Does Not Contain  ERROR: Installer returned 16
    Check Thininstaller Log Does Not Contain  Failed to create machine id. Error: Calling getGroupId on sophos-spl-group caused this error : Unknown group name


Thin Installer Installs Product Successfully With Product Arguments
    [Teardown]  Teardown With Temporary Directory Clean
    Setup For Test With Warehouse Containing Product
    Start Local Cloud Server

    Should Not Exist    ${SOPHOS_INSTALL}

    Run Default Thininstaller  thininstaller_args=${PRODUCT_MDR_ARGUMENT}  expected_return_code=0  override_location=https://localhost:1233  force_certs_dir=${SUPPORT_FILES}/sophos_certs

    Check MCS Router Running
    Check Correct MCS Password And ID For Local Cloud Saved
    Check Cloud Server Log Contains  products requested from deployment API: ['mdr']
    Check Cloud Server Log Contains  Register with ::ThisIsARegTokenFromTheDeploymentAPI

Thin Installer Repairs Broken Existing Installation
    [Teardown]  Teardown With Temporary Directory Clean
    # Also installs
    Setup For Test With Warehouse Containing Base
    # Break installation
    Should Exist  ${REGISTER_CENTRAL}
    Remove File  ${REGISTER_CENTRAL}
    Should Not Exist  ${REGISTER_CENTRAL}

    Run Default Thininstaller  expected_return_code=0  mcsurl=https://localhost:1233  override_location=https://localhost:1233  force_certs_dir=${SUPPORT_FILES}/sophos_certs

    Check Thininstaller Log Contains  Found existing installation here: /opt/sophos-spl
    Check Thininstaller Log Does Not Contain  ERROR
    Should Exist  ${REGISTER_CENTRAL}
    Remove Thininstaller Log
    Check Root Directory Permissions Are Not Changed
    ${mcsrouter_log} =  Mcs Router Log
    #there is a race condition where the mcsrouter can restart when
    #the thinstaller is overwriting the mcsrouter zip this causes an an expected critical exception
    Remove File  ${mcsrouter_log}
    Check Expected Base Processes Are Running

Thin Installer Force Works
    [Teardown]  Teardown With Temporary Directory Clean
    # Setup warehouse and install Base
    Setup For Test With Warehouse Containing Base
    Start Local Cloud Server

    # Remove install directory
    Should Exist  ${REGISTER_CENTRAL}
    Unmount All Comms Component Folders
    Remove Directory  /opt/sophos-spl  recursive=True
    Should Not Exist  ${REGISTER_CENTRAL}
    ${time} =  Get Current Date  exclude_millis=true
    ${message} =  Set Variable  Reloading.
    Run Default Thininstaller  thininstaller_args=${FORCE_ARGUMENT}  expected_return_code=0  override_location=https://localhost:1233  force_certs_dir=${SUPPORT_FILES}/sophos_certs
    Should Exist  ${REGISTER_CENTRAL}
    Check Thininstaller Log Contains  Installation complete, performing post install steps
    Should Have A Given Message In Journalctl Since Certain Time  ${message}  ${time}
    Should Have Set KillMode To Mixed
    Remove Thininstaller Log
    Check Root Directory Permissions Are Not Changed

Thin Installer Help Prints Correct Output
    Run Default Thininstaller With Args  0  --help
    Check Thininstaller Log Contains   Sophos Linux Protection Installer, help:
    Check Thininstaller Log Contains   Usage: [options]
    Check Thininstaller Log Contains   --help [-h]\t\t\tDisplay this summary
    Check Thininstaller Log Contains   --version [-v]\t\t\tDisplay version of installer
    Check Thininstaller Log Contains   --force\t\t\t\tForce re-install
    Check Thininstaller Log Contains   --group=<group>\t\t\tAdd this endpoint into the Sophos Central group specified
    Check Thininstaller Log Contains   --group=<path to sub group>\tAdd this endpoint into the Sophos Central nested\n\t\t\t\tgroup specified where path to the nested group\n\t\t\t\tis each group separated by a backslash\n\t\t\t\ti.e. --group=<top-level group>\\\\\<sub-group>\\\\\<bottom-level-group>\n\t\t\t\tor --group='<top-level group>\\\<sub-group>\\\<bottom-level-group>'

Thin Installer Help Prints Correct Output And Other Arguments Are Ignored
    Run Default Thininstaller With Args  0  --help  --other
    Check Thininstaller Log Contains   Sophos Linux Protection Installer, help:

Thin Installer Help Prints Correct Output And Preceding Arguments Are Ignored
    Run Default Thininstaller With Args  0  --other  --help
    Check Thininstaller Log Contains   Sophos Linux Protection Installer, help:

Thin Installer Help Takes Precedent Over Version Option And Prints Correct Output
    Run Default Thininstaller With Args  0  --version  --help
    Check Thininstaller Log Contains   Sophos Linux Protection Installer, help:
    Check Thininstaller Log Does Not Contain  Sophos Linux Protection Installer, version: 1.

Thin Installer Fails With Unexpected Argument
    Run Default Thininstaller With Args  23  --ThisIsUnexpected
    Check Thininstaller Log Contains  Error: Unexpected argument given: --ThisIsUnexpected --- aborting install. Please see '--help' output for list of valid arguments
    Remove Thininstaller Log

Thin Installer Fails With Invalid Group Names
    Run ThinInstaller With Bad Group Name And Check It Fails  te<st
    Run ThinInstaller With Bad Group Name And Check It Fails  te>st
    Run ThinInstaller With Bad Group Name And Check It Fails  te&st
    Run ThinInstaller With Bad Group Name And Check It Fails  te'st
    Run ThinInstaller With Bad Group Name And Check It Fails  te"st

    Run Default Thininstaller With Args  24  --group=
    Check Thininstaller Log Contains  Error: Group name not passed with '--group=' argument --- aborting install
    Remove Thininstaller Log

Thin Installer Fails With Oversized Group Name
    ${max_sized_group_name}=  Run Process  tr -dc A-Za-z0-9 </dev/urandom | head -c 1024  shell=True
    Run Default Thininstaller With Args  3  --group=${max_sized_group_name.stdout}

    ${over_sized_group_name}=  Run Process  tr -dc A-Za-z0-9 </dev/urandom | head -c 1025  shell=True
    Run Default Thininstaller With Args  25  --group=${over_sized_group_name.stdout}
    Check Thininstaller Log Contains  Error: Group name exceeds max size of: 1024 --- aborting install
    Remove Thininstaller Log

Thin Installer Fails With Duplicate Arguments
    Run Default Thininstaller With Args  26  --duplicate  --duplicate
    Check Thininstaller Log Contains  Error: Duplicate argument given: --duplicate --- aborting install
    Remove Thininstaller Log

Thin Installer Saves Group Names To Install Options
    Run Thin Installer And Check Argument Is Saved To Install Options File  --group=Group Name

Thin Installer With Invalid Product Arguments Fails
    Run Default Thininstaller With Args  27  --products=unsupported_product
    Check Thininstaller Log Contains   Error: Invalid product selected: unsupported_product --- aborting install

Thin Installer With Subset Of Product Args Args Fails
    Run Default Thininstaller With Args  27  --products=ntivir
    Check Thininstaller Log Contains   Error: Invalid product selected: ntivir --- aborting install

Thin Installer With No Product Arguments Fails
    Run Default Thininstaller With Args  27  --products=
    Check Thininstaller Log Contains   Error: Products not passed with '--products=' argument --- aborting install

Thin Installer With Only Commas As Product Arguments Fails
    Run Default Thininstaller With Args  27  --products=,
    Check Thininstaller Log Contains   Error: Products passed with trailing comma --- aborting install

Thin Installer With Spaces In Product Args Fails
    Run Default Thininstaller With Args  27  --products=mdr,\ ,antivirus
    Check Thininstaller Log Contains   Error: Product cannot be whitespace

Thin Installer With Duplicate Product Args Args Fails
    Run Default Thininstaller With Args  27  --products=mdr,antivirus,mdr
    Check Thininstaller Log Contains   Error: Duplicate product given: mdr --- aborting install

Thin Installer With Trailing Comma In Product Args Fails
    Run Default Thininstaller With Args  27  --products=antivirus,
    Check Thininstaller Log Contains   Error: Products passed with trailing comma --- aborting install

Thin Installer Passes MDR Products Arg To Base Installer
    Setup Warehouse
    Run Default Thinistaller With Product Args And Central  0  https://localhost:1233  ${SUPPORT_FILES}/sophos_certs  --products=mdr
    Thin Installer Calls Base Installer With Environment Variables For Product Argument  mdr

Thin Installer Passes AV Products Arg To Base Installer
    Setup Warehouse
    Run Default Thinistaller With Product Args And Central  0  https://localhost:1233  ${SUPPORT_FILES}/sophos_certs  --products=antivirus
    Thin Installer Calls Base Installer With Environment Variables For Product Argument  antivirus

Thin Installer Passes MDR And AV Products Arg To Base Installer
    Setup Warehouse
    Run Default Thinistaller With Product Args And Central  0  https://localhost:1233  ${SUPPORT_FILES}/sophos_certs  --products=mdr,antivirus
    Thin Installer Calls Base Installer With Environment Variables For Product Argument  mdr,antivirus

Thin Installer Passes None Products Arg To Base Installer
    Setup Warehouse
    Run Default Thinistaller With Product Args And Central  0  https://localhost:1233  ${SUPPORT_FILES}/sophos_certs  --products=none
    Thin Installer Calls Base Installer With Environment Variables For Product Argument  none

Thin Installer Passes No Products Args To Base Installer When None Are Given
    Setup Warehouse
    Run Default Thinistaller With Product Args And Central  0  https://localhost:1233  ${SUPPORT_FILES}/sophos_certs
    Thin Installer Calls Base Installer Without Environment Variables For Product Argument

Disable Auditd Argument Saved To Install Options
    Run Thin Installer And Check Argument Is Saved To Install Options File  --disable-auditd

Do Not Disable Auditd Argument Saved To Install Options
    Run Thin Installer And Check Argument Is Saved To Install Options File  --do-not-disable-auditd
