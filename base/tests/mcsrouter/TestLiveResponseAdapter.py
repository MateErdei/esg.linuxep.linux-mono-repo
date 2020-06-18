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
from mcsrouter.adapters.liveresponse_adapter import LiveResponseAdapter
import mcsrouter.utils.target_system_manager
import xml.sax.saxutils
import datetime


class LiveQueryCommand(mcs_commands.BasicCommand):
    def __init__(self, command_node, xml_text):
        mcs_commands.BasicCommand.__init__(self, None, command_node, xml_text )

    def complete(self):
        pass

def live_terminal_body(url, thumbprint):
    return \
f'<action type="sophos.mgt.action.InitiateLiveTerminal"><url>{url}</url><thumbprint>{thumbprint}</thumbprint></action>'


def get_node(xml_text):
    doc = mcsrouter.utils.xml_helper.parseString(xml_text)
    command_nodes = doc.getElementsByTagName('command')[0]
    return command_nodes

def create_live_response_command(name, url, thumbprint, correlation_id, creation_time):
    body = live_terminal_body(url, thumbprint)
    xml_string = r"""<command>
        <id>{}</id>
        <appId>LiveResponse</appId>
        <creationTime>{}</creationTime>
        <ttl>PT10000S</ttl>
        <body>{}</body>
      </command>""".format(correlation_id, creation_time, xml.sax.saxutils.escape(body))
    return LiveQueryCommand(get_node(xml_string), xml_string)

class TestLiveResponseAdapter(unittest.TestCase):

    def _log_contains(self, logs, message):
        for log_message in logs:
            if message in log_message:
                return
        raise AssertionError( "Messsage: {}, not found in logs: {}".format(message, logs))

    def test_live_response_adapter_process_command_xml_and_writes_file_to_temp_action_dir(self):
        fixed_datetime = datetime.datetime(2019, month=5, day=7, hour=13, minute=33, second=24, microsecond=870000)
        expected_date_string = "20190507133324870000"
        creation_time = "2020-06-09T09:15:23.000Z"
        creation_time_datetime = datetime.datetime.strptime(creation_time, "%Y-%m-%dT%H:%M:%S.%fZ")

        live_response_adapter = LiveResponseAdapter('LiveTerminal', INSTALL_DIR)
        self.assertEqual(live_response_adapter.get_app_id(), 'LiveTerminal')
        live_response_command = create_live_response_command(name="Test", url="url", thumbprint="thumbprint", correlation_id="correlation-id", creation_time=creation_time)
        mocked_open_function = mock.mock_open()
        with self.assertLogs("mcsrouter.adapters.generic_adapter", level='DEBUG') as logs:
            with mock.patch("builtins.open", mocked_open_function) as mock_i:
                with mock.patch('datetime.datetime') as mock_date:
                    mock_date.now.return_value = fixed_datetime
                    mock_date.strptime.return_value = creation_time_datetime
                    live_response_adapter.process_command(live_response_command)
        mock_i.assert_called_once_with(f"./install/tmp/actions/{expected_date_string}_LiveTerminal_correlation-id_action_{creation_time}_1591704123.xml", 'wb', 1)
        handle = mock_i()
        handle.write.assert_called_once_with(b'<action type="sophos.mgt.action.InitiateLiveTerminal"><url>url</url><thumbprint>thumbprint</thumbprint></action>')
