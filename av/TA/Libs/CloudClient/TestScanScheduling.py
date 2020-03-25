#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright (C) 2020 Sophos Plc, Oxford, England.
# All rights reserved.

import datetime
import time

import NextScanTime

while True:
    scan_time = NextScanTime.get_next_scan_time()
    print(datetime.datetime.now().isoformat(), "=>", scan_time.isoformat())
    time.sleep(119)
