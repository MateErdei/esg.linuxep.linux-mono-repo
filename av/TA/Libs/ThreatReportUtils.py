#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright (C) 2021 Sophos Plc, Oxford, England.
# All rights reserved.

import os
import xml.dom.minidom

try:
    from robot.api import logger

    logger.warning = logger.warn
except ImportError:
    import logging

    logger = logging.getLogger("ThreatReportUtils")

GL_MCS_EVENTS_DIRECTORY = "/opt/sophos-spl/base/mcs/event/"

GL_EXPECTED_CONTENTS = {
    "naugthyEicarThreatReport": [
        '''description="Found 'EICAR-AV-Test' in '/home/vagrant/this/is/a/directory/for/scanning/naugthy_eicar''',
        '''type="sophos.mgt.msg.event.threat"''',
        '''userId="n/a"''',
        '''domain="local"''',
        '''name="EICAR-AV-Test"''',
        '''scanType="203"''',
        '''status="200"''',
        '''id="Tfe8974b97b4b7a6a33b4c52acb4ffba0c11ebbf208a519245791ad32a96227d8"''',
        '''idSource="Tsha256(path,name)"''',
        '''file="naugthy_eicar"''',
        '''path="/home/vagrant/this/is/a/directory/for/scanning/"/>''',
        '''action="101"'''
    ],

    "naugthyEicarThreatReportAsNobody": [
        '''description="Found 'EICAR-AV-Test' in '/home/vagrant/this/is/a/directory/for/scanning/naugthy_eicar''',
        '''type="sophos.mgt.msg.event.threat"''',
        '''userId="n/a"''',
        '''domain="local"''',
        '''name="EICAR-AV-Test"''',
        '''scanType="203"''',
        '''status="200"''',
        '''id="Tfe8974b97b4b7a6a33b4c52acb4ffba0c11ebbf208a519245791ad32a96227d8"''',
        '''idSource="Tsha256(path,name)"''',
        '''file="naugthy_eicar"''',
        '''path="/home/vagrant/this/is/a/directory/for/scanning/"/>''',
        '''action="101"'''
    ],

    "encoded_eicars": [
        '''" " path="/tmp_test/encoded_eicars/"''',
        r'''WIERDPATH-eicar.com-VIRUS" path="/tmp_test/encoded_eicars/.\r/''',
        '''LATIN1-ENGLISH-For all good men-VIRUS''',
        r'''sh" path="/tmp_test/encoded_eicars/NEWLINEDIR\n/\n/bin/''',
        '''LATIN1-CHINESE--VIRUS''',
        '''SJIS-KOREAN--VIRUS''',
        '''ES-Español''',
        r'''\n''',
        '''UTF-8-FRENCH-à ta santé âge-VIRUS''',
        '''LATIN1-KOREAN--VIRUS''',
        '''SJIS-FRENCH- ta sant ge-VIRUS''',
        '''.‌.''',
        '''bash" path="/tmp_test/encoded_eicars/..‌/..‌/bin/''',
        '''SJIS-CHINESE-最場腔醴腔-VIRUS (Shift-JIS)''',
        '''PairDoubleQuote-&quot;VIRUS.com&quot;''',
        '''LATIN1-JAPANESE--VIRUS''',
        '''PairSingleQuote-&apos;VIRUS.com''',
        '''SingleDoubleQuote-&quot;-VIRUS.com''',
        r'''ASCII-\001\002\003\004\005\006\a\b\t\n\v\f\r\016\017\020\021\022\023\024\025\026\027\030\031\032\033\034\035\036\037 !&quot;#$%&amp;&apos;()*+,-.0123456789:;&lt;=&gt;?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~\177''',
        '''eicar.com''',
        r'''RANDOMGARBAGE-?Ñ8[Úm\025\001\002\a\t2\020\023 &quot;3DUfwª»ÌÝîÿ\tá¹©{êùx\002ÿþ6è\035²ÆÞeM#ö-VIRUS (Latin1)''',
        '''LATIN1-FRENCH-à ta santé âge-VIRUS (Latin1)''',
        '''COM1" ''',
        '''COM2" ''',
        r'''\[]:;|=.*?. "''',
        '''NUL" ''',
        '''LPT2" ''',
        '''LPT1" ''',
        '''AUX" ''',
        '''LPT3" ''',
        '''COM3" ''',
        '''CON" ''',
        '''PRN" ''',
        '''cantendwith." ''',
        '''COM4"''',
        '''EUC-JP-ENGLISH-For all good men-VIRUS''',
        '''SingleSingleQuote-&apos;-VIRUS.com''',
        '''EUC-JP-FRENCH-à ta santé âge-VIRUS (EUC-JP)''',
        '''SJIS-ENGLISH-For all good men-VIRUS''',
        '''EUC-JP-JAPANESE-ソフォスレイヤーアクセスる-VIRUS (EUC-JP)''',
        '''ES-NFC-Español''',
        '''EUC-JP-CHINESE-涴最場腔醴腔-VIRUS (EUC-JP)''',
        '''eicar.com" path="/tmp_test/encoded_eicars/.‌/''',
        '''EUC-JP-KOREAN--VIRUS''',
        '''ES-NFD-Español''',
        '''eicar.com" path="/tmp_test/encoded_eicars/‌./''',
        '''UTF-8-KOREAN-디렉토리입다-VIRUS''',
        '''path="/tmp_test/encoded_eicars/"''',
        '''UTF-8-CHINESE-涴跺最唗郔場腔醴腔-VIRUS''',
        '''ES-Español''',
        r'''0123456789:;&lt;=&gt;?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~\177''',
        '''SJIS-JAPANESE-ソフォスレイヤーアクセスる-VIRUS (Shift-JIS)''',
        '''UTF-8-ENGLISH-For all good men-VIRUS''',
        '''UTF-8-JAPANESE-ソフォスレイヤーアクセスる-VIRUS''',
        '''...''',
        '''HTML-&lt;bold&gt;BOLD&lt;font size=+5&gt;THIS IS SOME BIGGER TEXT-VIRUS'''
    ]
}


