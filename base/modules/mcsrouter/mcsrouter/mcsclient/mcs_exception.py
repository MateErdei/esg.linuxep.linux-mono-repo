"""
mcs_exception Module
"""


class MCSException(Exception):
    """
    MCSException class
    """
    pass


class MCSNetworkException(MCSException):
    """
    MCSNetworkException
    """
    pass


class MCSConnectionFailedException(MCSNetworkException):
    """
    MCSConnectionFailedException
    """
    pass


class MCSPolicyInvalidException(MCSException):
    """
    MCSPolicyInvalidException
    """
    pass
