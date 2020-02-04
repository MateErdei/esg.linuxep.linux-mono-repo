*** Settings ***
Documentation    Test Wdctl can remove a plugin

Library    Process
Library    OperatingSystem
Library    ${LIBS_DIRECTORY}/FullInstallerUtils.py
Library    ${LIBS_DIRECTORY}/Watchdog.py
Library    ${LIBS_DIRECTORY}/LogUtils.py

Resource  WatchdogResources.robot
Resource  ../installer/InstallerResources.robot

Test Setup  Require Fresh Install
Test Teardown  Wdctl Test Teardown

Default Tags  WDCTL
*** Test Cases ***
Test Wdctl can remove a plugin
    Should Exist   ${SOPHOS_INSTALL}/base/pluginRegistry/managementagent.json
    Wait Until Keyword Succeeds  10 seconds  0.5 seconds   Check Management Agent Running And Ready

    ${result} =    Run Process    ${SOPHOS_INSTALL}/bin/wdctl   removePluginRegistration   managementagent

    Log    "stdout = ${result.stdout}"
    Log    "stderr = ${result.stderr}"
    Should Be Equal As Integers    ${result.rc}    0

    Wait Until Keyword Succeeds  10 seconds  0.5 seconds   check managementagent not running

    Should Not Exist   ${SOPHOS_INSTALL}/base/pluginRegistry/managementagent.json

    Report Wdctl Log


Test Wdctl can stop a plugin
    [Tags]  WDCTL  MANAGEMENT_AGENT
    Should Exist   ${SOPHOS_INSTALL}/base/pluginRegistry/managementagent.json
    Wait Until Keyword Succeeds  10 seconds  0.5 seconds   Check Management Agent Running And Ready

    ${result} =    Run Process    ${SOPHOS_INSTALL}/bin/wdctl   stop   managementagent

    Log    "stdout = ${result.stdout}"
    Log    "stderr = ${result.stderr}"
    Should Be Equal As Integers    ${result.rc}    0

    Wait Until Keyword Succeeds  10 seconds  0.5 seconds   check managementagent not running

    Report Wdctl Log

Test Wdctl Reports Error When Stopping A Plugin Which Does Not Exist
    [Tags]  WDCTL

    ${result} =    Run Process    ${SOPHOS_INSTALL}/bin/wdctl   stop   thisPluginDoesNotExist
    Log    "stdout = ${result.stdout}"
    Log    "stderr = ${result.stderr}"
    Should Be Equal As Integers    ${result.rc}    2
    Should Contain   ${result.stderr}   Plugin "thisPluginDoesNotExist" not in registry

    ${wdctl_log}=  Get File  ${SOPHOS_INSTALL}/logs/base/wdctl.log
    Should Contain  ${wdctl_log}  Plugin "thisPluginDoesNotExist" not in registry

Test Wdctl reports error removing a plugin that does not exist
    [Tags]  WDCTL  MANAGEMENT_AGENT
    Should Exist   ${SOPHOS_INSTALL}/base/pluginRegistry/managementagent.json
    Wait Until Keyword Succeeds  10 seconds  0.5 seconds   Check Management Agent Running And Ready

    ${result} =    Run Process    ${SOPHOS_INSTALL}/bin/wdctl   removePluginRegistration   doesnotexist

    Log    "stdout = ${result.stdout}"
    Log    "stderr = ${result.stderr}"
    Should Be Equal As Integers    ${result.rc}    2

    Wait Until Keyword Succeeds  10 seconds  0.5 seconds   Check Management Agent Running And Ready
    Should Exist   ${SOPHOS_INSTALL}/base/pluginRegistry/managementagent.json

Test wdctl prints error message with no arguments
    ${result} =    Run Process    ${SOPHOS_INSTALL}/bin/wdctl
    Log    "stdout = ${result.stdout}"
    Log    "stderr = ${result.stderr}"
    Should Not Be Equal As Integers    ${result.rc}    0
    Should Contain   ${result.stderr}   Error: Wrong number of arguments expected 2

Test wdctl prints error message with only copyPluginRegistration argument
    Wdctl Fails With Only Command With Error Message  copyPluginRegistration  Error: Wrong number of arguments expected 2

Test wdctl prints error message with only start argument
    Wdctl Fails With Only Command With Error Message  start  Error: Wrong number of arguments expected 2

Test wdctl prints error message with only stop argument
    Wdctl Fails With Only Command With Error Message  stop  Error: Wrong number of arguments expected 2

Test wdctl prints error message with only removePluginRegistration argument
    Wdctl Fails With Only Command With Error Message  removePluginRegistration  Error: Wrong number of arguments expected 2

Test wdctl prints error message with 3 arguments
    ${result} =    Run Process    ${SOPHOS_INSTALL}/bin/wdctl  Argument1  Argument2  Argument3
    Log    "stdout = ${result.stdout}"
    Log    "stderr = ${result.stderr}"
    Should Not Be Equal As Integers    ${result.rc}    0
    Should Contain   ${result.stderr}   Error: Wrong number of arguments expected 2

