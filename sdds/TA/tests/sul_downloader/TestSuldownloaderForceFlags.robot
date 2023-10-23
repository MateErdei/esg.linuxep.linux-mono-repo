*** Settings ***
Suite Setup      Upgrade Resources Suite Setup
Suite Teardown   Upgrade Resources Suite Teardown

Test Setup       Require Uninstalled
Test Teardown    Upgrade Resources SDDS3 Test Teardown

Library    DateTime
Library     ${LIBS_DIRECTORY}/FakeSDDS3UpdateCacheUtils.py

Resource    ${COMMON_TEST_ROBOT}/InstallerResources.robot
Resource    ${COMMON_TEST_ROBOT}/SchedulerUpdateResources.robot
Resource    ${COMMON_TEST_ROBOT}/SDDS3Resources.robot
Resource    ${COMMON_TEST_ROBOT}/SulDownloaderResources.robot
Resource    ${COMMON_TEST_ROBOT}/UpgradeResources.robot

Force Tags  SULDOWNLOADER  LOAD6

*** Test Cases ***
Sul Downloader Installs does Force reinstall
    Start Local Cloud Server  --initial-alc-policy  ${SUPPORT_FILES}/CentralXml/ALC_policy_direct_just_base.xml    --initial-flags  ${SUPPORT_FILES}/CentralXml/FLAGS_forceUpdateFlags.json
    Generate Warehouse From Local Base Input  {"sdds3.force-update":"true"}
    ${handle}=  Start Local SDDS3 server with fake files
    Set Suite Variable    ${GL_handle}    ${handle}
    Setup Install SDDS3 Base
    Create File    ${MCS_DIR}/certs/ca_env_override_flag
    Create File  ${SOPHOS_INSTALL}/base/etc/sophosspl/flags-warehouse.json     {"sdds3.force-update":"true"}
    Run Process  chown  root:sophos-spl-group  ${SOPHOS_INSTALL}/base/etc/sophosspl/flags-warehouse.json
    Log File    ${SOPHOS_INSTALL}/base/etc/sophosspl/flags-warehouse.json

    Register With Local Cloud Server
    Wait Until Created    ${SOPHOS_INSTALL}/base/etc/sophosspl/flags-mcs.json  timeout=10 secs
    Create Local SDDS3 Override
    Trigger Update Now
    Wait Until Keyword Succeeds
    ...    60s
    ...    5s
    ...    SchedulerUpdateResources.Check SulDownloader Log Contains  Update success
    SchedulerUpdateResources.Check SulDownloader Log Contains  Triggering a force reinstall

Sul Downloader Installs does not Force reinstall when there is a marker file
    Start Local Cloud Server  --initial-alc-policy  ${SUPPORT_FILES}/CentralXml/ALC_policy_direct_just_base.xml    --initial-flags  ${SUPPORT_FILES}/CentralXml/FLAGS_forceUpdateFlags.json
    Generate Warehouse From Local Base Input  {"sdds3.force-update":"true"}
    ${handle}=  Start Local SDDS3 server with fake files
    Set Suite Variable    ${GL_handle}    ${handle}
    Setup Install SDDS3 Base
    Create File    ${MCS_DIR}/certs/ca_env_override_flag
    Create File  ${SOPHOS_INSTALL}/base/etc/sophosspl/flags-warehouse.json     {"sdds3.force-update":"true"}
    Run Process  chown  root:sophos-spl-group  ${SOPHOS_INSTALL}/base/etc/sophosspl/flags-warehouse.json
    Create File    ${SOPHOS_INSTALL}/base/update/var/updatescheduler/update_forced_marker

    Register With Local Cloud Server
    Wait Until Created    ${SOPHOS_INSTALL}/base/etc/sophosspl/flags-mcs.json  timeout=10 secs
    Create Local SDDS3 Override
    Trigger Update Now
    Wait Until Keyword Succeeds
    ...    60s
    ...    5s
    ...    SchedulerUpdateResources.Check SulDownloader Log Contains  Update success
    Check Sul Downloader log does not contain  Triggering a force reinstall

Sul Downloader Installs does Force reinstall for pause Updates
    Start Local Cloud Server  --initial-alc-policy  ${SUPPORT_FILES}/CentralXml/ALC_FixedVersionPolicySDDS3BaseOnly.xml    --initial-flags  ${SUPPORT_FILES}/CentralXml/FLAGS_forceUpdateFlags.json
    Generate Warehouse From Local Base Input  {"sdds3.force-paused-update":"true"}
    ${handle}=  Start Local SDDS3 server with fake files
    Set Suite Variable    ${GL_handle}    ${handle}
    Setup Install SDDS3 Base
    Create File    ${MCS_DIR}/certs/ca_env_override_flag
    Create File  ${SOPHOS_INSTALL}/base/etc/sophosspl/flags-warehouse.json    {"sdds3.force-paused-update":"true"}
    Run Process  chown  root:sophos-spl-group  ${SOPHOS_INSTALL}/base/etc/sophosspl/flags-warehouse.json
    Register With Local Cloud Server
    Wait Until Created    ${SOPHOS_INSTALL}/base/etc/sophosspl/flags-mcs.json  timeout=10 secs
    Create Local SDDS3 Override
    Trigger Update Now
    Wait Until Keyword Succeeds
    ...    60s
    ...    5s
    ...    SchedulerUpdateResources.Check SulDownloader Log Contains  Update success
    SchedulerUpdateResources.Check SulDownloader Log Contains  Triggering a force reinstall

