#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright (C) 2020 Sophos Plc, Oxford, England.
# All rights reserved.

from __future__ import print_function

import datetime
import sys

def internal_get_next_scan_time(now):
    if now.minute < 28:
        nextTime = now.replace(minute=30, second=0, microsecond=0)
    elif now.minute < 58:
        nextTime = now.replace(minute=0, second=0, microsecond=0)
        nextTime += datetime.timedelta(hours=1)
    else:
        nextTime = now.replace(minute=30, second=0, microsecond=0)
        nextTime += datetime.timedelta(hours=1)

    assert((nextTime-now) <= datetime.timedelta(minutes=32))

    return nextTime

def get_next_scan_time():
    now = datetime.datetime.now()
    return internal_get_next_scan_time(now)


def main():
    now = datetime.datetime.now()
    now = now.replace(minute=5, second=0, microsecond=0)
    nextTime = internal_get_next_scan_time(now)
    print(str(now), "->", str(nextTime))

    now = now.replace(minute=28, second=0, microsecond=0)
    nextTime = internal_get_next_scan_time(now)
    print(str(now), "->", str(nextTime))

    now = now.replace(minute=45, second=0, microsecond=0)
    nextTime = internal_get_next_scan_time(now)
    print(str(now), "->", str(nextTime))

    now = now.replace(minute=58, second=0, microsecond=0)
    nextTime = internal_get_next_scan_time(now)
    print(str(now), "->", str(nextTime))

    return 0


if __name__ == "__main__":
    sys.exit(main())



