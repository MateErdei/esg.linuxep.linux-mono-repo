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


from urllib.request import urlopen as urllib_urlopen
from urllib.request import urlretrieve as urllib_urlretrieve
from urllib.error import URLError

LOGGER = None



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
    print("Getting %s metadata from %s" % (filename, url))
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
    print("Getting metadata from", url)
    data = get_json(url)
    if os.path.isfile(dest):
        if sha256hash(dest) == data['checksums']['sha256']:
            print("Not downloading - already matches sha256: ", data['checksums']['sha256'])
            return False

    url = data['downloadUri']
    print("Downloading from", url, "to", ensure_unicode(dest))
    urllib_urlretrieve(url, dest)
    assert sha256hash(dest) == data['checksums']['sha256']
    return True


def unpack(zip_file, dest):
    print("Extracting: {}".format(ensure_unicode(zip_file)))
    with zipfile.ZipFile(ensure_unicode(zip_file)) as z:
        safe_mkdir(dest)
        z.extractall(ensure_unicode(dest))


def process(baseurl, filename, dirname):
    zip_file = os.path.join("/tmp", filename)
    dest_dir = dirname

    latest = get_latest(baseurl, filename)
    zip_updated = download_url(latest, zip_file)
    if not os.path.isdir(dest_dir):
        # Force unpack and regen if dest_dir doesn't exist
        zip_updated = True
    if zip_updated:
        unpack(zip_file, dest_dir)
    return zip_updated


def run(argv):
    dest = os.environ.get("SYSTEMPRODUCT_TEST_INPUT", default="/tmp/system-product-test-inputs")
    try:
        if argv[1]:
            dest = argv[1]
    except IndexError:
        pass

    artifactory_base_url = "https://artifactory.sophos-ops.com/api/storage/esg-tap-component-store/com.sophos/"
    updated = process(artifactory_base_url + "ssplav-vdl/released", "vdl.zip", dest +"/vdl")
    updated = process(artifactory_base_url + "ssplav-mlmodel/released", "model.zip", dest +"/ml_model") or updated
    updated = process(artifactory_base_url + "ssplav-localrep/released", "reputation.zip", dest +"/local_rep") or updated
    return updated

def main(argv):
    run(argv)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
