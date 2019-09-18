#!/usr/bin/env python3
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
        with codecs.open(path, mode="w", encoding='utf-8') as file_to_write:
            file_to_write.write(data)
    except (OSError, IOError) as exception:
        logging.error("utf8 write failed with message: %s", str(exception))
