*** Settings ***
Test Setup      Setup Thininstaller Test
Test Teardown   Teardown

Library     ${libs_directory}/WarehouseGenerator.py
Library     ${libs_directory}/UpdateServer.py
Library     ${libs_directory}/ThinInstallerUtils.py
Library     ${libs_directory}/OSUtils.py
Library     ${libs_directory}/LogUtils.py
Library     ${libs_directory}/FullInstallerUtils.py
Library     ${libs_directory}/TemporaryDirectoryManager.py
Library     Process
Library     OperatingSystem

Resource  ../installer/InstallerResources.robot
Resource  ../GeneralTeardownResource.robot

Default Tags  THIN_INSTALLER

*** Keywords ***
Setup Warehouse
    Clear Warehouse Config
    Generate Install File In Directory     ./tmp/TestInstallFiles/${BASE_RIGID_NAME}
    Add Component Warehouse Config   ${BASE_RIGID_NAME}   ./tmp/TestInstallFiles/    ./tmp/temp_warehouse/
    Generate Warehouse
    Start Update Server    1233    ./tmp/temp_warehouse/customer_files/
    Start Update Server    1234    ./tmp/temp_warehouse/warehouses/sophosmain/
    Can Curl Url    https://localhost:1234/catalogue/sdds.live.xml
    Can Curl Url    https://localhost:1233

Setup Thininstaller Test
    Require Uninstalled
    Uninstall SAV
    Set Environment Variable  CORRUPTINSTALL  no
    Get Thininstaller
    Create Default Credentials File
    Build Default Creds Thininstaller From Sections
    Create Fake Savscan In Tmp

Teardown
    General Test Teardown
    Stop Update Server
    Stop Proxy Servers
    Teardown Reset Original Path
    Uninstall SAV
    Run Keyword If Test Failed    Dump Thininstaller Log
    Cleanup Files
    Require Uninstalled
    Remove Environment Variable  SOPHOS_INSTALL
    Remove Directory  ${CUSTOM_DIR_BASE}  recursive=$true
    Remove Fake Savscan In Tmp
    Run Keyword And Ignore Error    Delete Fake Sweep Symlink    /usr/bin
    Run Keyword And Ignore Error    Delete Fake Sweep Symlink    /usr/local/bin/
    Run Keyword And Ignore Error    Delete Fake Sweep Symlink    /bin
    Cleanup Temporary Folders

Cert Test Teardown
    Teardown
    Install System Ca Cert  SupportFiles/https/ca/root-ca.crt

Run ThinInstaller Instdir And Check It Fails
    [Arguments]    ${path}
    Log  ${path}
    Run Default Thininstaller With Args  19  --instdir=${path}
    Check Thininstaller Log Contains  The --instdir path provided contains invalid characters. Only alphanumeric and '/' '-' '_' '.' characters are accepted
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

*** Variables ***
${CUSTOM_DIR_BASE} =  /CustomPath

*** Test Case ***
Thin Installer can download test file from warehouse and execute it
    [Tags]  SMOKE  THIN_INSTALLER
    Setup Warehouse
    Run Default Thininstaller    0    https://localhost:1233
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
    Check Thininstaller Log Contains    Not enough space in / to install Sophos Managed Threat Response for Linux. You can install elsewhere by re-running this installer with the --instdir argument

Thin Installer fails to install to the tmp folder
    ${Install_Path}=  Set Variable  /tmp
    Run Default Thininstaller With Args  19  --instdir=${Install_Path}
    Check Thininstaller Log Contains  The --instdir path provided is in the non-persistent /tmp folder. Please choose a location that is persistent

Thin Installer fails to install to a folder within the temp folder
    ${Install_Path}=  Set Variable  /tmp/dir2/dir2
    Run Default Thininstaller With Args  19  --instdir=${Install_Path}
    Check Thininstaller Log Contains  The --instdir path provided is in the non-persistent /tmp folder. Please choose a location that is persistent

#The sophos-spl-user requires the right to execute files in the install directory.
#It can't do this unless execute all permissions are set on all directories that contain the install.
Thin Installer fails to install to a custom location with short path with the wrong permissions
    [Tags]  CUSTOM_LOCATION  THIN_INSTALLER
    ${Install_Path}=  Set Variable  ${CUSTOM_DIR_BASE}
    Create Directory  ${Install_Path}
    Run Process  chmod  700  ${Install_Path}
    Run Default Thininstaller With Args  19  --instdir=${Install_Path}
    Check Thininstaller Log Contains  because ${CUSTOM_DIR_BASE} does not have correct

Thin Installer fails to install to a custom location which already exists but is not a previous install location
    [Tags]  CUSTOM_LOCATION  THIN_INSTALLER
    ${Install_Path}=  Set Variable  ${CUSTOM_DIR_BASE}/tmp/dir1/dir2/dir3
    Create Directory  ${Install_Path}/sophos-spl
    Run Default Thininstaller With Args  19  --instdir=${Install_Path}
    Check Thininstaller Log Contains  already exists. Please either delete this folder or choose another location

