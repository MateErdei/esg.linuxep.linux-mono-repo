##Python library of utilities

from __future__ import print_function,division,unicode_literals

import gzip
import os
import shutil

import Build
from Process import *

class UtilBuildException(Build.BuildException):
    pass

def makedirs(directoryPath, mode=0o777):
    """Create a directory path, equivilent of os.makedirs but ignores the error if the
    directory already exists.
    """
    try:
        os.makedirs(directoryPath,mode)
    except OSError as e:
        if (e.errno == 17):
            ## already exists
            pass
        else:
            print("Unable to makedirs %s: %s (%d)"%(directoryPath,e.strerror,e.errno))
            raise
    pass

def delete(filepath):
    try:
        if os.path.isdir(filepath):
            shutil.rmtree(filepath)
        else:
            os.unlink(filepath)
        return True
    except OSError:
        return False

def setupDirectory(directoryPath):
    delete(directoryPath)
    makedirs(directoryPath,0o770)

def copy(src,dest):
    makedirs(os.path.dirname(dest))
    shutil.copy2(src,dest)

def ungzip(src,dest):
    assert src is not None
    assert dest is not None
    if src.endswith(".tar"):
        return copy(src,dest)

    assert src.endswith(".tgz") or src.endswith(".tar.gz")
    makedirs(os.path.dirname(dest))
    infile = gzip.open(src)
    outfile = open(dest,"w")

    while True:
        data = infile.read(1024)
        if len(data) == 0:
            break
        outfile.write(data)

    outfile.close()
    infile.close()

def unpack(srcfile, destdir):
    assert srcfile is not None
    assert destdir is not None
    setupDirectory(destdir)

    if srcfile.endswith(".tar"):
        extract = 'xf'
    elif srcfile.endswith(".tar.gz") or srcfile.endswith(".tgz"):
        extract = "xzf"
    else:
        raise UtilBuildException("%s isn't a known tarfile type"%srcfile)

    retCode = run(['tar',extract,srcfile,"-C",destdir])
    if retCode != 0:
        raise UtilBuildException("Failed to extract tarfile: %s"%srcfile)

def packTarfile(destfile, srcdir, filelist):
    assert destfile is not None
    assert srcdir is not None

    if destfile.endswith(".tar"):
        taroption = 'cf'
    elif destfile.endswith(".tar.gz") or destfile.endswith(".tgz"):
        taroption = "czf"
    elif destfile.endswith(".tar.bz2"):
        taroption = "cjf"
    elif destfile.endswith(".tar.Z"):
        taroption = "cZf"
    else:
        raise UtilBuildException("%s isn't a known tarfile type"%destfile)

    print("Generating",destfile)
    if os.path.exists(destfile):
        print("\tDeleting old tarfile")
        delete(destfile)

    args = ["tar",taroption,destfile,"-C",srcdir]
    args += filelist
    retCode = run(args)
    if retCode != 0:
        raise UtilBuildException("ARCHIVE: Failed to create tarfile: %s"%destfile)


def write(contents, dest, perms):
    makedirs(os.path.dirname(dest))
    f = open(dest, "w")
    f.write(contents)
    f.close()
    os.chmod(dest,perms)


def checkMachine(options):
    options.uname = os.uname()
    options.platform = options.uname[0].lower().replace("-","")
    options.isLinux   = (options.platform == "linux")
    options.isSolaris = (options.platform == "sunos")
    options.isHPUX = (options.platform == "hpux")

    if options.isLinux:
        options.processor = options.uname[4]
    elif options.isHPUX:
        (retCode, options.processor) = collectOutput(['uname','-m'], "")
        if retCode != 0:
            raise UtilBuildException("Failed to get ISA/processor type: %d"%retCode)
    else:
        (retCode, options.processor) = collectOutput(['uname','-p'], "")
        if retCode != 0:
            raise UtilBuildException("Failed to get ISA/processor type: %d"%retCode)
    options.processor = options.processor.strip()

    x86architectures = [ 'x86','i386','i86pc','i686']
    if options.processor in x86architectures:
        options.architecture = "x86"
    else:
        options.architecture = options.processor
