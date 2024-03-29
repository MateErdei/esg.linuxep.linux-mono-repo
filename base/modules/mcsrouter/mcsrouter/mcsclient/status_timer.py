#!/usr/bin/env python3
# Copyright 2019 Sophos Plc, Oxford, England.

"""
status_timer Module
"""

import mcsrouter.mcsclient.timer


class StatusTimer(mcsrouter.mcsclient.timer.Timer):
    """
    Object to indicate when the next status event should be sent.
    """

    def __init__(self, latency=30, interval=60 * 60 * 24):
        """
        @param latency the time to wait after a status is updated, before sending it onwards
        @param interval the time to wait to send a status even if nothing changes
        """
        super(StatusTimer, self).__init__(latency, interval)
        self.__m_latency = latency
        self.__m_interval = interval

    def status_updated(self):
        """
        status_updated
        """
        return self._latency_trigger()

    def status_sent(self):
        """
        status_sent
        """
        return self._interval_trigger()

    def send_status(self):
        """
        send_status
        """
        return self._triggered()

    def error_sending_status(self):
        """
        error_sending_status
        """
        self._handle_error()