Thin Installer fails to install to a custom location with long path with the wrong permissions in end folder
    [Tags]  CUSTOM_LOCATION  THIN_INSTALLER
    ${Install_Path}=  Set Variable  ${CUSTOM_DIR_BASE}/tmp/dir1/dir2/dir3
    Create Directory  ${Install_Path}
    Run Process  chmod  700  ${Install_Path}
    Run Default Thininstaller With Args  19  --instdir=${Install_Path}
    Check Thininstaller Log Contains  because ${Install_Path} does not have correct

Thin Installer fails to install to a custom location with long path with the wrong permissions in middle-ish folder
    [Tags]  CUSTOM_LOCATION  THIN_INSTALLER
    ${Install_Path}=  Set Variable  ${CUSTOM_DIR_BASE}/tmp/dir1/dir2/dir3/dir4/dir5/dir6
    ${Bad_Perm_Dir}=  Set Variable  ${CUSTOM_DIR_BASE}/tmp/dir1/dir2
    Create Directory  ${Install_Path}
    Run Process  chmod  700  ${Bad_Perm_Dir}
    Run Default Thininstaller With Args  19  --instdir=${Install_Path}
    Check Thininstaller Log Contains  because ${Bad_Perm_Dir} does not have correct

Thin Installer fails to install to a custom location with a path 51 Or Greater Characters Long
    [Tags]  CUSTOM_LOCATION  THIN_INSTALLER
    ${Install_Path}=  Set Variable  ${CUSTOM_DIR_BASE}/345678920234567893023456789402345678950
    Run Default Thininstaller With Args  19  --instdir=${Install_Path}
    Check Thininstaller Log Contains  The --instdir path provided is too long and needs to be 50 characters or less. ${Install_Path} is 51
    ${Install_Path}=  Set Variable  ${CUSTOM_DIR_BASE}/92023456789302345678945678941234567895023456789602
    Run Default Thininstaller With Args  19  --instdir=${Install_Path}
    Check Thininstaller Log Contains  The --instdir path provided is too long and needs to be 50 characters or less. ${Install_Path} is 62

Thin Installer fails to install alongside SAV
    [Tags]  SAV  THIN_INSTALLER
    Get And Install Sav
    Run Default Thininstaller    8
    Check Thininstaller Log Contains     This product cannot be run alongside Sophos Anti-Virus

Thin Installer fails to install alongside SAV with non-standard PATH
    [Tags]  SAV  THIN_INSTALLER
    ${path} =  Get System Path
    Get And Install Sav
    ## Change symlinks
    create non standard sweep symlink   /tmp
    run thininstaller with non standard path    8  /tmp:/bin:${path}
    Check Thininstaller Log Contains     This product cannot be run alongside Sophos Anti-Virus

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

Thin Installer Does Not Install When Existing Install Different To Intsdir Argument
    [Tags]  CUSTOM_LOCATION  THIN_INSTALLER
    # Install to default location
    Require Fresh Install
    # Install to custom location, check we get error code 7
    Run Default Thininstaller With Args  7  --instdir=/customdir
    Check Thininstaller Log Contains  Found existing installation here: /opt/sophos-spl
    Check Thininstaller Log Contains  Please uninstall Sophos Managed Threat Response for Linux before using this installer. You can run /opt/sophos-spl/bin/uninstall.sh
    remove_thininstaller_log


Thin Installer Tells Us It Is Governed By A License
    Run Default Thininstaller    3
    Check Thininstaller Log Contains    This software is governed by the terms and conditions of a licence agreement with Sophos Limited

Thin Installer Does Not Tell Us About Which Sweep
    Run Default Thininstaller    3
    Check Thininstaller Log Does Not Contain    which: no sweep in

Thin Installer Detects Sweep And Cancels Installation
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

Thin Installer Has Working Version Option
    Run Default Thininstaller With Args   0     --version
    Check Thininstaller Log Contains    Sophos Managed Threat Response for Linux Installer, version: 1.0.3

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
    Run Thininstaller With Non Standard Path  0  ${PATH}  https://localhost:1233
    Check Thininstaller Log Contains    INSTALLER EXECUTED

Thin Installer Succeeds When System Has Glibc Same As Build Machine
    Setup Warehouse
    ${buildGlibcVersion} =  Get Glibc Version From Thin Installer
    ${PATH} =  Create Fake Ldd Executable With Version As Argument And Add It To Path  ${buildGlibcVersion}
    Run Thininstaller With Non Standard Path  0  ${PATH}  https://localhost:1233
    Check Thininstaller Log Contains    INSTALLER EXECUTED

