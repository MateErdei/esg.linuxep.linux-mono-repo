*** Settings ***
Test Setup      Setup Thininstaller Test
Test Teardown   Thininstaller Test Teardown

Suite Setup      Setup sdds3 Update Tests
Suite Teardown   Cleanup sdds3 Update Tests

Library     ${COMMON_TEST_LIBS}/UpdateServer.py
Library     ${COMMON_TEST_LIBS}/ThinInstallerUtils.py
Library     ${COMMON_TEST_LIBS}/OSUtils.py
Library     ${COMMON_TEST_LIBS}/LogUtils.py
Library     ${COMMON_TEST_LIBS}/FullInstallerUtils.py
Library     ${COMMON_TEST_LIBS}/TemporaryDirectoryManager.py
Library     ${COMMON_TEST_LIBS}/MCSRouter.py
Library     ${COMMON_TEST_LIBS}/CentralUtils.py
Library     Process
Library     DateTime
Library     OperatingSystem

Resource    ${COMMON_TEST_ROBOT}/GeneralTeardownResource.robot
Resource    ${COMMON_TEST_ROBOT}/InstallerResources.robot
Resource    ${COMMON_TEST_ROBOT}/McsRouterResources.robot
Resource    ${COMMON_TEST_ROBOT}/SchedulerUpdateResources.robot
Resource    ${COMMON_TEST_ROBOT}/ThinInstallerResources.robot
Resource    ${COMMON_TEST_ROBOT}/UpgradeResources.robot

Force Tags  THIN_INSTALLER  thin_installer_main

*** Keywords ***
Teardown With Large Group Creation
    Teardown Group File With Large Group Creation
    Restore warehouse with fake sdds3 base


Setup TSL server 1_1
    Stop Local SDDS3 Server
    ${handle}=  Start Process  bash -x ${SUPPORT_FILES}/jenkins/runCommandFromPythonVenvIfSet.sh python3 ${COMMON_TEST_LIBS}/SDDS3server.py --launchdarkly ${VUT_LAUNCH_DARKLY} --sdds3 ${VUT_WAREHOUSE_ROOT} --protocol tls1_1  shell=true
    Set Suite Variable    ${GL_handle}    ${handle}
    Setup Thininstaller Test

Restore fake SDDS3 server
    Thininstaller Test Teardown
    Stop Local SDDS3 Server
    ${handle}=  Start Local SDDS3 server with fake files
    Set Suite Variable    ${GL_handle}    ${handle}

Setup warehouse With sdds3 base
    Clean up fake warehouse
    Generate Warehouse From Local Base Input

Restore warehouse with fake sdds3 base
    Thininstaller Test Teardown
    Clean up fake warehouse
    Generate Fake sdds3 warehouse

Restore warehouse with fake sdds3 base with force cleanup
    Restore warehouse with fake sdds3 base
    Run Keyword If Test Failed    Run Full Installer
    Run Keyword If Test Failed    Require Uninstalled

Cert Test Teardown
    Thininstaller Test Teardown
    Install_System_Ca_Cert  ${COMMON_TEST_UTILS}/server_certs/server-root.crt

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

Run Thin Installer And Check Argument Is Saved To Install Options File
    [Arguments]  ${argument}
    ${install_location}=  get_default_install_script_path
    ${thin_installer_cmd}=  Create List    ${install_location}   ${argument}
    Remove Directory  ${CUSTOM_TEMP_UNPACK_DIR}  recursive=True
    run_thininstaller_with_localhost_sdds_urls  ${thin_installer_cmd}   expected_return_code=0  cleanup=False  temp_dir_to_unpack_to=${CUSTOM_TEMP_UNPACK_DIR}
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
Thin Installer can download test file from warehouse and execute it
    [Tags]  SMOKE

    Run Default Thininstaller  0    cleanup=False    temp_dir_to_unpack_to=${CUSTOM_TEMP_UNPACK_DIR}
    ${compatibilityCheckResults} =  Get File    ${CUSTOM_THININSTALLER_REPORT_LOC}
    log  ${compatibilityCheckResults}
    Should Not Contain    ${compatibilityCheckResults}    false
    Should Contain    ${compatibilityCheckResults}    summary = successfully installed product
    Check Thininstaller Log Contains    Successfully installed product

Thin Installer Installs Product Successfully When A Large Number Of Users Are In One Group
    [Documentation]  Created for LINUXDAR-2249
    [Teardown]  Teardown With Large Group Creation
    Setup Group File With Large Group Creation
    Setup warehouse With sdds3 base

    Should Not Exist    ${SOPHOS_INSTALL}

    Run Default Thininstaller  expected_return_code=0

    Check Expected Base Processes Are Running

    Check Thininstaller Log Does Not Contain  ERROR: Installer returned 16
    Check Thininstaller Log Does Not Contain  Failed to create machine id. Error: Calling getGroupId on sophos-spl-group caused this error : Unknown group name



