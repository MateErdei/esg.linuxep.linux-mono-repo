#!/usr/bin/env python

from __future__ import absolute_import,print_function,division,unicode_literals

import os
import socket

try:
    import xmlrpc.client as xmlrpc_client
except ImportError:
    import xmlrpclib as xmlrpc_client


class SigningOracleClientSigner(object):
    """
    Communicate to an XML-RPC signing Oracle
    """
    def __init__(self, options, verbose=False):
        self.m_options = options
        assert(self.m_options.signing_oracle is not None)
        self.m_server = xmlrpc_client.ServerProxy(self.m_options.signing_oracle, verbose=verbose, allow_none=1)

    def encodedSignatureForFile(self, filepath):
        data = open(filepath).read()
        return self.__encodedSignatureForData(data)

    def __encodedSignatureForData(self, data):
        result = self.m_server.sign_file(data, "", None)
        if isinstance(result,list):
            result = "\n".join(result)+"\n"
        return result

    def public_cert(self):
        result = self.m_server.get_cert("pub", None)
        if isinstance(result,list):
            result = "\n".join(result)+"\n"
        return result

    def ca_cert(self):
        result = self.m_server.get_cert("ca", None)
        if isinstance(result,list):
            result = "\n".join(result)+"\n"
        return result

    def testSigning(self):
        result = self.m_server.sign_file("Sign this", "", None)
        if isinstance(result,list):
            result = "\n".join(result)+"\n"
        self.public_cert()
        self.ca_cert()
        return result

class Options(object):
    def __init__(self):
        self.signing_oracle = os.environ.get("SIGNING_ORACLE", "http://buildsign-m:8000")

def generate_manifest(dist, file_objects):
    options = Options()
    signer = SigningOracleClientSigner(options, verbose=True)
    try:
        signer.testSigning()
    except socket.gaierror:
        print("Failed to contact buildsign-m - trying buildsign-m.eng.sophos")
        options.signing_oracle = "http://buildsign-m.eng.sophos:8000"
        signer = SigningOracleClientSigner(options, verbose=True)
        signer.testSigning()

    manifest_path = os.path.join(dist, b"manifest.dat")
    output = open(manifest_path, "wb")
    for f in file_objects:
        display_path = b".\\" + f.m_path.replace(b"/", b"\\")
        output.write(b'"%s" %d %s\n' % (display_path, f.m_length, f.m_sha1))
        output.write(b'#sha256 %s\n' % f.m_sha256)
        output.write(b'#sha384 %s\n' % f.m_sha384)

    output.close()

    sig = signer.encodedSignatureForFile(manifest_path)

    output = open(manifest_path, "ab")
    output.write(sig)
    output.write(signer.public_cert())
    output.write(signer.ca_cert())
    output.close()
