##Python library

from __future__ import print_function,division,unicode_literals

import hashlib
import os
import stat

try:
    import xmlrpc.client as xmlrpclib
except ImportError:
    import xmlrpclib

import Util

import signinglib

class SigningException(signinglib.SigningException):
    pass

class DirectSigner(signinglib.SignerBase):
    """
    Perform signing directly - all performed by signinglib.SignerBase
    """
    pass

class SigningOracleClientSigner(object):
    """
    Communicate to an XML-RPC signing Oracle
    """
    def __init__(self, options, verbose=False):
        self.m_options = options
        assert(self.m_options.signing_oracle is not None)
        self.m_server = xmlrpclib.ServerProxy(self.m_options.signing_oracle, verbose=verbose, allow_none=1)

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

class FileInfo(object):
    def __init__(self, basedirectory, path):
        fullPath = os.path.join(basedirectory,path)
        f = open(fullPath)
        s = hashlib.sha1()
        sha256 = hashlib.sha256()
        length = 0
        while True:
            data = f.read(10240)
            if len(data) == 0:
                break
            s.update(data)
            sha256.update(data)
            length += len(data)
        f.close()
        self.m_sha1 = s.hexdigest()
        self.m_sha256 = s.hexdigest()
        self.m_size = length


class SignerBase(object):
    """
    Class for signing packages.

    Either uses remote or local signing
    """
    def __init__(self, options):
        self.m_options = options
        self.__m_tempDir = None

        if self.m_options.sign:
            ## Either direct signing or oracle remote signing
            if self.m_options.signing_oracle is not None:
                self.m_signer = SigningOracleClientSigner(self.m_options)
            else:
                self.m_signer = DirectSigner(self.m_options)
            self.m_signer.testSigning()
        else:
            ## Don't sign stuff
            self.m_signer = None

    def getSize(self, basedirectory, path):
        return os.stat(os.path.join(basedirectory,path))[stat.ST_SIZE]

    def getSha1(self, basedirectory, path):
        fullPath = os.path.join(basedirectory,path)
        f = open(fullPath)
        s = hashlib.sha1()
        while True:
            data = f.read(1024)
            if len(data) == 0:
                break
            s.update(data)
        f.close()
        return s.hexdigest()

    def signPackage(self, basedirectory, contents, destination,
                    inclusionStem="",
                    excludeManifests=True,
                    excludes=[],
                    contentsPrefix="sophos-av/",
                    includeSHA256=True):
        """
        @param basedirectory - The directory that contents and destination are relative to
        @param excludes - files to excludes from the manifest, above and beyond excluding any manifests
        @param contentsPrefix any extra prefix on contents, that should be removes from the entries in the manifest.
        """
        if not self.m_options.sign:
            return False

        ## Get the list of files first
        filelist = []
        for f in contents:
            if not f.startswith(inclusionStem):
                continue
            if excludeManifests and f.endswith(b"manifest.dat"):
                continue
            if f in excludes:
                continue
            assert f.startswith(contentsPrefix)
            filelist.append(f)

        if len(filelist) == 0:
            return False

        filelist.sort()

        fullDest = os.path.join(basedirectory,destination)
        d = open(fullDest, "w")
        for f in filelist:
            path = os.path.join(basedirectory,f)
            f = f[len(contentsPrefix):]
            f = b".\\"+f.replace(b"/", b"\\")
            info = FileInfo(basedirectory,path)

            d.write(b"\"%s\" %d %s\n"%(f, info.m_size, info.m_sha1))
            if includeSHA256:
                d.write(b"#sha256 %s\n"%info.m_sha256)

        d.close()
        ## Get signature
        signature = self.m_signer.encodedSignatureForFile(fullDest)

        d = open(fullDest,"a")
        d.write(signature)

        ## And put the key certificates in the manifest as well
        d.write(self.m_signer.public_cert())
        d.write(self.m_signer.ca_cert())

        d.close()

        ## Need to include manifest in individual tar
        if destination not in contents:
            contents.append(destination)
        return True

    def signSubpackage(self, basedirectory, filelist, subpackage):
        if len(filelist) == 0:
            return False

        if not self.m_options.sign:
            return False

        ## Deliberately don't add these manifests to the subpackages
        return self.signPackage(basedirectory, filelist,"sophos-av/%s/manifest.dat"%subpackage,"sophos-av/%s"%subpackage)

    def copyXmlFile(self, destinationdirectory, filename):
        src = os.path.join(self.m_options.destdir,filename) ## Relies on os.path.join handling late absolute paths
                ## e.g. os.path.join("/tmp","/etc") -> "/etc"
        dest = os.path.join(destinationdirectory,os.path.basename(filename))
        print("Copy",src,"to",dest)
        Util.copy(src,dest)

    def copyXmlFiles(self, destinationdirectory):
        self.copyXmlFile(destinationdirectory, self.m_options.sdf) ## default "sdf.xml"
        #~ self.copyXmlFile(destinationdirectory,  self.m_options.svf) ## default "svf.xml"

    def generatePackage(self, srcdirectory, destdirectory, filelist):
        path = os.path.join(self.m_options.destdir, destdirectory)
        Util.setupDirectory(path)
        ## Now copy files to path
        for f in filelist:
            src = os.path.join(srcdirectory,f)
            if f.startswith("sophos-av/"):
                f = f[len("sophos-av/"):]
            dest = os.path.join(path,f)
            Util.copy(src,dest)

        ## Copy XML files
        self.copyXmlFiles(path)

        return path

    def generateRootManifest(self,basedir, completeList,excludeFiles=[]):
        ## Get list of root files
        ## Create manifest
        filelist = []
        remainderStart = len("sophos-av/")
        for f in completeList:
            ## only include f if it is at the top of sophos-av
            assert f.startswith("sophos-av/")
            if f in excludeFiles:
                continue
            if not "/" in f[remainderStart:]:
                filelist.append(f)

        return self.signPackage(basedir,filelist,"sophos-av/root_manifest.dat")

    def generateManifests(self, basedir, filelist):
        ## Generate the root_manifest.dat as this also needs the original install.sh
        self.generateRootManifest(basedir,filelist)
        self.signSubpackage(basedir,filelist,"savi")
        self.signSubpackage(basedir,filelist,"talpa")
        self.signSubpackage(basedir,filelist,"sav-%s"%self.m_options.platform)
