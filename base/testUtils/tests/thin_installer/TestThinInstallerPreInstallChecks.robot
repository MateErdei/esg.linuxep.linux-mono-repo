*** Settings ***
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

Test Setup      Setup Thininstaller Test Without Local Cloud Server
Test Teardown   Thininstaller Test Teardown
*** Keywords ***

Remove SAV files
    Remove Fake Savscan In Tmp
    Run Keyword And Ignore Error    Delete Fake Sweep Symlink    /usr/bin
    Run Keyword And Ignore Error    Delete Fake Sweep Symlink    /usr/local/bin/
    Run Keyword And Ignore Error    Delete Fake Sweep Symlink    /bin
    Run Keyword And Ignore Error    Delete Fake Sweep Symlink    /tmp

SAV Teardown
    Remove SAV files
    Thininstaller Test Teardown

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

*** Variables ***
${PROXY_LOG}  ./tmp/proxy_server.log
${MCS_CONFIG_FILE}  ${SOPHOS_INSTALL}/base/etc/mcs.config
${CUSTOM_TEMP_UNPACK_DIR} =  /tmp/temporary-unpack-dir
@{FORCE_ARGUMENT} =  --force
@{UNINSTALL_SAV_ARGUMENT} =  --uninstall-sav

${BaseVUTPolicy}                    ${SUPPORT_FILES}/CentralXml/ALC_policy_direct_just_base.xml
*** Test Case ***
Thin Installer fails to install on system without enough memory
    Run Default Thininstaller With Fake Memory Amount    234
    Check Thininstaller Log Contains    This machine does not meet product requirements (total RAM: 234kB). The product requires at least 930000kB of RAM

Thin Installer fails to install on system without enough storage
    Run Default Thininstaller With Fake Small Disk
    Check Thininstaller Log Contains    Not enough space in / to install Sophos Protection for Linux. You need at least 2048mB to install Sophos Protection for Linux

Thin Installer Tells Us It Is Governed By A License
    Run Default Thininstaller    3
    Check Thininstaller Log Contains    This software is governed by the terms and conditions of a licence agreement with Sophos Limited

Thin Installer Does Not Tell Us About Which Sweep
    Run Default Thininstaller    3
    Check Thininstaller Log Does Not Contain    which: no sweep in

Thin Installer Detects Sweep And Cancels Installation If SAV can not be uninstalled
    ## Fake SAV installation has no uninstall script so can't be uninstalled
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

Thin Installer Detects Sweep And uninstalls SAV
    [Tags]  SAV  THIN_INSTALLER
    [Teardown]  SAV Teardown
    Create Fake Savscan In Tmp
    Create Fake SAV Uninstaller in Tmp
    Create Fake Sweep Symlink    /usr/bin
    ## Installer fails trying to talk to fakeCloud
    Run Default Thininstaller    3  thininstaller_args=${UNINSTALL_SAV_ARGUMENT}
    Check Thininstaller Log Contains    Found an existing installation of Sophos Anti-Virus in /tmp/i/am/fake/.
    Check Thininstaller Log Contains    This product cannot be run alongside Sophos Anti-Virus.
    Check Thininstaller Log Contains    Sophos Anti-Virus will be uninstalled:
    Check Thininstaller Log Contains    Sophos Anti-Virus has been uninstalled.
    ## Check we've run the uninstaller
    File Should Not Exist  /usr/bin/sweep
    Directory Should Not Exist  /tmp/i/am/fake

Thin Installer Has Working Version Option
    Run Default Thininstaller With Args   0     --version
    Check Thininstaller Log Contains    Sophos Protection for Linux Installer, version: 1.
    Directory Should Not Exist   ${SOPHOS_INSTALL}

Thin Installer Has Working Version Option With Other Arguments
    Run Default Thininstaller With Args   0     --version  --other
    Check Thininstaller Log Contains    Sophos Protection for Linux Installer, version: 1.
    Directory Should Not Exist   ${SOPHOS_INSTALL}

Thin Installer Has Working Version Option Where Preceding Arguments Are Ignored
    Run Default Thininstaller With Args   0     --other  --version
    Check Thininstaller Log Contains    Sophos Protection for Linux Installer, version: 1.
    Directory Should Not Exist   ${SOPHOS_INSTALL}

Thin Installer Has Working short Version Option
    Run Default Thininstaller With Args   0     -v
    Check Thininstaller Log Contains    Sophos Protection for Linux Installer, version: 1.
    Directory Should Not Exist   ${SOPHOS_INSTALL}

Thin Installer Has Working short Version Option With Other Arguments
    Run Default Thininstaller With Args   0     -v  --other
    Check Thininstaller Log Contains    Sophos Protection for Linux Installer, version: 1.
    Directory Should Not Exist   ${SOPHOS_INSTALL}

