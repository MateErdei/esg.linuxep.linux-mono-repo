#!/usr/bin/env python3
# Copyright 2019 Sophos Plc, Oxford, England.

"""
responses Module
"""

import logging
import gzip
import time

from mcsrouter.utils import timestamp


LOGGER = logging.getLogger(__name__)


class Response(object):
    """
    Response
    """

    def __init__(self, app_id, correlation_id, creation_time, body):
        self.m_app_id = app_id
        self.m_correlation_id = correlation_id
        self.m_creation_time = creation_time
        self.m_json_body = body
        self.m_gzip_body = self._get_compressed_json()
        self.m_json_body_size = self._get_decompressed_body_size()
        self.m_gzip_body_size = self._get_compressed_body_size()


    def get_command_path(self, endpoint_id):
        """
        get_command_path
        :param endpoint_id:
        :return: command_path as string
        """
        return "/responses/endpoint/{}/app_id/{}/correlation_id/{}/".format(
            endpoint_id,
            self.m_app_id,
            self.m_correlation_id)

    def _get_compressed_body_size(self):
        return len(self.m_gzip_body)

    def _get_decompressed_body_size(self):
        return len(self.m_json_body)

    def _get_compressed_json(self):
        return gzip.compress(bytes(self.m_json_body, 'utf-8'))

class Responses(object):
    """
    Responses Class
    """
    def __init__(self):
        """
        __init__
        """
        self.__m_responses = []
        self.__m_time_to_live = 30

    def add_response(self, app_id, correlation_id, creation_time, body):
        """
        add_response
        """
        self.__m_responses.append(
            Response(
                app_id,
                correlation_id,
                creation_time,
                body))

    def reset(self):
        """
        reset
        """
        self.__m_responses = []

    def get_responses(self):
        return self.__m_responses

    def _response_is_alive(self, response):
        if response.m_creation_time > timestamp.timestamp(time.time() - self.__m_time_to_live):
            return True
        return False

    def prune_old_responses(self):
        self.__m_responses = list(filter(self._response_is_alive, self.__m_responses))

    def has_responses(self):
        """
        has_responses
        """
        self.prune_old_responses()
        if self.__m_responses:
            return True
        return False
