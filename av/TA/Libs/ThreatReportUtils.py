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

    #TODO: uncomment when LINUXDAR-1721 fixed and also update list_of_expected_encoded_eicars file
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
        '''SJIS-CHINESE-最場腔醴腔-VIRUS''',
        '''PairDoubleQuote-&quot;VIRUS.com&quot;''',
        '''LATIN1-JAPANESE--VIRUS''',
        '''PairSingleQuote-&apos;VIRUS.com''',
        '''SingleDoubleQuote-&quot;-VIRUS.com''',
        # r'''ASCII-\1\2\3\4\5\6\a\b\t\n\v\f\r\016\017\020\021\022\023\024\025\026\027\030\031\032\033\034\035\036\037 !&quot;#$%&amp;&apos;()*+,-.0123456789:;&lt;=&gt;?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~\177''',
        '''eicar.com''',
        # r'''RANDOMGARBAGE-?Ñ8[Úm\025\1\2\a\t2\020\023 &quot;3DUfwª»ÌÝîÿ\tá¹©{êùx\2ÿþ6è\035²ÆÞeM#ö-VIRUS (Latin1)''',
        '''LATIN1-FRENCH-à ta santé âge-VIRUS''',
        '''COM1" ''',
        '''COM2" ''',
        '''\[]:;|=.*?. "''',
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
        '''EUC-JP-FRENCH-à ta santé âge-VIRUS''',
        '''SJIS-ENGLISH-For all good men-VIRUS''',
        '''EUC-JP-JAPANESE-ソフォスレイヤーアクセスる-VIRUS''',
        '''ES-NFC-Español''',
        '''EUC-JP-CHINESE-涴最場腔醴腔-VIRUS''',
        '''eicar.com" path="/tmp_test/encoded_eicars/.‌/''',
        '''EUC-JP-KOREAN--VIRUS''',
        '''ES-NFD-Español''',
        '''eicar.com" path="/tmp_test/encoded_eicars/‌./''',
        '''UTF-8-KOREAN-디렉토리입다-VIRUS''',
        '''path="/tmp_test/encoded_eicars/"''',
        '''UTF-8-CHINESE-涴跺最唗郔場腔醴腔-VIRUS''',
        '''ES-Español''',
        # r'''0123456789:;&lt;=&gt;?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~\177" path="/tmp_test/encoded_eicars/FULL-ASCII-\1\2\3\4\5\6\a\b\t\n\v\f\r\016\017\020\021\022\023\024\025\026\027\030\031\032\033\034\035\036\037 !&quot;#$%&amp;&apos;()*+,-./''',
        '''SJIS-JAPANESE-ソフォスレイヤーアクセスる-VIRUS''',
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

    for filename in events_list:
        with open(os.path.join(GL_MCS_EVENTS_DIRECTORY, filename), "r") as file:
            contents = file.read()
            for line in contents.splitlines():
                logger.info(">>> line: ".format(line))
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
        raise Exception(f"Missing string expecting: {unmatched_strings}")

    return 1
