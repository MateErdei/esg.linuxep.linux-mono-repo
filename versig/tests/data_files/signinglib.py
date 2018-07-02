
from __future__ import print_function,division,unicode_literals

import os
import sys
import tempfile
import textwrap
import base64

import Build
from Process import *


class SigningException(Build.BuildException):
    pass


def getSigningDirectory(options):
    keydir = options.signingdir
    if keydir is not None:
        if not os.path.isdir(keydir):
            raise SigningException("Key directory %s isn't a directory"%keydir)
        return keydir
    keydir = os.environ.get("SIGNING_KEY_DIR","/signing")
    if os.path.isdir(keydir):
        return keydir
    raise SigningException("Can't find signing directory: %s"%keydir)


def findKey(options):
    if options.key is not None:
        ## Explicitly specified
        return

    options.key = os.environ.get("SIGNING_KEY",None)
    if options.key is not None:
        ## Specified in environment
        return

    signingdir = getSigningDirectory(options)
    keydir = os.path.join(signingdir,"userkeys")

    if not os.path.isdir(keydir):
        print(keydir,"doesn't exist! Falling back to old signing directory",file=sys.stderr)
        keydir = signingdir

    contents = os.listdir(keydir)
    if len(contents) == 0:
        raise SigningException("Key directory %s is empty"%keydir)

    keys = [ x for x in contents if x.endswith(".pem") ]
    if len(keys) == 0:
        raise Build.BuildException("No .pem files in %s"%keydir)
    elif len(keys) == 1:
        ## Using the only key in the directory
        options.key = os.path.join(keydir,keys[0])
        return

    raise SigningException("Multiple keys (.pem files) in %s"%keydir)

def checkKey(options):
    if not os.path.isfile(options.key):
        raise SigningException("Key %s doesn't exist"%options.key)
    print("Signing with",options.key)

def findCertificate(options, previousValue, envoption, subdir):
    if previousValue is not None:
        return previousValue

    certificate = os.environ.get(envoption,None)
    if certificate is not None:
        return certificate

    keydir = os.path.join(getSigningDirectory(options),subdir)

    contents = os.listdir(keydir)
    if len(contents) == 0:
        raise SigningException("Key directory %s is empty"%keydir)

    certificates = [ x for x in contents if x.endswith(".crt") ]
    if len(certificates) == 0:
        raise Build.BuildException("No .crt files in %s"%keydir)
    if len(certificates) == 1:
        ## Using the only key in the directory
        return os.path.join(keydir,certificates[0])

    raise SigningException("Multiple certificates (.crt files) in %s"%keydir)

def findFirstCertificate(options):
    options.firstcertificate = findCertificate(options,options.firstcertificate,"SIGNING_CERTIFICATE","usercerts")

def findSecondCertificate(options):
    options.secondcertificate = findCertificate(options,options.secondcertificate,"INTERMEDIATE_CERTIFICATE","cacerts")

def checkFirstCertificate(options):
    if not os.path.isfile(options.firstcertificate):
        raise SigningException("Certificate %s doesn't exist"%options.firstcertificate)
    print("Signing certificate is",options.firstcertificate)

def checkSecondCertificate(options):
    if not os.path.isfile(options.secondcertificate):
        raise SigningException("Certificate %s doesn't exist"%options.secondcertificate)
    print("Intermediate certificate is",options.secondcertificate)

def addLibrary(path):
    existing = os.environ.get("LD_LIBRARY_PATH",None)
    if existing is None:
        os.environ["LD_LIBRARY_PATH"] = path
        return
    if path in existing.split(":"):
        return
    os.environ["LD_LIBRARY_PATH"] = existing+":"+path

def getBinariesDir():
    binariesdir = os.environ.get("BINARIES_DIR",None)
    if binariesdir is not None:
        return binariesdir
    redist = os.environ.get("REDIST",None)
    if redist is not None:
        return os.path.join(redist,"binaries")

    return None


