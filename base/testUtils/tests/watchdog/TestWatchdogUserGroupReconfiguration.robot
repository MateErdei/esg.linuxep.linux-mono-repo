*** Settings ***
Documentation   Tests that check WD will reconfigure the products user and group IDs to match what the user requested.

Library    Process
Library    OperatingSystem
Library    ../../libs/FullInstallerUtils.py
Library    ../../libs/MCSRouter.py
Library    ../../libs/UserAndGroupReconfigurationUtils.py
Library    ../../libs/Watchdog.py

Resource  ../av_plugin/AVResources.robot
Resource  ../edr_plugin/EDRResources.robot
Resource  ../event_journaler/EventJournalerResources.robot
Resource  ../installer/InstallerResources.robot
Resource  ../mcs_router/McsRouterResources.robot
Resource  WatchdogResources.robot

Suite Setup    Suite Setup

Test Setup       Watchdog User Group Test Setup
Test Teardown       Watchdog User Group Test Teardown

Default Tags    TAP_PARALLEL4  EXCLUDE_ON_COVERAGE

*** Test Cases ***
Watchdog Actual User And Group Config Has Correct Ids After Installation
    Require Fresh Install
    Verify Watchdog Config

Test Watchdog Reconfigures User and Group IDs
    ${update_scheduler_mark} =    mark_log_size    ${SOPHOS_INSTALL}/logs/base/sophosspl/updatescheduler.log
    Run Full Installer
    ...    --mcs-url    https://localhost:4443/mcs
    ...    --mcs-token    ThisIsARegToken
    ...    --allow-override-mcs-ca
    ...    --log-level  DEBUG

    Create Directory    ${SOPHOS_INSTALL}/base/update/cache/sdds3primary    # Needed by watchdog to search for plugins
    ${ids_before} =    Get User IDs of Installed Files

    # Install time IDs
    # Users
    ${sspl_local_uid_before} =              Get UID From Username    sophos-spl-local
    ${sspl_update_uid_before} =             Get UID From Username    sophos-spl-updatescheduler
    ${sspl_user_uid_before} =               Get UID From Username    sophos-spl-user
    # Groups
    ${sophos_spl_group_gid_before} =        Get GID From Groupname    sophos-spl-group
    ${sophos_spl_ipc_gid_before} =          Get GID From Groupname    sophos-spl-ipc

    # IDs we will request to change to
    # Users
    # sophos-spl-local 1995
    ${sspl_local_uid_requested} =  Set Variable  1995
    # sophos-spl-updatescheduler 1994
    ${sspl_update_uid_requested} =  Set Variable  1994
    # sophos-spl-user 1996
    ${sspl_user_uid_requested}  Set Variable  1996
    # Groups
    # sophos-spl-group 1996
    ${sophos_spl_group_gid_requested} =  Set Variable  1996
    # sophos-spl-ipc 1995
    ${sophos_spl_ipc_gid_requested} =  Set Variable  1995

    # Perform the ID changes requests by writing requested IDs json file and restarting the product
    ${requested_user_and_group_ids} =  Get File  ${SUPPORT_FILES}/watchdog/requested_user_group_ids.json
    Append To File  ${WD_REQUESTED_USER_GROUP_IDS}   ${requested_user_and_group_ids}
    # waiting for updatescheduler to finish reporting update success to central otherwise watchdog might need to kill it when the restart is done
    wait_for_log_contains_from_mark    ${update_scheduler_mark}    Sending status to Central    ${5}
    Restart Product
    Wait For Base Processes To Be Running

    # Check file after
    ${ids_after} =    Get User IDs of Installed Files

    check_changes_of_user_ids   ${ids_before}    ${ids_after}    ${sspl_local_uid_before}              ${sspl_local_uid_requested}
    check_changes_of_user_ids   ${ids_before}    ${ids_after}    ${sspl_update_uid_before}             ${sspl_update_uid_requested}
    check_changes_of_user_ids   ${ids_before}    ${ids_after}    ${sspl_user_uid_before}               ${sspl_user_uid_requested}

    check_changes_of_group_ids   ${ids_before}    ${ids_after}    ${sophos_spl_group_gid_before}       ${sophos_spl_group_gid_requested}
    check_changes_of_group_ids   ${ids_before}    ${ids_after}    ${sophos_spl_ipc_gid_before}         ${sophos_spl_ipc_gid_requested}

    # Check product UIDs and GIDs
    # Users after
    ${sspl_local_uid_after} =              Get UID From Username    sophos-spl-local
    ${sspl_update_uid_after} =             Get UID From Username    sophos-spl-updatescheduler
    ${sspl_user_uid_after} =               Get UID From Username    sophos-spl-user
    # Groups after
    ${sophos_spl_group_gid_after} =        Get GID From Groupname    sophos-spl-group
    ${sophos_spl_ipc_gid_after} =          Get GID From Groupname    sophos-spl-ipc

    Should Be Equal As Strings    ${sspl_local_uid_after}              ${sspl_local_uid_requested}
    Should Be Equal As Strings    ${sspl_update_uid_after}             ${sspl_update_uid_requested}
    Should Be Equal As Strings    ${sspl_user_uid_after}               ${sspl_user_uid_requested}

    Should Be Equal As Strings    ${sophos_spl_group_gid_after}        ${sophos_spl_group_gid_requested}
    Should Be Equal As Strings    ${sophos_spl_ipc_gid_after}          ${sophos_spl_ipc_gid_requested}

    Verify Product is Running Without Error After ID Change

