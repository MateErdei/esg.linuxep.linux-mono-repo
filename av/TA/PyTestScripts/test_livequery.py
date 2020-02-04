import os
import subprocess
import json
import logging

logger = logging.getLogger(__name__)


## queries and expected responses
top_2_processes_query = """{
    "type": "sophos.mgt.action.RunLiveQuery",
    "name": "Top 2 Processes",
    "query": "SELECT name, value FROM osquery_flags where name=='logger_stderr'"
    }"""
top_2_processes_response = """{
        "type": "sophos.mgt.response.RunLiveQuery",
        "queryMetaData": {"errorCode":0,"errorMessage":"OK","rows":1},
        "columnMetaData": [{"name":"name","type":"TEXT"},{"name":"value","type":"TEXT"}],
        "columnData": [["logger_stderr","false"]]
    }"""

no_column_query = """{
    "type": "sophos.mgt.action.RunLiveQuery",
    "name": "No column",
    "query": "SELECT name, value, invalid_column FROM osquery_flags where name=='logger_stderr'"
    }"""
no_column_response = """{ 
    "type": "sophos.mgt.response.RunLiveQuery",
    "queryMetaData": {"errorCode":1,"errorMessage":"no such column: invalid_column"}
    }"""

crash_query = ("{\n"
               "    \"type\": \"sophos.mgt.action.RunLiveQuery\",\n"
               "    \"name\": \"\",\n"
               "    \"query\": \"WITH RECURSIVE counting (curr, next) AS "
               "( SELECT 1,1 UNION ALL SELECT next, curr+1 FROM counting LIMIT 10000000000 ) SELECT group_concat(curr) FROM counting;\"}")
crash_query_response = """{
    "type": "sophos.mgt.response.RunLiveQuery",
    "queryMetaData": {"errorCode":101,"errorMessage":"Extension exited while running query"}
    }
    """

def detect_failure(func):
    def wrapper_function(sspl_mock, edr_plugin_instance):
        try:
            v = func(sspl_mock, edr_plugin_instance)
            return v
        except:
            edr_plugin_instance.set_failed()
            raise
    return wrapper_function

def check_responses_are_equivalent(actual_response, expected_response):
    try:
        print("Size of actual response: {}".format(len(actual_response)))
        response_dict = json.loads(actual_response)
        expected_response_dict = json.loads(expected_response)

        assert expected_response_dict['type'] == response_dict['type']
        if "columnMetaData" in expected_response:
            assert expected_response_dict['columnMetaData'] == response_dict['columnMetaData']
        if 'columnData' in expected_response:
            assert expected_response_dict['columnData'] == response_dict['columnData']
        if 'rows' in expected_response_dict['queryMetaData']:
            assert expected_response_dict['queryMetaData']["rows"] == response_dict['queryMetaData']["rows"]

        # we want to ignore durationMillis and sizeBytes
        assert expected_response_dict['queryMetaData']["errorCode"] == response_dict['queryMetaData']["errorCode"]
        assert expected_response_dict['queryMetaData']["errorMessage"] == response_dict['queryMetaData']["errorMessage"]
    except:
        print("Test live query failed.")
        print(actual_response[:1000])
        raise

def send_query( query_to_send, mock_management_agent):
    import uuid
    query_id = str(uuid.uuid4())
    mock_management_agent.send_plugin_action('edr', 'LiveQuery', query_id, query_to_send)
    response_filepath = "base/mcs/response/LiveQuery_{}_response.json".format(query_id)
    return response_filepath

def get_query_response(response_filepath, edr_plugin, response_timeout=10):
    file_content = edr_plugin.wait_file(response_filepath, response_timeout)
    assert 'sophos.mgt.response.RunLiveQuery' in file_content
    return file_content

def send_and_receive_query(query_to_send, mock_management_agent, edr_plugin, response_timeout=10):
    response_filepath = send_query(query_to_send, mock_management_agent)
    return get_query_response(response_filepath, edr_plugin, response_timeout)

def send_and_receive_query_and_verify(query_to_send, mock_management_agent, edr_plugin, expected_answer, response_timeout=10):
    file_content = send_and_receive_query(query_to_send, mock_management_agent, edr_plugin, response_timeout)
    check_responses_are_equivalent(file_content, expected_answer)


