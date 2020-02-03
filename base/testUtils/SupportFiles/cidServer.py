#! /usr/bin/env python

## Simple python web-server for CID serving
## Copied from perforce //dev/savlinux/regression/supportFiles/cidServer.py
# enable the with statement


import http.server
import re
import sys
from hashlib import md5
import time
import os
import socketserver
import optparse
import subprocess

loggingOn = False;

class PasswordDatabase(object):
    def __init__(self,username,password):
        self.__authentication = {username:password}

    def getPassword(self,username):
        return self.__authentication.get(username,None)

    def hasUser(self,username):
        return username in self.__authentication

    def isEnabled(self):
        return len(self.__authentication) != 0

class Authentication(object):
    pass

AUTHORIZATION = re.compile (
        #scheme  challenge
        '([^ ]+) (.*)',
        re.IGNORECASE
        )

class BasicAuthentication(Authentication):

    def __init__(self,passwords):
        self.__m_passwords = passwords

    def authorize(self,auth_info):
        [username,password] = auth_info
        return self.__m_passwords.getPassword(username) == password

    def getScheme(self):
        return "Basic"

    def return401(self,handler):
        handler.send_response(401)
        handler.send_header('WWW-Authenticate','%s realm="%s"' % (self.getScheme(),"sophos"))
        handler.end_headers()

    def authenticate(self,handler):
        auth_header = handler.headers.get("Authorization")
        if auth_header == None:
            print('no authorization info <%s>' % auth_header, file=sys.stderr)
            self.return401(handler)
            return False

        mo = AUTHORIZATION.match(auth_header)
        if mo == None:
            print('malformed authorization info <%s>' % auth_header, file=sys.stderr)
            handler.send_error(400, 'malformed authorization info <%s>' % auth_header)
            return False

        scheme = mo.group(1)
        cookie = mo.group(2)

        if scheme.upper() != "BASIC":
            self.return401(handler)
            return False

        import base64
        try:
            decoded = base64.b64decode(cookie)
        except:
            print('malformed authorization info <%s>' % cookie, file=sys.stderr)
            handler.send_error(400, 'malformed authorization info <%s>' % cookie)
            return False

        auth_info = decoded.split(':')
        if self.authorize(auth_info):
            return True

        print('bad authorization info <%s>' % auth_info, file=sys.stderr)
        self.return401(handler)
        return False

class DigestAuthentication(Authentication):

    def __init__(self,passwords):
        self.__m_passwords = passwords
        self.realm = "sophos"

    def H(self, data):
        return md5.md5(data).hexdigest()

    def KD(self, secret, data):
        return self.H(secret + ":" + data)

    def A1(self):
        # If the "algorithm" directive's value is "MD5" or is
        # unspecified, then A1 is:
        # A1 = unq(username-value) ":" unq(realm-value) ":" passwd
        username = self.params["username"]
        passwd = self.__m_passwords.getPassword(username)
        return "%s:%s:%s" % (username, self.realm, passwd)
        # This is A1 if qop is set
        # A1 = H( unq(username-value) ":" unq(realm-value) ":" passwd )
        #         ":" unq(nonce-value) ":" unq(cnonce-value)

    def A2(self):
        # If the "qop" directive's value is "auth" or is unspecified, then A2 is:
        # A2 = Method ":" digest-uri-value
        ret = self.method + ":" + self.uri
        print("\tA2=",ret, file=sys.stderr)
        return ret
        # Not implemented
        # If the "qop" value is "auth-int", then A2 is:
        # A2 = Method ":" digest-uri-value ":" H(entity-body)

    def response(self):
        if "qop" in self.params:
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
        return md5.md5("%d:%s" % (time.time(), self.realm)).hexdigest()

    def createAuthheader(self):
        header = 'Digest realm="%s", nonce="%s", algorithm="MD5", qop="auth"'% (self.realm, self._createNonce())
        print("AuthHeader:",header, file=sys.stderr)
        self._headers.append((
            "WWW-Authenticate",header
            )
            )

    def __authenticate(self, method, uri, authheader=""):
        """ Check the response for this method and uri with authheader

        returns a tuple with:
          - HTTP_CODE
          - a tuple with header info (key, value) or None
          - and the username which was authenticated or None
        """
        self.method = method
        self.uri = uri
        self.params = {}
        self._headers = []
        if authheader is None or authheader.strip() == "" or not authheader.upper().startswith("DIGEST "):
            print("Failing authentication with authheader:",authheader, file=sys.stderr)
            self.createAuthheader()
            return self._returnTuple(401)

        print("\tCLIENT AUTH HEADER:",authheader, file=sys.stderr)
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
            print("\tBad username:",self.params["username"], file=sys.stderr)
            self.createAuthheader()
            return self._returnTuple(401)

        if not self.params['uri'].endswith(self.uri):
            print("digest URI is not compatible with request URI:",self.params['uri'],self.uri, file=sys.stderr)
            return self._returnTuple(400)

        ## Once we have validated the URIs are compatible then we must use the specified
        ## URI to do the validatation
        self.uri = self.params['uri']

        # If qop is sent then cnonce and cn MUST be present
        if "qop" in self.params:
            if "cnonce" not in self.params \
               and "cn" in self.params:
                return self._returnTuple(400)
        # All else is OK, now check the response.
        if self.response() == self.params["response"]:
            return self._returnTuple(200)
        else:
            print("\tBad response:",self.response(),self.params['response'], file=sys.stderr)
            self.createAuthheader()
            return self._returnTuple(401)


    def authenticate(self,handler):
        (code,headers,username) = self.__authenticate(handler.command,handler.path,handler.headers.get("Authorization"))
        if code == 200:
            ## Let normal handling occue
            return True
        else:
            handler.send_response(code)
            for (header,value) in headers:
                handler.send_header(header,value)
            handler.end_headers()
        return False

