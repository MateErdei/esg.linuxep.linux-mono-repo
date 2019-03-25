"""
timestamp Module
"""

import time


def timestamp(time_argument=None):
    """
    timestamp
    """
    if time_argument is None:
        time_argument = time.time()
    nanoseconds = int((time_argument % 1) * 1000000)
    return time.strftime(
        "%Y-%m-%dT%H:%M:%S.",
        time.gmtime(time_argument)) + str(nanoseconds) + "Z"