Test Watchdog Can Reconfigure a Singular User ID
    ${update_scheduler_mark} =    mark_log_size    ${SOPHOS_INSTALL}/logs/base/sophosspl/updatescheduler.log
    Run Full Installer
    ...    --mcs-url    https://localhost:4443/mcs
    ...    --mcs-token    ThisIsARegToken
    ...    --allow-override-mcs-ca
    ...    --log-level  DEBUG

    Create Directory    ${SOPHOS_INSTALL}/base/update/cache/sdds3primary    # Needed by watchdog to search for plugins
    ${ids_before} =    Get User IDs of Installed Files

    # Install time IDs
    # Users
    ${sspl_local_uid_before} =              Get UID From Username    sophos-spl-local
    ${sspl_update_uid_before} =             Get UID From Username    sophos-spl-updatescheduler
    ${sspl_user_uid_before} =               Get UID From Username    sophos-spl-user
    # Groups
    ${sophos_spl_group_gid_before} =        Get GID From Groupname    sophos-spl-group
    ${sophos_spl_ipc_gid_before} =          Get GID From Groupname    sophos-spl-ipc

    # User ID we will request to change to
    # sophos-spl-user 1996
    ${sspl_user_uid_requested}  Set Variable  1996

    # Perform the ID changes requests by writing requested IDs json file and restarting the product
    Append To File  ${WD_REQUESTED_USER_GROUP_IDS}    {"users":{"sophos-spl-user":1996}}
    # waiting for updatescheduler to finish reporting update success to central otherwise watchdog might need to kill it when the restart is done
    wait_for_log_contains_from_mark    ${update_scheduler_mark}    Sending status to Central    ${5}
    Restart Product
    Wait For Base Processes To Be Running

    # Check file after
    ${ids_after} =    Get User IDs of Installed Files

    check_changes_of_user_ids   ${ids_before}    ${ids_after}    ${sspl_user_uid_before}    ${sspl_user_uid_requested}

    # Remaining IDs should have stayed the same
    check_changes_of_user_ids   ${ids_before}    ${ids_after}    ${sspl_local_uid_before}              ${sspl_local_uid_before}
    check_changes_of_user_ids   ${ids_before}    ${ids_after}    ${sspl_update_uid_before}             ${sspl_update_uid_before}
    check_changes_of_user_ids   ${ids_before}    ${ids_after}    ${sspl_user_uid_before}               ${sspl_user_uid_requested}

    check_changes_of_group_ids   ${ids_before}    ${ids_after}    ${sophos_spl_group_gid_before}       ${sophos_spl_group_gid_before}
    check_changes_of_group_ids   ${ids_before}    ${ids_after}    ${sophos_spl_ipc_gid_before}         ${sophos_spl_ipc_gid_before}

    # Check product UID has updated
    ${sspl_user_uid_after} =    Get UID From Username    sophos-spl-user
    Should Be Equal As Strings    ${sspl_user_uid_after}    ${sspl_user_uid_requested}

    Verify Product is Running Without Error After ID Change

