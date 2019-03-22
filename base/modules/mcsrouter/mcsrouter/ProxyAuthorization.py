#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright (C) 2017 Sophos Plc, Oxford, England.
# All rights reserved.
"""
ProxyAuthorization Module
"""

from __future__ import absolute_import, print_function, division, unicode_literals

import logging
import urllib2

parse_http_list = urllib2.parse_http_list
parse_keqv_list = urllib2.parse_keqv_list
LOGGER = logging.getLogger(__name__)


class SophosProxyDigestAuthHandler(urllib2.AbstractDigestAuthHandler):
    """
    SophosProxyDigestAuthHandler
    """

    def get_authorization(self, remote_host, chal, remote_port=443):
        """
        get_authorization
        """
        try:
            realm = chal['realm']
            nonce = chal['nonce'].encode("UTF-8")
            qop = chal.get('qop')
            algorithm = chal.get('algorithm', 'MD5')
            # mod_digest doesn't send an opaque, even though it isn't
            # supposed to be optional
            opaque = chal.get('opaque', None)
        except KeyError:
            return None

        H, KD = self.get_algorithm_impls(algorithm)
        if H is None:
            return None

        user, password = self.passwd.find_user_password()
        if user is None:
            return None

        A1 = b"%s:%s:%s" % (user, realm, password)
        uri = b"%s:%d" % (
            remote_host,
            remote_port)
        A2 = b"CONNECT:%s" % uri

        if qop == 'auth':
            if nonce == self.last_nonce:
                self.nonce_count += 1
            else:
                self.nonce_count = 1
                self.last_nonce = nonce

            nc_value = b'%08x' % self.nonce_count
            cnonce = self.get_cnonce(nonce)
            nonce_bit = b"%s:%s:%s:%s:%s" % (
                nonce, nc_value, cnonce, qop, H(A2))
            respdig = KD(H(A1), nonce_bit)
        elif qop is None:
            respdig = KD(H(A1), b"%s:%s" % (nonce, H(A2)))
        else:
            # XXX handle auth-int.
            raise urllib2.URLError("qop '%s' is not supported." % qop)

        # XXX should the partial digests be encoded too?

        base = 'username="%s", realm="%s", nonce="%s", uri="%s", ' \
               'response="%s"' % (user, realm, nonce, uri, respdig)
        if opaque:
            base += ', opaque="%s"' % opaque
        base += ', algorithm="%s"' % algorithm
        if qop:
            base += ', qop=auth, nc=%s, cnonce="%s"' % (nc_value, cnonce)
        return base


class ProxyAuthorization(object):
    """
    ProxyAuthorization
    """

    def __init__(self, proxy, remote_host, remote_port=443):
        """
        __init__
        """
        self.m_proxy = proxy
        self.m_proxy_handler = SophosProxyDigestAuthHandler(self)
        self.m_auth_header = self.m_proxy.auth_header()
        self.m_remote_host = remote_host
        self.m_remote_port = remote_port

    def auth_header(self):
        """
        auth_header
        """
        return self.m_auth_header

    def get_authenticate_header(self, response):
        """
        get_authenticate_header
        """
        headers = response.msg.headers
        for header in headers:
            if header.lower().startswith(b"proxy-authenticate: "):
                return header[len('Proxy-Authenticate: '):]

        LOGGER.error("No authentication header found: %s", str(headers))

    def update_auth_header(self, response):
        """
        @return True if we should retry
        """
        authentication_header = self.get_authenticate_header(response)
        if authentication_header is None:
            return False

        scheme = authentication_header.split()[0]
        if scheme.lower() != b"digest":
            LOGGER.warning("Proxy authentication scheme is %s", scheme)
            return False

        return self.update_digest_auth_header(authentication_header)

    def add_password(self):
        """
        add_password
        """
        pass

    def find_user_password(self):
        """
        find_user_password
        """
        return self.m_proxy.username(), self.m_proxy.password()

    def update_digest_auth_header(self, auth):
        """
        @return True if we should retry
        """
        token, challenge = auth.split(' ', 1)
        chal = parse_keqv_list(parse_http_list(challenge))

        auth = self.m_proxy_handler.get_authorization(
            self.m_remote_host, chal, remote_port=self.m_remote_port)

        if auth:
            self.m_auth_header = b'Digest %s' % auth
            return True

        LOGGER.error("Unable to get authorization!")
        return False
