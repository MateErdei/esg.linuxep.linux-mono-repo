*** Settings ***
Documentation   Tests that check WD will reconfigure the products user and group IDs to match what the user requested.

Library    Process
Library    OperatingSystem
Library    ${LIBS_DIRECTORY}/FullInstallerUtils.py
Library    ${LIBS_DIRECTORY}/Watchdog.py
Library    ${LIBS_DIRECTORY}/UserAndGroupReconfigurationUtils.py

Resource  ../installer/InstallerResources.robot
Resource  WatchdogResources.robot

Test Setup  Require Fresh Install

*** Test Cases ***
Test Watchdog Reconfigures User and Group IDs
    Wait For Base Processes To Be Running
    Verify Watchdog Actual User Group ID File
    ${ids_before} =    Get User IDs of Installed Files

    # Install time IDs
    # Users
    ${sspl_user_uid_before} =    Get UID From Username    sophos-spl-user
    ${sspl_local_uid_before} =    Get UID From Username    sophos-spl-local
    ${sspl_update_uid_before} =    Get UID From Username    sophos-spl-updatescheduler
    # Groups
    ${sophos_spl_group_gid_before} =    get_gid_from_groupname    sophos-spl-group
    ${sophos_spl_ipc_gid_before} =    get_gid_from_groupname    sophos-spl-ipc

    # IDs we will request to change to
    # Users
    # sophos-spl-user 1996
    ${sspl_user_uid_requested} =  Set Variable  1996
    # sophos-spl-local 1995
    ${sspl_local_uid_requested} =  Set Variable  1995
    # sophos-spl-updatescheduler 1994
    ${sspl_update_uid_requested} =  Set Variable  1994
    # Groups
    # sophos-spl-group 1996
    ${sophos_spl_group_gid_requested} =  Set Variable  1996
    # sophos-spl-ipc 1995
    ${sophos_spl_ipc_gid_requested} =  Set Variable  1995

    # Perform the ID changes requests by writing requested IDs json file and restarting the product
    ${requested_user_and_group_ids} =  Get File  ${SUPPORT_FILES}/watchdog/requested_user_group_ids.json
    Append To File  ${WD_REQUESTED_USER_GROUP_IDS}   ${requested_user_and_group_ids}
    Restart Product
    Wait For Base Processes To Be Running

    # Check file after
    ${ids_after} =    Get User IDs of Installed Files

    check_changes_of_user_ids   ${ids_before}    ${ids_after}    ${sspl_user_uid_before}    ${sspl_user_uid_requested}
    check_changes_of_user_ids   ${ids_before}    ${ids_after}    ${sspl_local_uid_before}    ${sspl_local_uid_requested}
    check_changes_of_user_ids   ${ids_before}    ${ids_after}    ${sspl_update_uid_before}    ${sspl_update_uid_requested}

    check_changes_of_group_ids   ${ids_before}    ${ids_after}    ${sophos_spl_group_gid_before}    ${sophos_spl_group_gid_requested}
    check_changes_of_group_ids   ${ids_before}    ${ids_after}    ${sophos_spl_ipc_gid_before}    ${sophos_spl_ipc_gid_requested}

    # Check product UIDs and GIDs
    # Users after
    ${sspl_user_uid_after} =    Get UID From Username    sophos-spl-user
    ${sspl_local_uid_after} =    Get UID From Username    sophos-spl-local
    ${sspl_update_uid_after} =    Get UID From Username    sophos-spl-updatescheduler
    # Groups after
    ${sophos_spl_group_gid_after} =    get_gid_from_groupname    sophos-spl-group
    ${sophos_spl_ipc_gid_after} =    get_gid_from_groupname    sophos-spl-ipc

    Should Be Equal As Strings    ${sspl_user_uid_after}    ${sspl_user_uid_requested}
    Should Be Equal As Strings    ${sspl_local_uid_after}    ${sspl_local_uid_requested}
    Should Be Equal As Strings    ${sspl_update_uid_after}    ${sspl_update_uid_requested}

    Should Be Equal As Strings    ${sophos_spl_group_gid_after}    ${sophos_spl_group_gid_requested}
    Should Be Equal As Strings    ${sophos_spl_ipc_gid_after}    ${sophos_spl_ipc_gid_requested}

    Check All Product Logs Do Not Contain Error
    Check All Product Logs Do Not Contain Critical


