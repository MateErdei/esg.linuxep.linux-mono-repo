#!/usr/bin/env python3

from __future__ import print_function

import glob
import os
import re
import shutil
import subprocess
import sys


def safe_mkdir(p):
    try:
        os.makedirs(p)
    except OSError:
        pass


def safe_delete(p):
    try:
        os.unlink(p)
        return True
    except OSError:
        return False


def safe_symlink(s, d):
    safe_delete(d)
    os.symlink(s, d)


def unpack_zip(z, extraction_filter=None):
    command = ['unzip', "-o", z]
    if extraction_filter is not None:
        command.append(extraction_filter)
    return subprocess.check_call(command)


def move_glob(g, d):
    for f in glob.glob(g):
        dest = os.path.join(d, os.path.basename(f))
        safe_delete(dest)
        print(f, "to", dest)
        os.rename(f, dest)


def copy_directory(src, dest):
    assert os.path.isdir(src), "%s doesn't exist" % src
    shutil.rmtree(dest, ignore_errors=True)
    safe_mkdir(os.path.dirname(dest))
    print("Copy dir %s to %s" % (src, dest))
    shutil.copytree(src, dest)


def copy_from(src, dest):
    assert os.path.exists(src), "%s doesn't exist" % src
    if os.path.isdir(src):
        shutil.rmtree(dest, ignore_errors=True)
        print("Copy dir %s to %s" % (src, dest))
        shutil.copytree(src, dest)
        return dest
    elif os.path.isfile(src):
        safe_mkdir(dest)
        dest_path = os.path.join(dest, os.path.basename(src))
        print("Copy file %s to %s" % (src, dest_path))
        shutil.copy(src, dest_path)
        return dest_path


def copy_from_base(base, src, dest):
    src = os.path.join(base, src)
    return copy_from(src, dest)


def unpack_tar_from(src, dest, extraction_filter=None):
    pwd = os.getcwd()
    os.chdir(dest)

    if not os.path.isfile(src):
        s = src
        while len(s) > 2:
            s = os.path.dirname(s)
            subprocess.call(['ls', '-l', s])

        raise Exception("Attempting to unpack %s which doesn't exist" % src)

    print("Unpacking %s" % src)
    if src.endswith('.bz2'):
        args = ['tar', 'xjvf', src]
    else:
        args = ['tar', 'xvf', src]

    if extraction_filter:
        args.extend(extraction_filter)
    
    subprocess.check_call(args)
    os.chdir(pwd)


def unzip_from(src, dest, extraction_filter=None):
    safe_mkdir(dest)
    pwd = os.getcwd()
    os.chdir(dest)

    print("Unpacking %s to %s" % (src, dest))
    ret = unpack_zip(src, extraction_filter=extraction_filter)
    os.chdir(pwd)

    return ret


def unpack_zip_from(base, src, dest, extraction_filter=None):
    src = os.path.join(base, src)
    return unzip_from(src, dest, extraction_filter=extraction_filter)


def rsync_from(src, dest, *extra_args):
    command = ["rsync", "-a", "--delete", src, dest]
    command += extra_args
    print(" ".join(command))
    subprocess.check_call(command)


def write_file(dest, contents):
    open(dest, "w").write(contents)


INPUT_DIR   = None
OUTPUT_DIR  = None
SUSI_DIR    = None
VERSION_DIR = None


def copy_from_input(relative_glob, dest):
    global INPUT_DIR
    assert INPUT_DIR is not None
    assert os.path.isdir(INPUT_DIR)
    sources = glob.glob(os.path.join(INPUT_DIR, relative_glob))
    if len(sources) == 0:
        subprocess.call(['ls', INPUT_DIR])
        raise Exception("Failed to find any sources for %s in %s"%(relative_glob, INPUT_DIR))

    res = None
    for s in sources:
        print("Copy from %s to %s"%(s, dest))
        copy_from(s, dest)
        res = s

    if len(sources) == 1:
        return res.replace(INPUT_DIR, dest)


