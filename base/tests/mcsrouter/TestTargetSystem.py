#!/usr/bin/env python
import json
import os
import unittest
from unittest import mock
import platform
import stat
from unittest.mock import patch, mock_open
import logging
logger = logging.getLogger("TestResponseReceiver")

import PathManager
import mcsrouter.utils.target_system_manager
import mcsrouter.targetsystem

UBUNTU_2_VERSION = ["Ubuntu 18.04 LTS", "Ubuntu", "18.04"]
UBUNTU_3_VERSION = ["Ubuntu 18.04.2 LTS", "Ubuntu", "18.04"]
CENTOS_LSB_1 = ["CentOS release 7 (Final)", "CentOS", "7"]
CENTOS_LSB_2 = ["CentOS release 7.4 (Final)", "CentOS", "7.4"]
CENTOS_LSB_3 = ["CentOS release 7.4.1.4 (Final)", "CentOS", "7.4.1.4"]
NO_LSB = [None, None, None]

@mock.patch('platform.node', return_value='examplehostname')
@mock.patch('platform.release', return_value='4.15.0-20-generic')
@mock.patch('platform.machine', return_value='x86_64')
class TestTargetSystem(unittest.TestCase):

    # OS Version Tests
    @mock.patch('mcsrouter.targetsystem._collect_lsb_release', return_value=UBUNTU_3_VERSION)
    def test_os_version_retrieves_three_part_ubuntu_version(self, *mockargs):
        target_system = mcsrouter.targetsystem.TargetSystem('/tmp/sophos-spl')
        self.assertEqual(('18', '04', '2'), target_system.os_version())

    @mock.patch('mcsrouter.targetsystem._collect_lsb_release', return_value=UBUNTU_2_VERSION)
    def test_os_version_retrieves_two_part_ubuntu_version(self, *mockargs):
        target_system = mcsrouter.targetsystem.TargetSystem('/tmp/sophos-spl')
        self.assertEqual(('18', '04', ''), target_system.os_version())

    @mock.patch('mcsrouter.targetsystem._collect_lsb_release', return_value=NO_LSB)
    def test_os_version_retrieves_two_part_ubuntu_version(self, *mockargs):
        target_system = mcsrouter.targetsystem.TargetSystem('/tmp/sophos-spl')
        self.assertEqual(['4', '15', '0-20-generic'], target_system.os_version())

    @mock.patch('mcsrouter.targetsystem._collect_lsb_release', return_value=NO_LSB)
    @mock.patch('builtins.open', new_callable=mock_open, read_data="CentOS Linux release 7.5.1804 (Core)")
    def test_os_version_retrieves_centos_vendor(self, *mockargs):
        target_system = mcsrouter.targetsystem.TargetSystem('/tmp/sophos-spl')
        self.assertEqual('centos', target_system.vendor())

    @mock.patch('mcsrouter.targetsystem._collect_lsb_release', return_value=CENTOS_LSB_1)
    def test_os_version_retrieves_centos_major_version(self, *mockargs):
        target_system = mcsrouter.targetsystem.TargetSystem('/tmp/sophos-spl')
        self.assertEqual('centos', target_system.vendor())
        self.assertEqual(['7', '', ''], target_system.os_version())

    @mock.patch('mcsrouter.targetsystem._collect_lsb_release', return_value=CENTOS_LSB_2)
    def test_os_version_retrieves_centos_major_and_minor_version(self, *mockargs):
        target_system = mcsrouter.targetsystem.TargetSystem('/tmp/sophos-spl')
        self.assertEqual('centos', target_system.vendor())
        self.assertEqual(['7', '4', ''], target_system.os_version())

    @mock.patch('mcsrouter.targetsystem._collect_lsb_release', return_value=CENTOS_LSB_3)
    def test_os_version_does_not_retrieve_four_centos_versions(self, *mockargs):
        target_system = mcsrouter.targetsystem.TargetSystem('/tmp/sophos-spl')
        self.assertEqual('centos', target_system.vendor())
        self.assertEqual(['7', '4', '1'], target_system.os_version())

    @mock.patch('mcsrouter.targetsystem._collect_lsb_release', return_value=NO_LSB)
    @mock.patch('builtins.open', new_callable=mock_open, read_data="Amazon Linux release")
    def test_os_version_retrieves_amazon_linux_vendor(self, *mockargs):
        target_system = mcsrouter.targetsystem.TargetSystem('/tmp/sophos-spl')
        self.assertEqual('amazon', target_system.vendor())

    @mock.patch('mcsrouter.targetsystem._collect_lsb_release', return_value=NO_LSB)
    @mock.patch('builtins.open', new_callable=mock_open, read_data="Oracle Linux Server release")
    def test_os_version_retrieves_oracle_vendor(self, *mockargs):
        target_system = mcsrouter.targetsystem.TargetSystem('/tmp/sophos-spl')
        self.assertEqual('oracle', target_system.vendor())

    @mock.patch('mcsrouter.targetsystem._collect_lsb_release', return_value=NO_LSB)
    @mock.patch('builtins.open', new_callable=mock_open, read_data="MIRACLE LINUX release")
    def test_os_version_retrieves_miracle_linux_vendor(self, *mockargs):
        target_system = mcsrouter.targetsystem.TargetSystem('/tmp/sophos-spl')
        self.assertEqual('miracle', target_system.vendor())



    @mock.patch('mcsrouter.targetsystem._collect_lsb_release', return_value=NO_LSB)
    @mock.patch('builtins.open', new_callable=mock_open, read_data="Fedora release 27")
    def test_os_version_retrieves_unknown_vendor(self, *mockargs):
        target_system = mcsrouter.targetsystem.TargetSystem('/tmp/sophos-spl')
        self.assertEqual('unknown', target_system.vendor())

    @mock.patch('mcsrouter.targetsystem._collect_lsb_release', return_value=UBUNTU_3_VERSION)
    def test_read_uname_calls_os_uname(self, *mockargs):
        target_system = mcsrouter.targetsystem.TargetSystem('/tmp/sophos-spl')
        uname = mcsrouter.targetsystem._read_uname()
        self.assertEqual(uname[0], 'examplehostname')
        self.assertTrue(platform.node.call_count, 1)
        self.assertTrue(platform.release.call_count, 1)
        self.assertTrue(platform.machine.call_count, 1)

    @mock.patch('mcsrouter.utils.path_manager.instance_metadata_config', return_value='/tmp/test-instance-metadata.json')
    @mock.patch('mcsrouter.utils.path_manager.temp_dir', return_value='/tmp/')
    def test_writing_to_metadata_conf_correctly_writes_to_file(self, *mockargs):
        test_filepath = '/tmp/test-instance-metadata.json'
        test_json = {"platform": "test",
                     "region": "test_region",
                     "accountId": "test_id",
                     "instanceId": "test_instance"
                     }
        mcsrouter.targetsystem.write_info_to_metadata_json(test_json)
        with open(test_filepath, 'r') as test_result:
            result = json.load(test_result)
        self.assertEqual(test_json, result)

        self.assertEqual(oct(os.stat(test_filepath)[stat.ST_MODE]), "0o100640")
        os.remove(test_filepath)

    @mock.patch("logging.Logger.warning")
    @mock.patch('mcsrouter.utils.path_manager.instance_metadata_config', return_value='/tmp/test-instance-metadata.json')
    @mock.patch('mcsrouter.utils.path_manager.temp_dir', return_value='/tmp/')
    @mock.patch('os.getuid', return_value=0)
    @mock.patch('mcsrouter.utils.atomic_write.atomic_write', return_value='')
    def test_chown_to_metadata_conf_logs_on_permission_error(self, *mockargs):

        test_json = {"platform": "test",
                     "region": "test_region",
                     "accountId": "test_id",
                     "instanceId": "test_instance"
                     }
        with patch('os.chown') as mock_open:
            mock_open.side_effect = PermissionError
            mcsrouter.targetsystem.write_info_to_metadata_json(test_json)
        self.assertEqual(logging.Logger.warning.call_count, 1)
        expected_args = mock.call("Cannot update file /tmp/test-instance-metadata.json with error : ")
        self.assertEqual(logging.Logger.warning.call_args, expected_args)

if __name__ == '__main__':
    unittest.main()

