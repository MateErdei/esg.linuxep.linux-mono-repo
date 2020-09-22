#!/usr/bin/env python3

# https://artifactory.sophos-ops.com/api/storage/esg-tap-component-store/com.sophos/ssplav-vdl/released/
# select latest

# lr data

# ml data

import hashlib
import json
import os
import sys
import urllib.request
import zipfile

LOGGER = None


def log(*x):
    global LOGGER
    if LOGGER is None:
        print(*x)
    else:
        LOGGER.info(" ".join(x))

def ensure_binary(s):
    if isinstance(s, bytes):
        return s
    return s.encode("UTF-8")


def ensure_unicode(s):
    if isinstance(s, str): # fix if we need to work on python2
        return s
    return s.decode("UTF-8")


def safe_mkdir(d):
    try:
        os.makedirs(d)
    except EnvironmentError:
        pass


def get_json(url):
    response = urllib.request.urlopen(url)
    output = response.read()
    data = json.loads(output)
    return data


def get_latest(url, filename):
    log("Getting %s metadata from %s" % (filename, url))
    data = get_json(url)
    children = data['children']
    res = []
    for c in children:
        if c['folder']:
            res.append(c['uri'])
    res.sort()

    target_url = url+res[-1] + "/" + filename
    return target_url


def sha256hash(f):
    hasher = hashlib.sha256()
    with open(f, "rb") as fp:
        while True:
            data = fp.read(64*1024)
            hasher.update(data)
            if len(data) == 0:
                break
    return hasher.hexdigest()


def download_url(url, dest):
    log("Getting metadata from", url)
    data = get_json(url)
    url = data['downloadUri']
    if os.path.isfile(dest):
        if sha256hash(dest) == data['checksums']['sha256']:
            log("Not downloading - already matches sha256: ", data['checksums']['sha256'])
            return False

    log("Downloading from", url, "to", ensure_unicode(dest))
    urllib.request.urlretrieve(url, dest)
    return True


def unpack(zip_file, dest):
    safe_mkdir(dest)
    with zipfile.ZipFile(open(zip_file, "r")) as z:
        z.extractall(dest)


DEST = ""


def process(baseurl, filename, dirname):
    latest = get_latest(baseurl, filename)
    zip_file = os.path.join(DEST, ensure_binary(filename))
    if download_url(latest, zip_file):
        unpack(zip_file, os.path.join(DEST, dirname))


def run(dest):
    global DEST
    DEST = dest
    safe_mkdir(DEST)
    artifactory_base_url = "https://artifactory.sophos-ops.com/api/storage/esg-tap-component-store/com.sophos/"
    process(artifactory_base_url + "ssplav-vdl/released", "vdl.zip", b"vdl")
    process(artifactory_base_url + "ssplav-mlmodel/released", "model.zip", b"ml_model")
    process(artifactory_base_url + "ssplav-localrep/released", "reputation.zip", b"local_rep")
    return 0


def main(argv):
    return run(argv[1])


if __name__ == "__main__":
    sys.exit(main(sys.argv))
