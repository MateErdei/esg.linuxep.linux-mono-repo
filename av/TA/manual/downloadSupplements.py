#!/usr/bin/env python3
from __future__ import absolute_import, print_function, division, unicode_literals
# Download supplements from Artifactory, select latest for LocalRep, MLData and VDL.

# https://artifactory.sophos-ops.com/api/storage/esg-tap-component-store/com.sophos/ssplav-vdl/released/
# select latest
# lr data
# ml data

import hashlib
import json
import os
import sys
import zipfile

PY2 = sys.version_info[0] == 2
if PY2:
    from urllib import urlopen as urllib_urlopen
    from urllib import urlretrieve as urllib_urlretrieve
else:
    from urllib.request import urlopen as urllib_urlopen
    from urllib.request import urlretrieve as urllib_urlretrieve

LOGGER = None


def log(*x):
    global LOGGER
    if LOGGER is None:
        print(*x)
    else:
        LOGGER.info(" ".join(x))

def ensure_binary(s):
    if s is None:
        return s
    if isinstance(s, bytes):
        return s
    return s.encode("UTF-8")


def ensure_unicode(s):
    if s is None:
        return s
    if isinstance(s, str):  # fix if we need to work on python2
        return s
    return s.decode("UTF-8")


def safe_mkdir(d):
    try:
        os.makedirs(d)
    except EnvironmentError:
        pass


def get_json(url):
    response = urllib_urlopen(url)
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
    urllib_urlretrieve(url, dest)
    assert sha256hash(dest) == data['checksums']['sha256']
    return True


def unpack(zip_file, dest):
    print("Extracting: {}".format(ensure_unicode(zip_file)))
    with zipfile.ZipFile(ensure_unicode(zip_file)) as z:
        safe_mkdir(dest)
        z.extractall(ensure_unicode(dest))


DEST = ""


def process(baseurl, filename, dirname):
    global DEST
    zip_file = os.path.join(DEST, ensure_binary(filename))
    dest_dir = os.path.join(DEST, dirname)
    if os.path.isdir(dest_dir) and not os.path.isfile(zip_file):
        # If we don't have a zip file but do have the dir, assume we are running in TAP
        log("Assuming %s is already suitable" % dest_dir)
        return False

    if os.environ.get("NO_DOWNLOAD", "0") == "1":
        log("Refusing to download - hope the dir is ok")
        return False

    latest = get_latest(baseurl, filename)
    zip_updated = download_url(latest, zip_file)
    if not os.path.isdir(dest_dir):
        # Force unpack and regen if dest_dir doesn't exist
        zip_updated = True
    if zip_updated:
        unpack(zip_file, dest_dir)
    return zip_updated


def run(dest):
    global DEST
    DEST = dest
    safe_mkdir(DEST)
    artifactory_base_url = "https://artifactory.sophos-ops.com/api/storage/esg-tap-component-store/com.sophos/"
    updated = process(artifactory_base_url + "ssplav-vdl/released", "vdl.zip", b"vdl")
    updated = process(artifactory_base_url + "ssplav-mlmodel/released", "model.zip", b"ml_model") or updated
    updated = process(artifactory_base_url + "ssplav-localrep/released", "reputation.zip", b"local_rep") or updated
    return updated


def main(argv):
    run(argv[1])
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