def test_google_component_tests(sspl_mock, edr_plugin_instance, caplog):
    caplog.set_level(logging.INFO)
    proc_path = os.path.join(sspl_mock.google_test_dir, "TestOsqueryProcessor")
    copyenv = os.environ.copy()
    copyenv["OVERRIDE_OSQUERY_BIN"] = os.path.join(sspl_mock.sspl, "plugins/edr/bin/osqueryd")
    copyenv["RUN_GOOGLE_COMPONENT_TESTS"] = "1"
    copyenv["LD_LIBRARY_PATH"] = os.path.join(sspl_mock.sspl, "plugins/edr/lib64/")
    popen = subprocess.Popen(proc_path, env=copyenv, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    results = popen.communicate()
    popen.wait()
    logger.info("{}".format(results[0].decode()))
    if popen.returncode != 0:
        raise AssertionError("Google tests failed. Also providing the stderr: \n{}".format(results[1].decode()))


@detect_failure
def test_edr_plugin_expected_responses_to_livequery(sspl_mock, edr_plugin_instance):
    edr_plugin_instance.start_edr()
    send_and_receive_query_and_verify(top_2_processes_query, sspl_mock.management, edr_plugin_instance, top_2_processes_response)
    send_and_receive_query_and_verify(no_column_query, sspl_mock.management, edr_plugin_instance, no_column_response)

    # check that empty name is also acceptable (Central sends empty name for user new queries)
    send_and_receive_query_and_verify("""{
    "type": "sophos.mgt.action.RunLiveQuery",
    "name": "",
    "query": "insert into processes values(path='foo')"
}
    """, sspl_mock.management, edr_plugin_instance, """{
                    "type": "sophos.mgt.response.RunLiveQuery",
                    "queryMetaData": {"errorCode":1,"errorMessage":"table processes may not be modified"}
                    }
    """)

    send_and_receive_query_and_verify(crash_query, sspl_mock.management, edr_plugin_instance,
                                      crash_query_response, response_timeout=100)

    # demonstrate that after a 'osquery crash' it is still possible to get normal and good answers
    send_and_receive_query_and_verify(top_2_processes_query, sspl_mock.management, edr_plugin_instance, top_2_processes_response)


@detect_failure
def test_edr_plugin_responses_to_queued_livequeries(sspl_mock, edr_plugin_instance):
    edr_plugin_instance.start_edr()

    query_dict = {"top 2 processes": top_2_processes_query, "crash query": crash_query, "no columns": no_column_query}
    expected_response_dict = {"top 2 processes": top_2_processes_response, "crash query": crash_query_response, "no columns": no_column_response}

    response_paths = dict()
    #queue queries
    for key, query in query_dict.items():
        response_file = send_query(query, sspl_mock.management)
        response_paths[key] = response_file

    #verify responses
    for key, response in expected_response_dict.items():
        actual_response = get_query_response(response_paths[key], edr_plugin_instance, response_timeout=100)
        check_responses_are_equivalent(actual_response, response)


@detect_failure
def test_edr_plugin_correct_report_for_queries_exceeding_10mb(sspl_mock, edr_plugin_instance):
    edr_plugin_instance.start_edr()
    large_query = """with recursive
cnt(x) as (values(1) union all select x+1 from cnt where x<550)
select * from osquery_flags cross join (select x from cnt);"""

    query_dict = {'type':"sophos.mgt.action.RunLiveQuery", 'name':"", 'query': large_query}
    exceed_10mb_query = json.dumps(query_dict)
    exceed_10mb_query_response = """ {
            "type": "sophos.mgt.response.RunLiveQuery",
            "queryMetaData": {"errorCode":100,"errorMessage":"Response data exceeded 10MB"},
            "columnMetaData": [{"name":"name","type":"TEXT"},
                                {"name":"type","type":"TEXT"},
                                {"name":"description","type":"TEXT"},
                                {"name":"default_value","type":"TEXT"},
                                {"name":"value","type":"TEXT"},
                                {"name":"shell_only","type":"INTEGER"},
                                {"name":"x","type":"TEXT"}]
            }
"""
    send_and_receive_query_and_verify(exceed_10mb_query, sspl_mock.management, edr_plugin_instance, exceed_10mb_query_response)

@detect_failure
def test_edr_plugin_receives_livequery_and_produces_answer(sspl_mock, edr_plugin_instance):
    edr_plugin_instance.start_edr()
    query = """{
    "type": "sophos.mgt.action.RunLiveQuery",
    "name": "Top 2 Processes",
    "query": "SELECT name, path FROM processes limit 2"
}
    """
    file_content = send_and_receive_query(query, sspl_mock.management, edr_plugin_instance)

    typePos = file_content.find('type')
    metaDataPos = file_content.find("queryMetaData")
    columnMetaDataPos = file_content.find("columnMetaData")
    columnDataPos = file_content.find("columnData")
    # demonstrate expected order of the main key values
    try:
        assert -1 < typePos < metaDataPos < columnMetaDataPos < columnDataPos
    except:
        print("Test live query failed.")
        print(file_content)
        raise


