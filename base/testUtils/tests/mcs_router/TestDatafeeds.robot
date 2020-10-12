*** Settings ***
Documentation    Test suite for end to end XDR datafeed tests
Library   OperatingSystem
Library   Process
Library    ${LIBS_DIRECTORY}/TemporaryDirectoryManager.py

Resource  McsRouterResources.robot

Test Setup  Test Setup
Test Teardown  Test Teardown

Suite Setup       Run Keywords
...               Setup MCS Tests

Suite Teardown    Run Keywords
...               Uninstall SSPL Unless Cleanup Disabled

Default Tags  MCS  FAKE_CLOUD  MCS_ROUTER  TAP_TESTS  DATAFEED

*** Test Cases ***
Basic XDR Datafeed Sent
    Override LogConf File as Global Level  DEBUG
    Register With Local Cloud Server
    Check Correct MCS Password And ID For Local Cloud Saved
    Start MCSRouter
    ${json_to_send} =   Set Variable  {"abc":"def123"}
    send_xdr_datafeed_result  scheduled_query  2001298948  ${json_to_send}
    Check Cloud Server Log For Scheduled Query   scheduled_query
    Check Cloud Server Log For Scheduled Query Body   scheduled_query   ${json_to_send}
    Cloud Server Log Should Not Contain  Failed to decompress response body content


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

    Check Cloud Server Log For Scheduled Query   scheduled_query
    Check Cloud Server Log For Scheduled Query Body   scheduled_query   ${ok_size_content_expected_to_be_sent}
    Cloud Server Log Should Not Contain  Failed to decompress response body content
    Cloud Server Log Should Not Contain  ${ok_size_content}
    Cloud Server Log Should Not Contain  ${too_big_content}

    # Check that the non json file was rejected
    Check MCS Router Log Contains   mcsrouter.adapters.datafeed_receiver <> Failed to load datafeed json file "/opt/sophos-spl/base/mcs/datafeed/scheduled_query-3000000003.json"

    # Check empty file not added (empty file not valid json)
    Check MCS Router Log Contains   mcsrouter.adapters.datafeed_receiver <> Failed to load datafeed json file "/opt/sophos-spl/base/mcs/datafeed/scheduled_query-3000000004.json"

    # Check that MCS detected datafeed results
    Check MCS Router Log Contains   mcsrouter.mcs <> Datafeed results present for datafeed ID: scheduled_query

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
    Check MCS Router Log Contains  mcsrouter.mcsclient.mcs_connection <> MCS request url=/data_feed/endpoint/ThisIsAnMCSID+1001/feed_id/scheduled_query
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

*** Keywords ***
Test Teardown
    MCSRouter Default Test Teardown
    Stop Local Cloud Server
    Cleanup Temporary Folders

Test Setup
    Start Local Cloud Server