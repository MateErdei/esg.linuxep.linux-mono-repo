*** Settings ***
Documentation    Test suite for end to end XDR datafeed tests
Library   OperatingSystem
Library   Process
Library    ${LIBS_DIRECTORY}/TemporaryDirectoryManager.py
Library    ${LIBS_DIRECTORY}/OSUtils.py
Library    ${LIBS_DIRECTORY}/ProcessUtils.py

Resource  McsRouterResources.robot

Test Setup  Test Setup
Test Teardown  Test Teardown

Suite Setup   Run Keywords
...           Setup MCS Tests  AND
...           Store Original Datafeed Config

Suite Teardown    Run Keywords
...               Uninstall SSPL Unless Cleanup Disabled

Default Tags  MCS  FAKE_CLOUD  MCS_ROUTER  TAP_TESTS  DATAFEED  TESTRUN2

*** Test Cases ***
Basic XDR Datafeed Sent
    Register With Local Cloud Server
    Check Correct MCS Password And ID For Local Cloud Saved
    Start MCSRouter
    ${json_to_send} =   Set Variable  {"abc":"def123"}
    send_xdr_datafeed_result  scheduled_query  2001298948  ${json_to_send}

    ${device_id} =  Wait Until Keyword Succeeds  30s  1s  Get Device ID From Config
    Check Cloud Server Log For Scheduled Query   scheduled_query  ${device_id}
    Check Cloud Server Log For Scheduled Query Body   scheduled_query   ${json_to_send}
    Cloud Server Log Should Not Contain  Failed to decompress response body content

Basic XDR Datafeed size is logged
    Register With Local Cloud Server
    Check Correct MCS Password And ID For Local Cloud Saved
    Create file  ${SOPHOS_INSTALL}/base/etc/sophosspl/datafeed_tracker   content={"time_sent":0,"size": 423}
    Start MCSRouter
    ${json_to_send} =   Set Variable  {"abc":"def123"}
    send_xdr_datafeed_result  scheduled_query  2001298948  ${json_to_send}
    ${device_id} =  Wait Until Keyword Succeeds  30s  1s  Get Device ID From Config
    Check Cloud Server Log For Scheduled Query   scheduled_query  ${device_id}
    Wait Until Keyword Succeeds
    ...  10s
    ...  1s
    ...  Check MCS Router Log Contains    we have sent 0.447kB of scheduled query data to Central

Large XDR Datafeed size is logged
    Register With Local Cloud Server
    Check Correct MCS Password And ID For Local Cloud Saved
    Copy file  ${SUPPORT_FILES}/CentralXml/large_size_datafeed_tracker  ${SOPHOS_INSTALL}/base/etc/sophosspl/datafeed_tracker
    Start MCSRouter
    ${json_to_send} =   Set Variable  {"abc":"def123"}
    send_xdr_datafeed_result  scheduled_query  2001298948  ${json_to_send}
    ${device_id} =  Wait Until Keyword Succeeds  30s  1s  Get Device ID From Config
    Check Cloud Server Log For Scheduled Query   scheduled_query  ${device_id}
    Wait Until Keyword Succeeds
    ...  10s
    ...  1s
    ...  Check MCS Router Log Contains    1000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000kB of scheduled query data to Central

Invalid Datafeed Filename Not Sent But Does not Block Other Datafeed Files
    [Documentation]  Written to test the eact scenario set out in LINUXDAR-2463
    Override LogConf File as Global Level  DEBUG
    Register With Local Cloud Server
    Check Correct MCS Password And ID For Local Cloud Saved
    ${mcsrouter_mark} =  Mark Log Size    ${MCS_ROUTER_LOG}
    Start MCSRouter
    ${json_to_send1} =   Set Variable  {"abc":"def456"}
    ${json_to_send2} =   Set Variable  {"abc":"def789"}
    send_xdr_datafeed_result  scheduled_query  invalid  ${json_to_send1}
    send_xdr_datafeed_result  scheduled_query  2001298948  ${json_to_send2}
    Wait For Log Contains From Mark  ${mcsrouter_mark}   Malformed datafeed file: scheduled_query-invalid.json

    Wait For Log Contains From Mark  ${mcsrouter_mark}   Queuing datafeed result for: scheduled_query, with timestamp: 2001298948
    ${device_id} =  Wait Until Keyword Succeeds  30s  1s  Get Device ID From Config
    Check Cloud Server Log For Scheduled Query   scheduled_query  ${device_id}
    Check Cloud Server Log For Scheduled Query Body   scheduled_query   ${json_to_send2}
    Cloud Server Log Should Not Contain  ${json_to_send1}
    Wait Until Keyword Succeeds
    ...  10s
    ...  1s
    ...   Directory Should Be Empty  ${MCS_DIR}/datafeed

