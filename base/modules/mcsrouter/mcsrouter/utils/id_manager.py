"""
id_manager Module
"""

import time

COUNTER = 0


def generate_id():
    """
    Generate an ID of the for <id>20131004092447000d</id>
    %Y%m%d%H%M%S<4*hexdigit>
    """
    #pylint: disable=global-statement
    current_time = time.time()
    global COUNTER
    current_counter = COUNTER
    COUNTER += 1
    return time.strftime("%Y%m%d%H%M%S", time.gmtime(current_time)) + "%04x" % current_counter