Test Watchdog Can Reconfigure a Singular Group ID
    ${update_scheduler_mark} =    mark_log_size    ${SOPHOS_INSTALL}/logs/base/sophosspl/updatescheduler.log
    Run Full Installer
    ...    --mcs-url    https://localhost:4443/mcs
    ...    --mcs-token    ThisIsARegToken
    ...    --allow-override-mcs-ca
    ...    --log-level  DEBUG

    Create Directory    ${SOPHOS_INSTALL}/base/update/cache/sdds3primary    # Needed by watchdog to search for plugins
    ${ids_before} =    Get User IDs of Installed Files

    # Install time IDs
    # Users
    ${sspl_local_uid_before} =              Get UID From Username    sophos-spl-local
    ${sspl_update_uid_before} =             Get UID From Username    sophos-spl-updatescheduler
    ${sspl_user_uid_before} =               Get UID From Username    sophos-spl-user
    # Groups
    ${sophos_spl_group_gid_before} =        Get GID From Groupname    sophos-spl-group
    ${sophos_spl_ipc_gid_before} =          Get GID From Groupname    sophos-spl-ipc

    # Group ID we will request to change to
    # sophos-spl-group 1996
    ${sophos_spl_group_gid_requested} =  Set Variable  1996

    # Perform the ID changes requests by writing requested IDs json file and restarting the product
    Append To File  ${WD_REQUESTED_USER_GROUP_IDS}   {"groups":{"sophos-spl-group":1996}}
    # waiting for updatescheduler to finish reporting update success to central otherwise watchdog might need to kill it when the restart is done
    wait_for_log_contains_from_mark    ${update_scheduler_mark}    Sending status to Central    ${5}
    Restart Product
    Wait For Base Processes To Be Running

    # Check file after
    ${ids_after} =    Get User IDs of Installed Files

    check_changes_of_group_ids   ${ids_before}    ${ids_after}    ${sophos_spl_group_gid_before}       ${sophos_spl_group_gid_requested}

    # Remaining IDs should have stayed the same
    check_changes_of_user_ids    ${ids_before}    ${ids_after}    ${sspl_local_uid_before}              ${sspl_local_uid_before}
    check_changes_of_user_ids    ${ids_before}    ${ids_after}    ${sspl_update_uid_before}             ${sspl_update_uid_before}
    check_changes_of_user_ids    ${ids_before}    ${ids_after}    ${sspl_user_uid_before}               ${sspl_user_uid_before}

    check_changes_of_group_ids   ${ids_before}    ${ids_after}    ${sophos_spl_ipc_gid_before}         ${sophos_spl_ipc_gid_before}

    # Check product GID has updated
    ${sophos_spl_group_gid_after} =        Get GID From Groupname    sophos-spl-group
    Should Be Equal As Strings    ${sophos_spl_group_gid_after}        ${sophos_spl_group_gid_requested}

    Verify Product is Running Without Error After ID Change

Custom User And Group IDs Are Written To Requested Config From ThinInstaller Args
    Copy File    ${SUPPORT_FILES}/watchdog/requested_user_group_ids_install_options    /tmp/InstallOptionsTestFile
    Set Environment Variable  INSTALL_OPTIONS_FILE  /tmp/InstallOptionsTestFile

    Run Full Installer Expecting Code  0

    Wait Until Keyword Succeeds
    ...    30 secs
    ...    2 secs
    ...    Log File    ${ETC_DIR}/install_options
    Wait Until Keyword Succeeds
    ...    60 secs
    ...    5 secs
    ...    Verify Requested Config Without Help Text    {"users":{"sophos-spl-local":1995,"sophos-spl-updatescheduler":1994,"sophos-spl-user":1996,"sophos-spl-av":1997,"sophos-spl-threat-detector":1998},"groups":{"sophos-spl-group":1996,"sophos-spl-ipc":1995}}
    Remove File    /tmp/InstallOptionsTestFile

