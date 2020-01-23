#!/usr/bin/env python

import os
import unittest
import mock
import platform
from mock import patch, mock_open
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



if __name__ == '__main__':
    unittest.main()

