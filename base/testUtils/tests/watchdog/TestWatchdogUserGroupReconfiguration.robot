*** Settings ***
Documentation   Tests that check WD will reconfigure the products user and group IDs to match what the user requested.

Library    Process
Library    OperatingSystem
Library    ${LIBS_DIRECTORY}/FullInstallerUtils.py
Library    ${LIBS_DIRECTORY}/MCSRouter.py
Library    ${LIBS_DIRECTORY}/UserAndGroupReconfigurationUtils.py
Library    ${LIBS_DIRECTORY}/Watchdog.py

Resource  ../av_plugin/AVResources.robot
Resource  ../edr_plugin/EDRResources.robot
Resource  ../event_journaler/EventJournalerResources.robot
Resource  ../installer/InstallerResources.robot
Resource  ../mcs_router/McsRouterResources.robot
Resource  WatchdogResources.robot

Test Setup       Watchdog User Group Test Setup
Test Teardown       Watchdog User Group Test Teardown

*** Test Cases ***
Test Watchdog Reconfigures User and Group IDs
    Require Fresh Install
    Install Plugins And Setup Local Cloud
    Verify Watchdog Actual User Group ID File
    ${ids_before} =    Get User IDs of Installed Files

    # Install time IDs
    # Users
    ${sspl_av_uid_before} =                 Get UID From Username    sophos-spl-av
    ${sspl_local_uid_before} =              Get UID From Username    sophos-spl-local
    ${sspl_threat_detector_uid_before} =    Get UID From Username    sophos-spl-threat-detector
    ${sspl_update_uid_before} =             Get UID From Username    sophos-spl-updatescheduler
    ${sspl_user_uid_before} =               Get UID From Username    sophos-spl-user
    # Groups
    ${sophos_spl_group_gid_before} =        Get GID From Groupname    sophos-spl-group
    ${sophos_spl_ipc_gid_before} =          Get GID From Groupname    sophos-spl-ipc

    # IDs we will request to change to
    # Users
    # sophos-spl-av 1997
    ${sspl_av_uid_requested} =  Set Variable    1997
    # sophos-spl-local 1995
    ${sspl_local_uid_requested} =  Set Variable  1995
    # sophos-spl-threat-detector 1998
    ${sspl_threat_detector_uid_requested} =  Set Variable    1998
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
    Restart Product
    Wait for All Processes To Be Running

    # Check file after
    ${ids_after} =    Get User IDs of Installed Files

    check_changes_of_user_ids   ${ids_before}    ${ids_after}    ${sspl_av_uid_before}                 ${sspl_av_uid_requested}
    check_changes_of_user_ids   ${ids_before}    ${ids_after}    ${sspl_local_uid_before}              ${sspl_local_uid_requested}
    check_changes_of_user_ids   ${ids_before}    ${ids_after}    ${sspl_threat_detector_uid_before}    ${sspl_threat_detector_uid_requested}
    check_changes_of_user_ids   ${ids_before}    ${ids_after}    ${sspl_update_uid_before}             ${sspl_update_uid_requested}
    check_changes_of_user_ids   ${ids_before}    ${ids_after}    ${sspl_user_uid_before}               ${sspl_user_uid_requested}

    check_changes_of_group_ids   ${ids_before}    ${ids_after}    ${sophos_spl_group_gid_before}       ${sophos_spl_group_gid_requested}
    check_changes_of_group_ids   ${ids_before}    ${ids_after}    ${sophos_spl_ipc_gid_before}         ${sophos_spl_ipc_gid_requested}

    # Check product UIDs and GIDs
    # Users after
    ${sspl_av_uid_after} =                 Get UID From Username    sophos-spl-av
    ${sspl_local_uid_after} =              Get UID From Username    sophos-spl-local
    ${sspl_threat_detector_uid_after} =    Get UID From Username    sophos-spl-threat-detector
    ${sspl_update_uid_after} =             Get UID From Username    sophos-spl-updatescheduler
    ${sspl_user_uid_after} =               Get UID From Username    sophos-spl-user
    # Groups after
    ${sophos_spl_group_gid_after} =        Get GID From Groupname    sophos-spl-group
    ${sophos_spl_ipc_gid_after} =          Get GID From Groupname    sophos-spl-ipc

    Should Be Equal As Strings    ${sspl_av_uid_after}                 ${sspl_av_uid_requested}
    Should Be Equal As Strings    ${sspl_local_uid_after}              ${sspl_local_uid_requested}
    Should Be Equal As Strings    ${sspl_threat_detector_uid_after}    ${sspl_threat_detector_uid_requested}
    Should Be Equal As Strings    ${sspl_update_uid_after}             ${sspl_update_uid_requested}
    Should Be Equal As Strings    ${sspl_user_uid_after}               ${sspl_user_uid_requested}

    Should Be Equal As Strings    ${sophos_spl_group_gid_after}        ${sophos_spl_group_gid_requested}
    Should Be Equal As Strings    ${sophos_spl_ipc_gid_after}          ${sophos_spl_ipc_gid_requested}

    Verify Product is Running Without Error After ID Change


