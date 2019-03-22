"""
timestamp Module
"""

import time


def timestamp(t=None):
    """
    timestamp
    """
    if t is None:
        t = time.time()
    nanoseconds = int((t % 1) * 1000000)
    return time.strftime(
        "%Y-%m-%dT%H:%M:%S.",
        time.gmtime(t)) + str(nanoseconds) + "Z"
