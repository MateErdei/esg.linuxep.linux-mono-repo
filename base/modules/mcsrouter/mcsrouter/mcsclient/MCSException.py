
class MCSException(Exception):
    """
    MCSException
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
