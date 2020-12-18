#!/usr/bin/env python3


import unittest
import xml.dom.minidom
import mock
from mock import patch, mock_open

import logging
logger = logging.getLogger("TestComputer")

import PathManager

import mcsrouter.computer
import mcsrouter.adapters.agent_adapter

def getTargetSystem():
    import mcsrouter.targetsystem
    return mcsrouter.targetsystem.TargetSystem()

class ComputerThatSkipsDirectCommand (mcsrouter.computer.Computer):
    def __init__(self):
        mcsrouter.computer.Computer.__init__(self)

    def direct_command(self, command):
        logger.info("Skipping execution of command: {}".format(command))



class TestcomputerCommonStatus(unittest.TestCase):
    @mock.patch('mcsrouter.adapters.agent_adapter.ComputerCommonStatus.get_mac_addresses', return_value=["12:34:56:78:12:34"])
    def testCommonStatusXml(self, *mockarg):
        adapter = mcsrouter.adapters.agent_adapter.AgentAdapter()
        status_xml = adapter.get_common_status_xml()

        ## Verify XML is valid
        doc = xml.dom.minidom.parseString(status_xml)
        doc.unlink()

class TestComputer(unittest.TestCase):
    @mock.patch('mcsrouter.adapters.agent_adapter.ComputerCommonStatus.get_mac_addresses', return_value=["12:34:56:78:12:34"])
    def testStatusXml(self, *mockarg):
        c = mcsrouter.computer.Computer()
        #
        # computerStatus = c.getCommonStatusXml()

        adapter = mcsrouter.adapters.agent_adapter.AgentAdapter()
        status_xml = adapter.get_common_status_xml()

        logger.debug(status_xml)

        ## Verify it is XML
        doc = xml.dom.minidom.parseString(status_xml)
        doc.unlink()

        platformStatus = adapter.get_platform_status()
        logger.debug(platformStatus)
        doc = xml.dom.minidom.parseString(platformStatus)
        doc.unlink()

        ## Header isn't valid XML on its own
        logger.debug(adapter.get_status_header())


    def testPlatformStatusContainsOSVersionInformation(self):
        adapter = mcsrouter.adapters.agent_adapter.AgentAdapter()

        platformStatus = adapter.get_platform_status()
        logger.debug(platformStatus)
        doc = xml.dom.minidom.parseString(platformStatus)

        major_version_element = doc.getElementsByTagName("osMajorVersion")[0].firstChild.data
        minor_version_element = doc.getElementsByTagName("osMinorVersion")[0].firstChild.data

        #check the values being sent are valid numbers for a major and minor versions
        self.assertTrue(int(major_version_element) > 0)
        self.assertTrue(int(minor_version_element) > 0)
        doc.unlink()


    def testFormatIPv6(self):
        self.assertEqual(mcsrouter.adapters.agent_adapter.format_ipv6("00010000000000000000000000000000"),"1::")
        self.assertEqual(mcsrouter.adapters.agent_adapter.format_ipv6("11110000000000000000000000000000"),"1111::")
        self.assertEqual(mcsrouter.adapters.agent_adapter.format_ipv6("11112222000000000000000000000000"),"1111:2222::")
        self.assertEqual(mcsrouter.adapters.agent_adapter.format_ipv6("11110000222200000000000000000000"),"1111:0:2222::")
        self.assertEqual(mcsrouter.adapters.agent_adapter.format_ipv6("11110333222200000000000000000000"),"1111:333:2222::")
        self.assertEqual(mcsrouter.adapters.agent_adapter.format_ipv6("11110033222200000000000000000000"),"1111:33:2222::")
        self.assertEqual(mcsrouter.adapters.agent_adapter.format_ipv6("11110003222200000000000000000000"),"1111:3:2222::")
        self.assertEqual(mcsrouter.adapters.agent_adapter.format_ipv6("fe80000000000000022186fffe29edfd"),"fe80::221:86ff:fe29:edfd")
        self.assertEqual(mcsrouter.adapters.agent_adapter.format_ipv6("fe80000000000000025056fffec00008"),"fe80::250:56ff:fec0:8")

        self.assertEqual(mcsrouter.adapters.agent_adapter.format_ipv6("20010db8000000000000000000000001"),"2001:db8::1")
        self.assertEqual(mcsrouter.adapters.agent_adapter.format_ipv6("20010db8000000010001000100010001"),"2001:db8:0:1:1:1:1:1")

    def testStatusDoesNotChange(self):
        c = mcsrouter.computer.Computer()
        self.assertFalse(c.has_status_changed())

    class FakePopen:
        def communicate(self):
            return (b"",b"")
        def wait(self):
            return 0
    @mock.patch("subprocess.Popen", return_value=FakePopen())
    @mock.patch("subprocess.check_output", return_value=b'some-hostname')
    @mock.patch("os.path.isfile", return_value=True)
    @patch('builtins.open', new_callable=mock_open)
    def testGroupStatusWithXmlMultipleEntriesInInstallOptionsFile(self, mo, *mockarg):
        # These side affects to mock reading are only needed because we're using mock builtins.open
        # so all uses of open need to be accounted for.
        side_affects = (
            # some values for IPv6 functions to work, not of interest to this test.
            mock_open(read_data="00000000000000000000000000000001").return_value,
            mock_open(
                read_data="00000000000000000000000000000001 01 80 10 80       lo    \nfe800000000000006c4e782bd302103d 02 40 20 80 enp0s31f6").return_value,
            mock_open(read_data="a\nb\nc\n--group=groupname\nd").return_value,
        )
        mo.side_effect = side_affects
        adapter = mcsrouter.adapters.agent_adapter.AgentAdapter()
        status_xml = adapter.get_common_status_xml()
        self.assertTrue("<deviceGroup>groupname</deviceGroup>" in status_xml)

    @mock.patch("subprocess.Popen", return_value=FakePopen())
    @mock.patch("subprocess.check_output", return_value=b'some-hostname')
    @mock.patch("os.path.isfile", return_value=True)
    @patch('builtins.open', new_callable=mock_open)
    def testGroupStatusXmlWithSingleEntryInInstallOptionsFile(self, mo, *mockarg):
        # These side affects to mock reading are only needed because we're using mock builtins.open
        # so all uses of open need to be accounted for.
        side_affects = (
            # some values for IPv6 functions to work, not of interest to this test.
            mock_open(read_data="00000000000000000000000000000001").return_value,
            mock_open(
                read_data="00000000000000000000000000000001 01 80 10 80       lo    \nfe800000000000006c4e782bd302103d 02 40 20 80 enp0s31f6").return_value,
            mock_open(read_data="--group=groupname2").return_value
        )
        mo.side_effect = side_affects
        adapter = mcsrouter.adapters.agent_adapter.AgentAdapter()
        status_xml = adapter.get_common_status_xml()
        self.assertTrue("<deviceGroup>groupname2</deviceGroup>" in status_xml)

    @mock.patch("subprocess.Popen", return_value=FakePopen())
    @mock.patch("subprocess.check_output", return_value=b'some-hostname')
    @mock.patch("os.path.isfile", return_value=True)
    @patch('builtins.open', new_callable=mock_open)
    def testGroupStatusXmlWithMalformedGroupOptionInInstallOptionsFile(self, mo, *mockarg):
        # These side affects to mock reading are only needed because we're using mock builtins.open
        # so all uses of open need to be accounted for.
        side_affects = (
            # some values for IPv6 functions to work, not of interest to this test.
            mock_open(read_data="00000000000000000000000000000001").return_value,
            mock_open(
                read_data="00000000000000000000000000000001 01 80 10 80       lo    \nfe800000000000006c4e782bd302103d 02 40 20 80 enp0s31f6").return_value,
            mock_open(read_data="--group=bad<group'name").return_value
        )
        mo.side_effect = side_affects
        adapter = mcsrouter.adapters.agent_adapter.AgentAdapter()
        status_xml = adapter.get_common_status_xml()
        self.assertTrue("<deviceGroup>" not in status_xml)

    @mock.patch("subprocess.Popen", return_value=FakePopen())
    @mock.patch("subprocess.check_output", return_value=b'some-hostname')
    @mock.patch("os.path.isfile", return_value=True)
    @patch('builtins.open', new_callable=mock_open)
    def testGroupStatusXmlWithEmptyGroupOptionInInstallOptionsFile(self, mo, *mockarg):
        # These side affects to mock reading are only needed because we're using mock builtins.open
        # so all uses of open need to be accounted for.
        side_affects = (
            # some values for IPv6 functions to work, not of interest to this test.
            mock_open(read_data="00000000000000000000000000000001").return_value,
            mock_open(
                read_data="00000000000000000000000000000001 01 80 10 80       lo    \nfe800000000000006c4e782bd302103d 02 40 20 80 enp0s31f6").return_value,
            mock_open(read_data="--group=").return_value
        )
        mo.side_effect = side_affects
        adapter = mcsrouter.adapters.agent_adapter.AgentAdapter()
        status_xml = adapter.get_common_status_xml()
        self.assertTrue("<deviceGroup>" not in status_xml)

    @mock.patch("os.path.isdir", return_value=True)
    @mock.patch("os.listdir", return_value=[])
    def test_run_commands_move_action_files_to_the_expected_path(self, *mockarg):
        """Test to demonstrate that the finalization of Computer.run_commands will move any file found in
        /tmp/sophos-spl/tmp/action to  /tmp/sophos-spl/base/mcs/action folder
        /tmp is set as the 'root folder'
        """

        commands = ["fake1", "fake2"]

        for initial_file_name, final_file_name in [("OrderMARK_SAV_action_timestamp.xml", "SAV_action_timestamp.xml"),
                                                   ("1_LiveQuery_correlation-id_timestamp_response.json", "LiveQuery_correlation-id_timestamp_response.json"),
                                                   ("AnyOtherFile.any", "AnyOtherFile.any"),
                                                   ("Appid_forgotMark.txt", "forgotMark.txt")]: # it expects the first element before underscore to be the mark, and will remove it if found
            c = ComputerThatSkipsDirectCommand()
            mocked_open_function = mock.MagicMock()
            temp_file_name = "/tmp/sophos-spl/tmp/action/" + initial_file_name
            final_file_name = "/tmp/sophos-spl/base/mcs/action/" + final_file_name
            glog_action = mock.MagicMock(side_effect=[[], [temp_file_name]])

            with mock.patch("mcsrouter.computer.os.rename", mocked_open_function) as mock_i:
                with mock.patch("mcsrouter.computer.glob.glob", glog_action) as glob_i:
                    c.run_commands(commands)
            mock_i.assert_called_once_with(temp_file_name, final_file_name)

if __name__ == '__main__':
    unittest.main()

