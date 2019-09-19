# Copyright 2019 Sophos Plc, Oxford, England.
"""
https Module
"""

# pylint: disable=too-many-ancestors
# pylint: disable=import-error
# pylint: disable=too-few-public-methods

import sophos_https


class InvalidCertificateException(sophos_https.InvalidCertificateException):
    """
    InvalidCertificateException
    """
    pass


class CertValidatingHTTPSConnection(sophos_https.CertValidatingHTTPSConnection):
    """
    CertValidatingHTTPSConnection
    """
    pass
