#!/usr/bin/env python3
"""
Method to convert from bytes to utf8 string
"""

def to_utf8(bytes_entry):
    if not isinstance(bytes_entry, bytes):
        raise TypeError( "Expected bytes but received: {}".format(type(bytes_entry)))
    return bytes_entry.decode('utf-8', 'replace')