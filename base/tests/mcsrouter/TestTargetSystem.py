#!/usr/bin/env python
# Copyright 2020-2023 Sophos Limited. All rights reserved.

import json
import os
import unittest
from unittest import mock
import platform
import stat
from unittest.mock import patch, mock_open
import logging
logger = logging.getLogger("TestTargetSystem")

import PathManager
import mcsrouter.utils.target_system_manager
import mcsrouter.targetsystem

UBUNTU_2_VERSION = ["Ubuntu 18.04 LTS", "Ubuntu", "18.04"]
UBUNTU_3_VERSION = ["Ubuntu 18.04.2 LTS", "Ubuntu", "18.04"]
CENTOS_OS_1 = ["CentOS release 7 (Final)", "CentOS", "7"]
CENTOS_OS_2 = ["CentOS release 7.4 (Final)", "CentOS", "7.4"]
CENTOS_OS_3 = ["CentOS release 7.4.1.4 (Final)", "CentOS", "7.4.1.4"]
AMAZON_OS_3 = ["Amazon Linux 2023", "Amazon Linux", "2023"]
MIRACLE_OS_3 = ["MIRACLE LINUX 8.4 (Peony)", "MIRACLE LINUX", "8"]
ORACLE_OS_3 = ["Oracle Linux Server 8.6", "Oracle Linux Server", "8.6"]
INCOMPLETE_OS_RELEASE_FILE = "VERSION=\"20.04.6 LTS (Focal Fossa)\"\n" \
                             "ID=ubuntu\n" \
                             "ID_LIKE=debian\n" \
                             "HOME_URL=\"https://www.ubuntu.com/\"\n" \
                             "SUPPORT_URL=\"https://help.ubuntu.com/\"\n" \
                             "BUG_REPORT_URL=\"https://bugs.launchpad.net/ubuntu/\"\n" \
                             "PRIVACY_POLICY_URL=\"https://www.ubuntu.com/legal/terms-and-policies/privacy-policy\"\n" \
                             "VERSION_CODENAME=focal\n" \
                             "UBUNTU_CODENAME=focal\n"
DUPLICATE_FIELDS_OS_RELEASE_FILE = "VERSION=\"20.04.6 LTS (Focal Fossa)\"\n" \
                                   "ID=ubuntu\n" \
                                   "ID_LIKE=debian\n" \
                                   "HOME_URL=\"https://www.ubuntu.com/\"\n" \
                                   "SUPPORT_URL=\"https://help.ubuntu.com/\"\n" \
                                   "BUG_REPORT_URL=\"https://bugs.launchpad.net/ubuntu/\"\n" \
                                   "PRIVACY_POLICY_URL=\"https://www.ubuntu.com/legal/terms-and-policies/privacy-policy\"\n" \
                                   "VERSION_CODENAME=focal\n" \
                                   "UBUNTU_CODENAME=focal\n" \
                                   "NAME=\"Ubuntu\"\n" \
                                   "NAME=\"CentOS\"\n" \
                                   "NAME=\"Amazon Linux\"\n" \
                                   "PRETTY_NAME=\"Ubuntu 20.04.6 LTS\"\n" \
                                   "PRETTY_NAME=\"Ubuntu 22.07 LTS\"\n" \
                                   "PRETTY_NAME=\"Ubuntu 18.01.9 LTS\"\n" \
                                   "VERSION_ID=20.04\n" \
                                   "VERSION_ID=22.07\n" \
                                   "VERSION_ID=18.01\n"
EMPTY_FIELDS_OS_RELEASE_FILE = "VERSION=\"20.04.6 LTS (Focal Fossa)\"\n" \
                               "ID=ubuntu\n" \
                               "ID_LIKE=debian\n" \
                               "HOME_URL=\"https://www.ubuntu.com/\"\n" \
                               "SUPPORT_URL=\"https://help.ubuntu.com/\"\n" \
                               "BUG_REPORT_URL=\"https://bugs.launchpad.net/ubuntu/\"\n" \
                               "PRIVACY_POLICY_URL=\"https://www.ubuntu.com/legal/terms-and-policies/privacy-policy\"\n" \
                               "VERSION_CODENAME=focal\n" \
                               "UBUNTU_CODENAME=focal\n" \
                               "NAME=\n" \
                               "PRETTY_NAME=\n" \
                               "VERSION_ID=\n"
TOO_LARGE_FIELDS_OS_RELEASE_FILE = "VERSION=\"20.04.6 LTS (Focal Fossa)\"\n" \
                                   "ID=ubuntu\n" \
                                   "ID_LIKE=debian\n" \
                                   "HOME_URL=\"https://www.ubuntu.com/\"\n" \
                                   "SUPPORT_URL=\"https://help.ubuntu.com/\"\n" \
                                   "BUG_REPORT_URL=\"https://bugs.launchpad.net/ubuntu/\"\n" \
                                   "PRIVACY_POLICY_URL=\"https://www.ubuntu.com/legal/terms-and-policies/privacy-policy\"\n" \
                                   "VERSION_CODENAME=focal\n" \
                                   "UBUNTU_CODENAME=focal\n" \
                                   f"NAME=\"Ubuntu{'a'*1000}\"\n" \
                                   f"PRETTY_NAME=\"Ubuntu 18.01.9 LTS{'a'*1000}\"\n" \
                                   f"VERSION_ID=18.01{'a'*1000}\n"
