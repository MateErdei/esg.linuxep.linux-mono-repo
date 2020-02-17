import logging
import urllib3
import http
import requests
OriginalVerifiedHTTPSConnection = urllib3.connection.VerifiedHTTPSConnection
from mcsrouter.sophos_https import Proxy, proxy_authorization, set_proxy_headers
logger = logging.getLogger("proxy")

# in order to use this work-around, add the following lines to the module that will add support for proxy
# this is necessary in order to 'force' requests to use the digestproxyworkaround.ReplaceConnection class (which has the workaround)
# instead of its original one.
# import requests
# import urllib3
# import digestproxyworkaround
# urllib3.connectionpool.HTTPSConnectionPool.ConnectionCls = digestproxyworkaround.ReplaceConnection


class GlobalAuthentication:
    def __init__(self):
        self.user = None
        self.passw = None

    def set(self, user, passw):
        self.user = user
        self.passw = passw

    def get(self):
        return self.user, self.passw

    def clear(self):
        self.user = None
        self.passw = None


GLOBALAUTHENTICATION = GlobalAuthentication()

class ProxyTunnelException(RuntimeError):
    def __init__(self, msg, response):
        RuntimeError.__init__(self, msg)
        self.response = response


class ReplaceConnection(OriginalVerifiedHTTPSConnection):
    def __init__(self, **kwargs):
        OriginalVerifiedHTTPSConnection.__init__(self, **kwargs)

    def _tunnel(self):
        """This is an adapted copy of the python3.7/http/client.py:HTTPConnection::_tunnel method.
           The limitation of that method is that when it receives a non-ok from the request, it throws OSError,
           but does not provide the response (which is necessary in order to respond to the DIGEST 407 challenge)

           Please, be aware that after any upgrade to that library, this code should be reviewed.
           The whole purpose of this code is to detect that a 407 challenge has been issued, and throw a  ProxyTunnelException,
           otherwise, keep the previous behaviour.
           The connect method intercept the exception and respond to the challenge.
        """
        logger.debug('Replace connection being used')
        connect_str = "CONNECT %s:%d HTTP/1.0\r\n" % (self._tunnel_host,
                                                      self._tunnel_port)
        connect_bytes = connect_str.encode("ascii")
        self.send(connect_bytes)
        for header, value in self._tunnel_headers.items():
            header_str = "%s: %s\r\n" % (header, value)
            header_bytes = header_str.encode("latin-1")
            self.send(header_bytes)
        self.send(b'\r\n')

        response = self.response_class(self.sock, method=self._method)
        response.begin()
        (version, code, message) = response.version, response.code, response.reason

        if code != http.HTTPStatus.OK:
            self.close()
            errmsg = "Tunnel connection failed: %d %s" % (code,
                                                          message.strip())
            if code == http.HTTPStatus.PROXY_AUTHENTICATION_REQUIRED:
                raise ProxyTunnelException(errmsg, response=response)
            raise OSError(errmsg)

    def _get_digest_header(self, response):
        proxy_name, proxy_pass = GLOBALAUTHENTICATION.get()
        if not proxy_name or not proxy_pass:
            return None
        proxy_ = Proxy(self.host, username=proxy_name, password=proxy_pass)
        auth = proxy_authorization.ProxyAuthorization(proxy_,  self._tunnel_host, self._tunnel_port )
        auth.update_auth_header(response)
        proxy_username_password = auth.auth_header()
        proxy_headers = set_proxy_headers(proxy_username_password)
        return proxy_headers['Proxy-authorization']


    def connect(self):
        try:
            OriginalVerifiedHTTPSConnection.connect(self)
        except ProxyTunnelException as perr:
            proxy_auth = self._get_digest_header(perr.response)
            print("{}".format(dict(perr.response.headers)))
            if proxy_auth:
                self._tunnel_headers['Proxy-authorization'] = proxy_auth
            else:
                raise
            logger.info("Handle proxy authentication request")
            OriginalVerifiedHTTPSConnection.connect(self)


if __name__=="__main__":
    cert='/home/pair/gitrepos/sspl-tools/everest-base/testUtils/SupportFiles/CloudAutomation/root-ca.crt.pem'
    proxy_url='http://localhost:3000'
    #proxy_url='http://localhost:3000'
    proxy_port=3000
    proxy_username='user'
    proxy_password='pass'
    url_host='localhost'
    url_port=8459
    proxy={'https':proxy_url,'http':proxy_url}
    # HACK; before and after the connection the GLOBAL AUTHENTICATION will be needed to be able to pass the
    # proxy authentication to the ReplaceConnection object
