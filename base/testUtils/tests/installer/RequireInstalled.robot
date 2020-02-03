*** Settings ***
Documentation    Setup an installation

Library    ${libs_directory}/FullInstallerUtils.py

Resource  ../installer/InstallerResources.robot

Default Tags  MANUAL  INSTALLER

*** Test Cases ***
Run Require Fresh Install
    Require Fresh Install


Force reinstall
    Uninstall SSPL
    Require Fresh Install