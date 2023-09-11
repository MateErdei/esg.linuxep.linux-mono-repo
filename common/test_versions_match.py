# Copyright 2023 Sophos Limited. All rights reserved.

import sys
import zipfile
import os
import glob
import xml.dom.minidom
import shutil


def main(argv):
    installer = argv[1]
    os.mkdir("installer")

    with zipfile.ZipFile(installer, "r") as zip_ref:
        zip_ref.extractall("installer")

    doc = xml.dom.minidom.parse("installer/SDDS-Import.xml")
    version = doc.getElementsByTagName("Version")[0].firstChild.data

    for file in glob.glob("installer/**/VERSION.ini", recursive=True):
        with open(file) as f:
            lines = f.read().split("\n")
            for line in lines:
                if line.startswith("PRODUCT_VERSION = "):
                    version_ini_version = line[len("PRODUCT_VERSION = "):]
                    if version_ini_version != version:
                        raise Exception(f"VERSION.ini version {version_ini_version} doesn't match SDDS-Import.xml version {version}. It may have been caused by a slight cache mismatch (which is unlikely but theoretically possible with the way versioning is done). Try adding an inconsequential code change to flush the cache and produce a new version.")

    shutil.rmtree("installer")


if __name__ == "__main__":
    main(sys.argv)
