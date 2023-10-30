*** Settings ***

Library   String
Library     ${COMMON_TEST_LIBS}/PushServerUtils.py

Resource    ${COMMON_TEST_ROBOT}/McsRouterResources.robot


Suite Setup      Setup MCS Tests
Suite Teardown   Uninstall SSPL Unless Cleanup Disabled

Test Teardown    Run Keywords
...              Stop Local Cloud Server    AND
...              MCSRouter Default Test Teardown  AND
...			     Stop System Watchdog   AND
...              Override LogConf File as Global Level  INFO

Force Tags  FAKE_CLOUD  MCS  MCS_ROUTER  TAP_PARALLEL1

*** Test Case ***
MCS creates and updates flags file
    Start Local Cloud Server   --initial-flags  ${SUPPORT_FILES}/CentralXml/FLAGS_testflags.json
    Register With Local Cloud Server
    Check Correct MCS Password And ID For Local Cloud Saved
    Start MCSRouter
    Wait New MCS Policy Downloaded
    Wait Until Created    ${SOPHOS_INSTALL}/base/mcs/policy/flags.json    timeout=20 seconds

    ${CONTENTS} =  Get File   ${SOPHOS_INSTALL}/base/etc/sophosspl/flags-mcs.json
    Should Contain  ${CONTENTS}  "livequery.network-tables.available" : true,
    Should Contain  ${CONTENTS}  "endpoint.flag2.enabled" : false,
    Should Contain  ${CONTENTS}  "endpoint.flag3.enabled" : false

    Remove File  ${SOPHOS_INSTALL}/base/etc/sophosspl/flags-mcs.json
    Create File  ${SOPHOS_INSTALL}/base/etc/sophosspl/flags-mcs.json

    ${CONTENTS} =  Get File   ${SOPHOS_INSTALL}/base/etc/sophosspl/flags-mcs.json
    Should Not Contain  ${CONTENTS}  "livequery.network-tables.available" : true,

    Send Policy File  mcs  ${SUPPORT_FILES}/CentralXml/MCS_policy_Short_Flags_Polling.xml
    Wait Until Keyword Succeeds
      ...  10 secs
      ...  1 secs
      ...  Check Mcsrouter Log Contains In Order  Checking for updates to mcs flags  Checking for updates to mcs flags

    Wait Until Keyword Succeeds
      ...  10 secs
      ...  1 secs
      ...  Mcs Flags contain Network tables

MCS Combines Flags Files
    Create File  /opt/sophos-spl/base/etc/sophosspl/flags-warehouse.json  {"livequery.network-tables.available" : "true","endpoint.flag3.enabled" : "always","endpoint.flag4.enabled" : "true", "endpoint.flag5.enabled" : "always"}
    Start Local Cloud Server  --initial-flags  ${SUPPORT_FILES}/CentralXml/FLAGS_testflags.json
    Register With Local Cloud Server
    Check Correct MCS Password And ID For Local Cloud Saved
    Start MCSRouter
    Wait New MCS Policy Downloaded
    Wait Until Created    ${SOPHOS_INSTALL}/base/mcs/policy/flags.json    timeout=20 seconds
    ${CONTENTS} =  Get File   ${SOPHOS_INSTALL}/base/mcs/policy/flags.json
    Should Contain  ${CONTENTS}  "livequery.network-tables.available": true
    Should Contain  ${CONTENTS}  "endpoint.flag2.enabled": false
    Should Contain  ${CONTENTS}  "endpoint.flag3.enabled": true
    Should Contain  ${CONTENTS}  "endpoint.flag4.enabled": false
    Should Contain  ${CONTENTS}  "endpoint.flag5.enabled": true

MCS handles empty warehouse flags
    Create File  /opt/sophos-spl/base/etc/sophosspl/flags-warehouse.json  "{}"
    Start Local Cloud Server  --initial-flags  ${SUPPORT_FILES}/CentralXml/FLAGS_testflags.json
    Register With Local Cloud Server
    Check Correct MCS Password And ID For Local Cloud Saved
    Start MCSRouter
    Wait New MCS Policy Downloaded
    Wait Until Keyword Succeeds
          ...  10 secs
          ...  1 secs
          ...  File Should Exist  ${SOPHOS_INSTALL}/base/mcs/policy/flags.json
    ${CONTENTS} =  Get File   ${SOPHOS_INSTALL}/base/mcs/policy/flags.json
    Should Contain  ${CONTENTS}  "livequery.network-tables.available": false
    Should Contain  ${CONTENTS}  "endpoint.flag2.enabled": false
    Should Contain  ${CONTENTS}  "endpoint.flag3.enabled": false

*** Keywords ***
Mcs Flags contain Network tables
    ${CONTENTS} =  Get File   ${SOPHOS_INSTALL}/base/etc/sophosspl/flags-mcs.json
    Should Contain  ${CONTENTS}  "livequery.network-tables.available" : true,