*** Settings ***
Documentation    Test installation on different filsystems

Library    ${LIBS_DIRECTORY}/LogUtils.py
Library    ${LIBS_DIRECTORY}/OSUtils.py

Resource  ../installer/InstallerResources.robot
Resource  ../GeneralTeardownResource.robot

Default Tags  INSTALLER

Test Teardown   XFS Install Tests Teardown
Test Setup      XFS Install Tests Setup

*** Test Cases ***
Install On XFS With ftype Disabled
    [Tags]  MANUAL
    Set Test Variable  ${xfsFakeDev}  /root/xfs-test-dev
    Set Test Variable  ${xfsMountPoint}  /root/xfs-test-mountpoint
    Create File  ${xfsFakeDev}
    Create Directory  ${xfsMountPoint}

    # Make sure that the general test teardown uses the correct directories.
    Set Global Variable  ${SOPHOS_INSTALL_ORIGINAL}  ${SOPHOS_INSTALL}
    Set Global Variable  ${SOPHOS_INSTALL}   ${xfsMountPoint}

    ${result} =    Run Process  dd  if\=/dev/zero  of\=${xfsFakeDev}  bs\=1024k  count\=1000
    Log  ${result.stdout}
    Log  ${result.stderr}
    Should Be Equal As Integers    ${result.rc}    0   msg=Could not zero out file to be used for xfs device.

    ${result} =    Run Process  mkfs.xfs  -m  crc\=0  -n  ftype\=0  ${xfsFakeDev}
    Log  ${result.stdout}
    Log  ${result.stderr}
    Should Be Equal As Integers    ${result.rc}    0   msg=Could not make XFS file system.

    ${result} =    Run Process  mount  -t  xfs  ${xfsFakeDev}  ${xfsMountPoint}
    Log  ${result.stdout}
    Log  ${result.stderr}
    Should Be Equal As Integers    ${result.rc}    0   msg=Could not mount XFS filesystem device onto mount point.

    Run Full Installer  --instdir  ${xfsMountPoint}
    Check Expected Base Processes Are Running

*** Keywords ***
XFS Install Tests Teardown
    General Test Teardown
    Set Global Variable  ${SOPHOS_INSTALL}  ${SOPHOS_INSTALL_ORIGINAL}
    Uninstall from XFS
    Unmount XFS
    Remove File   ${xfsFakeDev}
    Remove Directory    ${xfsMountPoint}  recursive=True
    Ensure Uninstalled
    Turn On SELinux

XFS Install Tests Setup
    Uninstall SSPL
    Turn Off SELinux

Unmount XFS
    ${result} =    Run Process  umount  ${xfsMountPoint}
    Log  ${result.stdout}
    Log  ${result.stderr}
    Should Be Equal As Integers    ${result.rc}    0   msg=Could not unmount XFS mount point.

Uninstall from XFS
    ${result} =    Run Process  ${xfsMountPoint}/bin/uninstall.sh  --force
    Log  ${result.stdout}
    Log  ${result.stderr}
    Should Be Equal As Integers    ${result.rc}    0   msg=Could not uninstall from XFS mount.