Retrieve JWT Tokens from Central
    Override LogConf File as Global Level  DEBUG
    Register With Local Cloud Server
    Check Correct MCS Password And ID For Local Cloud Saved
    Start MCSRouter
    Wait Until Keyword Succeeds
    ...  30s
    ...  1s
    ...  Check Log Contains String N Times   ${SOPHOS_INSTALL}/logs/base/sophosspl/mcsrouter.log   MCS Router Log   Setting Tenant ID: example-tenant-id   1
    Check Log Contains String N Times   ${SOPHOS_INSTALL}/logs/base/sophosspl/mcsrouter.log   MCS Router Log   Setting Device ID: ThisIsADeviceID+1001  1
    Check Log Contains String N Times   ${SOPHOS_INSTALL}/logs/base/sophosspl/mcsrouter.log   MCS Router Log   Setting JWT Token: JWT_TOKEN-ThisIsADeviceID+1001  1
    JWT Token Is Updated In MCS Config

Retrieve JWT Tokens from Central only once per connection
    Override LogConf File as Global Level  DEBUG
    Register With Local Cloud Server
    Check Correct MCS Password And ID For Local Cloud Saved
    Mark Mcsrouter Log
    Start MCSRouter
    Wait Until Keyword Succeeds
    ...  10s
    ...  1s
    ...  Check MCS Router Running
    Wait Until Keyword Succeeds
    ...  30s
    ...  1s
    ...  Check Marked Mcsrouter Log Contains String N Times   Setting Tenant ID: example-tenant-id   1
    Check Marked Mcsrouter Log Contains String N Times   Setting Device ID: ThisIsADeviceID+1001  1
    Check Marked Mcsrouter Log Contains String N Times   Setting JWT Token: JWT_TOKEN-ThisIsADeviceID+1001  1
    JWT Token Is Updated In MCS Config
    Log File  ${MCS_CONFIG}
    Log File  ${MCS_POLICY_CONFIG}

    Wait Until Keyword Succeeds
    ...  30s
    ...  2s
    ...  Check Marked Mcsrouter Log Contains String N Times   Re-entered main loop   3

    # Checking the JWT token hasn't been re-requested after several loops
    Check Marked Mcsrouter Log Contains String N Times   Setting Tenant ID: example-tenant-id   1

JWT Tokens expire and a new token is requested
    Override LogConf File as Global Level  DEBUG
    Register With Local Cloud Server
    Check Correct MCS Password And ID For Local Cloud Saved
    Start MCSRouter
    Wait Until Keyword Succeeds
    ...  30s
    ...  1s
    ...  Check Log Contains String N Times   ${SOPHOS_INSTALL}/logs/base/sophosspl/mcsrouter.log   MCS Router Log   Setting Tenant ID: example-tenant-id   1
    Check Log Contains String N Times   ${SOPHOS_INSTALL}/logs/base/sophosspl/mcsrouter.log   MCS Router Log   Setting Device ID: ThisIsADeviceID+1001  1
    Check Log Contains String N Times   ${SOPHOS_INSTALL}/logs/base/sophosspl/mcsrouter.log   MCS Router Log   Setting JWT Token: JWT_TOKEN-ThisIsADeviceID+1001  1

    Wait Until Keyword Succeeds
    ...  50s
    ...  5s
    ...  Check Log Contains String N Times   ${SOPHOS_INSTALL}/logs/base/sophosspl/mcsrouter.log   MCS Router Log   Setting Tenant ID: example-tenant-id   2