Test wdctl prints error message with missing config
    Wdctl Fails Command With Error Message  copyPluginRegistration  /tmp/NotAFile  source file does not exist.

Test wdctl should print error message when trying to copy plugin with name longer than 20 chars
    Create File   ${SOPHOS_INSTALL}/tmp/123456789102345678920.json  Dummy Content
    Wdctl Fails Command With Error Message  copyPluginRegistration  ${SOPHOS_INSTALL}/tmp/123456789102345678920.json  Plugin name is longer than the maximum 20 characters.

Test wdctl prints error message when passed an unknown command
    Wdctl Fails Command With Error Message  doesnotexist  doesnotexist  Unknown command:

Test wdctl prints error message when stopping plugin that is in registry but not running
    Create File  ${SOPHOS_INSTALL}/base/pluginRegistry/doesexist.json
    Wdctl Fails Command With Error Message  start  doesexist  Failed to start

Test wdctl prints error message when starting nonexistent plugin
    Wdctl Fails Command With Error Message  start  doesnotexist  Failed to start

#Empty configuration file tests

Test wdctl should not print error message with empty plugin registration file
    Create File   ${SOPHOS_INSTALL}/tmp/EmptyPluginConfig.json
    Copy Plugin Registry Should Succeed  ${SOPHOS_INSTALL}/tmp/EmptyPluginConfig.json

Test wdctl should print error message when starting empty plugin registration
    Create File   ${SOPHOS_INSTALL}/tmp/EmptyPluginConfig.json
    Copy Plugin Registry Should Succeed  ${SOPHOS_INSTALL}/tmp/EmptyPluginConfig.json
    Should Fail Start  EmptyPluginConfig

# Incorrect json content tests

Test wdctl should not print error message with junk content plugin registration file
    Create File   ${SOPHOS_INSTALL}/tmp/NotJsonFile.json  NotJsonContent
    Copy Plugin Registry Should Succeed  ${SOPHOS_INSTALL}/tmp/NotJsonFile.json


Test wdctl should print error message when starting plugin with junk content plugin registration
    Create File   ${SOPHOS_INSTALL}/tmp/NotJsonFile.json  NotJsonContent
    Copy Plugin Registry Should Succeed  ${SOPHOS_INSTALL}/tmp/NotJsonFile.json
    Should Fail Start  NotJsonFile

Test wdctl should not print error message with incorrect json content plugin registration file
    Copy Plugin Registry Should Succeed  tests/mcs_router/installfiles/sav.json

Test wdctl should print error message when starting plugin with incorrect json content plugin registration
    Copy Plugin Registry Should Succeed  tests/mcs_router/installfiles/sav.json
    Should Fail Start  sav


*** Keywords ***


Should Fail Start
    [Arguments]   ${PluginName}
    Wdctl Fails Command With Error Message   start  ${PluginName}  Failed to start

Copy Plugin Registry Should Succeed
    [Tags]  SMOKE  WDCTL
    [Arguments]   ${FileName}
    ${result} =    Run Process    ${SOPHOS_INSTALL}/bin/wdctl   copyPluginRegistration   ${FileName}
    Log    "stdout = ${result.stdout}"
    Log    "stderr = ${result.stderr}"
    Should Be Equal As Integers    ${result.rc}    0

Wdctl Fails Command With Error Message
    [Arguments]  ${Command}   ${Argument}   ${ErrorMessage}
    Log  ${Command}
    Log  ${Argument}
    Log  ${ErrorMessage}
    ${result} =    Run Process    ${SOPHOS_INSTALL}/bin/wdctl   ${Command}   ${Argument}
    Log    "stdout = ${result.stdout}"
    Log    "stderr = ${result.stderr}"
    Should Not Be Equal As Integers    ${result.rc}    0
    Should Contain   ${result.stderr}   ${ErrorMessage}
    Should Not Contain   ${result.stderr}   Critical unhandled

Wdctl Succeeds Command
    [Arguments]  ${Command}   ${Argument}
     Log  ${Command}
     Log  ${Argument}
     ${result} =    Run Process    ${SOPHOS_INSTALL}/bin/wdctl   ${Command}   ${Argument}
     Should Be Equal As Integers  ${result.rc}  0  Wdctl command didn't succeed

Wdctl Fails With Only Command With Error Message
    [Arguments]  ${Command}   ${ErrorMessage}
    Log  ${Command}
    Log  ${ErrorMessage}
    ${result} =    Run Process    ${SOPHOS_INSTALL}/bin/wdctl   ${Command}
    Log    "stdout = ${result.stdout}"
    Log    "stderr = ${result.stderr}"
    Should Not Be Equal As Integers    ${result.rc}    0
    Should Contain   ${result.stderr}   ${ErrorMessage}


