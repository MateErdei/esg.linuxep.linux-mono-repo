*** Settings ***
Documentation    Tests to verify we can register successfully with
...              fake cloud and save the ID and password we receive.
...              Also tests bad registrations, and deregistrations.

Library     ${LIBS_DIRECTORY}/OSUtils.py
Library     ${libs_directory}/PushServerUtils.py
Library     String

#Suite Setup      Setup MCS Tests
Suite Teardown   Uninstall SSPL Unless Cleanup Disabled

Resource  ../installer/InstallerResources.robot
Resource  McsRouterResources.robot

Test Setup  Run Keywords
...         Setup MCS Tests  AND
...         Start System Watchdog

Test Teardown    Run Keywords
...              Stop Local Cloud Server  AND
...              MCSRouter Default Test Teardown  AND
...			     Stop System Watchdog  AND
...              Push Server Test Teardown

Default Tags  MCS  FAKE_CLOUD  MCS_ROUTER
Force Tags  LOAD3

*** Test Case ***

Successful Register With Cloud And Migrate To Another Cloud Server
    [Tags]  MCS  MCS_ROUTER  FAKE_CLOUD

#    TODO if we need debug we can turn it on but I want to see what the logs look like in normal state to tody up logging code.
#    Override LogConf File as Global Level  DEBUG

    Start Local Cloud Server  --initial-mcs-policy  ${SUPPORT_FILES}/CentralXml/MCS_Push_Policy_PushFallbackPoll.xml
    Start MCS Push Server
    Wait For MCS Router To Be Running
    Register With Local Cloud Server
    Check Correct MCS Password And ID For Local Cloud Saved
    Check Cloud Server Log Does Not Contain  Processing deployment api call
    Check Cloud Server Log Contains  Register with ::ThisIsARegToken

    ${original_mcsid}  get_value_from_ini_file  MCSID  ${SOPHOS_INSTALL}/base/etc/sophosspl/mcs.config
    ${original_jwt_token}  get_value_from_ini_file  jwt_token  ${SOPHOS_INSTALL}/base/etc/sophosspl/mcs.config

    Log File  ${SOPHOS_INSTALL}/base/etc/sophosspl/mcs_policy.config
    ${mcs_policy_config_ts_orig}    Get Modified Time   ${SOPHOS_INSTALL}/base/etc/sophosspl/mcs_policy.config

    Log File  ${SOPHOS_INSTALL}/base/etc/sophosspl/mcs.config
    ${mcs_config_ts_orig}    Get Modified Time   ${SOPHOS_INSTALL}/base/etc/sophosspl/mcs.config

    Log File  ${SOPHOS_INSTALL}/base/etc/mcs.config
    ${mcs_root_config_ts_orig}    Get Modified Time   ${SOPHOS_INSTALL}/base/etc/mcs.config

    Log File  ${SOPHOS_INSTALL}/base/etc/sophosspl/current_proxy
    ${current_proxy_ts_orig}    Get Modified Time   ${SOPHOS_INSTALL}/base/etc/sophosspl/current_proxy

    # Populate purge directories with garbage
    Create File  ${MCS_DIR}/event/garbage_event  Please recycle
    ${result} =  Run Process  chown  sophos-spl-local:sophos-spl-group  ${MCS_DIR}/event/garbage_event

    Create File  ${MCS_DIR}/action/garbage_action  Please recycle
    ${result} =  Run Process  chown  sophos-spl-local:sophos-spl-group  ${MCS_DIR}/action/garbage_action

    Create File  ${MCS_DIR}/policy/garbage_policy  Please recycle
    ${result} =  Run Process  chown  sophos-spl-local:sophos-spl-group  ${MCS_DIR}/policy/garbage_policy

    Create File  ${MCS_DIR}/datafeed/garbage_result  Please recycle
    ${result} =  Run Process  chown  sophos-spl-local:sophos-spl-group  ${MCS_DIR}/datafeed/garbage_result

    Trigger Migration Now

    # Long wait due to Push Server triggering reduced polling
     Wait Until Keyword Succeeds
    ...  120s
    ...  10s
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

    Log File  ${SOPHOS_INSTALL}/base/etc/sophosspl/current_proxy
    ${current_proxy_ts_new}    Get Modified Time   ${SOPHOS_INSTALL}/base/etc/sophosspl/current_proxy

    Should Not Be Equal As Strings  ${mcs_policy_config_ts_new}  ${mcs_policy_config_ts_orig}
    Should Not Be Equal As Strings  ${mcs_config_ts_new}         ${mcs_config_ts_orig}
    Should Not Be Equal As Strings  ${mcs_root_config_ts_new}    ${mcs_root_config_ts_orig}
    Should Not Be Equal As Strings  ${current_proxy_ts_new}      ${current_proxy_ts_orig}

    ${new_mcsid}  get_value_from_ini_file  MCSID  ${SOPHOS_INSTALL}/base/etc/sophosspl/mcs.config
    ${new_jwt_token}  get_value_from_ini_file  jwt_token  ${SOPHOS_INSTALL}/base/etc/sophosspl/mcs.config

    Should Not Be Equal As Strings  ${original_mcsid}  ${new_mcsid}
    Should Not Be Equal As Strings  ${original_jwt_token}  ${new_jwt_token}

    File Should Not Exist  ${MCS_DIR}/event/garbage_event
    File Should Not Exist  ${MCS_DIR}/action/garbage_action
    File Should Not Exist  ${MCS_DIR}/policy/garbage_policy
    File Should Not Exist  ${MCS_DIR}/datafeed/garbage_result

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
    ${push_server_address} =  Set Variable  localhost:4443
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  5 secs
    ...  Check Log Contains String N Times   ${SOPHOS_INSTALL}/logs/base/sophosspl/mcsrouter.log   MCS Router Log   Established MCS Push Connection   2
    Check MCSRouter Log Contains   Push client successfully connected to ${push_server_address} directly


