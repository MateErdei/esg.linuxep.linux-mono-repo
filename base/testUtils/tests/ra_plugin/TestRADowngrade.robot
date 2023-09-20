*** Settings ***
Documentation    Testing RA Downgrade Logs

Resource  ../installer/InstallerResources.robot
Resource  ResponseActionsResources.robot

Suite Setup     Require Fresh Install
Suite Teardown  Require Uninstalled

Test Setup    Install Response Actions Directly

Force Tags  LOAD7
Default Tags   RESPONSE_ACTIONS_PLUGIN  TAP_PARALLEL5

*** Test Cases ***

RA Log Files Are Saved When Downgrading
    Downgrade Response Actions
    # check that the log folder contains the downgrade-backup directory
    Directory Should Exist  ${SOPHOS_INSTALL}/plugins/responseactions/log/downgrade-backup

    # check that the downgrade-backup directory contains the RA log files
    File Should Exist  ${SOPHOS_INSTALL}/plugins/responseactions/log/downgrade-backup/responseactions.log