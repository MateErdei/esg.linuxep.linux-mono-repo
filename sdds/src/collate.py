import os
import sys
import exceptions
import subprocess
from shutil import copy2, rmtree
from optparse import OptionParser
import xml.etree.ElementTree as ET
from xml.parsers.expat import ExpatError
from glob import glob
import re
import stat
import string
from datetime import datetime
import tempfile

usage = "%prog [options] collationfile"

#indirection function overridable in unittest


def callManifestSign(folder):
    subprocess.check_call(['python.exe',
                           '..\\signing\\manifest\\manifest_sign.py',
                           folder])

#indirection function overridable in unittest


def callMakeSddsImport(spv, folder):
    subprocess.check_call(['python.exe', '..\\scripts\\make_sdds_import.py',
                           '-d', spv, '-f', folder])

inputdir = os.path.join("..", "inputs")


class Package:
    def __init__(self, packageNode, outputFolder):
        # The name of the package. This will be the folder
        # name that the processed package is placed in
        self.__name = packageNode.find("name").text

        # If they have specified an outputname
        if packageNode.find("outputname") is not None:
            self.__folder = os.path.join(
                outputFolder, packageNode.find("outputname").text)
        else:
            self.__folder = os.path.join(outputFolder, self.__name)

        # The path to a fileset to include in this package.
        # The path is relative to inputdir.
        # There can be multiple input tags.
        self.__inputs = [i.text for i in packageNode.findall("input")]

        # If they have specified a sdf
        self.__sdf = packageNode.find("sdf") is not None
        self.__sdfName = self.__name
        if self.__sdf:
            if (packageNode.find("sdf").text is not None and
                    packageNode.find("sdf").text != ""):
                self.__sdfName = packageNode.find("sdf").text

        # If they have specified a licence
        self.__license = packageNode.find("license") is not None
        self.__licenseName = self.__name

        # If they have specified a package version
        self.__version = packageNode.find("version") is not None
        if self.__version:
            self.__versionText = packageNode.find("version").text
            if self.__versionText == "":
                self.__version = False

        self.__manifest = packageNode.find("manifest") is not None

        self.__literalVersion = None
        self.__autoVersionFile = None
        self.__autoVersionName = None

        # The scripts generate an SDDS-Import.xml file in packages
        self.__sddsimport = packageNode.find("sddsimport") is not None
        if self.__sddsimport:
            if packageNode.find("sddsimport/version/literal") is not None:
                self.__literalVersion = \
                    packageNode.find("sddsimport/version/literal").text
            elif packageNode.find("sddsimport/version/auto") is not None:
                self.__autoVersionFile = \
                    packageNode.find("sddsimport/version/auto/file").text
                self.__autoVersionDepth = int(
                    packageNode.find("sddsimport/version/auto/depth").text)
            elif packageNode.find("sddsimport/version/filename") is not None:
                self.__autoVersionName = \
                    packageNode.find("sddsimport/version/filename/path").text
                self.__autoVersionMask = \
                    packageNode.find("sddsimport/version/filename/mask").text
                self.__autoVersionFormat = \
                    packageNode.find("sddsimport/version/filename/format").text

        # Specifies files to deletefrom the input filesets.
        # See The <delete> tag below.
        self.__deletions = packageNode.findall('delete/glob')

    def traceInfo(self, code, string):
        print ("%s Collate (INF%s) "
               "packageName=\"%s\": %s" % (datetime.now().isoformat(), code,
                                           self.__name, string))

    def traceError(self, code, string):
        print ("%s Collate (ERR%s) "
               "packageName=\"%s\": %s" % (datetime.now().isoformat(), code,
                                           self.__name, string))

    def copyInputs(self):
        os.makedirs(self.__folder)
        self.traceInfo("001", "Copying %i inputs" % len(self.__inputs))
        for i in self.__inputs:
            input = os.path.join(inputdir, i)
            if (os.path.isdir(input)):
                self.copytree(input, self.__folder)
            else:
                self.traceInfo("001.1", "Folder %s does not exist, skipped" % input)

    def copyData(self):
        dataDir = os.path.join('data', self.__name)
        if os.path.exists(dataDir):
            self.traceInfo("001.2", "Copying data directory %s" % dataDir)
            self.copytree(dataDir, self.__folder)

    def createLicense(self):
        if self.__license:
            self.traceInfo("002", "Copying escdp.dat for %s" %
                           self.__licenseName)
            copy2(os.path.join("license_files",
                  self.__licenseName, "escdp.dat"), self.__folder)

    def createVersion(self):
        if self.__version:
            self.traceInfo("002.1", "Creating version file for %s" % self.__name)
            f = open(os.path.join(self.__folder, "version"), "w")
            f.write(self.__versionText)
            f.close()

    def createManifest(self):
        if self.__manifest:
            self.traceInfo("003", "Manifesting %s" % self.__folder)
            callManifestSign(self.__folder)

    def createSddsImport(self):
        if self.__sddsimport:
            self.traceInfo("004", "Creating import file")
            version = self.findVersion()
            self.__subst = {'product_version_id': version,
                            'product_version_label': version}
            spvpath = self.createSpv()
            callMakeSddsImport(spvpath, self.__folder)
            os.remove(spvpath)

    def createSdf(self):
        if self.__sdf:
            self.traceInfo("005", "Copying sdf.xml for %s" % self.__sdfName)
            copy2(
                os.path.join("sdf", self.__sdfName, "sdf.xml"), self.__folder)

    def getVersionNumber(self, filename):
        try:
            import win32api
            print "imported win32api"
        except:
            self.traceError("002", "Autoversioning not avaiable "
                            "with this version of python, "
                            "win32api must be installed")
            raise
        if not os.path.exists(filename):
            self.traceError("001", "file\"%s\" does not exist" % filename)
            raise exceptions.IOError()
        info = win32api.GetFileVersionInfo(filename, "\\")
        ms = info['ProductVersionMS']
        ls = info['ProductVersionLS']
        return (win32api.HIWORD(ms), win32api.LOWORD(ms),
                win32api.HIWORD(ls), win32api.LOWORD(ls))

    def findVersion(self):
        if self.__autoVersionFile:
            autoversionfp = os.path.join(self.__folder, self.__autoVersionFile)
            self.traceInfo("006", "Finding auto version: %s" % autoversionfp)
            autoversioninfo = self.getVersionNumber(
                autoversionfp)[0:self.__autoVersionDepth]
            return ".".join([str(i) for i in autoversioninfo])
        elif self.__literalVersion:
            self.traceInfo("007", "Finding litteral version")
            return self.__literalVersion
        elif self.__autoVersionName:
            self.traceInfo("008", "Finding auto version name")
            files = glob(os.path.join(self.__folder, self.__autoVersionName))
            result = re.search(self.__autoVersionMask, files[0])
            numbers = [int(s) for s in result.groups()]
            return self.__autoVersionFormat % tuple(numbers)
        else:
            self.traceInfo("009", "No version information specified in input")
            return None

    def createSpv(self):
        self.traceInfo("010", "Creating spv")
        f = open(os.path.join("sdds_spv_templates", self.__name,
                 "%s_spv.xml" % self.__name), "r")
        t = f.read()
        f.close()
        t = t % self.__subst
        handle, path = tempfile.mkstemp(prefix='spv', dir=".", text=True)
        f = os.fdopen(handle, 'w')
        f.write(t)
        f.close()
        return path

    def deleteUnwantedFiles(self):
        if self.__deletions:
            for d in self.__deletions:
                for f in glob(os.path.join(self.__folder, d.text)):
                    self.traceInfo("011", "Deleting unwanted file %s" % f)
                    os.chmod(f, stat.S_IWRITE)
                    os.remove(f)

    def copytree(self, src, dst):
        names = os.listdir(src)
        if not os.path.exists(dst) and not "_meta" in dst:
            os.mkdir(dst)
        errors = []
        for name in names:
            srcname = os.path.join(src, name)
            dstname = os.path.join(dst, name)
            try:
                if "_meta" in src:
                    pass
                elif os.path.isdir(srcname):
                    self.copytree(srcname, dstname)
                else:
                    self.traceInfo("015",
                                   "copying %s to %s" % (srcname, dstname))
                    copy2(srcname, dstname)
            except (IOError, os.error), why:
                errors.append((srcname, dstname, why))
            # catch the Error from the recursive copytree so that we can
            # continue with other files
            except Error, err:
                errors.extend(err.args[0])
        if errors:
            raise Error(errors)

    def assemble(self):
        self.copyInputs()
        self.copyData()
        self.createLicense()
        self.createVersion()
        self.deleteUnwantedFiles()
        self.createSdf()
        self.createManifest()
        self.createSddsImport()


class CommandLine:
    def __init__(self):
        self.parser = OptionParser(usage=usage)

    def parse(self):
        (options, args) = self.parser.parse_args()
        return (options, args)

    def help(self):
        self.parser.print_help()


class Error(exceptions.EnvironmentError):
    pass


class InvalidXML(exceptions.RuntimeError):
    pass


def run(file, outputFolder):
    try:
        tree = ET.parse(file)
    except ExpatError, e:
        print ("%s Collate (ERR%s): Invalid XML %s"
               % (datetime.now().isoformat(), "003", str(e)))
        raise InvalidXML(e)
    packages = []
    try:
        packageNodes = tree.findall("package")
    except Exception, e:
        raise InvalidXML(e)
    for p in packageNodes:
        packages.append(Package(p, outputFolder))
    for p in packages:
        p.assemble()


if __name__ == "__main__":
    command = CommandLine()
    (options, args) = command.parse()
    if len(args) < 1:
        command.help()
        sys.exit()

    outputFolder = os.path.join("..", "SDDS-Ready-Packages")

    if os.path.exists(outputFolder):
        rmtree(outputFolder)
    os.mkdir(outputFolder)

    f = file(args[0], "r")

    run(f, outputFolder)
