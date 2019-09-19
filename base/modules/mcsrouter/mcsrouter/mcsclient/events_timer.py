#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# Copyright 2019 Sophos Plc, Oxford, England.
"""
events_timer Module
"""

import sys
import time


class EventsTimer:
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

    def __init__(
            self,
            regulation_delay=3,
            max_regulation_delay=60,
            max_events=20):
        """
        __init__
        """
        self.__m_regulation_delay = regulation_delay
        self.__m_max_regulation_delay = max_regulation_delay
        self.__m_max_events = max_events
        self.__m_count_events = 0
        self.__m_next_time = -1
        self.__m_first_event_time = -1

    def relative_time(self):
        """
        relative_time
        """
        if self.__m_next_time < 0:
            return sys.maxsize

        if self.__m_count_events >= self.__m_max_events:
            return 0

        return self.__m_next_time - time.time()

    def event_added(self):
        """
        event_added
        """
        self.__m_count_events += 1

        if self.__m_first_event_time < 0:
            # First event to send
            self.__m_first_event_time = time.time()
            self.__m_next_time = time.time() + self.__m_regulation_delay
        else:
            # Another event in the regulation delay from the first event
            self.__m_next_time = min(
                time.time() + self.__m_regulation_delay,
                self.__m_first_event_time + self.__m_max_regulation_delay
            )

    def events_sent(self):
        """
        events_sent
        """
        self.__m_count_events = 0
        self.__m_next_time = -1
        self.__m_first_event_time = -1

    def error_sending_events(self):
        """
        Error sending the events, so retry in 30 seconds
        """
        self.__m_next_time = time.time() + 30

    def send_events(self):
        """
        Should the pending events be sent
        """
        if self.__m_next_time < 0:
            return False

        if self.__m_count_events >= self.__m_max_events:
            return True

        return self.__m_next_time < time.time()