Sul Downloader Installs does not Force reinstall when there is a marker file for pause update
    Start Local Cloud Server  --initial-alc-policy  ${SUPPORT_FILES}/CentralXml/ALC_FixedVersionPolicySDDS3BaseOnly.xml    --initial-flags  ${SUPPORT_FILES}/CentralXml/FLAGS_forceUpdateFlags.json
    Generate Warehouse From Local Base Input  {"sdds3.force-paused-update":"true"}
    ${handle}=  Start Local SDDS3 server with fake files
    Set Suite Variable    ${GL_handle}    ${handle}
    Setup Install SDDS3 Base
    Create File    ${MCS_DIR}/certs/ca_env_override_flag
    Create File    ${SOPHOS_INSTALL}/base/update/var/updatescheduler/paused_update_forced_marker
    Create File  ${SOPHOS_INSTALL}/base/etc/sophosspl/flags-warehouse.json  {"sdds3.force-paused-update":"true"}
    Run Process  chown  root:sophos-spl-group  ${SOPHOS_INSTALL}/base/etc/sophosspl/flags-warehouse.json
    Register With Local Cloud Server
    Wait Until Created    ${SOPHOS_INSTALL}/base/etc/sophosspl/flags-mcs.json  timeout=10 secs
    Create Local SDDS3 Override
    Trigger Update Now
    Wait Until Keyword Succeeds
    ...    60s
    ...    5s
    ...    SchedulerUpdateResources.Check SulDownloader Log Contains  Update success
    Check Sul Downloader log does not contain  Triggering a force reinstall

Sul Downloader Installs does not Force reinstall when there is a scheduled update for paused
    [Timeout]    10 minutes
    Start Local Cloud Server  --initial-alc-policy  ${SUPPORT_FILES}/CentralXml/ALC_FixedVersionPolicySDDS3BaseOnly.xml    --initial-flags  ${SUPPORT_FILES}/CentralXml/FLAGS_forceUpdateFlags.json
    Generate Warehouse From Local Base Input  {"sdds3.force-paused-update":"true"}
    ${handle}=  Start Local SDDS3 server with fake files
    Set Suite Variable    ${GL_handle}    ${handle}
    Setup Install SDDS3 Base
    Create File    ${MCS_DIR}/certs/ca_env_override_flag

    Create Local SDDS3 Override
    Register With Local Cloud Server
    Wait Until Created    ${SOPHOS_INSTALL}/base/etc/sophosspl/flags-mcs.json  timeout=10 secs

    Wait Until Keyword Succeeds
    ...    60s
    ...    5s
    ...    Check SulDownloader Log Contains String N Times   Update success  1

    #setup for supplement only update
    Create File  ${SOPHOS_INSTALL}/base/etc/sophosspl/flags-warehouse.json  {"sdds3.force-paused-update":"true"}
    Run Process  chown  root:sophos-spl-group  ${SOPHOS_INSTALL}/base/etc/sophosspl/flags-warehouse.json
    Wait Until Keyword Succeeds
      ...  60 secs
      ...  5 secs
      ...  File Should contain  ${SOPHOS_INSTALL}/base/etc/sophosspl/flags-mcs.json    true
    ${BasicPolicyXml} =  Get File  ${SUPPORT_FILES}/CentralXml/ALC_policy_scheduled_update_Fixed_version.xml
    ${Date} =  Get Current Date
    ${ScheduledDate} =  Add Time To Date  ${Date}  120 seconds
    ${ScheduledDay} =  Convert Date  ${ScheduledDate}  result_format=%A
    ${ScheduledTime} =  Convert Date  ${ScheduledDate}  result_format=%H:%M:00
    ${NewPolicyXml} =  Replace String  ${BasicPolicyXml}  REPLACE_DAY  ${ScheduledDay}
    ${NewPolicyXml} =  Replace String  ${NewPolicyXml}  REPLACE_TIME  ${ScheduledTime}
    ${NewPolicyXml} =  Replace String  ${NewPolicyXml}  Frequency="40"  Frequency="7"
    Create File  ${SOPHOS_INSTALL}/tmp/ALC_policy_scheduled_update.xml  ${NewPolicyXml}
    Log File  ${SOPHOS_INSTALL}/tmp/ALC_policy_scheduled_update.xml
    Wait Until Keyword Succeeds
      ...  60 secs
      ...  5 secs
      ...  File Should contain  ${UPDATE_CONFIG}    "forcePausedUpdate": true
    Stop Update Scheduler

    create File     ${SOPHOS_INSTALL}/base/update/var/updatescheduler/supplement_only.marker
    Run Process  systemctl  start  sophos-spl-update

    Wait Until Keyword Succeeds
    ...    60s
    ...    5s
    ...    Check SulDownloader Log Contains String N Times   Update success  2
    Check Sul Downloader log does not contain  Triggering a force reinstall

    Remove File     ${SOPHOS_INSTALL}/base/update/var/updatescheduler/supplement_only.marker
    Run Process  systemctl    start  sophos-spl-update

    Wait Until Keyword Succeeds
    ...    60s
    ...    5s
    ...    Check SulDownloader Log Contains String N Times   Update success  3
    SchedulerUpdateResources.Check SulDownloader Log Contains  Triggering a force reinstall

