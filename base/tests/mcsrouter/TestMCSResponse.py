import unittest
import sys

import PathManager
import httplib


import mcsrouter.utils.config
import mcsrouter.mcsclient.mcs_connection
import mcsrouter.mcsclient.mcs_exception

def createTestConfig(url="http://localhost/foobar"):
    config = mcsrouter.utils.config.Config()
    #the mcs_connection only check there is a file not that it is valid
    config.set("CAFILE", "/etc/fstab")
    config.set("MCSURL", url)
    config.set("MCSID", "")
    config.set("MCSPassword", "")
    return config

class dummyResponse(object):
    class dummyMsg(object):
        def getallmatchingheaders(self, name):
            return []

    def __init__(self, length=100, rep_length=-1, status=200, reason=""):
        self.status = status
        self.reason = reason
        self.msg = self.dummyMsg()

        self.__m_length = length
        if rep_length == -1:
            self.__m_rep_length = length
        else:
            self.__m_rep_length = rep_length

    def getheaders(self):
        return [ ( "Content-Length", self.__m_rep_length ) ]

    def read(self, length):
        return "a" * min(length, self.__m_length)

class dummyConnection(object):
    def __init__(self, **kwargs):
        self.__m_kwargs = kwargs

    def request(self, method, url, body=None, headers={}):
        pass

    def close(self):
        pass

    def getresponse(self):
        return dummyResponse(**self.__m_kwargs)

# stub the underlying connection
class dummyMCSConnection(mcsrouter.mcsclient.mcs_connection.MCSConnection):
    def __init__(self, **kwargs):
        config = createTestConfig("http://dummy.mcs.url:8080/")
        super(dummyMCSConnection, self).__init__(config)
        self.__m_kwargs = kwargs

        self.m_jwt_token = None
        self.m_device_id = None
        self.m_tenant_id = None
        self.m_jwt_expiration_timestamp = 1585708424.2720187

    def _MCSConnection__create_connection_and_get_response(self, requestData):
        print "in good one"
        if not self._MCSConnection__m_connection:
            self._MCSConnection__m_connection = dummyConnection(**self.__m_kwargs)
        return self._MCSConnection__get_response(requestData)


class TestMCSResponse(unittest.TestCase):
    ## test response sizes

    def testCorrectLength(self):
        conn = dummyMCSConnection(length=100)
        caps = conn.capabilities()
        self.assertEqual( len(caps), 100 )

    def testMissingLength(self):
        conn = dummyMCSConnection(length=100, rep_length=None)
        caps = conn.capabilities()
        self.assertEqual( len(caps), 100 )

    def testZeroLength(self):
        conn = dummyMCSConnection(length=0)
        caps = conn.capabilities()
        self.assertEqual( len(caps), 0 )

    def testImplicitZeroLength(self):
        conn = dummyMCSConnection(length=0, rep_length=None)
        caps = conn.capabilities()
        self.assertEqual( len(caps), 0 )

    def testTooLong(self):
        conn = dummyMCSConnection(length=101, rep_length=100)
        with self.assertRaises(mcsrouter.mcsclient.mcs_exception.MCSNetworkException) as cm:
            caps = conn.capabilities()
        self.assertEqual( str(cm.exception), "Response too long" )

    def testTooShort(self):
        conn = dummyMCSConnection(length=99, rep_length=100)
        with self.assertRaises(mcsrouter.mcsclient.mcs_exception.MCSNetworkException) as cm:
            caps = conn.capabilities()
        self.assertEqual( str(cm.exception), "Response too short" )

    def testExplicitOverMax(self):
        length = mcsrouter.mcsclient.mcs_connection.MCS_DEFAULT_RESPONSE_SIZE_MIB * 10 * 1024 * 1024 + 1
        conn = dummyMCSConnection(length=length)
        with self.assertRaises(mcsrouter.mcsclient.mcs_exception.MCSNetworkException) as cm:
            caps = conn.capabilities()
        self.assertEqual( str(cm.exception), "Content-Length too large" )

    def testImplicitOverMax(self):
        length = mcsrouter.mcsclient.mcs_connection.MCS_DEFAULT_RESPONSE_SIZE_MIB * 1024 * 1024 + 1
        conn = dummyMCSConnection(length=length, rep_length=None)
        with self.assertRaises(mcsrouter.mcsclient.mcs_exception.MCSNetworkException) as cm:
            caps = conn.capabilities()
        self.assertEqual( str(cm.exception), "Response too long" )

    ## test status error codes

    def testUnauthorized(self):
        conn = dummyMCSConnection(status=httplib.UNAUTHORIZED)
        with self.assertRaises(mcsrouter.mcsclient.mcs_connection.MCSHttpUnauthorizedException) as cm:
            caps = conn.capabilities()
        self.assertEqual( cm.exception.error_code(), httplib.UNAUTHORIZED )

    def testServiceUnavailable(self):
        conn = dummyMCSConnection(status=httplib.SERVICE_UNAVAILABLE)
        with self.assertRaises(mcsrouter.mcsclient.mcs_connection.MCSHttpServiceUnavailableException) as cm:
            caps = conn.capabilities()
        self.assertEqual( cm.exception.error_code(), httplib.SERVICE_UNAVAILABLE )

    def testGatewayTimeout(self):
        conn = dummyMCSConnection(status=httplib.GATEWAY_TIMEOUT)
        with self.assertRaises(mcsrouter.mcsclient.mcs_connection.MCSHttpGatewayTimeoutException) as cm:
            caps = conn.capabilities()
        self.assertEqual( cm.exception.error_code(), httplib.GATEWAY_TIMEOUT )

    def testOtherError(self):
        # test a random error code.
        conn = dummyMCSConnection(status=httplib.EXPECTATION_FAILED, reason="bogus error")
        with self.assertRaises(mcsrouter.mcsclient.mcs_connection.MCSHttpException) as cm:
            caps = conn.capabilities()
        self.assertEqual( cm.exception.error_code(), httplib.EXPECTATION_FAILED )

# except ImportError:
#     print >>sys.stderr,sys.path
#     class TestMCSResponse(unittest.TestCase):
#         pass

if __name__ == '__main__':
    unittest.main()
