#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright (C) 2017 Sophos Plc, Oxford, England.
# All rights reserved.
"""
proxy_authorization Module
"""



import logging
import urllib.request
import urllib.error
import urllib.parse

LOGGER = logging.getLogger(__name__)


class SophosProxyDigestAuthHandler(urllib.request.AbstractDigestAuthHandler):
    """
    SophosProxyDigestAuthHandler
    """

    def get_proxy_authorization(self, remote_host, chal, remote_port=443):
        """
        get_proxy_authorization
        """
        #pylint: disable=too-many-locals
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
        #pylint: disable=invalid-name
        H, KD = self.get_algorithm_impls(algorithm)
        if H is None:
            return None

        user, password = self.passwd.find_user_password()
        if user is None:
            return None

        A1 = "%s:%s:%s" % (user, realm, password)
        uri = b"%s:%d" % (
            remote_host.encode("utf-8"),
            remote_port)
        A2 = "CONNECT:%s" % uri

        H_A1_as_bytes = H(A1).encode("utf-8")
        H_A2_as_bytes = H(A2).encode("utf-8")

        if qop == 'auth':
            if nonce == self.last_nonce:
                self.nonce_count += 1
            else:
                self.nonce_count = 1
                self.last_nonce = nonce

            nc_value = b'%08x' % self.nonce_count
            cnonce = self.get_cnonce(nonce)

            qop_as_bytes = qop.encode("utf-8")

            # nonce_bit = b"%s:%s:%s:%s:%s" % (
            #     nonce, nc_value, cnonce, qop_as_bytes, H_A2_as_bytes)
            nonce_bit = "{}{}{}{}{}".format(
                nonce, nc_value, cnonce, qop_as_bytes, H_A2_as_bytes)
            nonce_bit = nonce_bit.encode("utf-8")

            respdig = KD(H_A1_as_bytes, nonce_bit)
        elif qop is None:
            respdig = KD(H_A1_as_bytes, b"%s:%s" % (nonce, H_A2_as_bytes))
        else:
            # TODO: handle auth-int.
            raise urllib.error.URLError("qop '%s' is not supported." % qop)

        # TODO: should the partial digests be encoded too?

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
    ProxyAuthorization class
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
        #pylint: disable=no-self-use
        msg = response.msg
        proxy_key = [key for key in msg.keys() if key.lower().startswith('proxy-authenticate')]
        if proxy_key:
            return msg.get(proxy_key[0])

        LOGGER.error("No authentication header found: {}".format(msg.as_string()))
        return None

    def update_auth_header(self, response):
        """
        @return True if we should retry
        """
        authentication_header = self.get_authenticate_header(response)
        if authentication_header is None:
            return False

        scheme = authentication_header.split()[0]
        if scheme.lower().encode() != b"digest":
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
        token, challenge = auth.split(' ', 1) #pylint: disable=unused-variable
        chal = urllib.request.parse_keqv_list(urllib.request.parse_http_list(challenge))

        auth = self.m_proxy_handler.get_proxy_authorization(
            self.m_remote_host, chal, remote_port=self.m_remote_port)

        if auth:
            self.m_auth_header = b'Digest %s' % auth.encode("utf-8")
            return True

        LOGGER.error("Unable to get authorization!")
        return False
