*** Settings ***
Library    ${LIBS_DIRECTORY}/MCSRouter.py
Library    ${LIBS_DIRECTORY}/UserAndGroupReconfigurationUtils.py

Resource    ${COMMON_TEST_ROBOT}/ProductAcceptanceTestsResources.robot
Resource    ${COMMON_TEST_ROBOT}/UpgradeResources.robot
Resource    ${COMMON_TEST_ROBOT}/WatchdogResources.robot

Suite Setup      Upgrade Resources Suite Setup
Suite Teardown   Upgrade Resources Suite Teardown

Test Setup       Require Uninstalled
Test Teardown    Upgrade Resources SDDS3 Test Teardown

# TODO LINUXDAR-7327: Fix and re-enable for SLES15
Force Tags  LOAD9    SMOKE    EXCLUDE_SLES15

*** Test Cases ***
Reconfigure All Sophos Users And Groups In Installed Product
    [Tags]    WATCHDOG

    # Install VUT product
    Start Local Cloud Server
    ${handle} =    Start Local SDDS3 Server
    Set Suite Variable    ${GL_handle}    ${handle}
    ${update_scheduler_mark} =    mark_log_size    ${SOPHOS_INSTALL}/logs/base/sophosspl/updatescheduler.log
    ${sul_mark} =    mark_log_size    ${SULDOWNLOADER_LOG_PATH}
    Configure And Run SDDS3 Thininstaller    0    https://localhost:8080    https://localhost:8080
    Override LogConf File as Global Level    DEBUG
    wait_for_log_contains_from_mark    ${sul_mark}    Update success    150

    Verify Watchdog Actual User Group ID File
    ${ids_before} =    Get User And Group Ids Of Files    ${SOPHOS_INSTALL}

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
    # waiting for updatescheduler to finish reporting update success to central otherwise watchdog might need to kill it when the restart is done
    wait_for_log_contains_from_mark    ${update_scheduler_mark}    Sending status to Central    ${10}
    Restart Product
    Wait for All Processes To Be Running

    # Check file after
    ${ids_after} =    Get User And Group Ids Of Files    ${SOPHOS_INSTALL}

    Check Changes Of User Ids   ${ids_before}    ${ids_after}    ${sspl_av_uid_before}                 ${sspl_av_uid_requested}
    Check Changes Of User Ids   ${ids_before}    ${ids_after}    ${sspl_local_uid_before}              ${sspl_local_uid_requested}
    Check Changes Of User Ids   ${ids_before}    ${ids_after}    ${sspl_threat_detector_uid_before}    ${sspl_threat_detector_uid_requested}
    Check Changes Of User Ids   ${ids_before}    ${ids_after}    ${sspl_update_uid_before}             ${sspl_update_uid_requested}
    Check Changes Of User Ids   ${ids_before}    ${ids_after}    ${sspl_user_uid_before}               ${sspl_user_uid_requested}

    Check Changes Of Group Ids   ${ids_before}    ${ids_after}    ${sophos_spl_group_gid_before}       ${sophos_spl_group_gid_requested}
    Check Changes Of Group Ids   ${ids_before}    ${ids_after}    ${sophos_spl_ipc_gid_before}         ${sophos_spl_ipc_gid_requested}

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

Reconfigure All Sophos Users And Groups When Installing Product Using Thin Installer
    [Tags]    THININSTALLER

    Start Local Cloud Server
    ${handle} =    Start Local SDDS3 Server
    Set Suite Variable    ${GL_handle}    ${handle}

    ${args} =    Catenate
    ...    --user-ids-to-configure=sophos-spl-local:1995,sophos-spl-updatescheduler:1994,sophos-spl-user:1996,sophos-spl-av:1997,sophos-spl-threat-detector:1998
    ...    --group-ids-to-configure=sophos-spl-group:1996,sophos-spl-ipc:1995

    ${sul_mark} =    mark_log_size    ${SULDOWNLOADER_LOG_PATH}
    Configure And Run SDDS3 Thininstaller    0    https://localhost:8080    https://localhost:8080    args=${args}
    Override LogConf File as Global Level    DEBUG
    wait_for_log_contains_from_mark    ${sul_mark}    Update success    150

    Verify Watchdog Actual User Group ID File

    Wait for All Processes To Be Running

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


*** Keywords ***
Verify Product is Running Without Error After ID Change
    #LINUXDAR-4015 There won't be a fix for this error, please check the ticket for more info
    Mark Expected Error In Log       ${SOPHOS_INSTALL}/plugins/runtimedetections/log/runtimedetections.log    runtimedetections <> Could not enter supervised child process

    Check All Product Logs Do Not Contain Error
    Check All Product Logs Do Not Contain Critical
