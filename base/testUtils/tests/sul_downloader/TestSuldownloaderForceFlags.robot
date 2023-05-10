*** Settings ***
Suite Setup      Suite Setup Without Ostia
Suite Teardown   Suite Teardown Without Ostia

Test Setup       Test Setup
Test Teardown    Run Keywords
...                Stop Local SDDS3 Server   AND
...                 Test Teardown

Library    DateTime
Library     ${LIBS_DIRECTORY}/FakeSDDS3UpdateCacheUtils.py

Resource    ../scheduler_update/SchedulerUpdateResources.robot
Resource    ../installer/InstallerResources.robot
Resource    ../update/SDDS3Resources.robot
Resource    ../upgrade_product/UpgradeResources.robot
Resource    SulDownloaderResources.robot

Default Tags  SULDOWNLOADER
Force Tags  LOAD6

*** Variables ***
${sdds3_server_output}                      /tmp/sdds3_server.log

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
    Wait Until Keyword Succeeds
      ...  10 secs
      ...  1 secs
      ...  File Should Exist  ${SOPHOS_INSTALL}/base/etc/sophosspl/flags-mcs.json
    Create Local SDDS3 Override
    Trigger Update Now
    Wait Until Keyword Succeeds
    ...    60s
    ...    5s
    ...    Check SulDownloader Log Contains  Update success
    Check SulDownloader Log Contains  Triggering a force reinstall

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
    Wait Until Keyword Succeeds
      ...  10 secs
      ...  1 secs
      ...  File Should Exist  ${SOPHOS_INSTALL}/base/etc/sophosspl/flags-mcs.json
    Create Local SDDS3 Override
    Trigger Update Now
    Wait Until Keyword Succeeds
    ...    60s
    ...    5s
    ...    Check SulDownloader Log Contains  Update success
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
    Wait Until Keyword Succeeds
      ...  10 secs
      ...  1 secs
      ...  File Should Exist  ${SOPHOS_INSTALL}/base/etc/sophosspl/flags-mcs.json
    Create Local SDDS3 Override
    Trigger Update Now
    Wait Until Keyword Succeeds
    ...    60s
    ...    5s
    ...    Check SulDownloader Log Contains  Update success
    Check SulDownloader Log Contains  Triggering a force reinstall

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
    Wait Until Keyword Succeeds
      ...  10 secs
      ...  1 secs
      ...  File Should Exist  ${SOPHOS_INSTALL}/base/etc/sophosspl/flags-mcs.json
    Create Local SDDS3 Override
    Trigger Update Now
    Wait Until Keyword Succeeds
    ...    60s
    ...    5s
    ...    Check SulDownloader Log Contains  Update success
    Check Sul Downloader log does not contain  Triggering a force reinstall

Sul Downloader Installs does not Force reinstall when there is a scheduled update for paused
    [Tags]    TESTFAILURE
    Start Local Cloud Server  --initial-alc-policy  ${SUPPORT_FILES}/CentralXml/ALC_FixedVersionPolicySDDS3BaseOnly.xml    --initial-flags  ${SUPPORT_FILES}/CentralXml/FLAGS_forceUpdateFlags.json
    Generate Warehouse From Local Base Input  {"sdds3.force-paused-update":"true"}
    ${handle}=  Start Local SDDS3 server with fake files
    Set Suite Variable    ${GL_handle}    ${handle}
    Setup Install SDDS3 Base
    Create File    ${MCS_DIR}/certs/ca_env_override_flag

    Register With Local Cloud Server
    Wait Until Keyword Succeeds
      ...  10 secs
      ...  1 secs
      ...  File Should Exist  ${SOPHOS_INSTALL}/base/etc/sophosspl/flags-mcs.json
    Create Local SDDS3 Override
    Trigger Update Now
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
    ${ScheduledDate} =  Add Time To Date  ${Date}  60 seconds
    ${ScheduledDay} =  Convert Date  ${ScheduledDate}  result_format=%A
    ${ScheduledTime} =  Convert Date  ${ScheduledDate}  result_format=%H:%M:00
    ${NewPolicyXml} =  Replace String  ${BasicPolicyXml}  REPLACE_DAY  ${ScheduledDay}
    ${NewPolicyXml} =  Replace String  ${NewPolicyXml}  REPLACE_TIME  ${ScheduledTime}
    ${NewPolicyXml} =  Replace String  ${NewPolicyXml}  Frequency="40"  Frequency="7"
    Create File  ${SOPHOS_INSTALL}/tmp/ALC_policy_scheduled_update.xml  ${NewPolicyXml}
    Log File  ${SOPHOS_INSTALL}/tmp/ALC_policy_scheduled_update.xml
    Send Policy File  alc    ${SOPHOS_INSTALL}/tmp/ALC_policy_scheduled_update.xml
    ${ups_mark} =  mark_log_size  ${SOPHOS_INSTALL}/logs/base/sophosspl/updatescheduler.log
    Send Policy File  alc    ${SOPHOS_INSTALL}/tmp/ALC_policy_scheduled_update.xml
    wait_for_log_contains_from_mark  ${ups_mark}  Scheduling product updates for   15
    Trigger Update Now
    Wait Until Keyword Succeeds
    ...    60s
    ...    5s
    ...    Check SulDownloader Log Contains String N Times   Update success  2
    Check Sul Downloader log does not contain  Triggering a force reinstall

    sleep    60
    Trigger Update Now
    Wait Until Keyword Succeeds
    ...    60s
    ...    5s
    ...    Check SulDownloader Log Contains String N Times   Update success  3
    Check SulDownloader Log Contains  Triggering a force reinstall