Thin Installer Has Working short Version Option Where Preceding Arguments Are Ignored
    Run Default Thininstaller With Args   0     --other  -v
    Check Thininstaller Log Contains    Sophos Protection for Linux Installer, version: 1.
    Directory Should Not Exist   ${SOPHOS_INSTALL}

Check Installer Does Not Contain Todos Or Fixmes
    [Tags]   THIN_INSTALLER
    @{stringsToCheck}=  Create List   FIXME  TODO
    ${Result}=  Check If Unwanted Strings Are In ThinInstaller   ${stringsToCheck}
    Should Be Equal  ${Result}  ${False}   Error: Installer contains at least one of the following un-wanted values: ${stringsToCheck} which are not allowed in release

Thin Installer Fails When System Has Glibc Less Than Build Machine
    [Tags]  SMOKE  THIN_INSTALLER
    ${LowGlibcVersion} =  Set Variable  1.0
    ${buildGlibcVersion} =  Get Glibc Version From Thin Installer
    ${PATH} =  Create Fake Ldd Executable With Version As Argument And Add It To Path  ${LowGlibcVersion}
    Run Thininstaller With Non Standard Path  21  ${PATH}  https://localhost:1233
    Check Thininstaller Log Contains    Failed to install on unsupported system. Detected GLIBC version 1.0 < required ${buildGlibcVersion}

Thin Installer Fails When No Path In Systemd File
    # Install to default location and break it
    Create Initial Installation

    ${serviceDir} =  Get Service Folder

    Log File  ${serviceDir}/sophos-spl.service

    ${result}=  Run Process  sed  -i  s/SOPHOS_INSTALL.*/SOPHbroken/  ${serviceDir}/sophos-spl.service
    Should Be Equal As Integers    ${result.rc}    0

    Log File  ${serviceDir}/sophos-spl.service

    ${result}=  Run Process  systemctl  daemon-reload

    Log File  ${serviceDir}/sophos-spl.service

    Build Default Creds Thininstaller From Sections
    Run Default Thininstaller  20

    Check Thininstaller Log Contains  An existing installation of Sophos Protection for Linux was found but could not find the installed path.
    Check Thininstaller Log Does Not Contain  ERROR
    Check Root Directory Permissions Are Not Changed

Thin Installer Help Prints Correct Output
    Run Default Thininstaller With Args  0  --help
    Check Thininstaller Log Contains   Sophos Protection for Linux Installer, help:
    Check Thininstaller Log Contains   Usage: [options]
    Check Thininstaller Log Contains   --help [-h]\t\t\tDisplay this summary
    Check Thininstaller Log Contains   --version [-v]\t\t\tDisplay version of installer
    Check Thininstaller Log Contains   --force\t\t\t\tForce re-install
    Check Thininstaller Log Contains   --group=<group>\t\t\tAdd this endpoint into the Sophos Central group specified
    Check Thininstaller Log Contains   --group=<path to sub group>\tAdd this endpoint into the Sophos Central nested\n\t\t\t\tgroup specified where path to the nested group\n\t\t\t\tis each group separated by a backslash\n\t\t\t\ti.e. --group=<top-level group>\\\\\<sub-group>\\\\\<bottom-level-group>\n\t\t\t\tor --group='<top-level group>\\\<sub-group>\\\<bottom-level-group>'
    Check Thininstaller Log Contains   --uninstall-sav\t\tUninstall Sophos Anti-Virus if installed
    Directory Should Not Exist   ${SOPHOS_INSTALL}

Thin Installer Help Prints Correct Output And Other Arguments Are Ignored
    Run Default Thininstaller With Args  0  --help  --other
    Check Thininstaller Log Contains   Sophos Protection for Linux Installer, help:
    Directory Should Not Exist   ${SOPHOS_INSTALL}

Thin Installer Help Prints Correct Output And Preceding Arguments Are Ignored
    Run Default Thininstaller With Args  0  --other  --help
    Check Thininstaller Log Contains   Sophos Protection for Linux Installer, help:
    Directory Should Not Exist   ${SOPHOS_INSTALL}

Thin Installer Help Takes Precedent Over Version Option And Prints Correct Output
    Run Default Thininstaller With Args  0  --version  --help
    Check Thininstaller Log Contains   Sophos Protection for Linux Installer, help:
    Check Thininstaller Log Does Not Contain  Sophos Protection for Linux Installer, version: 1.
    Directory Should Not Exist   ${SOPHOS_INSTALL}

Thin Installer Short Help Option Prints Correct Output
    Run Default Thininstaller With Args  0  -h
    Check Thininstaller Log Contains   Sophos Protection for Linux Installer, help:
    Directory Should Not Exist   ${SOPHOS_INSTALL}

Thin Installer Short Help Option Prints Correct Output And Other Arguments Are Ignored
    Run Default Thininstaller With Args  0  -h  --other
    Check Thininstaller Log Contains   Sophos Protection for Linux Installer, help:
    Directory Should Not Exist   ${SOPHOS_INSTALL}

