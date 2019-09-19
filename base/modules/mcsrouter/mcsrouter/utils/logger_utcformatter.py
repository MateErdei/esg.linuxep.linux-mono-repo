#!/usr/bin/env python3
# Copyright 2019 Sophos Plc, Oxford, England.


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