def get_abs_dest_relative_to_base_dir(base, rel_dest=None):
    if rel_dest is None:
        return base
    else:
        return os.path.join(base, rel_dest)


def get_abs_dest_relative_to_version_dir(rel_dest=None):
    global VERSION_DIR
    return get_abs_dest_relative_to_base_dir(VERSION_DIR, rel_dest)


def copy_from_input_to_version_dir(relative_glob, dest=None):
    dest = get_abs_dest_relative_to_version_dir(dest)
    return copy_from_input(relative_glob, dest)


def copy_from_input_to_susi_dir(relative_glob, dest=None):
    global SUSI_DIR
    dest = get_abs_dest_relative_to_base_dir(SUSI_DIR, dest)
    return copy_from_input(relative_glob, dest)


def unpack_zip_from_input_to_version_dir(rel_src, rel_dest, extraction_filter=None):
    dest = get_abs_dest_relative_to_version_dir(rel_dest)
    return unpack_zip_from(INPUT_DIR, rel_src, dest, extraction_filter=extraction_filter)


def unpack_tar_from_input_to_version_dir(rel_src, rel_dest, extraction_filter=None):
    global INPUT_DIR
    dest = get_abs_dest_relative_to_version_dir(rel_dest)
    src = os.path.join(INPUT_DIR, rel_src)
    return unpack_tar_from(src, dest, extraction_filter)


LIBRARY_RE = re.compile(r"^(.*\.so.*)\.(\d+)$")


def create_library_symlinks(d):
    for base, dirs, files in os.walk(d):
        handled = []
        files.sort(reverse=True)
        for f in files:
            if ".so" not in f:
                continue
            if f in handled:
                continue
            while True:
                mo = LIBRARY_RE.match(f)
                if mo is None:
                    break
                dest = os.path.join(base, mo.group(1))
                print("Linking %s to %s in %s" % (dest, f, base))
                safe_symlink(f, dest)
                handled.append(mo.group(1))
                f = mo.group(1)


def delete_duplicated_libraries(d):
    for base, dirs, files in os.walk(d):
        files.sort(reverse=True)
        for f in files:
            while True:
                mo = LIBRARY_RE.match(f)
                if mo is None:
                    break
                p = os.path.join(base, mo.group(1))
                if safe_delete(p):
                    print("Deleted %s" % p)
                f = mo.group(1)


