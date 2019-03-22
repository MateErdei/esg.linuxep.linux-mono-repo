#!/usr/bin/env python
"""
atomic_write Module
"""

import os
import logging
import codecs


def atomic_write(path, tmp_path, data):
    """
    atomic_write
    :param path:
    :param tmp_path:
    :param data:
    """
    try:
        if not isinstance(data, unicode):
            data = unicode(data, 'utf-8', 'replace')
        with codecs.open(tmp_path, mode="w", encoding='utf-8') as file:
            file.write(data)
        os.rename(tmp_path, path)
    except (OSError, IOError) as exception:
        logging.error("Atomic write failed with message: {}".format(exception))
