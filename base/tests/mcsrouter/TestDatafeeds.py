import unittest
import mock
import logging
import zlib

import PathManager
import mcsrouter.mcsclient.datafeeds
import mcsrouter.utils.write_json

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
        datafeeds.add_datafeed_result("/tmp/filepath4", feed_id, "invalid", content)
        datafeed_results = datafeeds.get_datafeeds()
        self.assertEqual(len(datafeed_results), 4)
        self.assertFalse(datafeeds._datafeed_result_is_alive(datafeed_results[0]))
        self.assertFalse(datafeeds._datafeed_result_is_alive(datafeed_results[1]))
        self.assertTrue(datafeeds._datafeed_result_is_alive(datafeed_results[2]))
        self.assertFalse(datafeeds._datafeed_result_is_alive(datafeed_results[3]))


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


    def test_datafeed_compression_is_deflate(self):
        feed_id = "feed_id"
        content = '{key1: "value1", key2: "value2"}'
        datafeeds = mcsrouter.mcsclient.datafeeds.Datafeeds(feed_id)
        datafeeds.add_datafeed_result("/tmp/filepath1", feed_id, "1601033100", content)
        datafeed_results = datafeeds.get_datafeeds()
        self.assertEqual(len(datafeed_results), 1)
        self.assertEqual(datafeed_results[0].m_file_path, "/tmp/filepath1")
        self.assertEqual(datafeed_results[0].m_creation_time, "1601033100")
        self.assertEqual(datafeed_results[0].m_json_body, content)
        expected_compressed = zlib.compress(bytes(content, "utf-8"))
        self.assertEqual(expected_compressed, datafeed_results[0].m_compressed_body)
        self.assertEqual(len(expected_compressed), datafeed_results[0].m_compressed_body_size)

    @mock.patch("mcsrouter.utils.write_json.update_datafeed_size", return_value="jake is cool af")
    def test_send_datafeed_files_counts_total_compressed_size_properly(self, *mockargs):
        mocked_datafeeds = [
            mcsrouter.mcsclient.datafeeds.Datafeeds("scheduled_query"),
            mcsrouter.mcsclient.datafeeds.Datafeeds("not_scheduled_query"),
            mcsrouter.mcsclient.datafeeds.Datafeeds("stil_not_scheduled_query")
        ]

        body1 = "garbage"
        body1size = len(zlib.compress(bytes(body1, 'utf-8')))
        body2 = "not a real body"
        body2size = len(zlib.compress(bytes(body2, 'utf-8')))
        body3 = "a third one"
        body3size = len(zlib.compress(bytes(body3, 'utf-8')))
        size_total = body1size + body2size + body3size

        mocked_datafeeds[0].add_datafeed_result("/tmp/filepath1", "scheduled_query", "1601033100", body1)
        mocked_datafeeds[0].add_datafeed_result("/tmp/filepath2", "scheduled_query", "1601033100", body2)
        mocked_datafeeds[1].add_datafeed_result("/tmp/filepath3", "not_scheduled_query", "1601033100", body3)

        comms = mock.Mock()
        comms.send_datafeeds = lambda x, y: True
        v2_datafeed_available = mock.Mock()
        mcsrouter.mcsclient.datafeeds.Datafeeds.send_datafeed_files(v2_datafeed_available, mocked_datafeeds, comms)
        mcsrouter.utils.write_json.update_datafeed_size.assert_called_with(size_total)