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
        '''type="1"''',
        '''name="EICAR"''',
        '''scanType="203"''',
        '''status="200"''',
        '''id="1"''',
        '''idSource="1"''',
        '''file="naugthy_eicar"''',
        '''path="/home/vagrant/this/is/a/directory/for/scanning/"/>''',
        '''action="101"'''
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
    expected_map = {s: 0 for s in expected_strings}

    for filename in events_list:
        with open(os.path.join(GL_MCS_EVENTS_DIRECTORY, filename), "r") as file:
            contents = file.read()
            for line in contents.splitlines():
                for xml_part in expected_strings:
                    if xml_part in line:
                        expected_map[xml_part] += 1

        unmatched_strings = [ s for s in expected_map.keys() if expected_map[s] == 0 ]
        expected_matching_lines = len(expected_strings)
        if len(unmatched_strings) > 0:
            logger.error(f"Missing string from message {filename}, expecting: {unmatched_strings}")
            logger.error(f"Actual contents: {contents}")
            raise Exception(f"Missing string from message {filename}, expecting: {unmatched_strings}")

    return 1
