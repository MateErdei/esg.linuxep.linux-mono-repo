*** Settings ***

Library   String
Library     ${LIBS_DIRECTORY}/PushServerUtils.py

Resource  McsRouterResources.robot


Suite Setup      Setup MCS Tests
Suite Teardown   Uninstall SSPL Unless Cleanup Disabled


Test Teardown    Run Keywords
...              Stop Local Cloud Server    AND
...              MCSRouter Default Test Teardown  AND
...			     Stop System Watchdog   AND
...              Override LogConf File as Global Level  INFO

Default Tags  FAKE_CLOUD  MCS  MCS_ROUTER

*** Test Case ***
MCS creates and updates flags file
    Install Register And Wait First MCS Policy
    Wait Until Keyword Succeeds
      ...  10 secs
      ...  1 secs
      ...  File Should Exist  ${SOPHOS_INSTALL}/base/etc/sophosspl/flags-mcs.json

    ${CONTENTS} =  Get File   ${SOPHOS_INSTALL}/base/etc/sophosspl/flags-mcs.json
    Should Contain  ${CONTENTS}  "endpoint.flag1.enabled" : true,
    Should Contain  ${CONTENTS}  "endpoint.flag2.enabled" : false,
    Should Contain  ${CONTENTS}  "endpoint.flag3.enabled" : true

    Remove File  ${SOPHOS_INSTALL}/base/etc/sophosspl/flags-mcs.json
    Create File  ${SOPHOS_INSTALL}/base/etc/sophosspl/flags-mcs.json

    ${CONTENTS} =  Get File   ${SOPHOS_INSTALL}/base/etc/sophosspl/flags-mcs.json
    Should Not Contain  ${CONTENTS}  "endpoint.flag1.enabled" : true,

    Send Policy File  mcs  ${SUPPORT_FILES}/CentralXml/MCS_policy_Short_Flags_Polling.xml
    Wait Until Keyword Succeeds
      ...  10 secs
      ...  1 secs
      ...  Check Mcsrouter Log Contains In Order  Checking for updates to mcs flags  Checking for updates to mcs flags

    ${CONTENTS} =  Get File   ${SOPHOS_INSTALL}/base/etc/sophosspl/flags-mcs.json
    Should Contain  ${CONTENTS}  "endpoint.flag1.enabled" : true,
