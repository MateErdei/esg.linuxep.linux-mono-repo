import unittest
import mock
import logging

import PathManager
import mcsrouter.mcsclient.datafeeds

logger = logging.getLogger("TestDatafeeds")

class TestDatafeeds(unittest.TestCase):
    def test_datafeed_results_can_added(self):
        feed_id = "feed_id"
        content = '{key1: "value1", key2: "value2"}'
        datafeeds = mcsrouter.mcsclient.datafeeds.Datafeeds(feed_id)
        datafeeds.add_datafeed_result("/tmp/filepath1", feed_id, "1601033100", content)
        datafeeds.add_datafeed_result("/tmp/filepath2", feed_id, "1601033200", content)
        datafeeds.add_datafeed_result("/tmp/filepath3", feed_id, "1601033300", content)
        datafeed_results = datafeeds.get_datafeeds()
        self.assertEqual(len(datafeed_results), 3)
        self.assertEqual(datafeed_results[0].m_file_path, "/tmp/filepath1")
        self.assertEqual(datafeed_results[0].m_creation_time, "1601033100")
        self.assertEqual(datafeed_results[0].m_json_body, content)
        self.assertEqual(datafeed_results[1].m_file_path, "/tmp/filepath2")
        self.assertEqual(datafeed_results[1].m_creation_time, "1601033200")
        self.assertEqual(datafeed_results[1].m_json_body, content)
        self.assertEqual(datafeed_results[2].m_file_path, "/tmp/filepath3")
        self.assertEqual(datafeed_results[2].m_creation_time, "1601033300")
        self.assertEqual(datafeed_results[2].m_json_body, content)


    def test_datafeed_results_can_be_sorted(self):
        feed_id = "feed_id"
        content = '{key1: "value1", key2: "value2"}'
        datafeeds = mcsrouter.mcsclient.datafeeds.Datafeeds(feed_id)
        datafeeds.add_datafeed_result("/tmp/filepath1", feed_id, "1601033100", content)
        datafeeds.add_datafeed_result("/tmp/filepath3", feed_id, "1601033300", content)
        datafeeds.add_datafeed_result("/tmp/filepath2", feed_id, "1601033200", content)
        datafeed_results = datafeeds.get_datafeeds()
        self.assertEqual(len(datafeed_results), 3)

        datafeeds.sort_oldest_to_newest()
        self.assertEqual(datafeed_results[0].m_file_path, "/tmp/filepath1")
        self.assertEqual(datafeed_results[0].m_creation_time, "1601033100")
        self.assertEqual(datafeed_results[0].m_json_body, content)
        self.assertEqual(datafeed_results[1].m_file_path, "/tmp/filepath2")
        self.assertEqual(datafeed_results[1].m_creation_time, "1601033200")
        self.assertEqual(datafeed_results[1].m_json_body, content)
        self.assertEqual(datafeed_results[2].m_file_path, "/tmp/filepath3")
        self.assertEqual(datafeed_results[2].m_creation_time, "1601033300")
        self.assertEqual(datafeed_results[2].m_json_body, content)

        datafeeds.sort_newest_to_oldest()
        self.assertEqual(datafeed_results[0].m_file_path, "/tmp/filepath3")
        self.assertEqual(datafeed_results[0].m_creation_time, "1601033300")
        self.assertEqual(datafeed_results[1].m_file_path, "/tmp/filepath2")
        self.assertEqual(datafeed_results[1].m_creation_time, "1601033200")
        self.assertEqual(datafeed_results[2].m_file_path, "/tmp/filepath1")
        self.assertEqual(datafeed_results[2].m_creation_time, "1601033100")

    def test_prune_old_datafeed_files_are_removed(self):
        feed_id = "feed_id"
        content = '{key1: "value1", key2: "value2"}'
        datafeeds = mcsrouter.mcsclient.datafeeds.Datafeeds(feed_id)
        datafeeds.add_datafeed_result("/tmp/filepath1", feed_id, "1501033100", content)
        datafeeds.add_datafeed_result("/tmp/filepath2", feed_id, "1501033200", content)
        # This timestamp (2601033300) is far in the future (2052) so will be not too old.
        datafeeds.add_datafeed_result("/tmp/filepath3", feed_id, "2601033300", content)
        datafeed_results = datafeeds.get_datafeeds()
        self.assertEqual(len(datafeed_results), 3)
        datafeeds.prune_old_datafeed_files()
        # Take another reference to the datafeeds list
        datafeed_results = datafeeds.get_datafeeds()
        self.assertEqual(len(datafeed_results), 1)

    def test_prune_old_datafeed_files_passes_if_no_files(self):
        feed_id = "feed_id"
        datafeeds = mcsrouter.mcsclient.datafeeds.Datafeeds(feed_id)
        datafeed_results = datafeeds.get_datafeeds()
        self.assertEqual(len(datafeed_results), 0)
        datafeeds.prune_old_datafeed_files()

        # Take another reference to the datafeeds list
        datafeed_results = datafeeds.get_datafeeds()
        self.assertEqual(len(datafeed_results), 0)

    def test_prune_old_datafeed_files_passes_if_no_old_files(self):
        feed_id = "feed_id"
        content = '{key1: "value1", key2: "value2"}'
        datafeeds = mcsrouter.mcsclient.datafeeds.Datafeeds(feed_id)

        # This timestamp (2601033300) is far in the future (2052) so will be not too old.
        datafeeds.add_datafeed_result("/tmp/filepath1", feed_id, "2601033100", content)
        datafeeds.add_datafeed_result("/tmp/filepath2", feed_id, "2601033200", content)
        datafeeds.add_datafeed_result("/tmp/filepath3", feed_id, "2601033300", content)

        datafeed_results = datafeeds.get_datafeeds()
        self.assertEqual(len(datafeed_results), 3)
        datafeeds.prune_old_datafeed_files()

        # Take another reference to the datafeeds list
        datafeed_results = datafeeds.get_datafeeds()
        self.assertEqual(len(datafeed_results), 3)

    def test_datafeed_result_is_alive_for_old_and_new_files(self):
        feed_id = "feed_id"
        content = '{key1: "value1", key2: "value2"}'
        datafeeds = mcsrouter.mcsclient.datafeeds.Datafeeds(feed_id)
        datafeeds.add_datafeed_result("/tmp/filepath1", feed_id, "1501033100", content)
        datafeeds.add_datafeed_result("/tmp/filepath2", feed_id, "1501033200", content)
        # This timestamp (2601033300) is far in the future (2052) so will be not too old.
        datafeeds.add_datafeed_result("/tmp/filepath3", feed_id, "2601033300", content)
        datafeed_results = datafeeds.get_datafeeds()
        self.assertEqual(len(datafeed_results), 3)
        self.assertFalse(datafeeds._datafeed_result_is_alive(datafeed_results[0]))
        self.assertFalse(datafeeds._datafeed_result_is_alive(datafeed_results[1]))
        self.assertTrue(datafeeds._datafeed_result_is_alive(datafeed_results[2]))


    def test_datafeed_load_config(self):
        feed_id = "feed_id"
        config = """
        {
            "retention_seconds": 1209600,
            "max_backlog_bytes": 1000000000,
            "max_send_freq_seconds": 5,
            "max_upload_bytes": 10000000,
            "max_item_size_bytes": 10000000
        }"""
        with mock.patch("builtins.open", mock.mock_open(read_data=config)) as mock_file:
            datafeeds = mcsrouter.mcsclient.datafeeds.Datafeeds(feed_id)
            mock_file.assert_called_with("/tmp/sophos-spl/base/etc/datafeed-config-feed_id.json", 'r')

        self.assertEqual(datafeeds.get_time_to_live(), 1209600)
        self.assertEqual(datafeeds.get_max_backlog(), 1000000000)
        self.assertEqual(datafeeds.get_max_send_freq(), 5)
        self.assertEqual(datafeeds.get_max_upload_at_once(), 10000000)
        self.assertEqual(datafeeds.get_max_size_single_feed_result(), 10000000)

    def test_datafeed_load_config_defaults_if_file_missing(self):
        feed_id = "feed_id"
        datafeeds = mcsrouter.mcsclient.datafeeds.Datafeeds(feed_id)
        self.assertEqual(datafeeds.get_time_to_live(), 1209600)
        self.assertEqual(datafeeds.get_max_backlog(), 1000000000)
        self.assertEqual(datafeeds.get_max_send_freq(), 5)
        self.assertEqual(datafeeds.get_max_upload_at_once(), 10000000)
        self.assertEqual(datafeeds.get_max_size_single_feed_result(), 10000000)



