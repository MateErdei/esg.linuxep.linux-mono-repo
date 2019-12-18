import os
import unittest
import mock
import logging

BUILD_DIR=os.environ.get("ABS_BUILDDIR",".")
INSTALL_DIR=os.path.join(BUILD_DIR,"install")

import PathManager

import mcsrouter.mcs
import mcsrouter.mcsclient.mcs_exception
import mcsrouter.mcsclient.mcs_connection
import mcsrouter.mcsclient.mcs_commands as mcs_commands
from mcsrouter.adapters.livequery_adapter import LiveQueryAdapter
import mcsrouter.utils.target_system_manager
import xml.sax.saxutils
import json
import datetime


class LiveQueryCommand(mcs_commands.BasicCommand):
    def __init__(self, command_node, xml_text):
        mcs_commands.BasicCommand.__init__(self, None, command_node, xml_text )

    def complete(self):
        pass

def osquery_body(name, query):
    d = {'type': 'sophos.mgt.action.RunLiveQuery', 'name': name, 'query': query}
    return json.dumps(d)

def get_node(xml_text):
    doc = mcsrouter.utils.xml_helper.parseString(xml_text)
    command_nodes = doc.getElementsByTagName('command')[0]
    return command_nodes

def createlive_query_command(name, query, correlation_id, creationTime):
    body = osquery_body(name, query)
    xml_string = r"""<command>
        <id>{}</id>
        <appId>LiveQuery</appId>
        <creationTime>{}</creationTime>
        <ttl>PT10000S</ttl>
        <body>{}</body>
      </command>""".format(correlation_id, creationTime, xml.sax.saxutils.escape(body))
    return LiveQueryCommand(get_node(xml_string), xml_string)

def create_live_query_with_very_large_body(correlation_id, creationTime):
    max_byte_size = 1024 * 1024
    body = "large" * max_byte_size
    xml_string = r"""<command>
        <id>{}</id>
        <appId>LiveQuery</appId>
        <creationTime>{}</creationTime>
        <ttl>PT10000S</ttl>
        <body>{}</body>
      </command>""".format(correlation_id, creationTime, xml.sax.saxutils.escape(body))
    return LiveQueryCommand(get_node(xml_string), xml_string)



class TestLiveQueryAdapter(unittest.TestCase):

    def _log_contains(self, logs, message):
        for log_message in logs:
            if message in log_message:
                return
        raise AssertionError( "Messsage: {}, not found in logs: {}".format(message, logs))

    def testLiveQueryAdapterProcessCommandXmlAndWritesFileToTempActionDir(self):
        fixed_datetime = datetime.datetime(2019, month=5, day=7, hour=13, minute=33, second=24, microsecond=870000)
        expected_date_string = "20190507133324870000"

        livequery_adapter = LiveQueryAdapter('LiveQuery', INSTALL_DIR)
        self.assertEqual(livequery_adapter.get_app_id(), 'LiveQuery')
        live_query_command = createlive_query_command(name="Test", query="select * from process", correlation_id="correlation-id", creationTime="FakeTime")
        mocked_open_function = mock.mock_open()
        with self.assertLogs("mcsrouter.adapters.livequery_adapter", level='DEBUG') as logs:
            with mock.patch("builtins.open", mocked_open_function) as mock_i:
                with mock.patch('datetime.datetime') as mock_date:
                    mock_date.now.return_value = fixed_datetime
                    livequery_adapter.process_command(live_query_command)
        mock_i.assert_called_once_with('./install/tmp/actions/{}_LiveQuery_correlation-id_FakeTime_request.json'.format(expected_date_string), 'wb', 1)
        handle = mock_i()
        handle.write.assert_called_once_with(b'{"type": "sophos.mgt.action.RunLiveQuery", "name": "Test", "query": "select * from process"}')
        self._log_contains(logs.output, 'Query saved to path')

    def testLiveQueryWithXmlEntitiesWillBeCorrectEscaped(self):
        fixed_datetime = datetime.datetime(2019, month=5, day=7, hour=13, minute=33, second=24, microsecond=870000)
        expected_date_string = "20190507133324870000"

        livequery_adapter = LiveQueryAdapter('LiveQuery', INSTALL_DIR)
        self.assertEqual(livequery_adapter.get_app_id(), 'LiveQuery')
        query = """select time from process where time > 30 and name != "hello" """
        live_query_command = createlive_query_command(name="Test", query=query, correlation_id="correlation-id", creationTime="FakeTime")
        mocked_open_function = mock.mock_open()
        with self.assertLogs("mcsrouter.adapters.livequery_adapter", level='DEBUG') as logs:
            with mock.patch("builtins.open", mocked_open_function) as mock_i:
                with mock.patch('datetime.datetime') as mock_date:
                    mock_date.now.return_value = fixed_datetime
                    livequery_adapter.process_command(live_query_command)
        mock_i.assert_called_once_with('./install/tmp/actions/{}_LiveQuery_correlation-id_FakeTime_request.json'.format(expected_date_string), 'wb', 1)
        handle = mock_i()
        expected_file_content = b'{"type": "sophos.mgt.action.RunLiveQuery", "name": "Test", "query": "select time from process where time > 30 and name != \\"hello\\" "}'
        handle.write.assert_called_once_with(expected_file_content)
        entries = json.loads(expected_file_content)
        self.assertEqual(entries['query'], query)
        self._log_contains(logs.output, 'Query saved to path')


    def test_livequery_will_reject_request_whose_body_exceed_limit(self):
        fixed_datetime = datetime.datetime(2019, month=5, day=7, hour=13, minute=33, second=24, microsecond=870000)

        livequery_adapter = LiveQueryAdapter('LiveQuery', INSTALL_DIR)
        self.assertEqual(livequery_adapter.get_app_id(), 'LiveQuery')
        live_query_command = create_live_query_with_very_large_body(correlation_id="correlation-id", creationTime="FakeTime")
        mocked_open_function = mock.mock_open()
        with self.assertLogs("mcsrouter.adapters.livequery_adapter", level='DEBUG') as logs:
            with mock.patch("builtins.open", mocked_open_function) as mock_i:
                with mock.patch('datetime.datetime') as mock_date:
                    mock_date.now.return_value = fixed_datetime
                    livequery_adapter.process_command(live_query_command)
        mock_i.assert_not_called()
        self._log_contains(logs.output, 'ERROR:mcsrouter.adapters.livequery_adapter:Rejecting the query because it exceeds maximum allowed size. Query size (5242880) bytes. Allowed (1048576) bytes.')





if __name__ == '__main__':
    import logging
    unittest.main()