#!/usr/bin/env python

import Timer


class StatusTimer(Timer.Timer):
    """
    Object to indicate when the next status event should be sent.
    """

    def __init__(self, latency=30, interval=60 * 60 * 24):
        """
        @param latency the time to wait after a status is updated, before sending it onwards
        @param interval the time to wait to send a status even if nothing changes
        """
        super(StatusTimer, self).__init__(latency, interval)

    def status_updated(self):
        return self._latency_trigger()

    def status_sent(self):
        return self._interval_trigger()

    def send_status(self):
        return self._triggered()

    def error_sending_status(self):
        self._handle_error()
