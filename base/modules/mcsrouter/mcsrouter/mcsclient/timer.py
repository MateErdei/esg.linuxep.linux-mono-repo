#!/usr/bin/env python
"""
timer Module
"""

import time


class Timer:
    """
    Object to indicate when the next status event should be sent.
    """

    def __init__(self, latency=30, interval=60 * 60 * 24):
        """
        @param latency the time to wait after a status is updated, before sending it onwards
        @param interval the time to wait to send a status even if nothing changes
        """
        self.__m_latency = latency
        self.__m_interval = interval
        self.__m_next_time = time.time()

    def absolute_time(self):
        """
        absolute_time
        """
        return self.__m_next_time

    def relative_time(self):
        """
        relative_time
        """
        return self.__m_next_time - time.time()

    def _latency_trigger(self):
        """
        Something has happened, we need to do an event at the latency timer
        """
        if self.relative_time() > self.__m_latency:
            self.__m_next_time = time.time() + self.__m_latency

    def _interval_trigger(self):
        """
        We've done the operation, so go for the interval now
        """
        self.__m_next_time = time.time() + self.__m_interval

    def _triggered(self):
        """
        _triggered
        """
        return self.relative_time() <= 0

    def _handle_error(self):
        """
        If we got an error, then try again in 30 seconds
        """
        self.__m_next_time = time.time() + 30