Thin Installer fails to download test file from warehouse if certificate is not installed
    [Teardown]  Cert Test Teardown
    Cleanup_System_Ca_Certs
    Run Default Thininstaller    33
    Check Thininstaller Log Contains    SPL installation will fail as a connection to the SUS server could not be established

Thin Installer Cannot Connect to Central timeout
    [Setup]  Setup Thininstaller Test Without Local Cloud Server
    Start Local Cloud Server  --timeout
    Run Default Thininstaller    ${33}    https://localhost:4443

Thin Installer Will Not Connect to Central If Connection Has TLS below TLSv1_2
    [Tags]  SMOKE
    [Setup]  Setup Thininstaller Test Without Local Cloud Server
    Start Local Cloud Server   --tls   tlsv1_1    --initial-alc-policy    ${SUPPORT_FILES}/CentralXml/ALC_policy/ALC_policy_base_only.xml
    Cloud Server Log Should Contain      SSL version: _SSLMethod.PROTOCOL_TLSv1_1

    TRY
        Run Default Thininstaller    ${3}    https://localhost:4443
        Check Thininstaller Log Contains    Failed to connect to Sophos Central at https://localhost:4443 (cURL error is [SSL connect error]). Please check your firewall rules
    EXCEPT    Thin Installer exited with exit code: 33 but was expecting: 3
        Run Default Thininstaller    ${33}    https://localhost:4443
        Check Thininstaller Log Contains    SPL installation will fail as a connection to Sophos Central could not be established
    END

Thin Installer SUL Library Will Not Connect to Warehouse If Connection Has TLS below TLSv1_2
    [Setup]  Setup TSL server 1_1
    [Teardown]  Restore fake SDDS3 server
    Start Local Cloud Server
    Run Default Thininstaller    33
    # Different curl versions have different TLS version error messages:
    check_thininstaller_log_contains_pattern    (unsupported protocol|SSL_ERROR_UNSUPPORTED_VERSION|SSL_ERROR_SYSCALL|TLS alert)

Thin Installer With Space In Name Works
    Run Default Thininstaller With Different Name    SophosSetup (1).sh    0
    Check Thininstaller Log Contains    Successfully installed product

Thin Installer Cannot Connect to Central
    [Tags]  SMOKE
    [Setup]  Setup Thininstaller Test Without Local Cloud Server
    Run Default Thininstaller    ${33}    https://localhost:4443
    Check Thininstaller Log Contains   SPL installation will fail as a connection to Sophos Central could not be established

Thin Installer handles broken jwt
    [Tags]  SMOKE
    [Setup]  Setup Thininstaller Test Without Local Cloud Server
    Start Local Cloud Server  --force-break-jwt
    Run Default Thininstaller    52    https://localhost:4443/mcs
    Check Thininstaller Log Contains  Failed to get JWT from Sophos Central

Thin Installer Registers Existing Installation
    [Tags]  MCS_ROUTER
    [Teardown]  Teardown With Temporary Directory Clean
    Setup base Install
    Start Local Cloud Server

    # Force an installation
    Run Default Thininstaller  expected_return_code=0

    Check Thininstaller Log Does Not Contain  ERROR
    Check Cloud Server Log Contains  Register with ::ThisIsARegToken\n
    Check Cloud Server Log Does Not Contain  Register with ::ThisIsARegTokenFromTheDeploymentAPI

Thin Installer Doesnt Remove Existing Files In Installation Directory
    [Tags]  SMOKE
    ${testfile} =    Set Variable    testfile
    ${content} =    Set Variable    content

    Create Directory    ${SOPHOS_INSTALL}
    Create File    ${SOPHOS_INSTALL}/${testfile}    ${content}

    Start Local Cloud Server
    Run Default Thininstaller    expected_return_code=19
    Check Thininstaller Log Contains  The intended destination for Sophos Protection for Linux: ${SOPHOS_INSTALL} already exists. Please either move or delete this directory.

    Directory Should Exist    ${SOPHOS_INSTALL}
    ${filecontents}  Get File  ${SOPHOS_INSTALL}/${testfile}
    Should Be Equal As Strings    ${filecontents}    ${content}

Thin Installer Does Not Pass Customer Token Argument To Register Central When No Product Selection Arguments Given
    [Tags]  MCS_ROUTER
    [Teardown]  Teardown With Temporary Directory Clean
    Setup base Install
    Start Local Cloud Server
    ${registerCentralArgFilePath} =  replace_register_central_with_script_that_echos_args

    # Force an installation
    Run Default Thininstaller  expected_return_code=0

    Check Thininstaller Log Does Not Contain  ERROR
    ${registerCentralArgs} =  Get File  ${registerCentralArgFilePath}
    # Register central arguments should not contain --customer-token by default, as older installations may not
    # Be able to process the argument
    Should Be Equal As Strings  '${registerCentralArgs.strip()}'   'ThisIsARegToken https://localhost:4443/mcs'


