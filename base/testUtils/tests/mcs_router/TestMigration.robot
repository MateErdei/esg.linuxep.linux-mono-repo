*** Settings ***
Documentation    Tests to verify we can register successfully with
...              fake cloud and save the ID and password we receive.
...              Also tests bad registrations, and deregistrations.

Library     ${LIBS_DIRECTORY}/OSUtils.py
Library     ${libs_directory}/PushServerUtils.py
Library     String

#Suite Setup      Setup MCS Tests
Suite Teardown   Uninstall SSPL Unless Cleanup Disabled

Resource    ${COMMON_TEST_ROBOT}/InstallerResources.robot
Resource    ${COMMON_TEST_ROBOT}/McsRouterResources.robot

Test Setup  Run Keywords
...         Setup MCS Tests  AND
...         Start System Watchdog

Test Teardown    Run Keywords
...              Stop Local Cloud Server  AND
...              MCSRouter Default Test Teardown  AND
...			     Stop System Watchdog  AND
...              Push Server Test Teardown

Force Tags  MCS  FAKE_CLOUD  MCS_ROUTER  TAP_PARALLEL5

*** Test Case ***

Successful Register With Cloud And Migrate To Another Cloud Server
    Start Local Cloud Server  --initial-mcs-policy  ${SUPPORT_FILES}/CentralXml/MCS_Push_Policy_PushFallbackPoll.xml
    Start MCS Push Server
    Wait For MCS Router To Be Running
    Register With Local Cloud Server
    Check Correct MCS Password And ID For Local Cloud Saved
    Check Cloud Server Log Does Not Contain  Processing deployment api call
    Check Cloud Server Log Contains  Register with ::ThisIsARegToken

    Wait Until Created  ${MCS_CONFIG}  timeout=20 seconds
    Wait Until Created  ${MCS_POLICY_CONFIG}  timeout=20 seconds

    Wait Until Keyword Succeeds
    ...  5s
    ...  1s
    ...  Mcs Config Has Key  MCSID

    ${original_mcsid}  get_value_from_ini_file  MCSID  ${MCS_CONFIG}

    Wait Until Keyword Succeeds
    ...  5s
    ...  1s
    ...  Mcs Config Has Key  jwt_token

    ${original_jwt_token}  get_value_from_ini_file  jwt_token  ${MCS_CONFIG}

    Log File  ${MCS_POLICY_CONFIG}
    ${mcs_policy_config_ts_orig}    Get Modified Time   ${MCS_POLICY_CONFIG}

    Log File  ${MCS_CONFIG}
    ${mcs_config_ts_orig}    Get Modified Time   ${MCS_CONFIG}

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
    Wait Until Created  ${SHS_STATUS_FILE}  timeout=2 minutes
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

    Wait Until Created  ${MCS_CONFIG}  timeout=20 seconds
    Wait Until Created  ${MCS_POLICY_CONFIG}  timeout=30 seconds

    Log File  ${MCS_POLICY_CONFIG}
    ${mcs_policy_config_ts_new}    Get Modified Time   ${MCS_POLICY_CONFIG}

    Log File  ${MCS_CONFIG}
    ${mcs_config_ts_new}    Get Modified Time   ${MCS_CONFIG}

    Log File  ${SOPHOS_INSTALL}/base/etc/mcs.config
    ${mcs_root_config_ts_new}    Get Modified Time   ${SOPHOS_INSTALL}/base/etc/mcs.config

    Log File  ${SOPHOS_INSTALL}/base/etc/sophosspl/current_proxy
    ${current_proxy_ts_new}    Get Modified Time   ${SOPHOS_INSTALL}/base/etc/sophosspl/current_proxy

    Should Not Be Equal As Strings  ${mcs_policy_config_ts_new}  ${mcs_policy_config_ts_orig}
    Should Not Be Equal As Strings  ${mcs_config_ts_new}         ${mcs_config_ts_orig}
    Should Not Be Equal As Strings  ${mcs_root_config_ts_new}    ${mcs_root_config_ts_orig}
    Should Not Be Equal As Strings  ${current_proxy_ts_new}      ${current_proxy_ts_orig}

    ${new_mcsid}  get_value_from_ini_file  MCSID  ${MCS_CONFIG}

    Should Not Be Equal As Strings  ${original_mcsid}  ${new_mcsid}
    Wait Until Keyword Succeeds
    ...  10s
    ...  1s
    ...  JWT token has changed  ${original_jwt_token}

    File Should Not Exist  ${MCS_DIR}/event/garbage_event
    File Should Not Exist  ${MCS_DIR}/action/garbage_action
    File Should Not Exist  ${MCS_DIR}/policy/garbage_policy
    File Should Not Exist  ${MCS_DIR}/datafeed/garbage_result

    Wait Until Keyword Succeeds
    ...  60s
    ...  5s
    ...  Check Envelope Log Contains  /statuses/endpoint/${new_mcsid}

    Check Log Contains In Order
    ...  ${MCS_ENVELOPE_LOG}
    ...  /statuses/endpoint/${new_mcsid}
    ...  <status><appId>MCS</appId>
    ...  <status><appId>ALC</appId>
    ...  <status><appId>SDU</appId>
    ...  <status><appId>SHS</appId>
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
    Start Local Cloud Server  --initial-mcs-policy  ${SUPPORT_FILES}/CentralXml/MCS_Push_Policy_PushFallbackPoll.xml
    Start MCS Push Server
    Wait For MCS Router To Be Running
    Register With Local Cloud Server
    Check Correct MCS Password And ID For Local Cloud Saved
    Check Cloud Server Log Does Not Contain  Processing deployment api call
    Check Cloud Server Log Contains  Register with ::ThisIsARegToken

    Wait Until Created  ${MCS_CONFIG}  timeout=20 seconds
    Wait Until Created  ${MCS_POLICY_CONFIG}  timeout=20 seconds

    Wait Until Keyword Succeeds
    ...  5s
    ...  1s
    ...  Mcs Config Has Key  MCSID

    ${original_mcsid}  get_value_from_ini_file  MCSID  ${MCS_CONFIG}

    Wait Until Keyword Succeeds
    ...  5s
    ...  1s
    ...  Mcs Config Has Key  jwt_token

    ${original_jwt_token}  get_value_from_ini_file  jwt_token  ${MCS_CONFIG}

    Log File  ${MCS_POLICY_CONFIG}
    ${mcs_policy_config_ts_orig}    Get Modified Time   ${MCS_POLICY_CONFIG}

     Log File  ${SOPHOS_INSTALL}/base/etc/mcs.config
     ${mcs_root_config_ts_orig}    Get Modified Time   ${SOPHOS_INSTALL}/base/etc/mcs.config

     Log File  ${MCS_CONFIG}
     ${mcs_config_content_orig}   get file   ${MCS_CONFIG}

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

    File Should Not Exist  ${MCS_TMP_DIR}/migration_action.xml

    Wait Until Keyword Succeeds
    ...  60s
    ...  5s
    ...  Check MCS Router Log Contains  Migration request failed:

    Wait Until Created  ${MCS_CONFIG}  timeout=20 seconds
    Wait Until Created  ${MCS_POLICY_CONFIG}  timeout=20 seconds

    Log File  ${MCS_POLICY_CONFIG}
    ${mcs_policy_config_ts_new}    Get Modified Time   ${MCS_POLICY_CONFIG}

    Log File  ${MCS_CONFIG}
    ${mcs_config_ts_new}    Get Modified Time   ${MCS_CONFIG}

    Log File  ${SOPHOS_INSTALL}/base/etc/mcs.config
    ${mcs_root_config_ts_new}    Get Modified Time   ${SOPHOS_INSTALL}/base/etc/mcs.config

    Should Be Equal As Strings  ${mcs_policy_config_ts_new}  ${mcs_policy_config_ts_orig}
    Should Be Equal As Strings  ${mcs_root_config_ts_new}  ${mcs_root_config_ts_orig}

    Log File  ${MCS_CONFIG}
    ${mcs_config_content_new}   get file   ${MCS_CONFIG}
    Should Be Equal As Strings  ${mcs_config_content_new}  ${mcs_config_content_orig}

    ${new_mcsid}  get_value_from_ini_file  MCSID  ${MCS_CONFIG}
    ${new_jwt_token}  get_value_from_ini_file  jwt_token  ${MCS_CONFIG}

    Should Be Equal As Strings  ${original_mcsid}  ${new_mcsid}
    Should Be Equal As Strings  ${original_jwt_token}  ${new_jwt_token}

    File Should Exist  ${MCS_DIR}/action/garbage_action
    File Should Exist  ${MCS_DIR}/policy/garbage_policy
    File Should Exist  ${SOPHOS_INSTALL}/base/etc/sophosspl/current_proxy

    # Check push connection has been re-established
    ${push_server_address} =  Set Variable  localhost:4443
    #TODO renable LINUXDAR-5589