Thin Installer Short Help Option Prints Correct Output And Preceding Arguments Are Ignored
    Run Default Thininstaller With Args  0  --other  -h
    Check Thininstaller Log Contains   Sophos Protection for Linux Installer, help:
    Directory Should Not Exist   ${SOPHOS_INSTALL}

Thin Installer Short Help Option Takes Precedent Over Version Option And Prints Correct Output
    Run Default Thininstaller With Args  0  --version  -h
    Check Thininstaller Log Contains   Sophos Protection for Linux Installer, help:
    Check Thininstaller Log Does Not Contain  Sophos Protection for Linux Installer, version: 1.
    Directory Should Not Exist   ${SOPHOS_INSTALL}

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

Thin Installer With Trailing Comma in Custom UID Arg Fails
    Run Default Thininstaller With Args  28  --user-ids-to-configure=sophos-spl-local:100,sophos-spl-updatescheduler:101,
    Check Thininstaller Log Contains  Error: Requested group/user ids to configure passed with trailing comma --- aborting install

Thin Installer With Empty Custom UID Arg Fails
    Run Default Thininstaller With Args  28  --user-ids-to-configure=
    Check Thininstaller Log Contains  Error: Requested group/user ids to configure not passed with argument --- aborting install

Thin Installer With Invalid Custom UID Arg Fails
    Run Default Thininstaller With Args  28  --user-ids-to-configure=localuser:100
    Check Thininstaller Log Contains  Error: Requested group/user id to configure is not valid: localuser:100 --- aborting install

Thin Installer With Custom UID Arg That Is Not An Integer Fails
    Run Default Thininstaller With Args  28  --user-ids-to-configure=sophos-spl-updatescheduler:uid
    Check Thininstaller Log Contains  Error: Requested group/user id to configure is not valid: sophos-spl-updatescheduler:uid --- aborting install

Thin Installer With Customer UID Arg That Does Not Match Expected Syntax Fails
    Run Default Thininstaller With Args  28  --user-ids-to-configure=sophos-spl-user=101
    Check Thininstaller Log Contains  Error: Requested group/user id to configure is not valid: sophos-spl-user=101 --- aborting install

Thin Installer With Duplicate Username In Custom UID Args Fails
    Run Default Thininstaller With Args  28  --user-ids-to-configure=sophos-spl-local:100,sophos-spl-local:101
    Check Thininstaller Log Contains  Error: Duplicate user name given: sophos-spl-local:101 --- aborting install

Thin Installer With Duplicate ID In Custom UID Args Fails
    Run Default Thininstaller With Args  28  --user-ids-to-configure=sophos-spl-local:100,sophos-spl-updatescheduler:100
    Check Thininstaller Log Contains  Error: Duplicate id given: sophos-spl-updatescheduler:100 --- aborting install

Thin Installer With Trailing Comma in Custom GID Arg Fails
    Run Default Thininstaller With Args  28  --group-ids-to-configure=sophos-spl-group:100,sophos-spl-ipc:101,
    Check Thininstaller Log Contains  Error: Requested group/user ids to configure passed with trailing comma --- aborting install

Thin Installer With Empty Custom GID Arg Fails
    Run Default Thininstaller With Args  28  --group-ids-to-configure=
    Check Thininstaller Log Contains  Error: Requested group/user ids to configure not passed with argument --- aborting install

Thin Installer With Invalid Custom GID Arg Fails
    Run Default Thininstaller With Args  28  --group-ids-to-configure=localgroup:100
    Check Thininstaller Log Contains  Error: Requested group/user id to configure is not valid: localgroup:100 --- aborting install

Thin Installer With Custom GID Arg That Is Not An Integer Fails
    Run Default Thininstaller With Args  28  --group-ids-to-configure=sophos-spl-group:gid
    Check Thininstaller Log Contains  Error: Requested group/user id to configure is not valid: sophos-spl-group:gid --- aborting install

Thin Installer With Customer GID Arg That Does Not Match Expected Syntax Fails
    Run Default Thininstaller With Args  28  --group-ids-to-configure=sophos-spl-group=100
    Check Thininstaller Log Contains  Error: Requested group/user id to configure is not valid: sophos-spl-group=100 --- aborting install

Thin Installer With Duplicate Group Name In Custom UID Args Fails
    Run Default Thininstaller With Args  28  --group-ids-to-configure=sophos-spl-group:100,sophos-spl-group:101
    Check Thininstaller Log Contains  Error: Duplicate user name given: sophos-spl-group:101 --- aborting install

Thin Installer With Duplicate ID In Custom GID Args Fails
    Run Default Thininstaller With Args  28  --group-ids-to-configure=sophos-spl-group:100,sophos-spl-ipc:100
    Check Thininstaller Log Contains  Error: Duplicate id given: sophos-spl-ipc:100 --- aborting install
