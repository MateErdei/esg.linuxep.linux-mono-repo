#!/usr/bin/env python3
"""
atomic_write Module
"""

import logging
import os

from .utf8_write import utf8_write


def atomic_write(path, tmp_path, data):
    """
    atomic_write
    :param path:
    :param tmp_path:
    :param data:
    """
    try:
        utf8_write(tmp_path, data)
        os.rename(tmp_path, path)
    except (OSError, IOError) as exception:
        logging.error("Atomic write failed with message: %s", str(exception))
