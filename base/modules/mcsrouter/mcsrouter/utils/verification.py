#!/usr/bin/env python3
# Copyright 2024 Sophos Limited. All rights reserved.

class MCSUrlVerificationException(Exception):
    """
    MCSUrlVerificationException
    """
    pass


def check_url(url):
    strippedurl = url
    if url.startswith("https://"):
        strippedurl = url[8:]
    elif url.startswith("http://"):
        strippedurl = url[7:]
    if strippedurl.count("//") > 0:
        raise MCSUrlVerificationException(f"{url} contains a double slash")
    hostname = strippedurl.split("/")[0]
    if hostname.count(":") > 1:
        raise MCSUrlVerificationException(f"{url} contains extra :")
    elif hostname.count(":") == 1:
        parts = hostname.split(":")
        if len(parts) != 2:
            raise MCSUrlVerificationException(f"url {url} is malformed")
        port = parts[1]
        if not port.isnumeric():
            raise MCSUrlVerificationException(f"port in {url} not a number")
        host = parts[0]
    else:
        host = hostname

    if len(host) < 1:
        raise MCSUrlVerificationException(f"hostname in url {url} is empty")
    for char in host:
        if not is_valid_char(char):
            raise MCSUrlVerificationException(f"{char} is not a valid character for a hostname in {url}")


def is_valid_char(char):
    if char.isalnum():
        return True
    elif char in ['_', '.', '-']:
        return True

    return False