Ensure correct sending protocol handles all possible datafeed states at same time
    Override LogConf File as Global Level  DEBUG
    Register With Local Cloud Server
    Check Correct MCS Password And ID For Local Cloud Saved

    ${datafeed_config_path} =   Set Variable  ${ETC_DIR}/datafeed-config-scheduled_query.json
    remove file  ${datafeed_config_path}
    Copy File  ${SUPPORT_FILES}/datafeed_configs/datafeed_config_a.json  ${datafeed_config_path}
    ${result} =  Run Process  chown  sophos-spl-local:sophos-spl-group  ${datafeed_config_path}
    Should Be Equal As Integers    ${result.rc}    0
    file should exist  ${ETC_DIR}/datafeed-config-scheduled_query.json

    ${ok_size_content} =   Set Variable  {"abc":"def123"}
    ${ok_size_content_expected_to_be_sent} =   Set Variable  {"a":"to be sent"}
    ${too_big_content} =   Set Variable  {"a":"fakecontent123456789123"}

    # old single result but size ok
    send_xdr_datafeed_result  scheduled_query  1301298941  ${ok_size_content}
    file should exist  ${MCS_DIR}/datafeed/scheduled_query-1301298941.json

    # old single result and too large
    send_xdr_datafeed_result  scheduled_query  1301298942  ${too_big_content}
    file should exist  ${MCS_DIR}/datafeed/scheduled_query-1301298942.json

    # new enough and size ok
    send_xdr_datafeed_result  scheduled_query  3000000001   ${ok_size_content_expected_to_be_sent}
    file should exist  ${MCS_DIR}/datafeed/scheduled_query-3000000001.json

    # new enough but too large
    send_xdr_datafeed_result  scheduled_query  3000000002  ${too_big_content}
    file should exist  ${MCS_DIR}/datafeed/scheduled_query-3000000002.json

    # new enough but invalid json
    send_xdr_datafeed_result  scheduled_query  3000000003   not json
    file should exist  ${MCS_DIR}/datafeed/scheduled_query-3000000003.json

    # new enough but empty
    send_xdr_datafeed_result  scheduled_query  3000000004
    file should exist  ${MCS_DIR}/datafeed/scheduled_query-3000000004.json

    # Fill backlog with results that are new enough and less than max item size
    send_xdr_datafeed_result  scheduled_query  2900000004  ${ok_size_content}
    send_xdr_datafeed_result  scheduled_query  2900000005  ${ok_size_content}
    send_xdr_datafeed_result  scheduled_query  2900000006  ${ok_size_content_expected_to_be_sent}
    send_xdr_datafeed_result  scheduled_query  2900000007  ${ok_size_content_expected_to_be_sent}
    send_xdr_datafeed_result  scheduled_query  2900000008  ${ok_size_content_expected_to_be_sent}
    send_xdr_datafeed_result  scheduled_query  2900000009  ${ok_size_content_expected_to_be_sent}
    send_xdr_datafeed_result  scheduled_query  2900000010  ${ok_size_content_expected_to_be_sent}
    send_xdr_datafeed_result  scheduled_query  2900000011  ${ok_size_content_expected_to_be_sent}
    send_xdr_datafeed_result  scheduled_query  2900000012  ${ok_size_content_expected_to_be_sent}
    send_xdr_datafeed_result  scheduled_query  2900000013  ${ok_size_content_expected_to_be_sent}
    send_xdr_datafeed_result  scheduled_query  2900000014  ${ok_size_content_expected_to_be_sent}
    send_xdr_datafeed_result  scheduled_query  2900000015  ${ok_size_content_expected_to_be_sent}

    Start MCSRouter
    Wait Until Keyword Succeeds
    ...  30s
    ...  1s
    ...  Check MCS Router Log Contains   Loaded config for datafeed
    Check Mcsrouter Log Does Not Contain  Could not load config for datafeed, using default values

    # Wait until we're done sending everything up before checking product logs and fake cloud logs.
    Wait Until Keyword Succeeds
    ...  30s
    ...  1s
    ...  Check MCS Router Log Contains  No datafeed result files
    ${device_id} =  Wait Until Keyword Succeeds  30s  1s  Get Device ID From Config
    Check Cloud Server Log For Scheduled Query   scheduled_query  ${device_id}
    Check Cloud Server Log For Scheduled Query Body   scheduled_query   ${ok_size_content_expected_to_be_sent}
    Cloud Server Log Should Not Contain  Failed to decompress response body content
    Cloud Server Log Should Not Contain  ${ok_size_content}
    Cloud Server Log Should Not Contain  ${too_big_content}

    # Check that the non json file was rejected
    Check MCS Router Log Contains   mcsrouter.adapters.datafeed_receiver <> Failed to load datafeed json file "/opt/sophos-spl/base/mcs/datafeed/scheduled_query-3000000003.json"

    # Check empty file not added (empty file not valid json)
    Check MCS Router Log Contains   mcsrouter.adapters.datafeed_receiver <> Failed to load datafeed json file "/opt/sophos-spl/base/mcs/datafeed/scheduled_query-3000000004.json"

    # Check that MCS detected datafeed results
    Check MCS Router Log Contains   Datafeed results present for datafeed ID: scheduled_query

    Check MCS Router Log Contains  mcsrouter.mcs <> Queuing datafeed result for: scheduled_query, with timestamp: 2900000014
    Check MCS Router Log Contains  mcsrouter.mcs <> Queuing datafeed result for: scheduled_query, with timestamp: 3000000002
    Check MCS Router Log Contains  mcsrouter.mcs <> Queuing datafeed result for: scheduled_query, with timestamp: 2900000006
    Check MCS Router Log Contains  mcsrouter.mcs <> Queuing datafeed result for: scheduled_query, with timestamp: 2900000012
    Check MCS Router Log Contains  mcsrouter.mcs <> Queuing datafeed result for: scheduled_query, with timestamp: 2900000005
    Check MCS Router Log Contains  mcsrouter.mcs <> Queuing datafeed result for: scheduled_query, with timestamp: 3000000001
    Check MCS Router Log Contains  mcsrouter.adapters.datafeed_receiver <> Failed to load datafeed json file "/opt/sophos-spl/base/mcs/datafeed/scheduled_query-3000000004.json". Error: Expecting value: line 1 column 1 (char 0)
    Check MCS Router Log Contains  mcsrouter.mcs <> Queuing datafeed result for: scheduled_query, with timestamp: 2900000010
    Check MCS Router Log Contains  mcsrouter.mcs <> Queuing datafeed result for: scheduled_query, with timestamp: 1301298942
    Check MCS Router Log Contains  mcsrouter.mcs <> Queuing datafeed result for: scheduled_query, with timestamp: 2900000004
    Check MCS Router Log Contains  mcsrouter.mcs <> Queuing datafeed result for: scheduled_query, with timestamp: 1301298941
    Check MCS Router Log Contains  mcsrouter.adapters.datafeed_receiver <> Failed to load datafeed json file "/opt/sophos-spl/base/mcs/datafeed/scheduled_query-3000000003.json". Error: Expecting value: line 1 column 1 (char 0)
    Check MCS Router Log Contains  mcsrouter.mcs <> Queuing datafeed result for: scheduled_query, with timestamp: 2900000015
    Check MCS Router Log Contains  mcsrouter.mcs <> Queuing datafeed result for: scheduled_query, with timestamp: 2900000009
    Check MCS Router Log Contains  mcsrouter.mcs <> Queuing datafeed result for: scheduled_query, with timestamp: 2900000013
    Check MCS Router Log Contains  mcsrouter.mcs <> Queuing datafeed result for: scheduled_query, with timestamp: 2900000008
    Check MCS Router Log Contains  mcsrouter.mcs <> Queuing datafeed result for: scheduled_query, with timestamp: 2900000007
    Check MCS Router Log Contains  mcsrouter.mcs <> Queuing datafeed result for: scheduled_query, with timestamp: 2900000011

    Check MCS Router Log Contains   mcsrouter.mcsclient.mcs_connection <> Pruning old datafeed files, datafeed ID: scheduled_query
    Check MCS Router Log Contains   mcsrouter.mcsclient.datafeeds <> Datafeed result file too old, deleting: /opt/sophos-spl/base/mcs/datafeed/scheduled_query-1301298942.json
    Check MCS Router Log Contains   mcsrouter.mcsclient.datafeeds <> Removed scheduled_query datafeed file: /opt/sophos-spl/base/mcs/datafeed/scheduled_query-1301298942.json
    Check MCS Router Log Contains   mcsrouter.mcsclient.datafeeds <> Datafeed result file too old, deleting: /opt/sophos-spl/base/mcs/datafeed/scheduled_query-1301298941.json
    Check MCS Router Log Contains   mcsrouter.mcsclient.datafeeds <> Removed scheduled_query datafeed file: /opt/sophos-spl/base/mcs/datafeed/scheduled_query-1301298941.json
    file should not exist   /opt/sophos-spl/base/mcs/datafeed/scheduled_query-1301298941.json
    file should not exist   /opt/sophos-spl/base/mcs/datafeed/scheduled_query-1301298942.json

    Check MCS Router Log Contains   mcsrouter.mcsclient.mcs_connection <> Pruning datafeed files that are too large, datafeed ID: scheduled_query
    Check MCS Router Log Contains   mcsrouter.mcsclient.datafeeds <> Removed scheduled_query datafeed file: /opt/sophos-spl/base/mcs/datafeed/scheduled_query-3000000002.json
    file should not exist   /opt/sophos-spl/base/mcs/datafeed/scheduled_query-3000000002.json

    Check MCS Router Log Contains  mcsrouter.mcsclient.mcs_connection <> Pruning backlog of datafeed files, datafeed ID: scheduled_query
    Check MCS Router Log Contains  mcsrouter.mcsclient.datafeeds <> Current backlog size exceeded max 200 bytes, deleting: /opt/sophos-spl/base/mcs/datafeed/scheduled_query-2900000005.json
    Check MCS Router Log Contains  mcsrouter.mcsclient.datafeeds <> Removed scheduled_query datafeed file: /opt/sophos-spl/base/mcs/datafeed/scheduled_query-2900000005.json
    Check MCS Router Log Contains  mcsrouter.mcsclient.datafeeds <> Current backlog size exceeded max 200 bytes, deleting: /opt/sophos-spl/base/mcs/datafeed/scheduled_query-2900000004.json
    Check MCS Router Log Contains  mcsrouter.mcsclient.datafeeds <> Removed scheduled_query datafeed file: /opt/sophos-spl/base/mcs/datafeed/scheduled_query-2900000004.json
    file should not exist   /opt/sophos-spl/base/mcs/datafeed/scheduled_query-2900000005.json
    file should not exist   /opt/sophos-spl/base/mcs/datafeed/scheduled_query-2900000004.json

    Check MCS Router Log Contains  mcsrouter.mcsclient.mcs_connection <> Sorting datafeed files oldest to newest ready for sending, datafeed ID: scheduled_query
    Check MCS Router Log Contains  mcsrouter.mcsclient.mcs_connection <> Maximum single batch upload size is 100 bytes, datafeed ID: scheduled_query
    Check MCS Router Log Contains  mcsrouter.mcsclient.mcs_connection <> MCS request url=/v2/data_feed/device/ThisIsADeviceID+1001/feed_id/scheduled_query
    Check MCS Router Log Contains  Sent result, datafeed ID: scheduled_query, file: /opt/sophos-spl/base/mcs/datafeed/scheduled_query-2900000006.json
    Check MCS Router Log Contains  Sent result, datafeed ID: scheduled_query, file: /opt/sophos-spl/base/mcs/datafeed/scheduled_query-2900000007.json
    Check MCS Router Log Contains  Sent result, datafeed ID: scheduled_query, file: /opt/sophos-spl/base/mcs/datafeed/scheduled_query-2900000008.json
    Check MCS Router Log Contains  Sent result, datafeed ID: scheduled_query, file: /opt/sophos-spl/base/mcs/datafeed/scheduled_query-2900000009.json
    Check MCS Router Log Contains  Sent result, datafeed ID: scheduled_query, file: /opt/sophos-spl/base/mcs/datafeed/scheduled_query-2900000010.json
    Check MCS Router Log Contains  mcsrouter.mcsclient.datafeeds <> Removed scheduled_query datafeed file: /opt/sophos-spl/base/mcs/datafeed/scheduled_query-2900000006.json
    Check MCS Router Log Contains  mcsrouter.mcsclient.datafeeds <> Removed scheduled_query datafeed file: /opt/sophos-spl/base/mcs/datafeed/scheduled_query-2900000007.json
    Check MCS Router Log Contains  mcsrouter.mcsclient.datafeeds <> Removed scheduled_query datafeed file: /opt/sophos-spl/base/mcs/datafeed/scheduled_query-2900000008.json
    Check MCS Router Log Contains  mcsrouter.mcsclient.datafeeds <> Removed scheduled_query datafeed file: /opt/sophos-spl/base/mcs/datafeed/scheduled_query-2900000009.json
    Check MCS Router Log Contains  mcsrouter.mcsclient.datafeeds <> Removed scheduled_query datafeed file: /opt/sophos-spl/base/mcs/datafeed/scheduled_query-2900000010.json

    # Hit batch limit
    Check MCS Router Log Contains  mcsrouter.mcsclient.mcs_connection <> Can't send anymore datafeed results, at limit for now. Limit: 100

    # Next batch loop, MCS sees there are files but does not re-add them as they're already in the queue but can't be deleted until sent.
    Check MCS Router Log Contains  mcsrouter.mcs <> Already queued datafeed result for: scheduled_query, with timestamp: 2900000014
    Check MCS Router Log Contains  mcsrouter.mcs <> Already queued datafeed result for: scheduled_query, with timestamp: 2900000012
    Check MCS Router Log Contains  mcsrouter.mcs <> Already queued datafeed result for: scheduled_query, with timestamp: 3000000001
    Check MCS Router Log Contains  mcsrouter.mcs <> Already queued datafeed result for: scheduled_query, with timestamp: 2900000015
    Check MCS Router Log Contains  mcsrouter.mcs <> Already queued datafeed result for: scheduled_query, with timestamp: 2900000013
    Check MCS Router Log Contains  mcsrouter.mcs <> Already queued datafeed result for: scheduled_query, with timestamp: 2900000011

    Check MCS Router Log Contains  Sent result, datafeed ID: scheduled_query, file: /opt/sophos-spl/base/mcs/datafeed/scheduled_query-2900000011.json
    Check MCS Router Log Contains  Sent result, datafeed ID: scheduled_query, file: /opt/sophos-spl/base/mcs/datafeed/scheduled_query-2900000012.json
    Check MCS Router Log Contains  Sent result, datafeed ID: scheduled_query, file: /opt/sophos-spl/base/mcs/datafeed/scheduled_query-2900000013.json
    Check MCS Router Log Contains  Sent result, datafeed ID: scheduled_query, file: /opt/sophos-spl/base/mcs/datafeed/scheduled_query-2900000014.json
    Check MCS Router Log Contains  Sent result, datafeed ID: scheduled_query, file: /opt/sophos-spl/base/mcs/datafeed/scheduled_query-2900000015.json
    Check MCS Router Log Contains  Sent result, datafeed ID: scheduled_query, file: /opt/sophos-spl/base/mcs/datafeed/scheduled_query-3000000001.json
    Check MCS Router Log Contains  mcsrouter.mcsclient.datafeeds <> Removed scheduled_query datafeed file: /opt/sophos-spl/base/mcs/datafeed/scheduled_query-2900000011.json
    Check MCS Router Log Contains  mcsrouter.mcsclient.datafeeds <> Removed scheduled_query datafeed file: /opt/sophos-spl/base/mcs/datafeed/scheduled_query-2900000012.json
    Check MCS Router Log Contains  mcsrouter.mcsclient.datafeeds <> Removed scheduled_query datafeed file: /opt/sophos-spl/base/mcs/datafeed/scheduled_query-2900000013.json
    Check MCS Router Log Contains  mcsrouter.mcsclient.datafeeds <> Removed scheduled_query datafeed file: /opt/sophos-spl/base/mcs/datafeed/scheduled_query-2900000014.json
    Check MCS Router Log Contains  mcsrouter.mcsclient.datafeeds <> Removed scheduled_query datafeed file: /opt/sophos-spl/base/mcs/datafeed/scheduled_query-2900000015.json
    Check MCS Router Log Contains  mcsrouter.mcsclient.datafeeds <> Removed scheduled_query datafeed file: /opt/sophos-spl/base/mcs/datafeed/scheduled_query-3000000001.json

