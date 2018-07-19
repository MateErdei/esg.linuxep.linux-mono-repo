#!/usr/bin/env python

from __future__ import absolute_import, print_function, division, unicode_literals

import os
import socket
import sys

try:
    import xmlrpc.client as xmlrpc_client
except ImportError:
    import xmlrpclib as xmlrpc_client


import FileInfo


class SigningOracleClientSigner(object):
    """
    Communicate to an XML-RPC signing Oracle
    """
    def __init__(self, options, verbose=False):
        self.m_options = options
        assert(self.m_options.signing_oracle is not None)
        self.m_server = xmlrpc_client.ServerProxy(self.m_options.signing_oracle, verbose=verbose, allow_none=1)

    def encoded_signature_for_file(self, filepath):
        data = []
        with open(filepath) as f:
            data = f.read()
        return self.__encoded_signature_for_data(data)

    def __encoded_signature_for_data(self, data):
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

    def test_signing(self):
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
        with open(p, "rb") as f:
            c = f.read()
            return c.split("-----BEGIN SIGNATURE-----")[0]
    except EnvironmentError:
        return None


def generate_manifest(dist, file_objects):
    options = Options()
    manifest_path = os.path.join(dist, b"manifest.dat")

    previous_contents = read(manifest_path)
    new_contents = []

    for f in file_objects:
        display_path = b".\\" + f.m_path.replace(b"/", b"\\")
        new_contents.append(b'"%s" %d %s\n' % (display_path, f.m_length, f.m_sha1))
        new_contents.append(b'#sha256 %s\n' % f.m_sha256)
        new_contents.append(b'#sha384 %s\n' % f.m_sha384)

    new_contents = "".join(new_contents)
    if new_contents == previous_contents:
        return False

    with open(manifest_path, "wb") as output:
        output.write(new_contents)

    signer = SigningOracleClientSigner(options, verbose=True)
    try:
        signer.test_signing()
    except socket.gaierror:
        print("Failed to contact buildsign-m - trying buildsign-m.eng.sophos")
        options.signing_oracle = "http://buildsign-m.eng.sophos:8000"
        signer = SigningOracleClientSigner(options, verbose=True)
        signer.test_signing()

    sig = signer.encoded_signature_for_file(manifest_path)

    with open(manifest_path, "ab") as output:
        output.write(sig)
        output.write(signer.public_cert())
        output.write(signer.ca_cert())

    return True


def main(argv):
    if len(argv) < 2:
        print("Error: no output directory specified.")
        return 1

    dist = argv[1]
    file_objects = FileInfo.load_file_info(dist)
    generate_manifest(dist, file_objects)
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
