#!/usr/bin/env python3
# Copyright 2019 Sophos Plc, Oxford, England.

"""
utf8_write Module
"""
import codecs
import logging


def utf8_write(path, data):
    """
    utf8_write
    :param path:
    :param data:
    """
    try:
        if not isinstance(data, str):
            data = str(data, 'utf-8', 'replace')
        # Between Python 3.7 and 3.8, codecs.py changed their buffering default from 1 to -1. Now we explicitly set here.
        with codecs.open(path, mode="w", encoding='utf-8', buffering=-1) as file_to_write:
            file_to_write.write(data)
    except (OSError, IOError) as exception:
        logging.error("utf8 write failed with message: %s", str(exception))