MCS Sends Data Using V2 Method Regardless of Flags
    Create File  /opt/sophos-spl/base/etc/sophosspl/flags-warehouse.json  {"jwt-token.available" : "false", "mcs.v2.data_feed.available": "false"}
    Override LogConf File as Global Level  DEBUG
    Register With Local Cloud Server
    Check Correct MCS Password And ID For Local Cloud Saved
    Start MCSRouter
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  Check MCS Router Log Contains  'mcs.v2.data_feed.available': False
    Check MCS Router Log Contains  'jwt-token.available': False

    ${json_to_send} =   Set Variable  {"abc":"def123"}
    send_xdr_datafeed_result  scheduled_query  2001298948  ${json_to_send}
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  Check Cloud Server Log Contains    POST - /mcs/v2/data_feed/device/ThisIsADeviceID+1001/feed_id/scheduled_query
    Check Cloud Server Log For Scheduled Query Body   scheduled_query   ${json_to_send}
    Cloud Server Log Should Not Contain  Failed to decompress response body content
    Cloud Server Log Should Contain  Received and processed data via the v2 method
    Check MCS Router Log Contains  MCS request url=/v2/data_feed/device/ThisIsADeviceID+1001/feed_id/scheduled_query body size=24
    Check Mcsrouter Log Does Not Contain  MCS request url=/data_feed/endpoint/ThisIsAnMCSID+1001/feed_id/scheduled_query