class PostRequestHandler(http.server.BaseHTTPRequestHandler):
    authHandler = None
    latency = 0

    def log_message(self, format, *args):
        """Log an arbitrary message.

        This is used by all other logging functions.  Override
        it if you have specific logging wishes.

        The first argument, FORMAT, is a format string for the
        message to be logged.  If the format string contains
        any % escapes requiring parameters, they should be
        specified as subsequent arguments (it's just like
        printf!).

        The client host and current date/time are prefixed to
        every message.

        """

        sys.stderr.write("%s '%s\' - [%s] %s\n" %
                         ("localhost",
                          "", #self.headers.get('User-Agent', "-"),
                          self.log_date_time_string(),
                          format%args))

    def do_GET(self):
        self.send_response(200)

    def do_POST(self):
        """Respond to a POST request."""
        outputFile = self.path.strip("/")

        length = self.headers['content-length']
        data = self.rfile.read(int(length))
        with open(outputFile, 'w') as fh:
            fh.write(data)

        if PostRequestHandler.latency != 0:
            self.log_message("Sleeping for %f",PostRequestHandler.latency / 1000.0)
            time.sleep(PostRequestHandler.latency / 1000.0)

        self.send_response(200)

    def do_PUT(self):
        """Respond to a PUT request."""
        outputFile = self.path.strip("/")

        length = self.headers['content-length']
        data = self.rfile.read(int(length))
        if not os.path.exists(os.path.dirname(outputFile)):
            os.makedirs(os.path.dirname(outputFile))
        with open(outputFile, 'w') as fh:
            fh.write(data)


        if PostRequestHandler.latency != 0:
            self.log_message("Sleeping for %f",PostRequestHandler.latency / 1000.0)
            time.sleep(PostRequestHandler.latency / 1000.0)


        self.send_response(200)

