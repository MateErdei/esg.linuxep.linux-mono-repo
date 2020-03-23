import sys
import os

try:
    from robot.api import logger

    logger.warning = logger.warn
except ImportError:
    import logging

    logger = logging.getLogger("ThreatReportUtils")

GL_MCS_EVENTS_DIRECTORY = "/opt/sophos-spl/base/mcs/event/"

GL_EXPECTED_CONTENTS = {
    "naugthyEicarThreatReport": [
        '''description="Found 'EICAR' in '/home/vagrant/this/is/a/directory/for/scanning/naugthy_eicar''',
        '''type="sophos.mgt.msg.event.threat"''',
        '''userId="root"''',
        '''domain="local"''',
        '''name="EICAR"''',
        '''scanType="203"''',
        '''status="200"''',
        '''id="T2614dba1b04efcdaf6d31f57e0a6ce2eae55477b7831899263afc4d8ea82eb68"''',
        '''idSource="Tsha256(path,name)"''',
        '''file="naugthy_eicar"''',
        '''path="/home/vagrant/this/is/a/directory/for/scanning/"/>''',
        '''action="101"'''
    ],

    "encoded_eicars": [
        '''WIERDPATH-eicar.com-VIRUS" path="/tmp/encoded_eicars/.\r/''',
        '''LATIN1-ENGLISH-For all good men-VIRUS''',
        '''sh" path="/tmp/encoded_eicars/NEWLINEDIR\n/\n/bin/''',
        '''LATIN1-CHINESE--VIRUS''',
        '''SJIS-KOREAN--VIRUS''',
        '''ES-Español''',
        '''\n''',
        '''UTF-8-FRENCH-à ta santé âge-VIRUS''',
        '''LATIN1-KOREAN--VIRUS''',
        '''SJIS-FRENCH- ta sant ge-VIRUS''',
        '''.‌.''',
        '''bash" path="/tmp/encoded_eicars/..‌/..‌/bin/''',
        '''SJIS-CHINESE-最場腔醴腔-VIRUS (SJIS)''',
        '''PairDoubleQuote-"VIRUS.com"''',
        '''LATIN1-JAPANESE--VIRUS''',
        '''PairSingleQuote-'VIRUS.com''',
        '''SingleDoubleQuote-"-VIRUS.com''',
        '''ASCII-\1\2\3\4\5\6\a\b\t\n\v\f\r !"#$%&'()*+,-.0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_`abcdefghijklmnopqrstuvwxyz{|}~''',
        '''eicar.com''',
        '''RANDOMGARBAGE-?Ñ8[Úm\1\2\a\t2 "3DUfwª»ÌÝîÿ\tá¹©{êùx\2ÿþ6è²ÆÞeM#ö-VIRUS (Latin1)''',
        '''LATIN1-FRENCH-à ta santé âge-VIRUS (Latin1)''',
        '''COM1" ''',
        '''COM2" ''',
        ''' \[]:;|=.*?. " ''',
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
        '''SingleSingleQuote-'-VIRUS.com''',
        '''EUC-JP-FRENCH-à ta santé âge-VIRUS (EUC-JP)''',
        '''SJIS-ENGLISH-For all good men-VIRUS''',
        '''EUC-JP-JAPANESE-ソフォスレイヤーアクセスる-VIRUS (EUC-JP)''',
        '''ES-NFC-Español''',
        '''EUC-JP-CHINESE-涴最場腔醴腔-VIRUS (EUC-JP)''',
        '''eicar.com" path="/tmp/encoded_eicars/.‌/''',
        '''EUC-JP-KOREAN--VIRUS''',
        '''ES-NFD-Español''',
        '''eicar.com" path="/tmp/encoded_eicars/‌./''',
        '''UTF-8-KOREAN-디렉토리입다-VIRUS''',
        '''path="/tmp/encoded_eicars/"''',
        '''UTF-8-CHINESE-涴跺最唗郔場腔醴腔-VIRUS''',
        '''ES-Español''',
        '''0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_`abcdefghijklmnopqrstuvwxyz{|}~" path="/tmp/encoded_eicars/FULL-ASCII-\1\2\3\4\5\6\a\b\t\n\v\f\r !"#$%&'()*+,-./''',
        '''SJIS-JAPANESE-ソフォスレイヤーアクセスる-VIRUS (SJIS)''',
        '''UTF-8-ENGLISH-For all good men-VIRUS''',
        '''UTF-8-JAPANESE-ソフォスレイヤーアクセスる-VIRUS''',
        '''...''',
        '''HTML-<bold>BOLD<font size=+5>THIS IS SOME BIGGER TEXT-VIRUS'''
    ]
}


def check_threat_event_received_by_base(number_of_expected_events, event_type):
    """
    Check if all expected substrings are present in each event in the events directory
    :param number_of_expected_events:
    :param event_type:
    :return: throw Exception on failure. 1 for success
    """
    number_of_expected_events = int(number_of_expected_events)
    events_list = os.listdir(GL_MCS_EVENTS_DIRECTORY)
    actual_number_of_events = len(events_list)

    check_number_of_events_matches(actual_number_of_events, number_of_expected_events)

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
    Check if all expected substrings are present in each event in the events directory
    :param number_of_expected_events:
    :param event_type:
    :return: throw Exception on failure. 1 for success
    """
    number_of_expected_events = int(number_of_expected_events)
    events_list = os.listdir(GL_MCS_EVENTS_DIRECTORY)
    actual_number_of_events = len(events_list)

    check_number_of_events_matches(actual_number_of_events, number_of_expected_events)

    expected_strings = GL_EXPECTED_CONTENTS[event_type]
    # Dictionary mapping each expected string to how many times we've seen it.
    expected_map = {}

    for s in expected_strings:
        expected_map[s] = 0

    for filename in events_list:

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
        logger.error(f"Missing string expecting: {unmatched_strings}")
        logger.error(f"Actual contents: {contents}")
        raise Exception(f"Missing string expecting: {unmatched_strings}")

    return 1


def check_number_of_events_matches(actual_number_of_events, number_of_expected_events):
    if actual_number_of_events != number_of_expected_events:
        raise Exception("Number of actual events {} is not equals to the number of expected events  {}".
                        format(actual_number_of_events, number_of_expected_events))
