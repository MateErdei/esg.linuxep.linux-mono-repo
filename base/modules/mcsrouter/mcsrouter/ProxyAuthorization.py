#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright (C) 2017 Sophos Plc, Oxford, England.
# All rights reserved.

from __future__ import absolute_import,print_function,division,unicode_literals

import logging
logger = logging.getLogger(__name__)

import urllib2
parse_http_list = urllib2.parse_http_list
parse_keqv_list = urllib2.parse_keqv_list

class SophosProxyDigestAuthHandler(urllib2.AbstractDigestAuthHandler):
    def get_authorization(self, remoteHost, chal, remotePort=443):
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

        user, pw = self.passwd.find_user_password(realm, "")
        if user is None:
            return None

        A1 = b"%s:%s:%s" % (user, realm, pw)
        uri = b"%s:%d"%(
                remoteHost,
                remotePort)
        A2 = b"CONNECT:%s"%uri

        if qop == 'auth':
            if nonce == self.last_nonce:
                self.nonce_count += 1
            else:
                self.nonce_count = 1
                self.last_nonce = nonce

            ncvalue = b'%08x' % self.nonce_count
            cnonce = self.get_cnonce(nonce)
            noncebit = b"%s:%s:%s:%s:%s" % (nonce, ncvalue, cnonce, qop, H(A2))
            respdig = KD(H(A1), noncebit)
        elif qop is None:
            respdig = KD(H(A1), b"%s:%s" % (nonce, H(A2)))
        else:
            # XXX handle auth-int.
            raise URLError("qop '%s' is not supported." % qop)

        # XXX should the partial digests be encoded too?

        base = 'username="%s", realm="%s", nonce="%s", uri="%s", ' \
               'response="%s"' % (user, realm, nonce, uri, respdig)
        if opaque:
            base += ', opaque="%s"' % opaque
        base += ', algorithm="%s"' % algorithm
        if qop:
            base += ', qop=auth, nc=%s, cnonce="%s"' % (ncvalue, cnonce)
        return base


class ProxyAuthorization(object):
    def __init__(self, proxy, remoteHost, remotePort=443):
        self.m_proxy = proxy
        self.m_proxyHandler = SophosProxyDigestAuthHandler(self)
        self.m_authHeader = self.m_proxy.authHeader()
        self.m_remoteHost = remoteHost
        self.m_remotePort = remotePort

    def authHeader(self):
        return self.m_authHeader

    def getAuthicateHeader(self, response):
        headers = response.msg.headers
        for header in headers:
            if header.lower().startswith(b"proxy-authenticate: "):
                return header[len('Proxy-Authenticate: '):]

        logger.error("No authentication header found: %s",str(headers))

    def updateAuthHeader(self, response):
        """
        @return True if we should retry
        """
        authenticationHeader = self.getAuthicateHeader(response)
        if authenticationHeader is None:
            return False

        scheme = authenticationHeader.split()[0]
        if scheme.lower() != b"digest":
            logger.warning("Proxy authentication scheme is %s",scheme)
            return False

        return self.updateDigestAuthHeader(authenticationHeader)

    def add_password(self):
        pass

    def find_user_password(self, realm, authuri):
        return self.m_proxy.username(),self.m_proxy.password()

    def updateDigestAuthHeader(self, auth):
        """
        @return True if we should retry
        """
        token, challenge = auth.split(' ', 1)
        chal = parse_keqv_list(parse_http_list(challenge))

        auth = self.m_proxyHandler.get_authorization(self.m_remoteHost, chal,
            remotePort=self.m_remotePort)

        if auth:
            self.m_authHeader = b'Digest %s' % auth
            return True

        logger.error("Unable to get authorization!")
        return False
