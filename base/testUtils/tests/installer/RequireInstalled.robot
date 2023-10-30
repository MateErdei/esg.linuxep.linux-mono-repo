*** Settings ***
Documentation    Setup an installation

Library    ${COMMON_TEST_LIBS}/FullInstallerUtils.py

Resource   ${COMMON_TEST_ROBOT}/InstallerResources.robot

Force Tags  MANUAL  INSTALLER

*** Test Cases ***
Run Require Fresh Install
    Require Fresh Install


Force reinstall
    Uninstall SSPL
    Require Fresh Install