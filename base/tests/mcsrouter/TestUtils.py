


import os
import unittest
import mock
import sys
import json
import builtins

import logging
logger = logging.getLogger("TestUtils")

BUILD_DIR=os.environ.get("ABS_BUILDDIR",".")
INSTALL_DIR=os.path.join(BUILD_DIR,"install")

import PathManager

import mcsrouter.utils.plugin_registry as plugin_registry


class TestPluginRegistry(unittest.TestCase):
    @mock.patch('os.listdir', return_value=["dummyplugin.json"])
    def test_added_and_removed_app_ids(self, *mockargs):
        mocked_open_function = mock.mock_open(read_data="""{"policyAppIds":["ALC","MDR"]}""")

        with mock.patch("builtins.open", mocked_open_function):
            pr = plugin_registry.PluginRegistry(INSTALL_DIR)
            added, removed = pr.added_and_removed_app_ids()
            self.assertEqual( added, ['ALC', 'MDR'])
            self.assertEqual(removed, [])
        mocked_open_function = mock.mock_open(read_data="""{"policyAppIds":["MDR"]}""")

        with mock.patch("builtins.open", mocked_open_function):
            added, removed = pr.added_and_removed_app_ids()
            self.assertEqual( added, [])
            self.assertEqual(removed, ['ALC'])






if __name__ == '__main__':
    import logging
    unittest.main()
