#!/bin/env python3
# -*- coding: utf-8 -*-
# Copyright (C) 2021 Sophos Ltd
# All rights reserved.
from __future__ import absolute_import, print_function, division, unicode_literals

import inspect
import os
import pytest
import signal
import subprocess
import time

import logging
logger = logging.getLogger("test_threat_detector")
logger.setLevel(logging.DEBUG)