Thin Installer Registers Existing Installation With Product Args
    [Tags]  MCS_ROUTER
    [Teardown]  Teardown With Temporary Directory Clean
    Setup base Install
    Start Local Cloud Server

    # Force an installation
    Run Default Thininstaller  thininstaller_args=${PRODUCT_MDR_ARGUMENT}  expected_return_code=0

    Check Thininstaller Log Does Not Contain  ERROR
    Check Cloud Server Log Contains  Register with ::ThisIsARegTokenFromTheDeploymentAPI
    Check Cloud Server Log Does Not Contain  Register with ::ThisIsARegToken\n



Thin Installer Installs Product Successfully With Product Arguments
    [Teardown]  Restore warehouse with fake sdds3 base
    Setup warehouse With sdds3 base

    Should Not Exist    ${SOPHOS_INSTALL}

    Run Default Thininstaller  thininstaller_args=${PRODUCT_MDR_ARGUMENT}  expected_return_code=0

    Check MCS Router Running
    Check Correct MCS Password And ID For Local Cloud Saved
    Check Cloud Server Log Contains  products requested from deployment API: ['mdr']
    Check Cloud Server Log Contains  Register with ::ThisIsARegTokenFromTheDeploymentAPI

Thin Installer Repairs Broken Existing Installation
    [Teardown]  Restore warehouse with fake sdds3 base
    Setup warehouse With sdds3 base
    Setup base Install
    # Break installation
    Should Exist  ${REGISTER_CENTRAL}
    Remove File  ${REGISTER_CENTRAL}
    Should Not Exist  ${REGISTER_CENTRAL}

    Run Default Thininstaller  expected_return_code=0

    Check Thininstaller Log Contains  Found existing installation here: /opt/sophos-spl

    Check Thininstaller Log Does Not Contain  ERROR
    Should Exist  ${REGISTER_CENTRAL}

    Check Root Directory Permissions Are Not Changed
    #there is a race condition where the mcsrouter can restart when
    #the thinstaller is overwriting the mcsrouter zip this causes an an expected critical exception
    Remove File  ${SOPHOS_INSTALL}/logs/base/sophosspl/mcsrouter.log
    Check Expected Base Processes Are Running


Thin Installer Force Works
    [Teardown]  Restore warehouse with fake sdds3 base with force cleanup
    Setup warehouse With sdds3 base
    Run Full Installer

    # Remove install directory
    Should Exist  ${REGISTER_CENTRAL}
    Remove Directory  /opt/sophos-spl  recursive=True
    Should Not Exist  ${REGISTER_CENTRAL}
    ${time} =  Get Current Date  exclude_millis=true
    ${time} =  Subtract Time From Date  ${time}  1s  exclude_millis=true
    Run Default Thininstaller  thininstaller_args=${FORCE_ARGUMENT}  expected_return_code=0
    Should Exist  ${REGISTER_CENTRAL}
    Check Thininstaller Log Contains  Successfully installed product
    Should Have A Stopped Sophos Message In Journalctl Since Certain Time  ${time}
    Should Have Set KillMode To Mixed
    Check Root Directory Permissions Are Not Changed

Thin Installer Saves Group Names To Install Options
    Run Thin Installer And Check Argument Is Saved To Install Options File  --group=Group Name

Thin Installer Saves Requested GID and UIDs To Install Options
    Run Thin Installer And Check Argument Is Saved To Install Options File    --user-ids-to-configure=sophos-spl-local:100,sophos-spl-updatescheduler:101,sophos-spl-user:102,sophos-spl-av:103,sophos-spl-threat-detector:104 --group-ids-to-configure=sophos-spl-group:105,sophos-spl-ipc:106

Thin Installer Succeeds When System Has Glibc Greater Than Build Machine
    ${HighGlibcVersion} =  Set Variable  999.999
    ${PATH} =  Create Fake Ldd Executable With Version As Argument And Add It To Path  ${HighGlibcVersion}
    Run Thininstaller With Non Standard Path  0  ${PATH}
    Check Thininstaller Log Contains    Successfully installed product

Thin Installer Succeeds When System Has Glibc Same As Build Machine
    ${buildGlibcVersion} =  Get Glibc Version From Thin Installer
    ${PATH} =  Create Fake Ldd Executable With Version As Argument And Add It To Path  ${buildGlibcVersion}
    Run Thininstaller With Non Standard Path  0  ${PATH}
    Check Thininstaller Log Contains    Successfully installed product

Thin Installer Passes XDR Product Arg To Base Installer
    Run Default Thinistaller With Product Args And Central  0    --products=xdr
    Check Thininstaller Log Contains  Carrying out Preregistration for selected products: xdr


