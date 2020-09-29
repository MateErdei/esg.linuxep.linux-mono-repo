#!/usr/bin/env python3
# Copyright 2020 Sophos Plc, Oxford, England.

"""
datafeeds Module
"""

import logging
import gzip
import time
import os
import json

from mcsrouter.utils import path_manager

LOGGER = logging.getLogger(__name__)

class Datafeed(object):

    def __init__(self, file_path: str, datafeed_id_name: str, creation_time: str, body: str):
        self.m_file_path = file_path
        self.m_datafeed_id = datafeed_id_name
        self.m_creation_time = creation_time
        self.m_json_body = body
        self.m_gzip_body = self._get_compressed_json()
        self.m_json_body_size = self._get_decompressed_body_size()
        self.m_gzip_body_size = self._get_compressed_body_size()

    def remove_datafeed_file(self):
        if os.path.isfile(self.m_file_path):
            os.remove(self.m_file_path)
            LOGGER.debug("Removed {} datafeed file: {}".format(self.m_datafeed_id, self.m_file_path))

    def get_command_path(self, endpoint_id):
        """
        get_command_path
        :param endpoint_id:
        :return: command_path as string
        """
        if self.m_datafeed_id:
            return "/data_feed/endpoint/{}/feed_id/{}".format(endpoint_id, self.m_datafeed_id)

        return None

    def get_creation_time(self):
        return self.m_creation_time

    def get_json_body_size(self):
        return self.m_json_body_size

    def get_gzip_body_size(self):
        return self.m_gzip_body_size

    def _get_compressed_body_size(self):
        return len(self.m_gzip_body)

    def _get_decompressed_body_size(self):
        return len(self.m_json_body)

    def _get_compressed_json(self):
        return gzip.compress(bytes(self.m_json_body, 'utf-8'))

class Datafeeds(object):
    def __init__(self, feed_id: str):
        self.__m_feed_id = feed_id
        self.__config_file_path = os.path.join(path_manager.etc_dir(), "datafeed-config-{}.json".format(feed_id))
        self.__m_datafeeds = []
        self.__m_backoff_until_time = 0
        self._load_config()

    def _load_config(self):
        try:
            with open(self.__config_file_path, 'r') as config_file:
                config = json.loads(config_file.read())
                self.__m_time_to_live = config["retention_seconds"]
                self.__m_max_backlog = config["max_backlog_bytes"]
                self.__m_max_send_freq = config["max_send_freq_seconds"]
                self.__m_max_upload_at_once = config["max_upload_bytes"]
                self.__m_max_size_single_feed_result = config["max_item_size_bytes"]
                LOGGER.debug("Loaded config for datafeed, from: {}".format(self.__config_file_path))
        except Exception as ex:
            LOGGER.warning("Could not load config for datafeed, using default values. Error: {}".format(str(ex)))
            self.__m_time_to_live = 1209600
            self.__m_max_backlog = 1000000000
            self.__m_max_send_freq = 5
            self.__m_max_upload_at_once = 10000000
            self.__m_max_size_single_feed_result = 10000000

    def add_datafeed_result(self, file_path, datafeed_id, creation_time, body):
        if any(x.m_file_path == file_path for x in self.__m_datafeeds):
            return False

        self.__m_datafeeds.append(
            Datafeed(
                file_path,
                datafeed_id,
                creation_time,
                body))
        return True

    def purge(self):
        for datafeed in self.__m_datafeeds:
            datafeed.remove_datafeed_file()
        self.__m_datafeeds = []

    def get_feed_id(self):
        return self.__m_feed_id

    def get_time_to_live(self):
        return self.__m_time_to_live

    def get_max_backlog(self):
        return self.__m_max_backlog

    def get_max_upload_at_once(self):
        return self.__m_max_upload_at_once

    def get_max_size_single_feed_result(self):
        return self.__m_max_size_single_feed_result

    def get_datafeeds(self):
        return self.__m_datafeeds

    def get_backoff_until_time(self):
        return self.__m_backoff_until_time

    def set_backoff_until_time(self, back_off_until: float):
        self.__m_backoff_until_time = back_off_until

    def get_max_send_freq(self):
        return self.__m_max_send_freq

    def _datafeed_result_is_alive(self, datafeed_result: Datafeed):
        creation_time = float(datafeed_result.m_creation_time)
        time_to_live_seconds_ago = time.time() - self.__m_time_to_live
        return creation_time > time_to_live_seconds_ago

    def _datafeed_result_is_too_large(self, datafeed_result: Datafeed):
        return datafeed_result.m_json_body_size > self.__m_max_size_single_feed_result

    def prune_old_datafeed_files(self):
        datafeed_results_to_keep = []
        for datafeed_result in self.__m_datafeeds:
            if self._datafeed_result_is_alive(datafeed_result):
                datafeed_results_to_keep.append(datafeed_result)
            else:
                LOGGER.debug(f"Datafeed result file too old, deleting: {datafeed_result.m_file_path}")
                datafeed_result.remove_datafeed_file()
        self.__m_datafeeds = datafeed_results_to_keep

    def prune_too_large_datafeed_files(self):
        datafeed_results_to_keep = []
        for datafeed_result in self.__m_datafeeds:
            if self._datafeed_result_is_too_large(datafeed_result):
                LOGGER.debug(f"Datafeed result file too large, deleting: {datafeed_result.m_file_path}")
                datafeed_result.remove_datafeed_file()
            else:
                datafeed_results_to_keep.append(datafeed_result)
        self.__m_datafeeds = datafeed_results_to_keep

    def prune_backlog_to_max_size(self):
        current_backlog_size = 0
        datafeed_results_to_keep = []
        self.sort_newest_to_oldest()
        for datafeed_result in self.__m_datafeeds:
            current_backlog_size += datafeed_result.get_json_body_size()
            if current_backlog_size > self.get_max_backlog():
                LOGGER.debug(f"Current backlog size exceeded max {self.get_max_backlog()} bytes, deleting: {datafeed_result.m_file_path}")
                datafeed_result.remove_datafeed_file()
            else:
                datafeed_results_to_keep.append(datafeed_result)
        self.__m_datafeeds = datafeed_results_to_keep

    def prune_results_with_no_files(self):
        datafeed_results_to_keep = []
        for datafeed_result in self.__m_datafeeds:
            if os.path.exists(datafeed_result.m_file_path):
                datafeed_results_to_keep.append(datafeed_result)
        self.__m_datafeeds = datafeed_results_to_keep

    def sort_oldest_to_newest(self):
        self.__m_datafeeds.sort(key=lambda x: x.get_creation_time(), reverse=False)

    def sort_newest_to_oldest(self):
        self.__m_datafeeds.sort(key=lambda x: x.get_creation_time(), reverse=True)

    def has_results(self):
        """
        has_results
        """
        return len(self.__m_datafeeds) > 0
