#!/usr/bin/env python3
# Copyright 2019 Sophos Plc, Oxford, England.

"""
responses Module
"""

import logging



LOGGER = logging.getLogger(__name__)


class Response(object):
    """
    Response
    """

    def __init__(self, app_id, correlation_id, creation_time, body):
        self.m_app_id = app_id
        self.m_correlation_id = correlation_id
        self.m_creation_time = creation_time
        self.m_body = body

class Responses(object):
    """
    Responses Class
    """
    def __init__(self):
        """
        __init__
        """
        self.__m_responses = []

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

    def has_responses(self):
        """
        has_responses
        """
        if self.__m_responses:
            return True
        return False
