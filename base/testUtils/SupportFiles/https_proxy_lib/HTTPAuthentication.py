

from hashlib import md5


import base64
import re
import sys
import time

import logging
logger = logging.getLogger(__name__)

def hash_md5(data):
    input_data = data.encode('utf-8') if isinstance(data, str) else data
    hashed = md5(input_data).hexdigest()
    return hashed

class PasswordManager(object):
    def __init__(self, username, password):
        self.__m_username = username
        self.__m_password = password

    def getPassword(self, username):
        if username == self.__m_username:
            return self.__m_password
        return None

    def hasUser(self, username):
        return username == self.__m_username

class AuthHandler(object):
    def __init__(self,httprequest,wfile):
        self.wfile = wfile
        self.command = httprequest.command
        self.path = httprequest.rawpath
        authHeader = httprequest.findHeaderValue(httprequest.headers,"Proxy-Authorization",None)
        self.headers = {}

        self.outHeader = {}
        if authHeader != "":
            self.headers['Proxy-Authorization'] = authHeader

    def send_error(self,code, message="ERROR"):
        self.code = code
        self.message = message
        self.end_headers()

    def send_response(self,code,message="ERROR"):
        self.code = code
        if code == 407:
            self.message = "Proxy Authentication Required"
        else:
            self.message = message

    def send_407(self, message="Proxy Authentication Required"):
        print >>sys.stderr,'AuthHandler:send_407'
        self.code = 407
        self.message = message

    def send_header(self,keyword,value):
        self.outHeader[keyword] = value

    def end_headers(self):
        print >>sys.stderr,'AuthHandler:end_headers'
        body = ("<html><head><title>Error</title></head>" +
                "<body><h1>%s %s.</h1></body>"%(self.code,self.message) +
                "</html>")
        assert isinstance(body, str)

        response = "HTTP/1.0 %d %s\n"%(self.code,self.message)
        self.outHeader['Content-Type'] = "text/html"
        self.outHeader['Content-Length'] = str(len(body))
        #~ self.outHeader['Proxy-Connection'] = 'close'
        #~ self.outHeader['Connection'] = 'close'
        for (keyword,value) in self.outHeader.iteritems():
            response += "%s: %s\n"%(keyword,value)

        response += "\n"
        response = response.replace("\n","\r\n")
        response += body
        self.wfile.write(response)
        self.wfile.flush()

class Authentication(object):
    def getRealm(self):
        return "TestProxy"

class NullAuthentication(Authentication):
    def authenticate(self,handler):
        return True

AUTHORIZATION = re.compile (
        #scheme  challenge
        '([^ ]+) (.*)',
        re.IGNORECASE
        )

class BasicAuthentication(Authentication):

    def __init__(self, passwords):
        self.__m_passwords = passwords

    def authorize(self, auth_info):
        (username, password) = auth_info
        logger.info("Authenticated with {}:{}".format(username,password))
        return self.__m_passwords.getPassword(username) == password

    def getScheme(self):
        return "Basic"

    def return407(self, handler):
        handler.send_407()
        handler.send_header('Proxy-Authenticate', '{} realm="{}"'.format(self.getScheme(), self.getRealm()))
        handler.end_headers()
        return False

    def authenticate(self, handler):
        auth_header = handler.headers.get("Proxy-Authorization")
        if auth_header is None:
            logger.error('no authorization info')
            self.return407(handler)
            return False

        mo = AUTHORIZATION.match(auth_header)
        if mo is None:
            logger.error('malformed authorization info <{}>'.format(auth_header))
            handler.send_error(400, 'malformed authorization info <{}>'.format(auth_header))
            return False

        scheme = mo.group(1)
        cookie = mo.group(2)

        if scheme.upper() != self.getScheme().upper():
            self.return407(handler)
            return False

        try:
            decoded = base64.b64decode(cookie)
        except Exception as e:
            logger.error(e)
            logger.error('malformed authorization info "{}" from "{}"'.format(cookie,auth_header))
            handler.send_error(400, 'malformed authorization info "{}" from "{}"'.format(cookie,auth_header))
            return False

        auth_info = decoded.decode('utf-8').split(':')
        if self.authorize(auth_info):
            return True

        logger.error('bad authorization info {}'.format(auth_info))
        self.return407(handler)
        return False

