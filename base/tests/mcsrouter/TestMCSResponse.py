
import unittest

import http.client

import PathManager

import mcsrouter.utils.config
import mcsrouter.mcsclient.mcs_connection
import mcsrouter.mcsclient.mcs_exception

def createTestConfig(url="http://localhost/foobar", save=True):
    if not save:
        config = configWithoutSave()
    else:
        config = mcsrouter.utils.config.Config()
    #the mcs_connection only check there is a file not that it is valid
    config.set("CAFILE", "/etc/fstab")
    config.set("MCSURL", url)
    config.set("MCSID", "")
    config.set("MCSPassword", "")
    config.set("jwt_token", "")
    config.set("device_id", "")
    config.set("tenant_id", "")
    return config

class configWithoutSave(mcsrouter.utils.config.Config):
    def save(self, filename=None):
        pass

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
        return b"a" * min(length, self.__m_length)

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
    def __init__(self, save=True, **kwargs):
        config = createTestConfig("http://dummy.mcs.url:8080/", save)
        super(dummyMCSConnection, self).__init__(config)
        self.__m_kwargs = kwargs
        self.config = config
        self.m_jwt_token = None
        self.m_device_id = None
        self.m_tenant_id = None
        self.m_jwt_expiration_timestamp = 1585708424.2720187

    def _MCSConnection__create_connection_and_get_response(self, requestData):
        print("in good one")
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
        conn = dummyMCSConnection(status=http.client.UNAUTHORIZED)
        with self.assertRaises(mcsrouter.mcsclient.mcs_connection.MCSHttpUnauthorizedException) as cm:
            caps = conn.capabilities()
        self.assertEqual( cm.exception.error_code(), http.client.UNAUTHORIZED)

    def testServiceUnavailable(self):
        conn = dummyMCSConnection(status=http.client.SERVICE_UNAVAILABLE)
        with self.assertRaises(mcsrouter.mcsclient.mcs_connection.MCSHttpServiceUnavailableException) as cm:
            caps = conn.capabilities()
        self.assertEqual( cm.exception.error_code(), http.client.SERVICE_UNAVAILABLE)

    def testGatewayTimeout(self):
        conn = dummyMCSConnection(status=http.client.GATEWAY_TIMEOUT)
        with self.assertRaises(mcsrouter.mcsclient.mcs_connection.MCSHttpGatewayTimeoutException) as cm:
            caps = conn.capabilities()
        self.assertEqual( cm.exception.error_code(), http.client.GATEWAY_TIMEOUT)

    def testHTTPerror400(self):
        conn = dummyMCSConnection(status=http.client.BAD_REQUEST)
        with self.assertRaises(mcsrouter.mcsclient.mcs_connection.MCSHttpException) as cm:
            caps = conn.capabilities()
        self.assertEqual( cm.exception.error_code(), http.client.BAD_REQUEST)

    def testHTTPerror401(self):
        conn = dummyMCSConnection(status=http.client.UNAUTHORIZED)
        with self.assertRaises(mcsrouter.mcsclient.mcs_connection.MCSHttpException) as cm:
            caps = conn.capabilities()
        self.assertEqual( cm.exception.error_code(), http.client.UNAUTHORIZED)

    def testHTTPerror403(self):
        conn = dummyMCSConnection(status=http.client.FORBIDDEN)
        with self.assertRaises(mcsrouter.mcsclient.mcs_connection.MCSHttpException) as cm:
            caps = conn.capabilities()
        self.assertEqual( cm.exception.error_code(), http.client.FORBIDDEN)

    def testHTTPerror404(self):
        conn = dummyMCSConnection(status=http.client.NOT_FOUND)
        with self.assertRaises(mcsrouter.mcsclient.mcs_connection.MCSHttpException) as cm:
            caps = conn.capabilities()
        self.assertEqual( cm.exception.error_code(), http.client.NOT_FOUND)

    def testHTTPerror410(self):
        conn = dummyMCSConnection(status=http.client.GONE)
        with self.assertRaises(mcsrouter.mcsclient.mcs_connection.MCSHttpException) as cm:
            caps = conn.capabilities()
        self.assertEqual( cm.exception.error_code(), http.client.GONE)

    def testHTTPerror500(self):
        conn = dummyMCSConnection(status=http.client.INTERNAL_SERVER_ERROR)
        with self.assertRaises(mcsrouter.mcsclient.mcs_connection.MCSHttpException) as cm:
            caps = conn.capabilities()
        self.assertEqual( cm.exception.error_code(), http.client.INTERNAL_SERVER_ERROR )

    def testHTTPerror502(self):
        conn = dummyMCSConnection(status=http.client.BAD_GATEWAY)
        with self.assertRaises(mcsrouter.mcsclient.mcs_connection.MCSHttpException) as cm:
            caps = conn.capabilities()
        self.assertEqual( cm.exception.error_code(), http.client.BAD_GATEWAY )

    def testHTTPerror503(self):
        conn = dummyMCSConnection(status=http.client.SERVICE_UNAVAILABLE)
        with self.assertRaises(mcsrouter.mcsclient.mcs_connection.MCSHttpException) as cm:
            caps = conn.capabilities()
        self.assertEqual( cm.exception.error_code(), http.client.SERVICE_UNAVAILABLE )

    def testOtherError(self):
        # test a random error code.
        conn = dummyMCSConnection(status=http.client.EXPECTATION_FAILED, reason="bogus error")
        with self.assertRaises(mcsrouter.mcsclient.mcs_connection.MCSHttpException) as cm:
            caps = conn.capabilities()
        self.assertEqual( cm.exception.error_code(), http.client.EXPECTATION_FAILED )

# except ImportError:
#     print >>sys.stderr,sys.path
#     class TestMCSResponse(unittest.TestCase):
#         pass

if __name__ == '__main__':
    unittest.main()
