#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright (C) 2020 Sophos Plc, Oxford, England.
# All rights reserved.

import datetime

def get_next_scan_time():
    now = datetime.datetime.now()
    if now.minute < 30:
        now = now.replace(minute=30, second=0, microsecond=0)
    else:
        now = now.replace(minute=0, second=0, microsecond=0)
        now += datetime.timedelta(hours=1)
    return now
