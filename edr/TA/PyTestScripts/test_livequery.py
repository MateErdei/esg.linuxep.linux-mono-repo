
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


