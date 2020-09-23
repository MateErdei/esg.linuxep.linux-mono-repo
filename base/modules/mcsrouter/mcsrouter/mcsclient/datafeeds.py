#!/usr/bin/env python3
# Copyright 2020 Sophos Plc, Oxford, England.

"""
datafeeds Module
"""

import logging
import gzip
import time
import os

from mcsrouter.utils import timestamp


LOGGER = logging.getLogger(__name__)


# def lookup_datafeed_id(datafeed_id_name):
#     if datafeed_id_name == "scheduled":
#         return "scheduled_query"
#     return None


class Datafeed(object):

    def __init__(self, file_path, datafeed_id_name, creation_time, body):
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

    def _get_compressed_body_size(self):
        return len(self.m_gzip_body)

    def _get_decompressed_body_size(self):
        return len(self.m_json_body)

    def _get_compressed_json(self):
        return gzip.compress(bytes(self.m_json_body, 'utf-8'))


class Datafeeds(object):
    def __init__(self):
        self.__m_datafeeds = []
        self.__m_time_to_live = 300  # in seconds TODO change this to be proper value
        self.__m_max_size_single_feed_result = 300  # in bytes TODO change this to be proper value

    def add_datafeed_result(self, file_path, datafeed_name, creation_time, body):
        self.__m_datafeeds.append(
            Datafeed(
                file_path,
                datafeed_name,
                creation_time,
                body))

    def reset(self):
        self.__m_datafeeds = []

    def get_datafeeds(self):
        return self.__m_datafeeds

    def _datafeed_result_is_alive(self, datafeed_result: Datafeed):
        return datafeed_result.m_creation_time > timestamp.timestamp(time.time() - self.__m_time_to_live)

    def _datafeed_result_is_too_large(self, datafeed_result: Datafeed):
        return datafeed_result.m_gzip_body_size > self.__m_max_size_single_feed_result

    def prune_old_datafeed_files(self):
        for datafeed_result in self.__m_datafeeds:
            if not self._datafeed_result_is_alive(datafeed_result):
                datafeed_result.remove_datafeed_file()
                self.__m_datafeeds.remove(datafeed_result)

    def prune_too_large_datafeed_files(self):
        for datafeed_result in self.__m_datafeeds:
            if self._datafeed_result_is_too_large(datafeed_result):
                datafeed_result.remove_datafeed_file()
                self.__m_datafeeds.remove(datafeed_result)

    def has_results(self):
        """
        has_results
        """
        # TODO
        #self.prune_old_datafeed_files()
        if self.__m_datafeeds:
            return True
        return False