Register With Cloud And Fail To Migrate To Another Cloud Server
    [Tags]  MCS  MCS_ROUTER  FAKE_CLOUD

    Start Local Cloud Server  --initial-mcs-policy  ${SUPPORT_FILES}/CentralXml/MCS_Push_Policy_PushFallbackPoll.xml
    Start MCS Push Server
    Wait For MCS Router To Be Running
    Register With Local Cloud Server
    Check Correct MCS Password And ID For Local Cloud Saved
    Check Cloud Server Log Does Not Contain  Processing deployment api call
    Check Cloud Server Log Contains  Register with ::ThisIsARegToken

    ${original_mcsid}  get_value_from_ini_file  MCSID  ${SOPHOS_INSTALL}/base/etc/sophosspl/mcs.config
    ${original_jwt_token}  get_value_from_ini_file  jwt_token  ${SOPHOS_INSTALL}/base/etc/sophosspl/mcs.config

    Log File  ${SOPHOS_INSTALL}/base/etc/sophosspl/mcs_policy.config
    ${mcs_policy_config_ts_orig}    Get Modified Time   ${SOPHOS_INSTALL}/base/etc/sophosspl/mcs_policy.config

     Log File  ${SOPHOS_INSTALL}/base/etc/mcs.config
     ${mcs_root_config_ts_orig}    Get Modified Time   ${SOPHOS_INSTALL}/base/etc/mcs.config

     Log File  ${SOPHOS_INSTALL}/base/etc/sophosspl/mcs.config
     ${mcs_config_content_orig}   get file   ${SOPHOS_INSTALL}/base/etc/sophosspl/mcs.config

    # Populate some of the purge directories with garbage to prove they are purged during a migration
    # Events and datfeed rsults not included here beacuse the product removes them if they are invalid
    # or will send them and then remove them if they are valid so.
    Create File  ${MCS_DIR}/action/garbage_action  Please recycle
    Create File  ${MCS_DIR}/policy/garbage_policy  Please recycle
    Create File  ${SOPHOS_INSTALL}/base/etc/sophosspl/current_proxy  Please recycle

    Set Migrate To Reply With 401 Flag
    Trigger Migration Now

    # Long wait due to Push Server triggering reduced polling
    Wait Until Keyword Succeeds
    ...  120s
    ...  10s
    ...  Check MCS Router Log Contains  Attempting Central migration

    Wait Until Keyword Succeeds
    ...  60s
    ...  5s
    ...  Check MCS Router Log Contains  Migration request failed:

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

    Should Be Equal As Strings  ${mcs_policy_config_ts_new}  ${mcs_policy_config_ts_orig}
    Should Be Equal As Strings  ${mcs_root_config_ts_new}  ${mcs_root_config_ts_orig}

    Log File  ${SOPHOS_INSTALL}/base/etc/sophosspl/mcs.config
    ${mcs_config_content_new}   get file   ${SOPHOS_INSTALL}/base/etc/sophosspl/mcs.config
    Should Be Equal As Strings  ${mcs_config_content_new}  ${mcs_config_content_orig}

    ${new_mcsid}  get_value_from_ini_file  MCSID  ${SOPHOS_INSTALL}/base/etc/sophosspl/mcs.config
    ${new_jwt_token}  get_value_from_ini_file  jwt_token  ${SOPHOS_INSTALL}/base/etc/sophosspl/mcs.config

    Should Be Equal As Strings  ${original_mcsid}  ${new_mcsid}
    Should Be Equal As Strings  ${original_jwt_token}  ${new_jwt_token}

    File Should Exist  ${MCS_DIR}/action/garbage_action
    File Should Exist  ${MCS_DIR}/policy/garbage_policy
    File Should Exist  ${SOPHOS_INSTALL}/base/etc/sophosspl/current_proxy

    # Check push connection has been re-established
    ${push_server_address} =  Set Variable  localhost:4443
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  5 secs
    ...  Check Log Contains String N Times   ${SOPHOS_INSTALL}/logs/base/sophosspl/mcsrouter.log   MCS Router Log   Established MCS Push Connection   1
    Check MCSRouter Log Contains   Push client successfully connected to ${push_server_address} directly


*** Keywords ***
Backup Version Ini
    Copy File  ${SOPHOS_INSTALL}/base/VERSION.ini  /tmp/VERSION.ini.bk

Restore Version Ini
    Move File  /tmp/VERSION.ini.bk  ${SOPHOS_INSTALL}/base/VERSION.ini