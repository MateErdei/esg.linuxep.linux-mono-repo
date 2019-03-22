"""
id_manager Module
"""

import time

counter = 0


def id():
    """
    Generate an ID of the for <id>20131004092447000d</id>
    %Y%m%d%H%M%S<4*hexdigit>
    """
    t = time.time()
    global counter
    c = counter
    counter += 1
    return time.strftime("%Y%m%d%H%M%S", time.gmtime(t)) + "%04x" % c
