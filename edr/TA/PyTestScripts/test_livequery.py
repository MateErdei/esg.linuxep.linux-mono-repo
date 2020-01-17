import os
import subprocess

def test_google_component_tests(sspl_mock, edr_plugin_instance):
    proc_path = os.path.join(sspl_mock.google_test_dir, "TestOsqueryProcessor")
    copyenv = os.environ.copy()
    copyenv["OVERRIDE_OSQUERY_BIN"] = os.path.join(sspl_mock.sspl, "plugins/edr/bin/osqueryd")
    copyenv["RUN_GOOGLE_COMPONENT_TESTS"] = "1"
    copyenv["LD_LIBRARY_PATH"] = os.path.join(sspl_mock.sspl, "plugins/edr/lib64/")
    popen = subprocess.Popen(proc_path, env=copyenv, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    results = popen.communicate()
    print(results[0].decode())
    if popen.returncode != 0:
        raise AssertionError("Google tests failed. Also providing the stderr: \n{}".format(results[1].decode))


def test_edr_plugin_receives_livequery_and_produces_answer(sspl_mock, edr_plugin_instance):
    edr_plugin_instance.start_edr()
    agent = sspl_mock.management
    query = """{
    "type": "sophos.mgt.action.RunLiveQuery",
    "name": "Top 2 Processes",
    "query": "SELECT name, path FROM processes limit 2"
}
    """
    agent.send_plugin_action('edr', 'LiveQuery', "1234-56fd", query)
    file_content = edr_plugin_instance.wait_file("base/mcs/response/LiveQuery_1234-56fd_response.json")
    # file refers to runLiveQuery
    assert 'sophos.mgt.response.RunLiveQuery' in file_content
    typePos = file_content.find('type')
    metaDataPos = file_content.find("queryMetaData")
    columnMetaDataPos = file_content.find("columnMetaData")
    columnDataPos = file_content.find("columnData")
    # demonstrate expected order of the main key values
    assert -1 < typePos < metaDataPos < columnMetaDataPos < columnDataPos


