# Copyright 2019 Sophos Plc, Oxford, England.
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

class MCSCACertificateException(ValueError):
    """
    MCSCACertificateException : CA certificate file exception
    """