Test Watchdog Can Reconfigure a Singular User ID
    Wait For Base Processes To Be Running
    Verify Watchdog Actual User Group ID File
    ${ids_before} =    Get User IDs of Installed Files

    # Install time IDs
    # Users
    ${sspl_user_uid_before} =    Get UID From Username    sophos-spl-user
    ${sspl_local_uid_before} =    Get UID From Username    sophos-spl-local
    ${sspl_update_uid_before} =    Get UID From Username    sophos-spl-updatescheduler
    # Groups
    ${sophos_spl_group_gid_before} =    get_gid_from_groupname    sophos-spl-group
    ${sophos_spl_ipc_gid_before} =    get_gid_from_groupname    sophos-spl-ipc

    # ID we will request to change to
    # sophos-spl-user 1996
    ${sspl_user_uid_requested} =  Set Variable  1996

    # Perform the ID changes requests by writing requested IDs json file and restarting the product
    Append To File  ${WD_REQUESTED_USER_GROUP_IDS}    {"users":{"sophos-spl-user":1996}}
    Restart Product
    Wait For Base Processes To Be Running

    # Check file after
    ${ids_after} =    Get User IDs of Installed Files

    check_changes_of_user_ids   ${ids_before}    ${ids_after}    ${sspl_user_uid_before}    ${sspl_user_uid_requested}

    # Remaining IDs should have stayed the same
    check_changes_of_user_ids   ${ids_before}    ${ids_after}    ${sspl_local_uid_before}    ${sspl_local_uid_before}
    check_changes_of_user_ids   ${ids_before}    ${ids_after}    ${sspl_update_uid_before}    ${sspl_update_uid_before}
    check_changes_of_group_ids   ${ids_before}    ${ids_after}    ${sophos_spl_group_gid_before}    ${sophos_spl_group_gid_before}
    check_changes_of_group_ids   ${ids_before}    ${ids_after}    ${sophos_spl_ipc_gid_before}    ${sophos_spl_ipc_gid_before}

    # Check product UIDs and GIDs
    # Users after
    ${sspl_user_uid_after} =    Get UID From Username    sophos-spl-user
    Should Be Equal As Strings    ${sspl_user_uid_after}    ${sspl_user_uid_requested}

    Check All Product Logs Do Not Contain Error
    Check All Product Logs Do Not Contain Critical

Test Watchdog Can Reconfigure a Singular Group ID
    Wait For Base Processes To Be Running
    Verify Watchdog Actual User Group ID File
    ${ids_before} =    Get User IDs of Installed Files

    # Install time IDs
    # Users
    ${sspl_user_uid_before} =    Get UID From Username    sophos-spl-user
    ${sspl_local_uid_before} =    Get UID From Username    sophos-spl-local
    ${sspl_update_uid_before} =    Get UID From Username    sophos-spl-updatescheduler
    # Groups
    ${sophos_spl_group_gid_before} =    get_gid_from_groupname    sophos-spl-group
    ${sophos_spl_ipc_gid_before} =    get_gid_from_groupname    sophos-spl-ipc

    # IDs we will request to change to
    # Groups
    # sophos-spl-group 1996
    ${sophos_spl_group_gid_requested} =  Set Variable  1996

    # Perform the ID changes requests by writing requested IDs json file and restarting the product
    Append To File  ${WD_REQUESTED_USER_GROUP_IDS}   {"groups":{"sophos-spl-group":1996}}
    Restart Product
    Wait For Base Processes To Be Running

    # Check file after
    ${ids_after} =    Get User IDs of Installed Files

    check_changes_of_group_ids   ${ids_before}    ${ids_after}    ${sophos_spl_group_gid_before}    ${sophos_spl_group_gid_requested}

    # Remaining IDs should have stayed the same
    check_changes_of_user_ids   ${ids_before}    ${ids_after}    ${sspl_user_uid_before}    ${sspl_user_uid_before}
    check_changes_of_user_ids   ${ids_before}    ${ids_after}    ${sspl_local_uid_before}    ${sspl_local_uid_before}
    check_changes_of_user_ids   ${ids_before}    ${ids_after}    ${sspl_update_uid_before}    ${sspl_update_uid_before}
    check_changes_of_group_ids   ${ids_before}    ${ids_after}    ${sophos_spl_ipc_gid_before}    ${sophos_spl_ipc_gid_before}

    # Check product UIDs and GIDs
    # Groups after
    ${sophos_spl_group_gid_after} =    get_gid_from_groupname    sophos-spl-group
    Should Be Equal As Strings    ${sophos_spl_group_gid_after}    ${sophos_spl_group_gid_requested}

    Check All Product Logs Do Not Contain Error
    Check All Product Logs Do Not Contain Critical

*** Keywords ***

Verify Watchdog Actual User Group ID File
    Wait Until Keyword Succeeds
    ...  5 secs
    ...  1 secs
    ...  verify_watchdog_config

Get User IDs of Installed Files
    ${ids} =    get_user_and_group_ids_of_files    ${SOPHOS_INSTALL}
    [Return]    ${ids}