Thin Installer Passes MDR Products Arg To Base Installer
    Run Default Thinistaller With Product Args And Central  0    --products=mdr
    Check Thininstaller Log Contains  Carrying out Preregistration for selected products: mdr

Thin Installer Passes MDR and XDR Products Arg To Base Installer
    Run Default Thinistaller With Product Args And Central  0    --products=mdr,xdr
    Check Thininstaller Log Contains  Carrying out Preregistration for selected products: mdr,xdr

Thin Installer Passes AV Products Arg To Base Installer
    Run Default Thinistaller With Product Args And Central  0    --products=antivirus
    Check Thininstaller Log Contains  Carrying out Preregistration for selected products: antivirus

Thin Installer Passes AV and XDR Products Arg To Base Installer
    Run Default Thinistaller With Product Args And Central  0    --products=antivirus,xdr
    Check Thininstaller Log Contains  Carrying out Preregistration for selected products: antivirus,xdr

Thin Installer Passes MDR And AV Products Arg To Base Installer
    Run Default Thinistaller With Product Args And Central  0    --products=mdr,antivirus
    Check Thininstaller Log Contains  Carrying out Preregistration for selected products: mdr,antivirus

Thin Installer Passes MDR, AV, and XDR Products Arg To Base Installer
    Run Default Thinistaller With Product Args And Central  0    --products=mdr,antivirus,xdr
    Check Thininstaller Log Contains  Carrying out Preregistration for selected products: mdr,antivirus,xdr

Thin Installer Passes None Products Arg To Base Installer
    Run Default Thinistaller With Product Args And Central  0    --products=none
    Check Thininstaller Log Contains  Carrying out Preregistration for selected products: none

Thin Installer Passes No Products Args To Base Installer When None Are Given
    Run Default Thinistaller With Product Args And Central  0
    Check Thininstaller Log Does Not Contain  Carrying out Preregistration for selected products:

Disable Auditd Argument Saved To Install Options
    Run Thin Installer And Check Argument Is Saved To Install Options File  --disable-auditd

Do Not Disable Auditd Argument Saved To Install Options
    Run Thin Installer And Check Argument Is Saved To Install Options File  --do-not-disable-auditd

Thin Installer Passes MCS Config To Base Installer Via Args And Only One Registration Call Made
    Start Message Relay

    Create Default Credentials File  message_relays=dummyhost1:10000,1,2;localhost:20000,2,4
    Build Default Creds Thininstaller From Sections

    Run Default Thininstaller    0    cleanup=${False}

    Check Thininstaller Log Contains    Successfully installed product

    ${root_config_contents} =  Get File  ${SOPHOS_INSTALL}/base/etc/mcs.config
    ${policy_config_contents} =  Get File  ${MCS_CONFIG}
    Should Contain  ${policy_config_contents}  MCSID=ThisIsAnMCSID+1001
    Should Contain  ${policy_config_contents}  MCSPassword=ThisIsThePassword
    Should Contain  ${root_config_contents}  MCSToken=ThisIsARegToken
    Should Contain  ${root_config_contents}  CAFILE=${COMMON_TEST_UTILS}/server_certs/server-root.crt
    Should Contain  ${root_config_contents}  MCSURL=https://localhost:4443/mcs
    Should Contain  ${root_config_contents}  customerToken=ThisIsACustomerToken
    Should Contain  ${root_config_contents}  mcsConnectedProxy=localhost:20000

    # only one registration call in cloud server logs
    Check Cloud Server Log Contains  POST - /mcs/register (Sophos MCS Client)    occurs=1


Thin Installer Passes Proxy Used By Registration To Suldownloader During Install
    Start Message Relay
    Create Default Credentials File  message_relays=dummyhost1:10000,1,2;localhost:20000,2,4
    Build Default Creds Thininstaller From Sections
    Run Default Thininstaller    0    cleanup=${False}
    Check Thininstaller Log Contains    Successfully installed product
    Check Suldownloader Log Contains  Trying SUS request (https://localhost:8080) with proxy: localhost:20000
    Check Suldownloader Log Contains  SUS Request was successful


Thin Installer Passes Basic Auth Proxy Used By Registration To Suldownloader During Install
    [Setup]    Setup Thininstaller Test
    Start Proxy Server With Basic Auth    8192    username    password
    Create Default Credentials File
    Build Default Creds Thininstaller From Sections
    Run Default Thininstaller   expected_return_code=0  proxy=http://username:password@localhost:8192
    Check Thininstaller Log Contains    Successfully installed product
    Check Suldownloader Log Contains  Trying SUS request (https://localhost:8080) with proxy: http://username:password@localhost:8192
    Check Suldownloader Log Contains  SUS Request was successful

