#!/usr/bin/env python

import time


def generate_timestamp(when=None):
    if when is None:
        when = time.gmtime()
    return time.strftime("%Y%m%d %H%M%S", when)