def findOpenssl(options):
    if options.openssl is not None:
        return

    options.openssl = os.environ.get("OPENSSL",None)
    if options.openssl is not None:
        return

    binariesdir = getBinariesDir()
    if binariesdir is not None:
        openssl = os.path.join(binariesdir,options.platform,"ACETAO",options.architecture,"bin","openssl")
        print("Trying binariesdir openssl:",openssl)
        if os.path.isfile(openssl):
            ## Also need to change LD_LIBRARY_PATH
            addLibrary(os.path.join(binariesdir,options.platform,"ACETAO",options.architecture,"lib"))
            options.openssl = openssl
            return
    else:
       print("BINARIES_DIR not available")

    paths = os.environ['PATH'].split(':')
    for p in paths:
        temp = os.path.join(p,"openssl")
        if os.path.isfile(temp):
            options.openssl = temp
            return

    raise SigningException("Unable to find openssl")

def checkOpenssl(options):
    if options.openssl is None:
        raise SigningException("Can't find openssl")
    if not os.path.isfile(options.openssl):
        raise SigningException("%s isn't a file"%options.openssl)
    print("Using openssl:",options.openssl)
    print("Using LD_LIBRARY_PATH:",os.environ.get('LD_LIBRARY_PATH',"None"))

def checkSigningOptions(options):
    findKey(options)
    checkKey(options)

    findFirstCertificate(options)
    checkFirstCertificate(options)

    findSecondCertificate(options)
    checkSecondCertificate(options)

    if options.password is None:
        options.password = os.environ.get("SIGNING_PASSWORD",None)

    findOpenssl(options)
    checkOpenssl(options)


class SignerBase(object):
    MAX_ATTEMPTS = 10

    def __init__(self, options):
        self.m_options = options
        checkSigningOptions(options)


    def getPassword(self):
        if self.m_options.password is None:
            print("Enter password:")
            import getpass
            self.m_options.password = getpass.getpass("> ")
        return self.m_options.password

    def getKeyPath(self):
        return self.m_options.key

    def rawSignatureForFile(self, pathname):
        ## run openssl
        attempts = 0
        retCode = -1
        signature = ""
        while attempts < SignerBase.MAX_ATTEMPTS:
            attempts += 1

            password = self.getPassword()
            if password == "":
                ## Use environment - doesn't matter as the password is empty string...
                os.environ['OPENSSL_PASSWORD'] = ""
                passin = "env:OPENSSL_PASSWORD"
                readfd = -1
            else:
                (readfd, writefd) = os.pipe()
                os.write(writefd,password) ## rely on buffering
                os.close(writefd)
                passin = "fd:%d"%readfd

            (retCode, signature) = collectOutput(
                [self.m_options.openssl,"sha1","-passin",passin,"-sign",self.getKeyPath(),pathname],
                "")
            if readfd != -1:
                os.close(readfd)

            if retCode == 0 and signature != "":
                ## All good
                return signature

            ## Reset the password - maybe they got it wrong?
            self.m_options.password = None

        if retCode != 0:
            print("Failed to run openssl: %d"%retCode)
            raise SigningException("Failed to sign file %d times: %d"%(attempts,retCode))

        ## If retCode == 0, then signature must be ""
        assert signature == ""

        print("No signature generated")
        raise SigningException("Failed to sign file %d times: No signature generated"%(attempts))

    def __encodeSignature(self, raw_sig):
        lines = ["-----BEGIN SIGNATURE-----"]
        lines += textwrap.wrap(base64.b64encode(raw_sig), width=64)
        lines += ["-----END SIGNATURE-----\n"]
        return "\n".join(lines)

    def encodedSignatureForFile(self, filepath):
        raw_sig = self.rawSignatureForFile(filepath)
        return self.__encodeSignature(raw_sig)

    def encodedSignatureForData(self, data):
        ## write out the data.
        (tempfd,temppath) = tempfile.mkstemp(".ddta","signinglib")
        os.write(tempfd,data)
        os.close(tempfd)

        try:
            return self.encodedSignatureForFile(temppath)
        finally:
            os.unlink(temppath)

    def public_cert(self):
        return open(self.m_options.firstcertificate).read()

    def ca_cert(self):
        return open(self.m_options.secondcertificate).read()

    def signatureAndCertificatesForData(self, data):
        """
        Generate the entire trailing block -
            a base-64-encoded signature + markers, plus certificates
        """
        ## get signature etc
        result = self.encodedSignatureForData(data)
        result += self.public_cert()
        result += self.ca_cert()

        return result

    def testSigning(self):
        """
        Test that we can sign with the configured keys
        """
        result = self.encodedSignatureForData("Sign this")
        assert result != ""
        return result
