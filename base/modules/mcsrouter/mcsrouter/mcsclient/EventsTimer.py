#!/usr/bin/env python
# -*- coding: utf-8 -*-

import time
import sys

class EventsTimer(object):
    """
    MCS protocol Specification 15.2.4:
 RMS does not aggregate events. The principle here is that an event should
 be passed to the server as soon as possible and each event is sent by the
 RMS Agent as a separate message. However, different endpoint applications
 can generate multiple events over a short period of time. While the
 applications themselves often limit the number of events that can be
 generated, there is also scope to aggregate events over a short period of
 time to reduce the number of requests while only introducing a small delay
 in the delivery of events to the server. The MCS client shall perform this
 aggregation. See §19.11 and §19.12 for sequence diagrams illustrating this.
 The regulation delay (the time between an application generating an event
 and it being sent by a client if no other events are generated before) and
 the maximum regulation delay (the maximum delay of an event from being
 generated to being sent even if other events are generated) should both be
 configurable (§16). In order to limit the size of the aggregation provided
 to the server, the client shall be configurable with the maximum number of
 events it can provide in a single response.
    """
    def __init__(self, regulationDelay=3,maxRegulationDelay=60,maxEvents=20):
        self.__m_regulationDelay    = regulationDelay
        self.__m_maxRegulationDelay = maxRegulationDelay
        self.__m_maxEvents = maxEvents
        self.__m_countEvents = 0
        self.__m_nextTime = -1
        self.__m_firstEventTime = -1

    def relativeTime(self):
        if self.__m_nextTime < 0:
            return sys.maxint

        if self.__m_countEvents >= self.__m_maxEvents:
            return 0

        return self.__m_nextTime - time.time()

    def eventAdded(self):
        self.__m_countEvents += 1

        if self.__m_firstEventTime < 0:
            ## First event to send
            self.__m_firstEventTime = time.time()
            self.__m_nextTime = time.time() + self.__m_regulationDelay
        else:
            ## Another event in the regulation delay from the first event
            self.__m_nextTime = min(
                time.time() + self.__m_regulationDelay,
                self.__m_firstEventTime + self.__m_maxRegulationDelay
                )

    def eventsSent(self):
        self.__m_countEvents = 0
        self.__m_nextTime = -1
        self.__m_firstEventTime = -1

    def errorSendingEvents(self):
        """
        Error sending the events, so retry in 30 seconds
        """
        self.__m_nextTime = time.time() + 30

    def sendEvents(self):
        """
        Should the pending events be sent
        """
        if self.__m_nextTime < 0:
            return False

        if self.__m_countEvents >= self.__m_maxEvents:
            return True

        return self.__m_nextTime < time.time()
