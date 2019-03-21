
class MCSException(Exception):
    pass


class MCSNetworkException(MCSException):
    pass


class MCSConnectionFailedException(MCSNetworkException):
    pass


class MCSPolicyInvalidException(MCSException):
    pass
