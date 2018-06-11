#!/usr/bin/env python

import Timer

class StatusTimer(Timer.Timer):
    """
    Object to indicate when the next status event should be sent.
    """
    def __init__(self, latency=30, interval=60*60*24):
        """
        @param latency the time to wait after a status is updated, before sending it onwards
        @param interval the time to wait to send a status even if nothing changes
        """
        super(StatusTimer,self).__init__(latency,interval)

    def statusUpdated(self):
        return self._latencyTrigger()

    def statusSent(self):
        return self._intervalTrigger()

    def sendStatus(self):
        return self._triggered()

    def errorSendingStatus(self):
        self._handleError()
