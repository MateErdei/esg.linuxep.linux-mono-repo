# Copyright 2019 Sophos Plc, Oxford, England.
"""
Config exception module
"""

class ConfigException(Exception):
    """
    ConfigException class
    To be used when a configuration is broken and the application can not run without that configuration.
    """
    pass


def composeMessage( where, reason):
    return "Failed to configure {}. Reason: {}".format(where, reason)