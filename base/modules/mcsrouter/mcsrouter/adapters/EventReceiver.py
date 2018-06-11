#!/usr/bin/env python

from __future__ import print_function,division,unicode_literals

import os
import re

import logging
logger = logging.getLogger(__name__)


class EventReceiver(object):
    def __init__(self, installdir):
        if installdir is None:
            installdir = os.path.abspath("..")
        self.__m_installdir = installdir

    def receive(self):
        """
        Async receive call
        """

        eventdir = os.path.join(self.__m_installdir, "event")
        for eventfile in os.listdir(eventdir):
            mo = re.match(r"([A-Z]*)_event-(.*).xml", eventfile)
            filepath = os.path.join(eventdir, eventfile)
            if mo:
                appid = mo.group(1)
                time = os.path.getmtime(filepath)
                body = open(filepath).read()
                yield (appid, time, body)
            else:
                logger.warning("Malformed event file: %s", eventfile)

            os.remove(filepath)