INCORRECT_FORMAT_OS_RELEASE_FILE = "VERSION=\"20.04.6 LTS (Focal Fossa)\"\n" \
                                   "ID : ubuntu\n" \
                                   "ID_LIKE : debian\n" \
                                   "HOME_URL : \"https://www.ubuntu.com/\"\n" \
                                   "SUPPORT_URL : \"https://help.ubuntu.com/\"\n" \
                                   "BUG_REPORT_URL : \"https://bugs.launchpad.net/ubuntu/\"\n" \
                                   "PRIVACY_POLICY_URL : \"https://www.ubuntu.com/legal/terms-and-policies/privacy-policy\"\n" \
                                   "VERSION_CODENAME : focal\n" \
                                   "UBUNTU_CODENAME : focal\n" \
                                   f"NAME : \"Ubuntu\"\n" \
                                   f"PRETTY_NAME : \"Ubuntu 18.01.9 LTS\"\n" \
                                   f"VERSION_ID : 18.01\n"
INCORRECT_FORMAT_OS_RELEASE_FILE_2 = "VERSION=\"20.04.6 LTS (Focal Fossa)\"\n" \
                                   "ID : ubuntu\n" \
                                   "ID_LIKE : debian\n" \
                                   "HOME_URL : \"https://www.ubuntu.com/\"\n" \
                                   "SUPPORT_URL : \"https://help.ubuntu.com/\"\n" \
                                   "BUG_REPORT_URL : \"https://bugs.launchpad.net/ubuntu/\"\n" \
                                   "PRIVACY_POLICY_URL : \"https://www.ubuntu.com/legal/terms-and-policies/privacy-policy\"\n" \
                                   "VERSION_CODENAME : focal\n" \
                                   "UBUNTU_CODENAME : focal\n" \
                                   f"NAME=\"Ubuntu\"=\"CentOS\"\n" \
                                   f"PRETTY_NAME=\"Ubuntu 18.01.9 LTS\"=\"CentOS\"\n" \
                                   f"VERSION_ID=18.01=20\n"

