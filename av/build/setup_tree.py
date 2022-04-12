#!/usr/bin/env python3

from __future__ import print_function

import hashlib
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
    if not os.path.exists(dest):
        os.mkdir(dest)
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
        print("Copy from %s to %s" % (s, dest))
        copy_from(s, dest)
        res = s

    if len(sources) == 1:
        return res.replace(INPUT_DIR, dest)


def get_abs_dest_relative_to_base_dir(base, rel_dest=None):
    if rel_dest is None:
        return base
    else:
        return os.path.join(base, rel_dest)


def copy_from_input_to_susi_dir(relative_glob, dest=None):
    global SUSI_DIR
    dest = get_abs_dest_relative_to_base_dir(SUSI_DIR, dest)
    return copy_from_input(relative_glob, dest)


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


def dirhash(dirname):
    hash_func = hashlib.md5

    if not os.path.isdir(dirname):
        raise TypeError("{} is not a directory.".format(dirname))

    hashvalues = []
    for root, dirs, files in os.walk(dirname):
        dirs.sort()
        files.sort()

        for fname in files:
            hashvalues.append(_filehash(os.path.join(root, fname), hash_func))

    return _reduce_hash(hashvalues, hash_func)


def _filehash(filepath, hashfunc):
    hasher = hashfunc()
    blocksize = 64 * 1024

    if not os.path.exists(filepath):
        return hasher.hexdigest()

    with open(filepath, "rb") as fp:
        while True:
            data = fp.read(blocksize)
            if not data:
                break
            hasher.update(data)
    return hasher.hexdigest()


def _reduce_hash(hashlist, hashfunc):
    hasher = hashfunc()
    for hashvalue in sorted(hashlist):
        hasher.update(hashvalue.encode("utf-8"))
    return hasher.hexdigest()


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

    ## Get SUSI - has to be first, since it deletes dest
    copy_from_input("susi/lib", SUSI_DIR)

    ## Create initial update cache
    UPDATE_CACHE_DIR = os.path.join(OUTPUT_DIR, "susi_update_source")
    os.mkdir(UPDATE_CACHE_DIR)

    SUSICORE_DIR = os.path.join(UPDATE_CACHE_DIR, "susicore")
    LRLIB_DIR = os.path.join(UPDATE_CACHE_DIR, "lrlib")
    MLLIB_DIR = os.path.join(UPDATE_CACHE_DIR, "mllib")
    SOPHTAINER_DIR = os.path.join(UPDATE_CACHE_DIR, "libsophtainer")
    SAVI_DIR = os.path.join(UPDATE_CACHE_DIR, "libsavi")
    GRLIB_DIR = os.path.join(UPDATE_CACHE_DIR, "libglobalrep")
    RULES_DIR = os.path.join(UPDATE_CACHE_DIR, "rules")
    UPDATER_DIR = os.path.join(UPDATE_CACHE_DIR, "libupdater")

    copy_from_input("susi/lib/libsusicore.so*", SUSICORE_DIR)

    ## libupdater
    copy_from_input("susi/lib/libupdater.so*", UPDATER_DIR)

    ## rules
    copy_from_input("rules", RULES_DIR)

    ## libsavi
    copy_from_input("libsavi/release/libsavi.so.3.2.*", SAVI_DIR)

    ## libluajit
    copy_from_input("luajit/lib/libluajit*.so.2.*", SUSICORE_DIR)

    ## icu
    copy_from_input("icu/lib/libicu*", SUSICORE_DIR)

    ## boost
    unpack_tar_from(os.path.join(INPUT_DIR, "boost/boost.tar.bz2"), SUSICORE_DIR, ['--wildcards',
                                                                                 'lib/libboost_chrono.so.*',
                                                                                 'lib/libboost_locale.so.*',
                                                                                 'lib/libboost_system.so.*',
                                                                                 'lib/libboost_thread.so.*',
                                                                                 '--strip=1'])

    ## libarchive
    copy_from_input("libarchive/lib/*", SUSICORE_DIR)

    ## httpsrequester / globalrep?
    copy_from_input("gr/lib/*", GRLIB_DIR)

    ## libsophtainer
    copy_from_input("libsophtainer/release/libsophtainer.so", SOPHTAINER_DIR)

    ## local-rep
    copy_from_input("lrlib/liblocalreputation.so", LRLIB_DIR)

    ## ml-lib
    unpack_tar_from(os.path.join(INPUT_DIR, "mllib/linux-x64-model-gcc4.8.1.tar"), MLLIB_DIR)

    ## Calculate hashes
    susicore_checksum = dirhash(SUSICORE_DIR)
    lrlib_checksum = dirhash(LRLIB_DIR)
    mllib_checksum = dirhash(MLLIB_DIR)
    libsophtainer_checksum = dirhash(SOPHTAINER_DIR)
    libsavi_checksum = dirhash(SAVI_DIR)
    libglobalrep_checksum = dirhash(GRLIB_DIR)
    rules_checksum = dirhash(RULES_DIR)

    manifest = """susicore {}
lrlib {}
mllib {}
libsophtainer {}
libsavi {}
libglobalrep {}
rules {}
""".format(
        susicore_checksum,
        lrlib_checksum,
        mllib_checksum,
        libsophtainer_checksum,
        libsavi_checksum,
        libglobalrep_checksum,
        rules_checksum)

    write_file(os.path.join(UPDATE_CACHE_DIR, "nonsupplement_manifest.txt"), manifest)
    write_file(os.path.join(UPDATE_CACHE_DIR, "version_manifest.txt"), "version1")

    SUSI_SDDS_DIR = os.path.join(OUTPUT_DIR, "susi_sdds")
    print("Create SDDS ready fileset")
    copy_directory(UPDATE_CACHE_DIR, os.path.join(SUSI_SDDS_DIR, "update_source"))
    delete_duplicated_libraries(SUSI_SDDS_DIR)

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
