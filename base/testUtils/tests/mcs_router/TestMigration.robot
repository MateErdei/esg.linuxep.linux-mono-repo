*** Settings ***
Documentation    Tests to verify we can register successfully with
...              fake cloud and save the ID and password we receive.
...              Also tests bad registrations, and deregistrations.

Library     ${LIBS_DIRECTORY}/OSUtils.py
Library     ${libs_directory}/PushServerUtils.py
Library     String

Suite Setup      Setup MCS Tests
Suite Teardown   Uninstall SSPL Unless Cleanup Disabled

Resource  ../installer/InstallerResources.robot
Resource  McsRouterResources.robot

Test Setup       Run Keywords
...              Start Local Cloud Server  AND
...              Backup Version Ini

Test Teardown    Run Keywords
...              Stop Local Cloud Server  AND
...              MCSRouter Default Test Teardown  AND
...			     Stop System Watchdog  AND
...              Restore Version Ini

Default Tags  MCS  FAKE_CLOUD  REGISTRATION  MCS_ROUTER
Force Tags  LOAD3

*** Test Case ***

Successful Register With Cloud And Migrate To Another Cloud Server
    [Tags]  AMAZON_LINUX  MCS  MCS_ROUTER  FAKE_CLOUD

#    [Teardown]  Push Server Teardown with MCS Fake Server

#    TODO if we need debug we can turn it on but I want to see what the logs look like in normal state to tody up logging code.
#    Override LogConf File as Global Level  DEBUG
    Start MCS Push Server
    #n https://127.0.0.1:8459
    Start System Watchdog
    Register With Local Cloud Server
    Check Correct MCS Password And ID For Local Cloud Saved
    Check Cloud Server Log Does Not Contain  Processing deployment api call
    Check Cloud Server Log Contains  Register with ::ThisIsARegToken
    Wait For MCS Router To Be Running
    Wait New MCS Policy Downloaded

    ${original_mcsid}  get_value_from_ini_file  MCSID  ${SOPHOS_INSTALL}/base/etc/sophosspl/mcs.config
    ${original_jwt_token}  get_value_from_ini_file  jwt_token  ${SOPHOS_INSTALL}/base/etc/sophosspl/mcs.config

    Log File  ${SOPHOS_INSTALL}/base/etc/sophosspl/mcs_policy.config
    ${mcs_policy_config_ts_orig}    Get Modified Time   ${SOPHOS_INSTALL}/base/etc/sophosspl/mcs_policy.config

    Log File  ${SOPHOS_INSTALL}/base/etc/sophosspl/mcs.config
    ${mcs_config_ts_orig}    Get Modified Time   ${SOPHOS_INSTALL}/base/etc/sophosspl/mcs.config

    # TODO we need ot modify base installer for to delete this config.
     Log File  ${SOPHOS_INSTALL}/base/etc/mcs.config
     ${mcs_root_config_ts_orig}    Get Modified Time   ${SOPHOS_INSTALL}/base/etc/mcs.config

    # Populate purge directories with garbage
    Create File  ${SOPHOS_INSTALL}/base/mcs/event/garbage_event  Please recycle
    Create File  ${SOPHOS_INSTALL}/base/mcs/action/garbage_action  Please recycle
    Create File  ${SOPHOS_INSTALL}/base/mcs/policy/garbage_policy  Please recycle
    Create File  ${SOPHOS_INSTALL}/base/mcs/datafeed/garbage_result  Please recycle

    # todo ${SOPHOS_INSTALL}/base/etc/sophosspl/current_proxy


    # -rw-r----- 1 sophos-spl-local sophos-spl-group 143 Sep  7 12:42 /opt/sophos-spl/base/etc/mcs.config

    Trigger Migration Now

     Wait Until Keyword Succeeds
    ...  10s
    ...  1s
    ...  Check MCS Router Log Contains  Attempting Central migration

    Wait Until Keyword Succeeds
    ...  60s
    ...  5s
    ...  Check MCS Router Log Contains  Successfully migrated Sophos Central account

    Wait Until Keyword Succeeds
    ...  20s
    ...  1s
    ...  File Should Exist  ${SOPHOS_INSTALL}/base/etc/sophosspl/mcs.config

    Wait Until Keyword Succeeds
    ...  20s
    ...  1s
    ...  File Should Exist  ${SOPHOS_INSTALL}/base/etc/sophosspl/mcs_policy.config

    Log File  ${SOPHOS_INSTALL}/base/etc/sophosspl/mcs_policy.config
    ${mcs_policy_config_ts_new}    Get Modified Time   ${SOPHOS_INSTALL}/base/etc/sophosspl/mcs_policy.config

    Log File  ${SOPHOS_INSTALL}/base/etc/sophosspl/mcs.config
    ${mcs_config_ts_new}    Get Modified Time   ${SOPHOS_INSTALL}/base/etc/sophosspl/mcs.config

    Log File  ${SOPHOS_INSTALL}/base/etc/mcs.config
    ${mcs_root_config_ts_new}    Get Modified Time   ${SOPHOS_INSTALL}/base/etc/mcs.config

    Should Not Be Equal As Strings  ${mcs_policy_config_ts_new}  ${mcs_policy_config_ts_orig}
    Should Not Be Equal As Strings  ${mcs_config_ts_new}  ${mcs_config_ts_orig}
    Should Not Be Equal As Strings  ${mcs_root_config_ts_new}  ${mcs_root_config_ts_orig}

    ${new_mcsid}  get_value_from_ini_file  MCSID  ${SOPHOS_INSTALL}/base/etc/sophosspl/mcs.config
    ${new_jwt_token}  get_value_from_ini_file  jwt_token  ${SOPHOS_INSTALL}/base/etc/sophosspl/mcs.config

    Should Not Be Equal As Strings  ${original_mcsid}  ${new_mcsid}
    Should Not Be Equal As Strings  ${original_jwt_token}  ${new_jwt_token}

    File Should Not Exist  ${MCS_DIR}/event/garbage_event
    File Should Not Exist  ${MCS_DIR}/action/garbage_action
    File Should Not Exist  ${MCS_DIR}/policy/garbage_policy
    File Should Not Exist  ${MCS_DIR}/datafeed/garbage_result
    #    Log File  ${SOPHOS_INSTALL}/base/etc/mcs.config

    Wait Until Keyword Succeeds
    ...  60s
    ...  5s
    ...  Check Envelope Log Contains  /statuses/endpoint/${new_mcsid}

    Check Log Contains In Order
    ...  ${BASE_LOGS_DIR}/sophosspl/mcs_envelope.log
    ...  /statuses/endpoint/${new_mcsid}
    ...  <status><appId>MCS</appId>
    ...  <status><appId>ALC</appId>
    ...  <status><appId>SDU</appId>
    ...  <status><appId>APPSPROXY</appId>
    ...  <status><appId>AGENT</appId>

    # Check push connection has been re-established
