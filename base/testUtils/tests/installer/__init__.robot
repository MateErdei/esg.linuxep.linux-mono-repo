*** Settings ***
Documentation    Tests for the installer
Library    OperatingSystem


Suite Setup  Installer Suite Setup

Force Tags  LOAD4

*** Variables ***
${TMP_DIR}      ${CURDIR}/../../temp

*** Keywords ***
Installer Suite Setup
    Remove Directory   ${TMP_DIR}  recursive=True