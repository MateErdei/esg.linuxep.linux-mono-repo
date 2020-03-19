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
        '''WIERDPATH-eicar.com-VIRUS" path="/tmp/JATEST/.\r/''',
        '''LATIN1-ENGLISH-For all good men-VIRUS" path="/tmp/JATEST/''',
        '''sh" path="/tmp/JATEST/NEWLINEDIR\n/\n/bin/''',
        '''LATIN1-CHINESE--VIRUS" path="/tmp/JATEST/''',
        '''eicar‮  ,txt.com" path="/tmp/JATEST/''',
        '''SJIS-KOREAN--VIRUS" path="/tmp/JATEST/''',
        '''ES-Español" path="/tmp/JATEST/''',
        ''''\n" path="/tmp/JATEST/''',
        '''UTF-8-FRENCH-à ta santé âge-VIRUS" path="/tmp/JATEST/''',
        '''LATIN1-KOREAN--VIRUS" path="/tmp/JATEST/''',
        '''SJIS-FRENCH- ta sant ge-VIRUS" path="/tmp/JATEST/''',
        ''''.‌." path="/tmp/JATEST/''',
        '''bash" path="/tmp/JATEST/..‌/..‌/bin/''',
        '''PairDoubleQuote-"VIRUS.com"" path="/tmp/JATEST/''',
        '''SingleDoubleQuote-"-VIRUS.com" path="/tmp/JATEST/''',
        '''LATIN1-JAPANESE--VIRUS" path="/tmp/JATEST/''',
        '''PairSingleQuote-'VIRUS.com'" path="/tmp/JATEST/''',
        '''ASCII-\1\2\3\4\5\6\a\b\t\n\v\f\r !"#$%&'()*+,-.0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_`abcdefghijklmnopqrstuvwxyz{|}~" path="/tmp/JATEST/''',
        '''eicar.com" path="/tmp/JATEST/''',
        '''RANDOMGARBAGE-?Ñ8[Úm\1\2\a\t2 "3DUfwª»ÌÝîÿ\tá¹©{êùx\2ÿþ6è²ÆÞeM#ö-VIRUS (Latin1)" path="/tmp/JATEST/''',
        ''''" " path="/tmp/JATEST/''',
        '''LATIN1-FRENCH-à ta santé âge-VIRUS (Latin1)" path="/tmp/JATEST/''',
        '''SJIS-CHINESE-最場腔醴腔-VIRUS (SJIS)" path="/tmp/JATEST/''',
        '''COM1" path="/tmp/JATEST/Windows/''',
        '''COM2" path="/tmp/JATEST/Windows/''',
        ''' \[]:;|=.*?. " path="/tmp/JATEST/Windows/''',
        '''NUL" path="/tmp/JATEST/Windows/''',
        '''LPT1" path="/tmp/JATEST/Windows/''',
        '''LPT2" path="/tmp/JATEST/Windows/''',
        '''AUX" path="/tmp/JATEST/Windows/''',
        '''LPT3" path="/tmp/JATEST/Windows/''',
        '''COM3" path="/tmp/JATEST/Windows/''',
        '''CON" path="/tmp/JATEST/Windows/''',
        '''PRN" path="/tmp/JATEST/Windows/''',
        '''cantendwith." path="/tmp/JATEST/Windows/''',
        '''COM4" path="/tmp/JATEST/Windows/''',
        '''EUC-JP-ENGLISH-For all good men-VIRUS" path="/tmp/JATEST/''',
        '''SingleSingleQuote-'-VIRUS.com" path="/tmp/JATEST/''',
        '''SJIS-ENGLISH-For all good men-VIRUS" path="/tmp/JATEST/''',
        '''EUC-JP-JAPANESE-ソフォスレイヤーアクセスる-VIRUS (EUC-JP)" path="/tmp/JATEST/''',
        '''EUC-JP-FRENCH-à ta santé âge-VIRUS (EUC-JP)" path="/tmp/JATEST/''',
        '''EUC-JP-CHINESE-涴最場腔醴腔-VIRUS (EUC-JP)" path="/tmp/JATEST/''',
        '''ES-NFC-Español" path="/tmp/JATEST/''',
        '''eicar.com" path="/tmp/JATEST/.‌/''',
        '''EUC-JP-KOREAN--VIRUS" path="/tmp/JATEST/''',
        '''ES-NFD-Español" path="/tmp/JATEST/''',
        '''eicar.com" path="/tmp/JATEST/‌./''',
        '''UTF-8-KOREAN-디렉토리입다-VIRUS" path="/tmp/JATEST/''',
        ''' " path="/tmp/JATEST/''',
        '''UTF-8-CHINESE-涴跺最唗郔場腔醴腔-VIRUS" path="/tmp/JATEST/''',
        '''ES-Español" path="/tmp/JATEST/''',
        '''0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_`abcdefghijklmnopqrstuvwxyz{|}~" path="/tmp/JATEST/FULL-ASCII-\1\2\3\4\5\6\a\b\t\n\v\f\r !"#$%&'()*+,-./''',
        '''UTF-8-ENGLISH-For all good men-VIRUS" path="/tmp/JATEST/''',
        '''UTF-8-JAPANESE-ソフォスレイヤーアクセスる-VIRUS" path="/tmp/JATEST/''',
        '''SJIS-JAPANESE-ソフォスレイヤーアクセスる-VIRUS (SJIS)" path="/tmp/JATEST/''',
        ''''..." path="/tmp/JATEST/''',
        '''HTML-<bold>BOLD<font size=+5>THIS IS SOME BIGGER TEXT-VIRUS" path="/tmp/JATEST/'''
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

    if actual_number_of_events != number_of_expected_events:
        raise Exception("Number of actual events {} is not equals to the number of expected events  {}".
                        format(actual_number_of_events, number_of_expected_events))

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
