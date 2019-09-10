#!/usr/bin/env python3
"""
Method to convert from bytes to utf8 string
"""

def to_utf8(bytes_entry):
    print(type(bytes_entry))
    assert(isinstance(bytes_entry, bytes))
    return bytes_entry.decode('utf-8', 'replace')