MCS Does Not Crash When There Is A Large Volume Of Datafeed Results
    Override LogConf File as Global Level  INFO
    Register With Local Cloud Server
    Check Correct MCS Password And ID For Local Cloud Saved
    Stop MCSRouter If Running
    Wait Until Keyword Succeeds
    ...  10s
    ...  1s
    ...  Check MCS Router Not Running
    Generate Large Amount Of Datafeed Results
    Start MCSRouter
    Wait For MCS Router To Be Running
    ${mcsrouter_pid} =  Get MCSRouter PID

    Wait Until Keyword Succeeds
    ...  120s
    ...  1s
    ...  Check MCS Router Log Contains  mcsrouter.mcsclient.datafeeds <> Current backlog size exceeded max

    ${mcsrouter_mem}=    get_process_memory    ${mcsrouter_pid}
    log    ${mcsrouter_mem}
    Should Be True  int(${mcsrouter_mem})/1000000 < int(500)

    Wait Until Keyword Succeeds
    ...  60s
    ...  5s
    ...  Datafeed Dir Less Than 1GB


*** Keywords ***
Test Teardown
    MCSRouter Default Test Teardown
    Stop Local Cloud Server
    Cleanup Temporary Folders
    Create File  ${SOPHOS_INSTALL}/base/etc/datafeed-config-scheduled_query.json  ${SCHEDULED_QUERY_DATAFEED_CONFIG}

Test Setup
    # We need the same device ID in the policy as fake cloud will generate otherwise we will request a new JWT.
    # MCS gets a new JWT if the deivce ID changes to support de-dupe, the ID will be: ThisIsADeviceID+1001
    Start Local Cloud Server  --initial-mcs-policy  ${SUPPORT_FILES}/CentralXml/MCS_policy_with_same_device_ID_as_fake_cloud.xml

Datafeed Dir Less Than 1GB
    ${size_mb}=    Get Datafeed Directory Size In MB
    Should Be True  ${size_mb} < 1024

Store Original Datafeed Config
    ${scheduled_query_config_orignal_content}=    Get File   ${SOPHOS_INSTALL}/base/etc/datafeed-config-scheduled_query.json
    Set Suite Variable    ${SCHEDULED_QUERY_DATAFEED_CONFIG}   ${scheduled_query_config_orignal_content}