#    Wait Until Keyword Succeeds
#    ...  30 secs
#    ...  5 secs
#    ...  Check Log Contains String N Times   ${SOPHOS_INSTALL}/logs/base/sophosspl/mcsrouter.log   MCS Router Log   Established MCS Push Connection   1
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  5 secs
    ...  Check MCSRouter Log Contains   Push client successfully connected to ${push_server_address} directly


Migrate From Account With Message Relay To One Without
    Start Simple Proxy Server    3333
    Start Local Cloud Server  --initial-mcs-policy  ${SUPPORT_FILES}/CentralXml/MCS_policy_with_proxy.xml
    Start MCS Push Server
    Wait For MCS Router To Be Running
    Register With Local Cloud Server
    Check Correct MCS Password And ID For Local Cloud Saved
    Check Cloud Server Log Does Not Contain  Processing deployment api call
    Check Cloud Server Log Contains  Register with ::ThisIsARegToken

    Wait Until Created  ${MCS_CONFIG}    20s
    Wait Until Created  ${MCS_POLICY_CONFIG}    20s
    Wait Until Keyword Succeeds    5s     1s    Mcs Config Has Key  MCSID
    ${original_mcsid}  get_value_from_ini_file  MCSID  ${MCS_CONFIG}

    Wait Until Keyword Succeeds    10s    1s    Mcs Config Has Key  jwt_token
    ${original_jwt_token}  get_value_from_ini_file  jwt_token  ${MCS_CONFIG}
    Wait Until Created    ${SHS_STATUS_FILE}    timeout=2 minutes

    Log File  ${MCS_POLICY_CONFIG}
    Log File  ${MCS_CONFIG}
    Log File  ${SOPHOS_INSTALL}/base/etc/mcs.config
    Log File  ${SOPHOS_INSTALL}/base/etc/sophosspl/current_proxy

    ${mcs_router_mark} =  mark_log_size  ${MCS_ROUTER_LOG}
    Trigger Migration Now
    wait_for_log_contains_from_mark  ${mcs_router_mark}  Attempting Central migration   40
    wait_for_log_contains_from_mark  ${mcs_router_mark}  Successfully migrated Sophos Central account   20

    # Instruct fake central to send us an MCS policy with no MRs in.
    Send Policy File  mcs  ${SUPPORT_FILES}/CentralXml/MCS_Push_Policy_PushFallbackPoll.xml
    wait_for_log_contains_from_mark  ${mcs_router_mark}  Checking for updates to mcs flags   20

    Wait Until Created  ${MCS_CONFIG}    20s
    Wait Until Created  ${MCS_POLICY_CONFIG}    20s

    Log File  ${MCS_POLICY_CONFIG}
    Log File  ${MCS_CONFIG}
    Log File  ${SOPHOS_INSTALL}/base/etc/mcs.config
    Log File  ${SOPHOS_INSTALL}/base/etc/sophosspl/current_proxy

    ${new_mcsid}  get_value_from_ini_file  MCSID  ${MCS_CONFIG}
    Should Not Be Equal As Strings  ${original_mcsid}  ${new_mcsid}

    Wait Until Keyword Succeeds    10s    1s    JWT token has changed  ${original_jwt_token}

    Wait Until Keyword Succeeds    60s    5s    Check Envelope Log Contains  /statuses/endpoint/${new_mcsid}
    Check Log Contains In Order
    ...  ${MCS_ENVELOPE_LOG}
    ...  /statuses/endpoint/${new_mcsid}
    ...  <status><appId>MCS</appId>
    ...  <status><appId>ALC</appId>
    ...  <status><appId>SDU</appId>
    ...  <status><appId>SHS</appId>
    ...  <status><appId>APPSPROXY</appId>
    ...  <status><appId>AGENT</appId>

    wait_for_log_contains_from_mark  ${mcs_router_mark}        Push client successfully connected to localhost:4443 directly   30

    # Make sure that the current_proxy file is empty now that we have switched to a different account with no MRs
    ${current_proxy} =  Get File  ${SOPHOS_INSTALL}/base/etc/sophosspl/current_proxy
    Should Be Equal As Strings    ${current_proxy}    {}

*** Keywords ***
JWT token has changed
    [Arguments]  ${old_token}
    ${new_jwt_token}  get_value_from_ini_file  jwt_token  ${MCS_CONFIG}
    Should Not Be Equal As Strings  ${old_token}  ${new_jwt_token}

Mcs Config Has Key
    [Arguments]  ${key}
    get_value_from_ini_file  ${key}  ${MCS_CONFIG}

Backup Version Ini
    Copy File  ${SOPHOS_INSTALL}/base/VERSION.ini  /tmp/VERSION.ini.bk

Restore Version Ini
    Move File  /tmp/VERSION.ini.bk  ${SOPHOS_INSTALL}/base/VERSION.ini