Sul Downloader Installs does not Force reinstall when there is a scheduled update for non paused
    Start Local Cloud Server  --initial-alc-policy  ${SUPPORT_FILES}/CentralXml/ALC_policy_direct_just_base.xml    --initial-flags  ${SUPPORT_FILES}/CentralXml/FLAGS_forceUpdateFlags.json
    Generate Warehouse From Local Base Input  {"sdds3.force-update":"true"}
    ${handle}=  Start Local SDDS3 server with fake files
    Set Suite Variable    ${GL_handle}    ${handle}
    Setup Install SDDS3 Base
    Create File    ${MCS_DIR}/certs/ca_env_override_flag

    Create Local SDDS3 Override
    Register With Local Cloud Server
    Wait Until Created    ${SOPHOS_INSTALL}/base/etc/sophosspl/flags-mcs.json  timeout=10 secs

    Wait Until Keyword Succeeds
    ...    60s
    ...    5s
    ...    Check SulDownloader Log Contains String N Times   Update success  1

    #setup for supplement only update
    Create File  ${SOPHOS_INSTALL}/base/etc/sophosspl/flags-warehouse.json  {"sdds3.force-update":"true"}
    Run Process  chown  root:sophos-spl-group  ${SOPHOS_INSTALL}/base/etc/sophosspl/flags-warehouse.json
    Wait Until Keyword Succeeds
      ...  60 secs
      ...  5 secs
      ...  File Should contain  ${SOPHOS_INSTALL}/base/etc/sophosspl/flags-mcs.json    true
    ${BasicPolicyXml} =  Get File  ${SUPPORT_FILES}/CentralXml/ALC_policy_scheduled_update.xml
    ${Date} =  Get Current Date
    ${ScheduledDate} =  Add Time To Date  ${Date}  120 seconds
    ${ScheduledDay} =  Convert Date  ${ScheduledDate}  result_format=%A
    ${ScheduledTime} =  Convert Date  ${ScheduledDate}  result_format=%H:%M:00
    ${NewPolicyXml} =  Replace String  ${BasicPolicyXml}  REPLACE_DAY  ${ScheduledDay}
    ${NewPolicyXml} =  Replace String  ${NewPolicyXml}  REPLACE_TIME  ${ScheduledTime}
    ${NewPolicyXml} =  Replace String  ${NewPolicyXml}  Frequency="40"  Frequency="7"
    Create File  ${SOPHOS_INSTALL}/tmp/ALC_policy_scheduled_update.xml  ${NewPolicyXml}
    Log File  ${SOPHOS_INSTALL}/tmp/ALC_policy_scheduled_update.xml
    Wait Until Keyword Succeeds
      ...  60 secs
      ...  5 secs
      ...  File Should contain  ${UPDATE_CONFIG}    "forceUpdate": true
    Stop Update Scheduler

    create File     ${SOPHOS_INSTALL}/base/update/var/updatescheduler/supplement_only.marker
    Run Process  systemctl  start  sophos-spl-update
    Wait Until Keyword Succeeds
    ...    60s
    ...    5s
    ...    Check SulDownloader Log Contains String N Times   Update success  2
    Check Sul Downloader log does not contain  Triggering a force reinstall

    Remove File     ${SOPHOS_INSTALL}/base/update/var/updatescheduler/supplement_only.marker
    Run Process  systemctl    start  sophos-spl-update

    Wait Until Keyword Succeeds
    ...    60s
    ...    5s
    ...    Check SulDownloader Log Contains String N Times   Update success  3
    SchedulerUpdateResources.Check SulDownloader Log Contains  Triggering a force reinstall


