


import unittest
import sys
import json

import logging
logger = logging.getLogger("TestProxyAuthorization")

import PathManager

import mcsrouter.proxy_authorization

class FakeProxy(object):
    def auth_header(self):
        return "Fake Auth Header"

    def host(self):
        return "fakeproxy"

    def username(self):
        return "username"

    def password(self):
        return "password"

    def port(self):
        return 8080

class FakeObject(object):
    pass

def getFakeResponse():
    fake = FakeObject()
    fake.msg = {'Server': 'BaseHTTP/0.3 Python/2.7.13\r\n', 'Date': 'Mon, 25 Sep 2017 15:57:58 GMT\r\n', 'Proxy-Authenticate': 'Digest realm="TestProxy", nonce="4fb2664f1c4e056d8f69f50e8dca65b1", algorithm="MD5", qop="auth"\r\n'}
    return fake

def createProxyAuthorization(proxy=None):
    if proxy is None:
        proxy = FakeProxy()
    return mcsrouter.proxy_authorization.ProxyAuthorization(proxy, "allegro")

class TestProxyAuthorization(unittest.TestCase):
    def testConstruction(self):
        x = createProxyAuthorization()

    def testExtractingHeader(self):
        fake = getFakeResponse()
        x = createProxyAuthorization()
        header = x.get_authenticate_header(fake)
        self.assertEqual(header, 'Digest realm="TestProxy", nonce="4fb2664f1c4e056d8f69f50e8dca65b1", algorithm="MD5", qop="auth"\r\n')

    def testUpdateHeader(self):
        fake = getFakeResponse()
        x = createProxyAuthorization()
        res = x.update_auth_header(fake)
        self.assertTrue(res)

if __name__ == '__main__':
    unittest.main()


