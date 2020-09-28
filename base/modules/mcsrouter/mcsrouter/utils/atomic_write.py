#!/usr/bin/env python3
# Copyright 2019 Sophos Plc, Oxford, England.

"""
atomic_write Module
"""

import logging
import os

from .utf8_write import utf8_write


def atomic_write(path, tmp_path, mode, data):
    """
    atomic_write
    :param path:
    :param tmp_path:
    :param mode:
    :param data:
    """
    try:
        utf8_write(tmp_path, data)
        os.chmod(tmp_path, mode)
        os.rename(tmp_path, path)
    except (OSError, IOError) as exception:
        logging.error("Atomic write failed with message: %s", str(exception))
