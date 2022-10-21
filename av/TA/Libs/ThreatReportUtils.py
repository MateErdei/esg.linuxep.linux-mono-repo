#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright (C) 2021-2022 Sophos Plc, Oxford, England.
# All rights reserved.

import os
import time
import xml.dom.minidom

try:
    from robot.api import logger

    logger.warning = logger.warn
except ImportError:
    import logging

    logger = logging.getLogger("ThreatReportUtils")

from robot.libraries.BuiltIn import BuiltIn

GL_MCS_EVENTS_DIRECTORY = "/opt/sophos-spl/base/mcs/event/"


def convert_to_unicode(p):
    assert isinstance(p, bytes)
    try:
        return p.decode("UTF-8")
    except UnicodeDecodeError:
        pass
    except AttributeError:
        logger.error("Expecting p (%s) to be bytes not unicode" % repr(p))
        return p

    for encoding in ("EUC-JP", "Shift-JIS", "Latin1"):
        try:
            return p.decode(encoding) + " (" + encoding + ")"
        except UnicodeDecodeError:
            pass

    return p

def convert_to_escaped_unicode(path):
    r"""

    * 0-6      \00o            NULL, start of heading, start of text, end of text, end of transmission, enquery, acknowledge
    * 7        \a              bell
    * 8        \b              backspace
    * 9        \t              tab
    * 10       \n              new line, line feed
    * 11       \v              vertical tab
    * 12       \f              form feed, new page
    * 13       \r              carriage return
    * 14                       shift out
    * 15                       shift in
    * 16-31   \0oo             data link escape, decide control 1-4, negative ack, synch idle, end of trans. block, cancel, end of medium
    *                          substitute, escape, file separator, group separator, record separator, unit seperator
    * 92(\)   \\               Back-slash
    * 127     \177
    *
    """
    path = convert_to_unicode(path)
    assert isinstance(path, str)
    res = []
    for c in path:
        try:
            o = ord(c)
        except TypeError as ex:
            logger.error("TypeError %s in %s" % (str(ex), repr(path)))
            raise

        if o < 0x20 or o == 127:
            c = {
                7 : r'\a',
                8 : r'\b',
                9 : r'\t',
                10: r'\n',
                11: r'\v',
                12: r'\f',
                13: r'\r',
            }.get(o, u"\\{:0>3o}".format(o))
        elif o == 92:
            c = u"\\\\"
        res.append(c)
    return u"".join(res)


def find_eicars(eicar_directory):
    if isinstance(eicar_directory, str):
        eicar_directory = eicar_directory.encode("UTF-8")
    ret = []
    for base, _, files in os.walk(eicar_directory):
        ret += [ convert_to_escaped_unicode(os.path.join(base, f)) for f in files ]

    if len(ret) == 0:
        raise AssertionError("Failed to find any eicars in %s" % eicar_directory)

    return ret


def get_filepath_notification_event(p):
    dom = xml.dom.minidom.parse(p)
    try:
        with open(p) as f:
            logger.info(f.read())

        items = dom.getElementsByTagName("path")
        logger.info(f"{len(items)}")
        assert len(items) == 1
        item = items[0]
        return ''.join(node.nodeValue for node in item.childNodes)
    finally:
        dom.unlink()

def get_eicars_from_notifications_events():
    events_list = os.listdir(GL_MCS_EVENTS_DIRECTORY)
    events_list = list(filter(lambda event: event.startswith('CORE'), events_list))
    ret = []
    for e in events_list:
        ret.append(get_filepath_notification_event(os.path.join(GL_MCS_EVENTS_DIRECTORY, e)))
    return ret

def safe_to_unicode(s):
    if isinstance(s, bytes):
        return s.decode("UTF-8", errors='backslashreplace')
    return s

def check_all_eicars_are_found(eicar_directory):
    eicars_on_disk = find_eicars(eicar_directory)
    eicars_reported = get_eicars_from_notifications_events()

    eicars_on_disk_remaining = {}
    for e in eicars_on_disk:
        eicars_on_disk_remaining[e] = 1

    eicars_reported_remaining = {}
    for e in eicars_reported:
        eicars_reported_remaining[e] = 1
        if e in eicars_on_disk_remaining:
            eicars_on_disk_remaining[e] = 0

    for e in eicars_on_disk:
        if e in eicars_reported_remaining:
            eicars_reported_remaining[e] = 0

    errors_found = False
    for e, v in eicars_on_disk_remaining.items():
        if v == 1:
            logger.error("%s not found in events" % safe_to_unicode(e))
            errors_found = True

    for e, v in eicars_reported_remaining.items():
        if v == 1:
            logger.error("%s not found in on disk" % safe_to_unicode(e))
            errors_found = True

    if errors_found:
        raise AssertionError("Eicars reported in events don't match eicars found on disk in %s" % eicar_directory)

def _list_eicars_not_in_av_log(expected_raw_xml_strings):
    builtin = BuiltIn()
    marked_log = builtin.run_keyword("get_marked_av_log")

    missing = []

    for e in expected_raw_xml_strings:
        if e not in marked_log:
            missing.append(e)

    return missing

def _xml_escape(s):
    for src, rep in [
        ('&', "&amp;"),
        ('"', "&quot;"),
        ("'", "&apos;"),
        ("<", "&lt;"),
        (">", "&gt;"),
            ]:
        s = s.replace(src, rep)
    return s

def _expected_raw_xml_string(eicars_on_disk):
    ret = []
    for e in eicars_on_disk:
        # expected String: <path>/tmp_test/encoded_eicars/SingleDoubleQuote-&quot;-VIRUS.com</path>
        ret.append(f'<path>{_xml_escape(e)}</path>')
    return ret


def wait_for_all_eicars_are_reported_in_av_log(eicar_directory, limit=15):
    start = time.time()
    eicars_on_disk = find_eicars(eicar_directory)
    expected_strings = _expected_raw_xml_string(eicars_on_disk)
    while time.time() < start + limit:
        missing = _list_eicars_not_in_av_log(expected_strings)
        if len(missing) == 0:
            # all good
            return True
        time.sleep(1)
    raise AssertionError("Failed to find all eicars within limit: %s" % missing)
