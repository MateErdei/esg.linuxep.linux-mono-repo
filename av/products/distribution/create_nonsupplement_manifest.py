# Copyright 2023 Sophos Limited. All rights reserved.

import hashlib
import os
import sys


def _reduce_hash(hashlist, hashfunc):
    hasher = hashfunc()
    for hashvalue in sorted(hashlist):
        hasher.update(hashvalue.encode("utf-8"))
    return hasher.hexdigest()

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


def hash_inputs(inputs):
    hash_func = hashlib.md5
    inputs.sort()
    hash_values = []
    for name, src in inputs:
        hash_values.append(_filehash(src, hash_func))
    return _reduce_hash(hash_values, hash_func)


def write_file(dest, contents):
    open(dest, "w").write(contents)


def main(argv):
    assert argv[1][0] == "@", "Expecting a parameter file"

    with open(argv[1][1:]) as params_file:
        args = params_file.read().split("\n")

    assert len(args) >= 1

    destination = args[0]

    inputs = {}

    for arg in args[1:]:
        if len(arg.strip()) == 0:
            break
        src, dest = arg.split("=", 1)
        assert dest.startswith("files/plugins/av/chroot/susi/update_source/")
        dest = dest[len("files/plugins/av/chroot/susi/update_source/"):]
        d, base = dest.split("/", 1)
        inputs.setdefault(d, []).append((base, src))

    print("inputs:", inputs)

    susicore_checksum = hash_inputs(inputs["susicore"])
    lrlib_checksum = hash_inputs(inputs["lrlib"])
    libsophtainer_checksum = hash_inputs(inputs["libsophtainer"])
    libsavi_checksum = hash_inputs(inputs["libsavi"])
    libglobalrep_checksum = hash_inputs(inputs["libglobalrep"])
    rules_checksum = hash_inputs(inputs["rules"])
    libupdater_checksum = hash_inputs(inputs["libupdater"])

    manifest = f"""susicore {susicore_checksum}
lrlib {lrlib_checksum}
libsophtainer {libsophtainer_checksum}
libsavi {libsavi_checksum}
libglobalrep {libglobalrep_checksum}
rules {rules_checksum}
libupdater {libupdater_checksum}
"""

    print("Manifest:", manifest)
    write_file(destination, manifest)

    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
