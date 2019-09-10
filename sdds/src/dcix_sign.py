#!/bin/python
#
#  DCIX signing script, adapted from manifest_sign.py
#

__version__ = "1.0.0"

from xmlrpclib import ServerProxy
import sys
import socket
from optparse import OptionParser

if __name__ == "__main__":
    parser = OptionParser(usage="%prog [options] file", version=__version__)
    parser.add_option("-s", "--server", default="http://buildsign-m:8000",
                      dest="server",  metavar="ADDRESS",
                      help="Use signing server at ADDRESS [default %default]")
    parser.add_option("-c", "--certs", default=None,
                      help="use TYPE certs [default %default]", metavar="TYPE")
    (options, args) = parser.parse_args()
    if not args:
        parser.error("You must provide a filename to sign")
    server = ServerProxy(options.server, allow_none=True)
    m = open(args[0], "rb")
    file_data = m.read()
    m.close()
    file = open(args[0], "a+b")
    try:
        file.write(server.sign_file(file_data, args[0], options.certs))
    except socket.error as e:
        print "ERROR - Signing Failed: %s" % e
        sys.exit(1)
    for cert in ("pub", "ca"):
        try:
            file.write(server.get_cert(cert, options.certs))
        except socket.error as e:
            print "ERROR - Failed to get cert %s: %s" % (cert, e)
            sys.exit(1)
    file.close()
    print "Manifest succesfully generated."
    sys.exit(0)
