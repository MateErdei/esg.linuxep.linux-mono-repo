*** Settings ***
Documentation    Port tests related to mcs policies and fake cloud that were originally present for SAV. [LINUXEP-7787]

Library   String
Library     ${LIBS_DIRECTORY}/PushServerUtils.py

Resource  McsRouterResources.robot


Suite Setup      Setup MCS Tests
Suite Teardown   Uninstall SSPL Unless Cleanup Disabled


Test Teardown    Run Keywords
...              Dump Log   ./tmp/proxy_server.log  AND
...              Stop Local Cloud Server    AND
...              MCSRouter Default Test Teardown  AND
...			     Stop System Watchdog   AND
...              Override LogConf File as Global Level  INFO

Default Tags  FAKE_CLOUD  MCS  MCS_ROUTER

*** Test Case ***

MCS creates flags file
   Install Register And Wait First MCS Policy
   Wait Until Keyword Succeeds
      ...  10 secs
      ...  1 secs
      ...  File Should Exist  ${SOPHOS_INSTALL}/base/etc/sophosspl/flags-mcs.json
   ${CONTENTS} =  Get File   ${SOPHOS_INSTALL}/base/etc/sophosspl/flags-mcs.json

   Should Contain  ${CONTENTS}  "endpoint.flag1.enabled" : "true",
   Should Contain  ${CONTENTS}  "endpoint.flag2.enabled" : "false",
   Should Contain  ${CONTENTS}  "endpoint.flag3.enabled" : "force"