def check_number_of_events_matches(number_of_expected_events):
    number_of_expected_events = int(number_of_expected_events)
    events_list = os.listdir(GL_MCS_EVENTS_DIRECTORY)
    actual_number_of_events = len(events_list)

    if actual_number_of_events != number_of_expected_events:
        raise Exception("Number of actual events {} is not equals to the number of expected events  {}".
                        format(actual_number_of_events, number_of_expected_events))

    return actual_number_of_events, events_list, number_of_expected_events


def check_threat_event_received_by_base(number_of_expected_events, event_type):
    """
    Check if all expected substrings are present in each event in the events directory
    :param number_of_expected_events:
    :param event_type:
    :return: throw Exception on failure. 1 for success
    """
    actual_number_of_events, events_list, number_of_expected_events = check_number_of_events_matches(
        number_of_expected_events)

    expected_strings = GL_EXPECTED_CONTENTS[event_type]
    # Dictionary mapping each expected string to how many times we've seen it.
    expected_map = {}

    for filename in events_list:
        for s in expected_strings:
            expected_map[s] = 0

        with open(os.path.join(GL_MCS_EVENTS_DIRECTORY, filename), "r") as file:
            contents = file.read()
            for line in contents.splitlines():
                for xml_part in expected_strings:
                    if xml_part in line:
                        expected_map[xml_part] += 1

        # Work out which strings haven't been matched already
        unmatched_strings = []
        for s, count in expected_map.items():
            if count == 0:
                unmatched_strings.append(s)

        if len(unmatched_strings) > 0:
            logger.error(f"Missing string from message {filename}, expecting: {unmatched_strings}")
            logger.error(f"Actual contents: {contents}")
            raise Exception(f"Missing string from message {filename}, expecting: {unmatched_strings}")

    return 1


def check_multiple_different_threat_events(number_of_expected_events, event_type):
    """
    Check if all expected substrings are present in at least one of the events in the events directory
    :param number_of_expected_events:
    :param event_type:
    :return: throw Exception on failure. 1 for success
    """
    actual_number_of_events, events_list, number_of_expected_events = check_number_of_events_matches(
        number_of_expected_events)

    expected_strings = GL_EXPECTED_CONTENTS[event_type]
    # Dictionary mapping each expected string to how many times we've seen it.
    expected_map = {}

    for s in expected_strings:
        expected_map[s] = 0

    no_matches = []

    for filename in events_list:
        with open(os.path.join(GL_MCS_EVENTS_DIRECTORY, filename), "r") as file:
            contents = file.read()

        found_match = False
        for line in contents.splitlines():
            for xml_part in expected_strings:
                if xml_part in line:
                    expected_map[xml_part] += 1
                    found_match = True
        if not found_match:
            no_matches.append(filename)

    # Work out which strings haven't been matched already
    unmatched_strings = []
    for s, count in expected_map.items():
        if count == 0:
            unmatched_strings.append(s)

    if len(unmatched_strings) > 0:
        for unmatched in unmatched_strings:
            logger.error(f"Failed to find: {unmatched}")

        if len(no_matches) == 0:
            logger.error("All %d files matched something however" % len(events_list))

        for filename in (no_matches or events_list):
            with open(os.path.join(GL_MCS_EVENTS_DIRECTORY, filename), "r") as file:
                contents = file.read()
            logger.info(f"{filename}: {contents}")

        raise Exception(f"Missing string expecting: {unmatched_strings}")

    return 1


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

        if o < 0x20:
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
    return ret


def get_filepath_notification_event(p):
    dom = xml.dom.minidom.parse(p)
    try:

        items = dom.getElementsByTagName("item")
        assert len(items) == 1
        item = items[0]
        return item.getAttribute("path") + item.getAttribute("file")
    finally:
        dom.unlink()

def get_eicars_from_notifications_events():
    events_list = os.listdir(GL_MCS_EVENTS_DIRECTORY)
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

    for e, v in eicars_on_disk_remaining.items():
        if v == 1:
            logger.error("%s not found in events" % safe_to_unicode(e))

    for e, v in eicars_reported_remaining.items():
        if v == 1:
            logger.error("%s not found in on disk" % safe_to_unicode(e))


