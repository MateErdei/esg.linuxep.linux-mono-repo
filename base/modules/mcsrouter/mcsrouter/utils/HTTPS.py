import httplib
import re
import socket
import sys
import ssl
import urllib2

import SophosHTTPS

class InvalidCertificateException(SophosHTTPS.InvalidCertificateException):
    pass

class CertValidatingHTTPSConnection(SophosHTTPS.CertValidatingHTTPSConnection):
    pass

