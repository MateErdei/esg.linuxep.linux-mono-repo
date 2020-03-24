#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright (C) 2020 Sophos Plc, Oxford, England.
# All rights reserved.

import time

import NextScanTime

while True:
    scan_time = NextScanTime.get_next_scan_time()
    print(scan_time.isoformat())
    time.sleep(119)
