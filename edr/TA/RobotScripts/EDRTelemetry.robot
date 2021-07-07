*** Settings ***
Documentation    Testing EDR Telemetry

Library         Process
Library         OperatingSystem
Library         Collections
Library         ../Libs/XDRLibs.py

Resource        EDRResources.robot
Resource        ComponentSetup.robot

Suite Setup     No Operation
Suite Teardown  Uninstall All

Test Setup      Run Keywords
...  Install With Base SDDS  AND
...  Check EDR Plugin Installed With Base

Test Teardown   EDR And Base Teardown

*** Test Cases ***
EDR Plugin Runs Scheduled Queries And Reports Telemetry
    Run Keyword And Ignore Error  Remove File  ${SOPHOS_INSTALL}/base/etc/logger.conf
    Create File  ${SOPHOS_INSTALL}/base/etc/logger.conf  [global]\nVERBOSITY = DEBUG\n
    Directory Should Be Empty  ${SOPHOS_INSTALL}/base/mcs/datafeed
    Remove File  ${SOPHOS_INSTALL}/plugins/edr/etc/osquery.conf.d/sophos-scheduled-query-pack.conf
    Remove File  ${SOPHOS_INSTALL}/plugins/edr/etc/osquery.conf.d/sophos-scheduled-query-pack.conf.DISABLED
    Remove File  ${SOPHOS_INSTALL}/plugins/edr/etc/osquery.conf.d/sophos-scheduled-query-pack.mtr.conf
    Remove File  ${SOPHOS_INSTALL}/plugins/edr/etc/osquery.conf.d/sophos-scheduled-query-pack.mtr.conf.DISABLED
    Copy File    ${EXAMPLE_DATA_PATH}/error-queries.conf  ${SOPHOS_INSTALL}/plugins/edr/etc/osquery.conf.d/error-queries.conf
    Enable XDR

    Wait Until Keyword Succeeds
    ...   30 secs
    ...   2 secs
    ...   EDR Plugin Log Contains  Error executing scheduled query bad-query:

    Wait Until Keyword Succeeds
    ...   20 secs
    ...   2 secs
    ...   Scheduled Query Log Contains  Executing query: endpoint_id

    ${edr_telemetry} =  Get Plugin Telemetry  edr
    ${telemetry_json} =  Evaluate  json.loads('''${edr_telemetry}''')  json
    ${scheduled_queries} =  Set Variable  ${telemetry_json['scheduled-queries']}
    List Should Contain Value  ${scheduled_queries}  bad-query
    ${bad_query_json} =  Set Variable  ${telemetry_json['scheduled-queries']['bad-query']}
    Dictionary Should Contain Key  ${bad_query_json}  query-error-count
    ${query_error_count} =  Get From Dictionary  ${bad_query_json}  query-error-count

    ${scheduled_query_log} =  Get File  ${SOPHOS_INSTALL}/plugins/edr/log/scheduledquery.log
    ${number_of_bad_queries} =  Set Variable  ${scheduled_query_log.count("bad-query")}
    Should Be True  ${number_of_bad_queries} > 0
    Should Be Equal As Integers  ${number_of_bad_queries}  ${query_error_count}

    List Should Contain Value  ${scheduled_queries}  endpoint_id
    ${endpoint_id_json} =  Set Variable  ${telemetry_json['scheduled-queries']['endpoint_id']}
    Dictionary Should Contain Key  ${endpoint_id_json}  record-size-avg
    Dictionary Should Not Contain Key  ${endpoint_id_json}  query-error-count
    ${records_count} =  Get From Dictionary  ${endpoint_id_json}  records-count
    Should Be True  ${records_count} >= 1