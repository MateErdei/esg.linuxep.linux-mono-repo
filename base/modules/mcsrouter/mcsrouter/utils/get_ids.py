#!/usr/bin/env python
"""
get_ids Module
"""

def get_uid(uid_string):
    """
    get_uid
    """
    # Try as a integer
    try:
        return int(uid_string, 10)  # id specified in decimal
    except ValueError:
        pass

    # Try in the passwd database
    import pwd
    try:
        password_struct = pwd.getpwnam(uid_string)
        return password_struct.pw_uid
    except KeyError:
        # The text doesn't exist as a user
        pass
    return 0


def get_gid(gid_string):
    """
    get_gid
    """
    # Try as a integer
    try:
        return int(gid_string, 10)  # id specified in decimal
    except ValueError:
        pass

    # Try in the passwd database
    import grp
    try:
        group_struct = grp.getgrnam(gid_string)
        return group_struct.gr_gid
    except KeyError:
        # The text doesn't exist as a group
        pass
    return 0