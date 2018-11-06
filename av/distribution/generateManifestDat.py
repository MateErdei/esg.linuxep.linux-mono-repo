#!/usr/bin/env python

from __future__ import absolute_import, print_function, division, unicode_literals

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

def read(p):
    try:
        c = open(p,"rb").read()
        return c.split("-----BEGIN SIGNATURE-----")[0]
    except EnvironmentError:
        return None

def generate_manifest(dist, file_objects):
    options = Options()
    manifest_path = os.path.join(dist, b"manifest.dat")

    previousContents = read(manifest_path)
    newContents = []

    for f in file_objects:
        display_path = b".\\" + f.m_path.replace(b"/", b"\\")
        newContents.append(b'"%s" %d %s\n' % (display_path, f.m_length, f.m_sha1))
        newContents.append(b'#sha256 %s\n' % f.m_sha256)
        newContents.append(b'#sha384 %s\n' % f.m_sha384)

    newContents = "".join(newContents)
    if newContents == previousContents:
        return False

    output = open(manifest_path, "wb")
    output.write(newContents)
    output.close()

    signer = SigningOracleClientSigner(options, verbose=True)
    try:
        signer.testSigning()
    except socket.gaierror:
        print("Failed to contact buildsign-m - trying buildsign-m.eng.sophos")
        options.signing_oracle = "http://buildsign-m.eng.sophos:8000"
        signer = SigningOracleClientSigner(options, verbose=True)
        signer.testSigning()

    sig = signer.encodedSignatureForFile(manifest_path)

    output = open(manifest_path, "ab")
    output.write(sig)
    output.write(signer.public_cert())
    output.write(signer.ca_cert())
    output.close()
    return True