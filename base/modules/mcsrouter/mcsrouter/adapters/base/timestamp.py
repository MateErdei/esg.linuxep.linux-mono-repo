#!/usr/bin/env python3
"""
timestamp Module
"""

import time


def generate_timestamp(when=None):
    """
    generate_timestamp
    :param when:
    :return:
    """
    if when is None:
        when = time.gmtime()
    return time.strftime("%Y%m%d %H%M%S", when)
