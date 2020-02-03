*** Settings ***
Library  OperatingSystem
Library    ${libs_directory}/FilesystemWatcher.py

Resource  McsRouterResources.robot

Suite Setup       Run Keywords
...               Setup MCS Tests  AND
...               Start Local Cloud Server

Suite Teardown  Run Keywords
...             Stop Local Cloud Server  AND
...             Uninstall SSPL Unless Cleanup Disabled

Default Tags  MCS  FAKE_CLOUD  MCS_ROUTER

*** Test Case ***
Update Now Received And Action File Written
    Register With Fake Cloud
    Check Cloud Server Log For Command Poll
    Trigger Update Now
    # Action will not be received until the next command poll
    Check Cloud Server Log For Command Poll    2
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  5 secs
    ...  Check Action File Exists    ALC_action_FakeTime.xml
    Check Temp Folder Doesnt Contain Atomic Files

Action Applied After Policies
    [Teardown]  Test With Filesystem Watcher Teardown

    Set Log Level For Component And Reset and Return Previous Log  mcs_router   DEBUG
    Register With Fake Cloud
    Check Default Policies Exist

    ${FileSystemWatcherLog}=  Set Variable  /tmp/fsw.log
    Setup Filesystem Watcher  ${SOPHOS_INSTALL}   match_patterns=["*.xml"]  log_file_path=${FileSystemWatcherLog}
    Start Filesystem Watcher

    Remove File   ${SOPHOS_INSTALL}/base/mcs/policy/ALC-1_policy.xml

    Queue Update Now  lagavulin
    Queue Update Now  laphroaig
    Queue Update Now  talisker
    Queue Update Now  jura

    Send Policy File  alc  ${SUPPORT_FILES}/CentralXml/ALC_policy_direct_just_base.xml
    Send Policy File  alc  ${SUPPORT_FILES}/CentralXml/ALC_policy_direct.xml
    Send Policy File  alc  ${SUPPORT_FILES}/CentralXml/ALC_policy_direct_base_and_example_plugin.xml
    Send Policy File  alc  ${SUPPORT_FILES}/CentralXml/ALC_policy_with_cache.xml

    Wait Until Keyword Succeeds
    ...  30 secs
    ...  1 secs
    ...  Check Log Contains In Order
         ...  ${FileSystemWatcherLog}
         ...  deleted: ${SOPHOS_INSTALL}/base/mcs/policy/ALC-1_policy.xml
         ...  created: ${SOPHOS_INSTALL}/tmp/policy/ALC-1_policy.xml
         ...  modified: ${SOPHOS_INSTALL}/tmp/policy/ALC-1_policy.xml
         ...  modified: ${SOPHOS_INSTALL}/tmp/policy/ALC-1_policy.xml
         ...  modified: ${SOPHOS_INSTALL}/tmp/policy/ALC-1_policy.xml
         ...  modified: ${SOPHOS_INSTALL}/tmp/policy/ALC-1_policy.xml
         ...  created: ${SOPHOS_INSTALL}/tmp/actions/
         ...  _ALC_action_lagavulin.xml
         ...  modified: ${SOPHOS_INSTALL}/tmp/actions/
         ...  _ALC_action_lagavulin.xml
         ...  created: ${SOPHOS_INSTALL}/tmp/actions/
         ...  _ALC_action_laphroaig.xml
         ...  modified: ${SOPHOS_INSTALL}/tmp/actions/
         ...  _ALC_action_laphroaig.xml
         ...  created: ${SOPHOS_INSTALL}/tmp/actions/
         ...  _ALC_action_talisker.xml
         ...  modified: ${SOPHOS_INSTALL}/tmp/actions/
         ...  _ALC_action_talisker.xml
         ...  created: ${SOPHOS_INSTALL}/tmp/actions/
         ...  _ALC_action_jura.xml
         ...  modified: ${SOPHOS_INSTALL}/tmp/actions/
         ...  _ALC_action_jura.xml
         ...  moved: ${SOPHOS_INSTALL}/tmp/policy/ALC-1_policy.xml to: ${SOPHOS_INSTALL}/base/mcs/policy/ALC-1_policy.xml
         ...  moved: ${SOPHOS_INSTALL}/tmp/actions/
         ...  _ALC_action_lagavulin.xml to: ${SOPHOS_INSTALL}/base/mcs/action/ALC_action_lagavulin.xml
         ...  moved: ${SOPHOS_INSTALL}/tmp/actions/
         ...  _ALC_action_laphroaig.xml to: ${SOPHOS_INSTALL}/base/mcs/action/ALC_action_laphroaig.xml
         ...  moved: ${SOPHOS_INSTALL}/tmp/actions/
         ...  _ALC_action_talisker.xml to: ${SOPHOS_INSTALL}/base/mcs/action/ALC_action_talisker.xml
         ...  moved: ${SOPHOS_INSTALL}/tmp/actions/
         ...  _ALC_action_jura.xml to: ${SOPHOS_INSTALL}/base/mcs/action/ALC_action_jura.xml


*** Keywords ***

Register With Fake Cloud
    Should Exist  ${REGISTER_CENTRAL}
    Register With Local Cloud Server
    Check Correct MCS Password And ID For Local Cloud Saved
    Start MCSRouter

Check Cloud Server Log For Command Poll
    [Arguments]    ${occurrence}=1
    Wait Until Keyword Succeeds
    ...  1 min
    ...  5 secs
    ...  Check Cloud Server Log Contains    GET - /mcs/commands/applications    ${occurrence}

Test With Filesystem Watcher Teardown
    Run Keywords
    ...  Stop Filesystem Watcher
    ...  MCSRouter Default Test Teardown