class PasswordHTTPRequestHandler(http.server.SimpleHTTPRequestHandler):
    authHandler = None
    latency = 0

    def log_message(self, format, *args):
        """Log an arbitrary message.

        This is used by all other logging functions.  Override
        it if you have specific logging wishes.

        The first argument, FORMAT, is a format string for the
        message to be logged.  If the format string contains
        any % escapes requiring parameters, they should be
        specified as subsequent arguments (it's just like
        printf!).

        The client host and current date/time are prefixed to
        every message.

        """

        try:
            useragent = self.headers.get('User-Agent', "-")
        except AttributeError:
            useragent = "-"


        sys.stderr.write("%s '%s\' - [%s] %s\n" %
                         ("localhost",
                          useragent,
                          self.log_date_time_string(),
                          format%args))

    def do_GET(self):
        global loggingOn

        if PasswordHTTPRequestHandler.latency != 0:
            time.sleep(PasswordHTTPRequestHandler.latency / 1000.0)

        if loggingOn:
            print("HTTP request: ",self.path, file=sys.stderr)
        if (
            PasswordHTTPRequestHandler.authHandler != None):
            cont = PasswordHTTPRequestHandler.authHandler.authenticate(self)
        else:
            cont = True
        if cont:
            http.server.SimpleHTTPRequestHandler.do_GET(self)
            ## otherwise the request has already been handled

    def translate_path(self, origpath):
        """
        Do trimming of SUM based CID paths
        """
        mo = re.match(r"/CIDs/S\d{3}/[^/]+(/.*)",origpath)
        if mo:
            path = mo.group(1)
        else:
            path = origpath

        ret = http.server.SimpleHTTPRequestHandler.translate_path(self,path)
        print("translate:",origpath,path,ret, file=sys.stderr)
        return ret


class SlowHandler(PasswordHTTPRequestHandler):
    # Different operating systems have different sleep granularities. To reach our
    # default target rate would mean 1ms sleeps which is at least 10x shorter than some
    # OS-es can do which results in 10x slower rate than we wanted. So we will transmit
    # group of bytes and then sleep for longer instead. As long as RATE is not set to less
    # than 1000 and average packet is not too short this has a good potential to work.

    RATE = 1000 # In communicaiton protocols prefixes are base10

    def copyfile(self, source, outputfile):
        start = time.time()
        sent = 0
        bs = (SlowHandler.RATE / 200)
        if bs == 0:
            bs = 1
        if bs > 1024:
            bs = 1024

        while True:
            duration = time.time() - start
            sleep = 1.0 * sent / SlowHandler.RATE - duration
            if sleep > 0:
                time.sleep(sleep)
            else:
                c = source.read(bs)
                outputfile.write(c)
                if len(c) < bs:
                    break
                sent += len(c)

class HangingHandler(PasswordHTTPRequestHandler):
    def copyfile(self, source, outputfile):
        print("Hanging while serving data for %s"%self.path, file=sys.stderr)
        while True:
            data = source.read()
            if data is None or len(data) == 0:
                break
            while True:
                outputfile.write(None)
                time.sleep(5)

class InfiniteHandler(PasswordHTTPRequestHandler):
    MANIFEST = False

    def isInfinite(self, path):
        return not (
            self.path.endswith("/") or
            self.path.endswith(".inf") or self.path.endswith(".upd") or
            (self.path.endswith(".dat") and not InfiniteHandler.MANIFEST)
            )


    def copyfile(self, source, outputfile):
        if not self.isInfinite(self.path):
            print("Serving %s normally"%self.path, file=sys.stderr)
            return PasswordHTTPRequestHandler.copyfile(self, source, outputfile)

        print("Serving zeros for ever for %s"%self.path, file=sys.stderr)

        data = "\x00" * 1024
        while True:
            outputfile.write(data)

    def send_head(self):
        """Common code for GET and HEAD commands.

        This sends the response code and MIME headers.

        Return value is either a file object (which has to be copied
        to the outputfile by the caller unless the command was HEAD,
        and must be closed by the caller under all circumstances), or
        None, in which case the caller has nothing further to do.

        """
        if not self.isInfinite(self.path):
            return PasswordHTTPRequestHandler.send_head(self)

        self.send_response(200)
        self.send_header("Content-type", "text/plain")
        self.send_header("Content-Length", "99999999999")
        self.send_header("Last-Modified", self.date_time_string(time.time()))
        self.end_headers()
        return open("/dev/null")