#    Wait Until Keyword Succeeds
#        ...  30 secs
#        ...  5 secs
#        ...  Check MCSRouter Log Contains   Established MCS Push Connection
#    Check MCSRouter Log Contains   Push client successfully connected to ${push_server_address} via localhost:4443
#    sleep  10
#    FAIL

Register With Cloud And Fail To Migrate To Another Cloud Server
    [Tags]  AMAZON_LINUX  MCS  MCS_ROUTER  FAKE_CLOUD
    Override LogConf File as Global Level  DEBUG
    Start System Watchdog
    Register With Local Cloud Server
    Check Correct MCS Password And ID For Local Cloud Saved
    Check Cloud Server Log Does Not Contain  Processing deployment api call
    Check Cloud Server Log Contains  Register with ::ThisIsARegToken
    File Exists With Permissions  ${SOPHOS_INSTALL}/logs/base/register_central.log  root  root  -rw-------

    set_migrate_to_reply_with_401_flag
    Trigger Migration Now
    sleep  60
    FAIL

    # Create Migration Action
    # Migrate via sending MCS action
    # Check Migration


*** Keywords ***
Backup Version Ini
    Copy File  ${SOPHOS_INSTALL}/base/VERSION.ini  /tmp/VERSION.ini.bk

Restore Version Ini
    Move File  /tmp/VERSION.ini.bk  ${SOPHOS_INSTALL}/base/VERSION.ini