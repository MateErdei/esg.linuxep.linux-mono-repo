#!/usr/bin/env python
import os
import time
import Signer

try:
    import hashlib
    sha1new = hashlib.sha1
    md5new = hashlib.md5
except ImportError:
    import sha
    sha1new = sha.new
    import md5
    md5new = md5.new


EXCLUDES = ['cidsync.upd','root.upd','master.upd','vdlsync.upd',
        'manifest.dat','root_manifest.dat','vdlmnfst.dat','stamp',
        'mrinit.conf','MRInit.conf','mrinit.custom','MRInit.custom','cac.pem',
        'talpa.dat','sav.dat','enginemnfst.dat',
        'customer_ID.txt','customer_id.txt']

## Additional files that should be excluded from manifest.dat files
MANIFEST_EXCLUDES = []

def safeDelete(path):
    try:
        os.unlink(path)
    except EnvironmentError:
        pass

class GenerateManifests(object):
    def __init__(self):
        self.__m_sha1cache = {}
        self.__m_sizecache = {}
        self.__m_PLATFORM = os.environ.get('PLATFORM','linux')

    def getfiles(self, basedir, excludes=EXCLUDES):
        filelist = []
        baselen = len(basedir)+1
        for (dir,dirs,files) in os.walk(basedir):
            filelist += [ os.path.join(dir,file)[baselen:] for file in files if file not in excludes ]
        return filelist

    def getSize(self, basedirectory, path):
        return self.__m_sizecache[os.path.join(basedirectory,path)]

    def getSha1(self, basedirectory, path):
        fullPath = os.path.join(basedirectory,path)
        cache = self.__m_sha1cache.get(fullPath,None)
        if cache is not None:
            return cache

        filesize = 0
        f = open(fullPath)
        s = sha1new()
        while True:
            data = f.read(128*1024)
            if len(data) == 0:
                break
            filesize += len(data)
            s.update(data)
        f.close()

        self.__m_sha1cache[fullPath] = s.hexdigest()
        self.__m_sizecache[fullPath] = filesize

        return self.__m_sha1cache[fullPath]

    def getMd5(self, basedirectory, path):
        fullPath = os.path.join(basedirectory,path)
        filesize = 0
        f = open(fullPath)
        s = md5new()
        while True:
            data = f.read(1024)
            if len(data) == 0:
                break
            filesize += len(data)
            s.update(data)
        f.close()

        self.__m_sizecache[fullPath] = filesize
        return s.hexdigest()

    def generateManifest(self, basedirectory, filelist, manifestname="manifest.dat"):
        if not os.path.isdir(basedirectory):
            return

        if len(filelist) == 0:
            return

        print " - getting file list"
        fullDest = os.path.join(basedirectory,manifestname)

        print " - generating file entries"
        safeDelete(fullDest)
        d = open(fullDest,"w")
        l = len(filelist)
        last = time.time()
        for n, f in enumerate(filelist):
            if time.time() > last + 30:
                print "%d%%"%(n*100/l)
                sys.stdout.flush()
                last = time.time()
            # Don't recursively include manifests in themselves.
            if f == manifestname:
                continue

            sum = self.getSha1(basedirectory,f)
            size = self.getSize(basedirectory,f)
            f = ".\\"+f.replace("/","\\")
            d.write("\"%s\" %d %s\n"%(f,size,sum))
        d.close()

        print " - generating signature"
        ## Get signature
        signature = self.__m_signer.encodedSignatureForFile(fullDest)

        d = open(fullDest,"a")
        d.write(signature)

        ## And put the key certificates in the manifest as well
        d.write(self.__m_certs)

        d.close()
        print " - done"

    def generateManifestDatFiles(self, basedir, filelist):
        self.generateManifest(basedir, filelist, "manifest.dat")

    def generateManifests(self, argv):
        self.__m_signer = Signer.Signer()
        before = time.time()
        cwd = os.getcwd()
        basedir = os.path.join(argv[1], argv[5])
        self.__m_signer.m_options.key = os.path.join(cwd, argv[2])
        self.__m_signer.password = argv[3]
        self.__m_certs = open( os.path.join(cwd, argv[4])).read()

        versionFile = os.path.join(basedir, "savShortVersion")
        if not os.path.isfile(versionFile):
            versionFile = os.path.join(basedir, "version")
        if os.path.isfile(versionFile):
            version = open(versionFile).read().strip()
            majorVersion = int(version.split(".")[0])
        else:
            majorVersion = 99

        filelist = self.getfiles(basedir, excludes=EXCLUDES+MANIFEST_EXCLUDES)

        if "suiteVersion" in filelist:
            filelist.remove("suiteVersion")
            try:
                filelist.remove("version")
            except ValueError:
                pass

        filelist.sort()

        self.generateManifestDatFiles(basedir, filelist)

        taken = time.time() - before
        print "  taken", taken



def main(argv):
    c = GenerateManifests()
    return c.generateManifests(argv)

if __name__ == "__main__":
    import sys
    sys.exit(main(sys.argv))