Requested Config Created From ThinInstaller Args Is Used To Configure Users And Groups
    Copy File    ${SUPPORT_FILES}/watchdog/requested_user_group_ids_base_install_options    /tmp/InstallOptionsTestFile
    Set Environment Variable  INSTALL_OPTIONS_FILE  /tmp/InstallOptionsTestFile

    Run Full Installer Expecting Code  0
    Wait For Base Processes To Be Running

    # Check product UIDs and GIDs
    # Users
    #TODO LINUXDAR-4267 Adapt when this story is completed to ensure AV users are set on startup
    ${sspl_local_uid} =              Get UID From Username    sophos-spl-local
    ${sspl_update_uid} =             Get UID From Username    sophos-spl-updatescheduler
    ${sspl_user_uid} =               Get UID From Username    sophos-spl-user
    # Groups
    ${sophos_spl_group_gid} =        Get GID From Groupname    sophos-spl-group
    ${sophos_spl_ipc_gid} =          Get GID From Groupname    sophos-spl-ipc

    Should Be Equal As Strings    ${sspl_local_uid}              1995
    Should Be Equal As Strings    ${sspl_update_uid}             1994
    Should Be Equal As Strings    ${sspl_user_uid}               1996

    Should Be Equal As Strings    ${sophos_spl_group_gid}        1996
    Should Be Equal As Strings    ${sophos_spl_ipc_gid}          1995
    Remove File    /tmp/InstallOptionsTestFile

Custom User And Group IDs Are Used To Create SPL Users And Groups From ThinInstaller Args
    Copy File    ${SUPPORT_FILES}/watchdog/requested_user_group_ids_base_install_options    /tmp/InstallOptionsTestFile
    Set Environment Variable  INSTALL_OPTIONS_FILE  /tmp/InstallOptionsTestFile

    Run Full Installer Expecting Code  0
    Wait For Base Processes To Be Running
    Stripped Requested User Group Config Matches Actual Config

    ${sspl_local_uid} =              Get UID From Username    sophos-spl-local
    ${sspl_update_uid} =             Get UID From Username    sophos-spl-updatescheduler
    ${sspl_user_uid} =               Get UID From Username    sophos-spl-user
    # Groups
    ${sophos_spl_group_gid} =        Get GID From Groupname    sophos-spl-group
    ${sophos_spl_ipc_gid} =          Get GID From Groupname    sophos-spl-ipc

    Should Be Equal As Strings    ${sspl_local_uid}              1995
    Should Be Equal As Strings    ${sspl_update_uid}             1994
    Should Be Equal As Strings    ${sspl_user_uid}               1996

    Should Be Equal As Strings    ${sophos_spl_group_gid}        1996
    Should Be Equal As Strings    ${sophos_spl_ipc_gid}          1995

    Remove File    /tmp/InstallOptionsTestFile

*** Keywords ***
Suite Setup
    Regenerate Certificates

Watchdog User Group Test Setup
    Require Uninstalled
    Set Local CA Environment Variable
    Override LogConf File as Global Level  DEBUG
    Cleanup Local Cloud Server Logs
    Start Local Cloud Server

Watchdog User Group Test Teardown
    General Test Teardown
    dump_cloud_server_log
    Require Uninstalled
    Stop Local Cloud Server

Get User IDs of Installed Files
    ${ids} =    get_user_and_group_ids_of_files    ${SOPHOS_INSTALL}
    [Return]    ${ids}

Verify Product is Running Without Error After ID Change
    # SulDownloader will try to connect to https://sustest.sophosupd.com and fail to authenticate
    Wait Until Created    ${SOPHOS_INSTALL}/logs/base/suldownloader.log
    Mark Expected Error In Log    ${SOPHOS_INSTALL}/logs/base/suldownloader.log    Failed to connect to repository: SUS request failed with error: Couldn't resolve host name
    Mark Expected Error In Log    ${SOPHOS_INSTALL}/logs/base/suldownloader.log    Failed to connect to repository: SUS request failed to connect to the server with error: Couldn't resolve host name
    Mark Expected Error In Log    ${SOPHOS_INSTALL}/logs/base/suldownloader.log    Failed to connect to repository: SUS request failed with error: Couldn't connect to server

    Wait Until Created    ${SOPHOS_INSTALL}/logs/base/sophosspl/updatescheduler.log
    Mark Expected Error In Log       ${SOPHOS_INSTALL}/logs/base/sophosspl/updatescheduler.log   Update Service (sophos-spl-update.service) failed.

    Check All Product Logs Do Not Contain Error
    Check All Product Logs Do Not Contain Critical
