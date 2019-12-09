#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright (C) 2019 Sophos Plc, Oxford, England.
# All rights reserved.

import os
import sys


SUPPORTFILEPATH = os.path.join(os.path.dirname(__file__), "..", "SupportFiles")

def addPathToSysPath(p):
    p = os.path.normpath(p)
    if p not in sys.path:
        sys.path.append(p)