class DigestAuthentication(Authentication):

    def __init__(self, passwords):
        self.__m_passwords = passwords
        self.realm = self.getRealm()

    def H(self, data):
        return hash_md5(data)

    def KD(self, secret, data):
        return self.H(secret + ":" + data)

    def A1(self):
        # If the "algorithm" directive's value is "MD5" or is
        # unspecified, then A1 is:
        # A1 = unq(username-value) ":" unq(realm-value) ":" passwd
        username = self.params["username"]
        passwd = self.__m_passwords.getPassword(username)
        return "{}:{}:{}".format(username, self.realm, passwd)
        # This is A1 if qop is set
        # A1 = H( unq(username-value) ":" unq(realm-value) ":" passwd )
        #         ":" unq(nonce-value) ":" unq(cnonce-value)

    def A2(self):
        # If the "qop" directive's value is "auth" or is unspecified, then A2 is:
        # A2 = Method ":" digest-uri-value
        return self.method + ":" + self.params['uri']
        # Not implemented
        # If the "qop" value is "auth-int", then A2 is:
        # A2 = Method ":" digest-uri-value ":" H(entity-body)

    def response(self):
        if 'qop' in self.params:
            # Check? and self.params["qop"].lower()=="auth":
            # If the "qop" value is "auth" or "auth-int":
            # request-digest  = <"> < KD ( H(A1),     unq(nonce-value)
            #                              ":" nc-value
            #                              ":" unq(cnonce-value)
            #                              ":" unq(qop-value)
            #                              ":" H(A2)
            #                      ) <">
            return self.KD(self.H(self.A1()), \
                           self.params["nonce"]
                           + ":" + self.params["nc"]
                           + ":" + self.params["cnonce"]
                           + ":" + self.params["qop"]
                           + ":" + self.H(self.A2()))
        else:
            # If the "qop" directive is not present (this construction is
            # for compatibility with RFC 2069):
            # request-digest  =
            #         <"> < KD ( H(A1), unq(nonce-value) ":" H(A2) ) > <">
            return self.KD(self.H(self.A1()), \
                           self.params["nonce"] + ":" + self.H(self.A2()))

    def _parseHeader(self, authheader):
        try:
            n = len("Digest ")
            authheader = authheader[n:].strip()
            items = authheader.split(", ")
            keyvalues = [i.split("=", 1) for i in items]
            keyvalues = [(k.strip(), v.strip().replace('"', '')) for k, v in keyvalues]
            self.params = dict(keyvalues)
        except:
            self.params = []

    def _returnTuple(self, code):
        return (code, self._headers, self.params.get("username", ""))

    def _createNonce(self):
        return hash_md5("{}:{}".format(time.time(), self.realm))

    def createAuthheader(self):
        self._headers.append((
            "Proxy-Authenticate",
            'Digest realm="{}", nonce="{}", algorithm="MD5", qop="auth"'.format(self.realm, self._createNonce())
            ))

    def __authenticate(self, method, uri, authheader=""):
        """ Check the response for this method and uri with authheader

        returns a tuple with:
          - HTTP_CODE
          - a tuple with header info (key, value) or None
          - and the username which was authenticated or None
        """
        logger.debug("uri={} method={} authheader={}".format(uri, method, authheader))
        self.method = method
        self.uri = uri
        self.params = {}
        self._headers = []
        if authheader is None or authheader.strip() == "" or not authheader.upper().startswith("DIGEST "):
            logger.error("Failing authentication with authheader: {}".format(authheader))
            self.createAuthheader()
            return self._returnTuple(407)
        self._parseHeader(authheader)
        if not len(self.params):
            return self._returnTuple(400)
        # Check for required parameters
        required = ["username", "realm", "nonce", "uri", "response"]
        for k in required:
            if k not in self.params:
                return self._returnTuple(400)
        # If the user is unknown we can deny access right away
        if not self.__m_passwords.hasUser(self.params["username"]):
            logger.error("Bad username: {}".format(self.params["username"]))
            self.createAuthheader()
            return self._returnTuple(407)
        # If qop is sent then cnonce and cn MUST be present
        if 'qop' in self.params:
            if 'cnonce' not in self.params \
               and 'cn' not in self.params:
                return self._returnTuple(400)

        if self.uri != self.params['uri']:
            logger.error("URI MISMATCH: %s vs %s",self.params["uri"],self.uri)
            if not self.uri.endswith(self.params["uri"]):
                return self._returnTuple(400)

        # All else is OK, now check the response.
        if self.response() == self.params["response"]:
            #~ logger.debug("Allowing DIGEST authentication")
            return self._returnTuple(200)
        else:
            logger.error("Bad response: expecting: {} got {}".format(self.response(), self.params["response"]))
            self.createAuthheader()
            return self._returnTuple(407)

    def authenticate(self,handler):
        authheader = handler.headers.get("Proxy-Authorization")
        (code,headers,username) = self.__authenticate(handler.command,handler.path,authheader)
        logger.debug("PROXY DIGEST AUTH: %s -> %d %s",authheader,code,str(headers))
        if code == 200:
            ## Let normal handling occue
            return True
        else:
            handler.send_response(code)
            for (header,value) in headers:
                handler.send_header(header,value)
            handler.end_headers()
        return False