@mock.patch('platform.node', return_value='examplehostname')
@mock.patch('platform.release', return_value='4.15.0-20-generic')
@mock.patch('platform.machine', return_value='x86_64')
class TestTargetSystem(unittest.TestCase):

    # OS Version Tests
    @mock.patch('mcsrouter.targetsystem._collect_os_release', return_value=UBUNTU_3_VERSION)
    def test_os_version_retrieves_three_part_ubuntu_version(self, *mockargs):
        target_system = mcsrouter.targetsystem.TargetSystem('/tmp/sophos-spl')
        self.assertEqual(('18', '04', '2'), target_system.os_version())

    @mock.patch('mcsrouter.targetsystem._collect_os_release', return_value=UBUNTU_2_VERSION)
    def test_os_version_retrieves_two_part_ubuntu_version(self, *mockargs):
        target_system = mcsrouter.targetsystem.TargetSystem('/tmp/sophos-spl')
        self.assertEqual(('18', '04', ''), target_system.os_version())

    @mock.patch('mcsrouter.targetsystem._collect_os_release', return_value=CENTOS_OS_1)
    def test_os_version_retrieves_centos_major_version(self, *mockargs):
        target_system = mcsrouter.targetsystem.TargetSystem('/tmp/sophos-spl')
        self.assertEqual('centos', target_system.vendor())
        self.assertEqual(['7', '', ''], target_system.os_version())

    @mock.patch('mcsrouter.targetsystem._collect_os_release', return_value=CENTOS_OS_2)
    def test_os_version_retrieves_centos_major_and_minor_version(self, *mockargs):
        target_system = mcsrouter.targetsystem.TargetSystem('/tmp/sophos-spl')
        self.assertEqual('centos', target_system.vendor())
        self.assertEqual(['7', '4', ''], target_system.os_version())

    @mock.patch('mcsrouter.targetsystem._collect_os_release', return_value=CENTOS_OS_3)
    def test_os_version_does_not_retrieve_four_centos_versions(self, *mockargs):
        target_system = mcsrouter.targetsystem.TargetSystem('/tmp/sophos-spl')
        self.assertEqual('centos', target_system.vendor())
        self.assertEqual(['7', '4', '1'], target_system.os_version())

    @mock.patch('mcsrouter.targetsystem._collect_os_release', return_value=AMAZON_OS_3)
    @mock.patch('builtins.open', new_callable=mock_open, read_data="Amazon Linux release")
    def test_os_version_retrieves_amazon_linux_vendor(self, *mockargs):
        target_system = mcsrouter.targetsystem.TargetSystem('/tmp/sophos-spl')
        self.assertEqual('amazon linux', target_system.vendor())

    @mock.patch('mcsrouter.targetsystem._collect_os_release', return_value=ORACLE_OS_3)
    @mock.patch('builtins.open', new_callable=mock_open, read_data="Oracle Linux Server release")
    def test_os_version_retrieves_oracle_vendor(self, *mockargs):
        target_system = mcsrouter.targetsystem.TargetSystem('/tmp/sophos-spl')
        self.assertEqual('oracle linux server', target_system.vendor())

    @mock.patch('mcsrouter.targetsystem._collect_os_release', return_value=MIRACLE_OS_3)
    @mock.patch('builtins.open', new_callable=mock_open, read_data="MIRACLE LINUX release")
    def test_os_version_retrieves_miracle_linux_vendor(self, *mockargs):
        target_system = mcsrouter.targetsystem.TargetSystem('/tmp/sophos-spl')
        self.assertEqual('miracle linux', target_system.vendor())

    @mock.patch('mcsrouter.targetsystem._collect_os_release', return_value=UBUNTU_3_VERSION)
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
    @mock.patch('mcsrouter.utils.filesystem_utils.atomic_write', return_value='')
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

    @mock.patch('mcsrouter.utils.filesystem_utils.return_lines_from_file', return_value=None)
    def test_os_release_file_not_existing_then_os_version_info_is_unknown(self, *mockargs):
        target_system = mcsrouter.targetsystem.TargetSystem('/tmp/sophos-spl')
        self.assertEqual(target_system.vendor(), "unknown")
        self.assertEqual(target_system.os_name(), "unknown")

    @mock.patch('mcsrouter.utils.filesystem_utils.return_lines_from_file', return_value={})
    def test_os_release_file_is_empty_then_os_version_info_is_unknown(self, *mockargs):
        target_system = mcsrouter.targetsystem.TargetSystem('/tmp/sophos-spl')
        self.assertEqual(target_system.vendor(), "unknown")
        self.assertEqual(target_system.os_name(), "unknown")

    @mock.patch('builtins.open', side_effect=OSError("mocking file cannot be read"))
    def test_os_release_file_cannot_be_read_then_os_version_info_is_unknown(self, *mockargs):
        target_system = mcsrouter.targetsystem.TargetSystem('/tmp/sophos-spl')
        self.assertEqual(target_system.vendor(), "unknown")
        self.assertEqual(target_system.os_name(), "unknown")

    @mock.patch('builtins.open', new_callable=mock_open, read_data=INCOMPLETE_OS_RELEASE_FILE)
    def test_os_release_file_does_not_contain_expected_fields_then_os_version_info_is_unknown(self, *mockargs):
        target_system = mcsrouter.targetsystem.TargetSystem('/tmp/sophos-spl')
        self.assertEqual(target_system.vendor(), "unknown")
        self.assertEqual(target_system.os_name(), "unknown")

    @mock.patch('builtins.open', new_callable=mock_open, read_data=DUPLICATE_FIELDS_OS_RELEASE_FILE)
    def test_os_release_file_contains_duplicate_fields_then_os_version_info_is_unknown(self, *mockargs):
        target_system = mcsrouter.targetsystem.TargetSystem('/tmp/sophos-spl')
        self.assertEqual(target_system.vendor(), "unknown")
        self.assertEqual(target_system.os_name(), "unknown")

    @mock.patch('builtins.open', new_callable=mock_open, read_data=EMPTY_FIELDS_OS_RELEASE_FILE)
    def test_os_release_file_contains_empty_fields_then_os_version_info_is_unknown(self, *mockargs):
        target_system = mcsrouter.targetsystem.TargetSystem('/tmp/sophos-spl')
        self.assertEqual(target_system.vendor(), "unknown")
        self.assertEqual(target_system.os_name(), "unknown")

    @mock.patch('builtins.open', new_callable=mock_open, read_data=TOO_LARGE_FIELDS_OS_RELEASE_FILE)
    def test_os_release_file_contains_too_large_fields_then_os_version_info_is_truncated(self, *mockargs):
        target_system = mcsrouter.targetsystem.TargetSystem('/tmp/sophos-spl')
        self.assertEqual(target_system.vendor(), f"ubuntu{'a'*(255 - 6)}")
        self.assertEqual(target_system.os_name(), "Ubuntu")

    @mock.patch('builtins.open', new_callable=mock_open, read_data=INCORRECT_FORMAT_OS_RELEASE_FILE)
    def test_os_release_file_has_incorrect_format_then_os_version_info_is_unknown(self, *mockargs):
        target_system = mcsrouter.targetsystem.TargetSystem('/tmp/sophos-spl')
        self.assertEqual(target_system.vendor(), "unknown")
        self.assertEqual(target_system.os_name(), "unknown")

    @mock.patch('builtins.open', new_callable=mock_open, read_data=INCORRECT_FORMAT_OS_RELEASE_FILE_2)
    def test_os_release_file_has_incorrect_format_2_then_os_version_info_is_unknown(self, *mockargs):
        target_system = mcsrouter.targetsystem.TargetSystem('/tmp/sophos-spl')
        self.assertEqual(target_system.vendor(), "unknown")
        self.assertEqual(target_system.os_name(), "unknown")


if __name__ == '__main__':
    unittest.main()

