#!/usr/bin/env python

import time

def generateTimestamp(when=None):
    if when is None:
        when = time.gmtime()
    return time.strftime("%Y%m%d %H%M%S", when)