Test Watchdog Can Reconfigure a Singular User ID
    Require Fresh Install
    Install Plugins And Setup Local Cloud
    Verify Watchdog Actual User Group ID File
    ${ids_before} =    Get User IDs of Installed Files

    # Install time IDs
    # Users
    ${sspl_av_uid_before} =                 Get UID From Username    sophos-spl-av
    ${sspl_local_uid_before} =              Get UID From Username    sophos-spl-local
    ${sspl_threat_detector_uid_before} =    Get UID From Username    sophos-spl-threat-detector
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
    Restart Product
    Wait for All Processes To Be Running

    # Check file after
    ${ids_after} =    Get User IDs of Installed Files

    check_changes_of_user_ids   ${ids_before}    ${ids_after}    ${sspl_user_uid_before}    ${sspl_user_uid_requested}

    # Remaining IDs should have stayed the same
    check_changes_of_user_ids   ${ids_before}    ${ids_after}    ${sspl_av_uid_before}                 ${sspl_av_uid_before}
    check_changes_of_user_ids   ${ids_before}    ${ids_after}    ${sspl_local_uid_before}              ${sspl_local_uid_before}
    check_changes_of_user_ids   ${ids_before}    ${ids_after}    ${sspl_threat_detector_uid_before}    ${sspl_threat_detector_uid_before}
    check_changes_of_user_ids   ${ids_before}    ${ids_after}    ${sspl_update_uid_before}             ${sspl_update_uid_before}
    check_changes_of_user_ids   ${ids_before}    ${ids_after}    ${sspl_user_uid_before}               ${sspl_user_uid_requested}

    check_changes_of_group_ids   ${ids_before}    ${ids_after}    ${sophos_spl_group_gid_before}       ${sophos_spl_group_gid_before}
    check_changes_of_group_ids   ${ids_before}    ${ids_after}    ${sophos_spl_ipc_gid_before}         ${sophos_spl_ipc_gid_before}

    # Check product UID has updated
    ${sspl_user_uid_after} =    Get UID From Username    sophos-spl-user
    Should Be Equal As Strings    ${sspl_user_uid_after}    ${sspl_user_uid_requested}

    Verify Product is Running Without Error After ID Change

Test Watchdog Can Reconfigure a Singular Group ID
    Require Fresh Install
    Install Plugins And Setup Local Cloud
    Verify Watchdog Actual User Group ID File
    ${ids_before} =    Get User IDs of Installed Files

    # Install time IDs
    # Users
    ${sspl_av_uid_before} =                 Get UID From Username    sophos-spl-av
    ${sspl_local_uid_before} =              Get UID From Username    sophos-spl-local
    ${sspl_threat_detector_uid_before} =    Get UID From Username    sophos-spl-threat-detector
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
    Restart Product
    Wait for All Processes To Be Running

    # Check file after
    ${ids_after} =    Get User IDs of Installed Files

    check_changes_of_group_ids   ${ids_before}    ${ids_after}    ${sophos_spl_group_gid_before}       ${sophos_spl_group_gid_requested}

    # Remaining IDs should have stayed the same
    check_changes_of_user_ids    ${ids_before}    ${ids_after}    ${sspl_av_uid_before}                 ${sspl_av_uid_before}
    check_changes_of_user_ids    ${ids_before}    ${ids_after}    ${sspl_local_uid_before}              ${sspl_local_uid_before}
    check_changes_of_user_ids    ${ids_before}    ${ids_after}    ${sspl_threat_detector_uid_before}    ${sspl_threat_detector_uid_before}
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
    Copy File    ${SUPPORT_FILES}/watchdog/requested_user_group_ids_install_options    /tmp/InstallOptionsTestFile
    Set Environment Variable  INSTALL_OPTIONS_FILE  /tmp/InstallOptionsTestFile

    Run Full Installer Expecting Code  0
    Wait For Base Processes To Be Running

    # Check product UIDs and GIDs
    # Users
    #TODO LINUXDAR-2972 Adapt when this ticket is implemented to ensure AV users are set on startup
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
    Copy File    ${SUPPORT_FILES}/watchdog/requested_user_group_ids_install_options    /tmp/InstallOptionsTestFile
    Set Environment Variable  INSTALL_OPTIONS_FILE  /tmp/InstallOptionsTestFile

    Run Full Installer Expecting Code  0

    Install Plugins And Setup Local Cloud
    Wait for All Processes To Be Running
    Stripped Requested User Group Config Matches Actual Config
    
    ${sspl_av_uid} =                 Get UID From Username    sophos-spl-av
    ${sspl_local_uid} =              Get UID From Username    sophos-spl-local
    ${sspl_threat_detector_uid} =    Get UID From Username    sophos-spl-threat-detector
    ${sspl_update_uid} =             Get UID From Username    sophos-spl-updatescheduler
    ${sspl_user_uid} =               Get UID From Username    sophos-spl-user
    # Groups
    ${sophos_spl_group_gid} =        Get GID From Groupname    sophos-spl-group
    ${sophos_spl_ipc_gid} =          Get GID From Groupname    sophos-spl-ipc

    Should Be Equal As Strings    ${sspl_av_uid}                 1997
    Should Be Equal As Strings    ${sspl_local_uid}              1995
    Should Be Equal As Strings    ${sspl_threat_detector_uid}    1998
    Should Be Equal As Strings    ${sspl_update_uid}             1994
    Should Be Equal As Strings    ${sspl_user_uid}               1996

    Should Be Equal As Strings    ${sophos_spl_group_gid}        1996
    Should Be Equal As Strings    ${sophos_spl_ipc_gid}          1995

    Verify Product is Running Without Error After ID Change
    Remove File    /tmp/InstallOptionsTestFile

