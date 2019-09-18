#!/usr/bin/env python3

"""
    UTCFormatter module
"""

import time
from logging import Formatter


class UTCFormatter(Formatter):
    """
    UTCFormatter class so all logs can show the same UTC time.
    """
    converter = time.gmtime
