#!/usr/bin/env python

from logging import Formatter
import time


class UTCFormatter(Formatter):
    """
    UTCFormatter class so all logs can show the same UTC time.
    """
    converter = time.gmtime