*** Keywords ***
Watchdog User Group Test Setup
    Require Uninstalled
    Override LogConf File as Global Level  DEBUG
    Cleanup Local Cloud Server Logs

Watchdog User Group Test Teardown
    General Test Teardown
    dump_cloud_server_log
    Require Uninstalled
    Stop Local Cloud Server

Install Plugins And Setup Local Cloud
    Regenerate Certificates
    Set Local CA Environment Variable
    Start Local Cloud Server
    Copy File  ${SUPPORT_FILES}/CentralXml/FLAGS_xdr_enabled.json  ${SOPHOS_INSTALL}/base/etc/sophosspl/flags-warehouse.json
    ${result} =  Run Process  chown  root:sophos-spl-group  ${SOPHOS_INSTALL}/base/etc/sophosspl/flags-warehouse.json
    Should Be Equal As Strings  0  ${result.rc}
    Register With Fake Cloud
    Install EDR Directly
    Create Query Packs
    Restart EDR Plugin
    Register Cleanup   Cleanup Query Packs
    Install Event Journaler Directly
    Install AV Plugin Directly

Wait for All Processes To Be Running
    Wait For Base Processes To Be Running
    Wait For EDR to be Installed
    Check Event Journaler Installed
    Check AV Plugin Installed Directly

Verify Watchdog Actual User Group ID File
    Wait Until Keyword Succeeds
    ...  5 secs
    ...  1 secs
    ...  verify_watchdog_config    expect_all_users=${True}

Get User IDs of Installed Files
    ${ids} =    get_user_and_group_ids_of_files    ${SOPHOS_INSTALL}
    [Return]    ${ids}

Verify Product is Running Without Error After ID Change
    Mark Expected Error In Log       ${SOPHOS_INSTALL}/logs/base/watchdog.log    ProcessMonitoringImpl <> /opt/sophos-spl/base/bin/mcsrouter died with exit code 1
    Mark Expected Error In Log       ${SOPHOS_INSTALL}/logs/base/sophosspl/sophos_managementagent.log    Failure on sending message to updatescheduler. Reason: No incoming data
    Mark Expected Error In Log       ${SOPHOS_INSTALL}/logs/base/sophosspl/mcsrouter.log    mcsrouter already running
    Mark Expected Critical In Log    ${SOPHOS_INSTALL}/logs/base/sophosspl/mcsrouter.log    Not registered: MCSID is not present
    Mark Expected Error In Log       ${SOPHOS_INSTALL}/logs/base/sophosspl/updatescheduler.log   Update Service (sophos-spl-update.service) failed.
    Mark Expected Error In Log       ${SOPHOS_INSTALL}/logs/base/suldownloader.log    TOKEN_HEADER_ERROR
    Mark Expected Error In Log       ${SOPHOS_INSTALL}/logs/base/suldownloader.log    Failed to connect to repository: SUS request received HTTP response code: 403 but was expecting: 200
    Mark Expected Error In Log       ${SOPHOS_INSTALL}/plugins/edr/log/edr.log    Failed to find query pack to extract scheduled query tags from
    Mark Expected Error In Log       ${SOPHOS_INSTALL}/plugins/edr/log/edr.log    Failed to set query packs to the correct version

    Check All Product Logs Do Not Contain Error
    Check All Product Logs Do Not Contain Critical
