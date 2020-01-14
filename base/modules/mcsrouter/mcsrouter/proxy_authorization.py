#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# Copyright (C) 2017 Sophos Plc, Oxford, England.
# All rights reserved.
"""
proxy_authorization Module
"""



import logging
import urllib.error
import urllib.parse
import urllib.request

LOGGER = logging.getLogger(__name__)


class SophosProxyDigestAuthHandler(urllib.request.AbstractDigestAuthHandler):
    """
    SophosProxyDigestAuthHandler
    """

    def get_proxy_authorization(self, remote_host, chal, remote_port=443):
        """
        get_proxy_authorization
        This method is an adaptation of AbstractDigestAuthHandler.get_authorization
        """
        #pylint: disable=too-many-locals
        try:
            realm = chal['realm']
            nonce = chal['nonce']
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

        A1 = "{}:{}:{}".format(user, realm, password)
        uri = "{}:{}".format(remote_host, remote_port)
        A2 = "CONNECT:%s" % uri

        H_A1_as_bytes = H(A1)
        H_A2_as_bytes = H(A2)

        if qop == 'auth':
            if nonce == self.last_nonce:
                self.nonce_count += 1
            else:
                self.nonce_count = 1
                self.last_nonce = nonce

            nc_value = '%08x' % self.nonce_count
            cnonce = self.get_cnonce(nonce)

            nonce_bit = "{}:{}:{}:{}:{}".format(
                nonce, nc_value, cnonce, qop, H_A2_as_bytes)

            respdig = KD(H_A1_as_bytes, nonce_bit)
        elif qop is None:
            respdig = KD(H_A1_as_bytes, "{}:{}".format(nonce, H_A2_as_bytes))
        else:
            # TODO: handle auth-int.
            raise urllib.error.URLError("qop '{}' is not supported.".format(qop))

        # TODO: should the partial digests be encoded too?

        base = 'username="{}", realm="{}", nonce="{}", uri="{}", ' \
               'response="{}"'.format(user, realm, nonce, uri, respdig)
        if opaque:
            base += ', opaque="{}"'.format(opaque)
        base += ', algorithm="{}"'.format(algorithm)
        if qop:
            base += ', qop=auth, nc={}, cnonce="{}"'.format(nc_value, cnonce)
        return base


class ProxyAuthorization:
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
            self.m_auth_header = 'Digest {}'.format(auth)
            return True

        LOGGER.error("Unable to get authorization!")
        return False
