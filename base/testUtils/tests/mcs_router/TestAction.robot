*** Settings ***
Library  OperatingSystem
Library    ${LIBS_DIRECTORY}/FilesystemWatcher.py

Resource  McsRouterResources.robot

Suite Setup       Run Keywords
...               Setup MCS Tests  AND
...               Start Local Cloud Server

Suite Teardown  Run Keywords
...             Stop Local Cloud Server  AND
...             Uninstall SSPL Unless Cleanup Disabled

Default Tags  MCS  FAKE_CLOUD  MCS_ROUTER
Force Tags  LOAD2
*** Variables ***
${FileSystemWatcherLog}  /tmp/fsw.log
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

Actions are removed when mcsrouter shutdown
    [Tags]  MCS  FAKE_CLOUD  MCS_ROUTER  TAP_TESTS
    Override LogConf File as Global Level  DEBUG
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
    ${r} =  Run Process  ps  -f  -C  python3  |  grep  /opt/sophos-spl/base/bin/python3  |  awk  '{print$2}'   shell=true
    ${result} =  Run Process  kill  ${r.stdout}
    Should Be Equal As Strings  ${result.rc}  0
    Wait Until Keyword Succeeds
    ...  5 secs
    ...  1 secs
    ...  Should Not Exist  ${SOPHOS_INSTALL}/base/mcs/action/ALC_action_FakeTime.xml
    Check Mcsrouter Log Contains   Removing file /opt/sophos-spl/base/mcs/action/ALC_action_FakeTime.xml as part of mcs shutdown

Action Applied After Policies
    [Teardown]  Test With Filesystem Watcher Teardown

    Set Log Level For Component And Reset and Return Previous Log  mcs_router   DEBUG
    Register With Fake Cloud
    Check Default Policies Exist
    Send Policy File  mcs  ${SUPPORT_FILES}/CentralXml/Cloud_MCS_policy_15sPollingDelay.xml
        # Check for default of 5 seconds with tolerance of 1 seconds
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  Check Mcsrouter Log Contains  COMMAND_CHECK_INTERVAL_MINIMUM=15

    Setup Filesystem Watcher  ${SOPHOS_INSTALL}  log_file_path=${FileSystemWatcherLog}
    Start Filesystem Watcher

    Remove File   ${SOPHOS_INSTALL}/base/mcs/policy/ALC-1_policy.xml

    Queue Update Now  lagavulin
    Queue Update Now  laphroaig
    Queue Update Now  talisker
    Queue Update Now  jura

    Send Policy File  alc  ${SUPPORT_FILES}/CentralXml/ALC_policy_direct_just_base.xml
    Send Policy File  alc  ${SUPPORT_FILES}/CentralXml/ALC_policy_direct.xml
    Send Policy File  alc  ${SUPPORT_FILES}/CentralXml/ALC_policy_delayed_updating.xml
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

Check Cloud Server Log For Command Poll
    [Arguments]    ${occurrence}=1
    Wait Until Keyword Succeeds
    ...  1 min
    ...  5 secs
    ...  Check Cloud Server Log Contains    GET - /mcs/commands/applications    ${occurrence}

Test With Filesystem Watcher Teardown
    Run Keywords
    ...  Stop Filesystem Watcher  AND
    ...  Run Keyword If Test Failed  Log file  ${FileSystemWatcherLog}   AND
    ...  MCSRouter Default Test Teardown