def main(argv):
    SCRIPT = __file__
    FULL_SCRIPT = os.path.abspath(SCRIPT)
    BUILD_DIR = os.path.dirname(FULL_SCRIPT)
    assert os.path.isfile(os.path.join(BUILD_DIR, "setup_tree.py"))
    BASE_DIR = os.path.dirname(BUILD_DIR)
    global INPUT_DIR
    global OUTPUT_DIR
    INPUT_DIR = os.path.join(BASE_DIR, "input", "susi_input")
    if not os.path.isdir(INPUT_DIR):
        print("Can't find INPUT_DIR", INPUT_DIR)
        subprocess.call(['ls', '-l', BUILD_DIR])
        subprocess.call(['ls', '-l', BASE_DIR])
        return 1

    print("Getting INPUT from %s" % INPUT_DIR)
    OUTPUT_DIR = os.path.join(BASE_DIR, "redist")
    shutil.rmtree(OUTPUT_DIR, ignore_errors=True)
    print("Putting OUTPUT to %s" % OUTPUT_DIR)

    global SUSI_DIR
    SUSI_DIR = os.path.join(OUTPUT_DIR, "susi_build")
    safe_mkdir(SUSI_DIR)
    os.chdir(SUSI_DIR)

    VERSION_NAME = "version1"
    global VERSION_DIR
    VERSION_DIR = os.path.join(SUSI_DIR, VERSION_NAME)

    ## Get SUSI - has to be first, since it deletes dest
    copy_from_input("susi/lib", VERSION_DIR)
    move_glob(os.path.join(VERSION_DIR, "libsusi.so*"), SUSI_DIR)
    ## Delete duplicated library
    delete_duplicated_libraries(SUSI_DIR)

    ## rules
    copy_from_input_to_version_dir("rules", "rules")

    ## openssl - maybe not required?

    ## libsavi
    copy_from_input("libsavi/libsavi.so.3.2.*", VERSION_NAME)

    ## libluajit
    copy_from_input("luajit/lib/libluajit*.so.2.*", VERSION_NAME)
    
    ## icu
    copy_from_input("icu/lib/libicu*", VERSION_NAME)

    ## boost
    unpack_tar_from_input_to_version_dir("boost/boost.tar.bz2", ".", ['--wildcards', 
        'lib/libboost_chrono.so.*', 
        'lib/libboost_locale.so.*', 
        'lib/libboost_system.so.*',
        'lib/libboost_thread.so.*',
        '--strip=1'])
        
    ## libarchive
    copy_from_input_to_version_dir("libarchive/lib/*")

    ## httpsrequester / globalrep?
    copy_from_input_to_version_dir("gr/lib/*")

    ## libsophtainer
    unpack_tar_from_input_to_version_dir("libsophtainer/libsophtainer.tar", None)

    ## local-rep
    copy_from_input("lrlib/liblocalrep.so", VERSION_NAME)

    ## ml-lib
    unpack_tar_from_input_to_version_dir("mllib/linux-x64-model-gcc4.8.1.tar", None)

    # TODO: Work out what's required here
    write_file(os.path.join(VERSION_NAME, "package_manifest.txt"), """20190715120000
ide.zip d04566aba1936ccc6750534565f564e2dc27bfac8c490cd061e77321d05c2cef
reputation.zip 6ad35588138b533639f1c10254742967141397b64cdb192f53e251892a9e0e84
model.zip f93993953d0294a4029cb903b401de0ef7478367f8a60008127e299c4dee9bb1
vdl.zip afb44576d7ed06f69274a04d304fc22e75b29495fb08aeee50c3598952697a5b
lua.zip a385a5c82acca024973a0e4ef3d947f960a9a130fd9eeedcdb69bb4d2435edda
susicore.tgz 1d1fae3df540395b1db4afafb6cc2d5848a66d537d4ab1a8f76185632e3a325b
lrlib.tar b90356514f7bc004177379f3d19f62766520dae30ffd503e94baec8ca5ce6e9b
mllib.tar c0250989363d1163801a01a519cf79b6e640594ff700dc289af76a59fbe207ce
libsophtainer.tar 643c733533fa71ad1737d8d5aabafa1b1164376186c95cfda83ed2122008169d
libsavi.tar ab1132ebef8779015579626af8bfc5169d4a6b689024a2487aa301804194bce8
libarchive.tgz cb29fb739db495f26b84cebc2407bf45809e318a8fc77593dfd71de880775bc8
""")

    ## Actually should just contain 'version1'
    write_file("version_manifest.txt", VERSION_NAME)

    SUSI_SDDS_DIR = os.path.join(OUTPUT_DIR, "susi_sdds")
    print("Create SDDS ready fileset")
    copy_directory(SUSI_DIR, SUSI_SDDS_DIR)
    delete_duplicated_libraries(SUSI_SDDS_DIR)

    print("Create build ready fileset")
    ## Setup symlinks
    # safe_symlink(
    # os.path.join("..", VERSION_NAME, "libssp.so.0"),
    # "lib/libssp.so.0"
    # )

    ## Setup SUSI include dir
    susi_include_dir = os.path.join(SUSI_DIR, "include")
    copy_from_input("susi/include", susi_include_dir)
    assert os.path.isdir(susi_include_dir)
    susi_interface_dir = os.path.join(susi_include_dir, "interface")
    for h in ("Susi.h", "SusiTypes.h"):
        copy_from(
            os.path.join(susi_include_dir, h),
            susi_interface_dir
        )

    create_library_symlinks(SUSI_DIR)

    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
