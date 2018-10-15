#!/usr/bin/env python

from __future__ import print_function,division,unicode_literals

import os
import re

import logging
import mcsrouter.utils.PathManager as PathManager
import mcsrouter.utils.XmlHelper as XmlHelper

logger = logging.getLogger(__name__)


class EventReceiver(object):
    def __init__(self, installdir):
        if installdir is not None:
            PathManager.INST = installdir

    def receive(self):
        """
        Async receive call
        """

        eventdir = os.path.join(PathManager.eventDir())
        for eventfile in os.listdir(eventdir):
            mo = re.match(r"([A-Z]*)_event-(.*).xml", eventfile)
            filepath = os.path.join(eventdir, eventfile)
            if mo:
                appid = mo.group(1)
                time = os.path.getmtime(filepath)
                body = XmlHelper.getXMLfileContentWithEscapedNonAsciiCode(filepath)
                yield (appid, time, body )
            else:
                logger.warning("Malformed event file: %s", eventfile)

            os.remove(filepath)

