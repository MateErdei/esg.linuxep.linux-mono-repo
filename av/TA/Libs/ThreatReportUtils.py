import sys
import os

base_mcs_directory = "/opt/sophos-spl/base/mcs/event/"

xml = {"naugthyEicarThreatReport": [
    '''description="Virus/spyware EICAR has been detected in /home/vagrant/this/is/a/directory/for/scanning/naugthy_eicar"''',
    '''type="sophos.mgt.msg.event.threat"''',
    '''<user userId="root"''',
    '''domain="local"/>''',
    '''<threat  type="1"''',
    '''name="EICAR"''',
    '''scanType="203"''',
    '''status="50"''',
    '''id="1"''',
    '''idSource="1">''',
    '''<item file="naugthy_eicar"''',
    '''path="/home/vagrant/this/is/a/directory/for/scanning/"/>''',
    '''<action action="104"/>''']}


def check_threat_event_recieved_by_base(number_of_expected_events, event_type):
    actual_number_of_events = len(next(os.walk(base_mcs_directory))[2])

    if actual_number_of_events is not int(number_of_expected_events):
        print("*WARN*: Number of actual events {} is not equals to the number of expected events  {}".format(actual_number_of_events, number_of_expected_events))
        return 0

    actual_matching_lines = 0
    for filename in os.listdir(base_mcs_directory):
        with open(os.path.join(base_mcs_directory, filename), "r") as file:
            for line in file:
                for xml_part in xml.get(event_type):
                    if xml_part in line:
                        actual_matching_lines += 1

    expected_matching_lines = len(xml.get(event_type))
    if actual_matching_lines is expected_matching_lines:
        return 1
    else:
        print("*WARN*: actual matching lines {} expected matching lines {}".format(actual_matching_lines, expected_matching_lines))
        return 0