def addOptions(parser):
    parser.add_option("-p","--port",action="store",type="int",dest="port",
                       help="Port to server CID from.",default=None)
    parser.add_option("-r","--root",action="store",type="string",dest="root",
                       help="Root for serving files",default=None)
    parser.add_option("-u","--user",action="store",type="string",dest="user",
                       help="Username for authentication",default=None)
    parser.add_option("--password",action="store",type="string",dest="password",
                      help="Password for authentication",default=None)
    parser.add_option("-b","--basic",action="store_true", dest="basic",
                      help="Use basic authentication",default=False)
    parser.add_option("-d","--digest",action="store_true", dest="digest",
                      help="Use digest authentication",default=False)
    parser.add_option("-s","--slow",action="store_true", dest="slow",
                      help="Serve files slowly",default=False)
    parser.add_option("-H","--hang",action="store_true", dest="hang",
                      help="Hang serving the first file",default=False)
    parser.add_option("--infinite",action="store_true", dest="infinite",
                      help="First data file is infinite",default=False)
    parser.add_option("--infinite-manifest",action="store_true", dest="infinite_manifest",
                      help="Manifest files are also infinite",default=False)

    ## rate is in bytes per second
    parser.add_option("--rate",action="store", type="int", dest="rate",
                      help="Rate limit for slow transfers in bytes/sec",default=None)

    ## latency in ms
    parser.add_option("--latency",action="store", type="int", dest="latency",
                      help="Delay before starting transfer in ms",default=0)

    parser.add_option("--loggingOn","--logging","--log",action="store_true", dest="loggingOn",
                      help="Hang serving the first file",default=False)
    parser.add_option("-P","--post","--put",action="store_true", dest="post",
                      help="Handle POST requests",default=False)
    parser.add_option("-S","--secure",action="store", dest="pem",
                      help="PEM file to enable HTTPS",default=None)
    parser.add_option("--tls1_0",action="store_true", dest="tls1_0",
                      help="Force TLS 1.0",default=False)
    parser.add_option("--tls1_1",action="store_true", dest="tls1_1",
                      help="Force TLS 1.1",default=False)
    parser.add_option("--tls1_2",action="store_true", dest="tls1_2",
                      help="Force TLS 1.2",default=False)
    parser.add_option("--bad_ciphers","--ciphers_not_in_restricted_list",action="store_true", dest="bad_ciphers",
                      help="Force only older ciphers",default=False)
    parser.add_option("--good_ciphers",action="store_true", dest="good_ciphers",
                      help="Force only good ciphers",default=False)
    parser.add_option("--all_ciphers",action="store_true", dest="all_ciphers",
                      help="Support all ciphers",default=False)

def getOptionsFromArgs(parser,args):
    (options, args) = parser.parse_args(args)

    if options.infinite_manifest:
        options.infinite = True
        InfiniteHandler.MANIFEST = True

    if options.rate is not None:
        options.slow = True

    options.secure = (options.pem is not None)

    if options.secure:
        if options.pem in ("","default","internal"):
            https_dir = os.path.join(os.environ['SUP'],"https")
            cert = "server-private.pem"
            subprocess.call(["make",cert],cwd=https_dir)
            options.pem = os.path.join(https_dir,cert)

        assert os.path.isfile(options.pem),"Failed to read PEM from %s"%options.pem


    if len(args) >= 1:
        if options.port is None:
            options.port = int(args[0])
        if len(args) >= 2:
            if options.root is None:
                options.root = args[1]

    if len(args) >= 4:
        if options.user is None:
            options.user = args[2]
        if options.password is None:
            options.password = args[3]

        if len(args) == 4:
            options.basic = True
        elif len(args) == 5:
            ## Extra argument for digest auth
            options.digest = True

    assert options.port is not None
    assert options.root is not None

    return options

def parseOptions(argv):
    parser = optparse.OptionParser(usage="%prog -p <port>",version="%prog 1.1")
    addOptions(parser)
    return getOptionsFromArgs(parser,argv[1:])

class cidServer(http.server.HTTPServer):
    allow_reuse_address = True

    def server_bind(self):
        """Override server_bind to _NOT_ store the server name.

        socket.getsockname() is broken on HP-UX ia64 with 64-bit of Python (returns
        arguments in reversed order and the IP address is wrong). I don't see that
        they are used anywhere excpept in classes derived from HTTPServer one so it
        should be safe to comment that bit out.

        """
        socketserver.TCPServer.server_bind(self)
        #host, port = self.socket.getsockname()[:2]
        #self.server_name = socket.getfqdn(host)
        #self.server_port = port

