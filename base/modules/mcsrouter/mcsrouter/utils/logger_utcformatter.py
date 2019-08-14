#!/usr/bin/env python
"""
UTCFormatter class so all logs can show the same UTC time.
"""

from logging import Formatter
import time


class UTCFormatter(Formatter):
    converter = time.gmtime