Sul Downloader Installs does not Force reinstall when there is a scheduled update for non paused
    [Tags]    TESTFAILURE
    Start Local Cloud Server  --initial-alc-policy  ${SUPPORT_FILES}/CentralXml/ALC_policy_direct_just_base.xml    --initial-flags  ${SUPPORT_FILES}/CentralXml/FLAGS_forceUpdateFlags.json
    Generate Warehouse From Local Base Input  {"sdds3.force-update":"true"}
    ${handle}=  Start Local SDDS3 server with fake files
    Set Suite Variable    ${GL_handle}    ${handle}
    Setup Install SDDS3 Base
    Create File    ${MCS_DIR}/certs/ca_env_override_flag

    Register With Local Cloud Server
    Wait Until Keyword Succeeds
      ...  10 secs
      ...  1 secs
      ...  File Should Exist  ${SOPHOS_INSTALL}/base/etc/sophosspl/flags-mcs.json
    Create Local SDDS3 Override
    Trigger Update Now
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
    ${ScheduledDate} =  Add Time To Date  ${Date}  60 seconds
    ${ScheduledDay} =  Convert Date  ${ScheduledDate}  result_format=%A
    ${ScheduledTime} =  Convert Date  ${ScheduledDate}  result_format=%H:%M:00
    ${NewPolicyXml} =  Replace String  ${BasicPolicyXml}  REPLACE_DAY  ${ScheduledDay}
    ${NewPolicyXml} =  Replace String  ${NewPolicyXml}  REPLACE_TIME  ${ScheduledTime}
    ${NewPolicyXml} =  Replace String  ${NewPolicyXml}  Frequency="40"  Frequency="7"
    Create File  ${SOPHOS_INSTALL}/tmp/ALC_policy_scheduled_update.xml  ${NewPolicyXml}
    Log File  ${SOPHOS_INSTALL}/tmp/ALC_policy_scheduled_update.xml

    ${ups_mark} =  mark_log_size  ${SOPHOS_INSTALL}/logs/base/sophosspl/updatescheduler.log
    Send Policy File  alc    ${SOPHOS_INSTALL}/tmp/ALC_policy_scheduled_update.xml
    wait_for_log_contains_from_mark  ${ups_mark}  Scheduling product updates for   15

    Trigger Update Now
    Wait Until Keyword Succeeds
    ...    60s
    ...    5s
    ...    Check SulDownloader Log Contains String N Times   Update success  2
    Check Sul Downloader log does not contain  Triggering a force reinstall

    sleep    60
    Trigger Update Now
    Wait Until Keyword Succeeds
    ...    60s
    ...    5s
    ...    Check SulDownloader Log Contains String N Times   Update success  3
    Check SulDownloader Log Contains  Triggering a force reinstall