GOOD_CIPHERS = ":".join(
    (
        "ECDHE-RSA-AES128-GCM-SHA256",   ## TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256
        "ECDHE-RSA-AES256-GCM-SHA384",   ## TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384
        "ECDHE-ECDSA-AES128-GCM-SHA256", ## TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256
        "ECDHE-ECDSA-AES256-GCM-SHA384", ## TLS_ECDHE_ECDSA_WITH_AES_256_GCM_SHA384
        "DHE-RSA-AES128-GCM-SHA256",     ## TLS_DHE_RSA_WITH_AES_128_GCM_SHA256
        "DHE-DSS-AES128-GCM-SHA256",     ## TLS_DHE_DSS_WITH_AES_128_GCM_SHA256
        "DHE-DSS-AES256-GCM-SHA384"      ## TLS_DHE_DSS_WITH_AES_256_GCM_SHA384
    ))

def main(argv):
    global loggingOn

    ppid = os.getppid()
    assert ppid != 1

    options = parseOptions(argv)
    print("Webserver running on %d, with root %s"%(options.port,options.root), file=sys.stderr)
    print("args:",str(argv[1:]), file=sys.stderr)

    if options.loggingOn:
        loggingOn = True

    if options.digest:
        pdb = PasswordDatabase(options.user,options.password)
        auth = DigestAuthentication(pdb)
        PasswordHTTPRequestHandler.authHandler = auth
        print("\tDigest authentication user=%s, pass=%s"%(options.user,options.password), file=sys.stderr)
    elif options.basic:
        pdb = PasswordDatabase(options.user,options.password)
        auth = BasicAuthentication(pdb)
        PasswordHTTPRequestHandler.authHandler = auth
        print("\tBasic authentication user=%s, pass=%s"%(options.user,options.password), file=sys.stderr)

    basedir = os.getcwd()
    if options.pem is not None and basedir is not None:
        options.pem = os.path.join(basedir,options.pem)

    os.chdir(options.root)
    PasswordHTTPRequestHandler.latency = options.latency
    PostRequestHandler.latency = options.latency

    if options.slow:
        if options.rate is not None:
            SlowHandler.RATE = options.rate
            print("Setting rate to %d B/s"%options.rate, file=sys.stderr)
        handler_class = SlowHandler
        print("Serving files slowly", file=sys.stderr)
    elif options.hang:
        handler_class = HangingHandler
        print("Hanging while serving first file", file=sys.stderr)
    elif options.infinite:
        handler_class = InfiniteHandler
        print("Infinite zeros for all files other than .inf .upd .dat", file=sys.stderr)
    elif options.post:
        handler_class = PostRequestHandler
    else:
        handler_class = PasswordHTTPRequestHandler

    server_class = cidServer
    ## [FM3362] - server on all devices for explicit non-localhost test
    server_address = ('',options.port)
    httpd = server_class(server_address, handler_class)

    if options.secure:
        import ssl
        if options.tls1_0:
            protocol = ssl.PROTOCOL_TLSv1
        elif options.tls1_1:
            protocol = ssl.PROTOCOL_TLSv1_1
        elif options.tls1_2:
            protocol = ssl.PROTOCOL_TLSv1_2
        else:
            protocol = ssl.PROTOCOL_SSLv23

        if options.bad_ciphers:
            ciphers = "RC4-SHA:AES256-SHA256:AES128-CCM:ECDHE-ECDSA-AES128-SHA256"
        elif options.good_ciphers:
            ciphers = GOOD_CIPHERS
        elif options.all_ciphers:
            ciphers = "ALL" ## not eNULL...
        else:
            ciphers = None

        httpd.socket = ssl.wrap_socket(
            httpd.socket,
            certfile=options.pem,
            server_side=True,
            ssl_version=protocol,
            ciphers=ciphers)

    # don't do this, it breaks all of our UNIX systems
    #httpd.socket.settimeout(1)

    ## Need some way of terminating???
    try:
        while os.getppid() == ppid:
            httpd.handle_request()
        print("Parent process went away, quitting!")
    except OSError as e:
        print("Serving failed with OSError:",e)
        print(os.getcwd(), file=sys.stderr)
        raise
    except Exception as e:
        print(("Failed with exception",e))
        print(os.getcwd(), file=sys.stderr)
        raise

    return 0


if __name__=="__main__":
    sys.exit(main(sys.argv))
