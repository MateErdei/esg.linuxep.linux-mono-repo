#!/usr/bin/env python3

import base64
import os
import signal
import subprocess
import textwrap


def subprocess_setup():
    # Python installs a SIGPIPE handler by default. This is usually not what
    # non-Python subprocesses expect.
    signal.signal(signal.SIGPIPE, signal.SIG_DFL)


def run(command):
    retCode = subprocess.call(command, preexec_fn=subprocess_setup)
    if retCode == None: retCode = 0
    return retCode


def runSilentOutput(command):
    null = open("/dev/null", "w")
    retCode = subprocess.call(command, stdout=null, preexec_fn=subprocess_setup)
    null.close()
    if retCode == None: retCode = 0
    return retCode


def collectOutput(command, stdin):
    proc = subprocess.Popen(command, stdout=subprocess.PIPE, stdin=subprocess.PIPE, preexec_fn=subprocess_setup)
    output = proc.communicate(stdin)[0]
    retCode = proc.wait()
    if retCode == None: retCode = 0
    return (retCode, output)


class Options(object):
    pass


class SigningException(Exception):
    pass


class Signer(object):
    def __init__(self):
        self.m_options = Options()
        # self.m_options.openssl = os.environ['TEST_ROOT'] + '/bin/openssl'
        self.m_options.openssl = os.environ['TEST_OPENSSL']
        os.environ['LD_LIBRARY_PATH'] = os.environ['OPENSSL_LD_LIBRARY_PATH']

    def getPassword(self):
        return ""

    def getKeyPath(self):
        return self.m_options.key

    def rawSignatureForFile(self, pathname):
        ## run openssl
        attempts = 0
        retCode = -1
        signature = ""
        MAX_ATTEMPTS = 3
        while attempts < MAX_ATTEMPTS:
            attempts += 1

            password = self.getPassword()
            if password == "":
                ## Use environment - doesn't matter as the password is empty string...
                os.environ['OPENSSL_PASSWORD'] = ""
                passin = "env:OPENSSL_PASSWORD"
                readfd = -1
            else:
                (readfd, writefd) = os.pipe()
                os.write(writefd, password)  ## rely on buffering
                os.close(writefd)
                passin = f"fd:{readfd}"

            (retCode, signature) = collectOutput(
                [self.m_options.openssl, "sha1", "-passin", passin, "-sign", self.getKeyPath(), pathname],
                "")
            if readfd != -1:
                os.close(readfd)

            if retCode == 0 and signature != "":
                ## All good
                return signature

            ## Reset the password - maybe they got it wrong?
            self.m_options.password = None

        if retCode != 0:
            print(f"Failed to run openssl: {retCode}")
            raise SigningException(f"Failed to sign file {attempts} times: {retCode}")

        ## If retCode == 0, then signature must be ""
        assert signature == ""

        print("No signature generated")
        raise SigningException(f"Failed to sign file {attempts} times: No signature generated")

    def __encodeSignature(self, raw_sig):
        lines = ["-----BEGIN SIGNATURE-----"]
        lines += textwrap.wrap(base64.b64encode(raw_sig).decode(), width=64)
        lines += ["-----END SIGNATURE-----\n"]
        return "\n".join(lines)

    def encodedSignatureForFile(self, filepath):
        raw_sig = self.rawSignatureForFile(filepath)
        return self.__encodeSignature(raw_sig)
