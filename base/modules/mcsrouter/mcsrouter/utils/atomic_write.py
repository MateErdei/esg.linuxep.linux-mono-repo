#!/usr/bin/env python
"""
atomic_write Module
"""

import os
import logging
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
