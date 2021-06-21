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
Library     OperatingSystem

Resource  ../installer/InstallerResources.robot
Resource  ../GeneralTeardownResource.robot
Resource  ThinInstallerResources.robot

Default Tags  THIN_INSTALLER

*** Keywords ***
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
    Create Fake Savscan In Tmp


Teardown
    General Test Teardown
    Stop Update Server
    Stop Proxy Servers
    Run Keyword If Test Failed   Dump Cloud Server Log
    Stop Local Cloud Server
    Teardown Reset Original Path
    Run Keyword If Test Failed    Dump Thininstaller Log
    Remove Thininstaller Log
    Cleanup Files
    Require Uninstalled
    Remove Environment Variable  SOPHOS_INSTALL
    Remove Directory  ${CUSTOM_DIR_BASE}  recursive=True
    Remove Directory  ${CUSTOM_TEMP_UNPACK_DIR}  recursive=True
    Remove Environment Variable  INSTALL_OPTIONS_FILE
    Remove Fake Savscan In Tmp
    Cleanup Temporary Folders

Remove SAV files
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
${CUSTOM_DIR_BASE} =  /CustomPath
${CUSTOM_TEMP_UNPACK_DIR} =  /tmp/temporary-unpack-dir

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
