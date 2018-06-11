#!/usr/bin/env python

import time

class Timer(object):
    """
    Object to indicate when the next status event should be sent.
    """
    def __init__(self, latency=30, interval=60*60*24):
        """
        @param latency the time to wait after a status is updated, before sending it onwards
        @param interval the time to wait to send a status even if nothing changes
        """
        self.__m_latency = latency
        self.__m_interval = interval
        self.__m_nextTime = time.time()

    def absoluteTime(self):
        return self.__m_nextTime

    def relativeTime(self):
        return self.__m_nextTime - time.time()

    def _latencyTrigger(self):
        """
        Something has happened, we need to do an event at the latency timer
        """
        if self.relativeTime() > self.__m_latency:
            self.__m_nextTime = time.time() + self.__m_latency

    def _intervalTrigger(self):
        """
        We've done the operation, so go for the interval now
        """
        self.__m_nextTime = time.time() + self.__m_interval

    def _triggered(self):
        return self.relativeTime() <= 0

    def _handleError(self):
        """
        If we got an error, then try again in 30 seconds
        """
        self.__m_nextTime = time.time() + 30
