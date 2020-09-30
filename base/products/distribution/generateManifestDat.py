#!/usr/bin/env python
# Copyright (C) 2019 Sophos Plc, Oxford, England.
# All rights reserved.


import os
import socket
import sys
import subprocess

import xmlrpc.client as xmlrpc_client
import fileInfo


class SigningOracleClientSigner(object):
    """
    Communicate to an XML-RPC signing Oracle
    """
    def __init__(self, options, verbose=False):
        self.m_options = options
        assert(self.m_options.signing_oracle is not None)
        self.m_server = xmlrpc_client.ServerProxy(self.m_options.signing_oracle, verbose=verbose, allow_none=True)

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
        c = open(p, "rb").read()
        return c.split(b"-----BEGIN SIGNATURE-----")[0]
    except EnvironmentError:
        return None


unicode_str = str
byte_str = bytes


def ensure_bytes(s):
    if isinstance(s, unicode_str):
        return s.encode("UTF-8")
    return s


def generate_manifest_old_api(dist, file_objects):
    options = Options()
    MANIFEST_NAME = os.environ.get("MANIFEST_NAME", "manifest.dat")
    manifest_path = os.path.join(dist, MANIFEST_NAME)

    if file_objects is None:
        file_objects = fileInfo.load_file_info(dist, None)

    previousContents = read(manifest_path)
    newContents = []

    for f in file_objects:
        display_path = ".\\" + f.m_path
        newContents.append('"%s" %d %s\n' % (display_path, f.m_length, f.m_sha1))
        newContents.append('#sha256 %s\n' % f.m_sha256)
        newContents.append('#sha384 %s\n' % f.m_sha384)

    if newContents == previousContents:
        return False

    output = open(manifest_path, "w")
    output.write("".join(newContents))
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
    sig = sig

    output = open(manifest_path, "a")
    output.write(sig)
    output.write(signer.public_cert())
    output.write(signer.ca_cert())
    output.close()
    return True


def generate_manifest_new_api(dist, file_objects=None):
    MANIFEST_NAME = os.environ.get("MANIFEST_NAME", "manifest.dat")
    manifest_path = os.path.join(dist, MANIFEST_NAME)
    exclusions = 'SDDS-Import.xml,'+MANIFEST_NAME  # comma separated string
    env = os.environ.copy()
    env['LD_LIBRARY_PATH'] = "/usr/lib:/usr/lib64"
    env['OPENSSL_PATH'] = "/usr/bin/openssl"
    previous_contents = read(manifest_path)
    result = subprocess.run(
        ['sb_manifest_sign', '--folder', dist, '--output', manifest_path, '--exclusions', exclusions]
        , stderr=subprocess.STDOUT, stdout=subprocess.PIPE, env=env,
        timeout=30
    )
    result.check_returncode()
    new_contents = read(manifest_path)
    return previous_contents != new_contents


def generate_manifest(dist, file_objects=None):
    try:
        return generate_manifest_new_api(dist, file_objects)
    except subprocess.CalledProcessError as ex:
        print("Unable to generate manifest.dat file with new-api: ", ex.returncode, str(ex))
        print("Output:", ex.output.decode("UTF-8", errors='replace'))
    return generate_manifest_old_api(dist, file_objects)


def main(argv):
    dist = argv[1]
    if len(argv) > 2:
        distribution_list = argv[2]
    else:
        distribution_list = None

    file_objects = fileInfo.load_file_info(dist, distribution_list)
    generate_manifest(dist, file_objects)

    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))