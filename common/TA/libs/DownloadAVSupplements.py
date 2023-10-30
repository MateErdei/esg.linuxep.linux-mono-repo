#!/usr/bin/env python3
from __future__ import absolute_import, print_function, division, unicode_literals
# Download supplements from Artifactory, select latest for LocalRep, MLData and VDL.

# https://artifactory.sophos-ops.com/api/storage/esg-tap-component-store/com.sophos/ssplav-vdl/released/
# select latest
# lr data
# ml data

import SDDS3supplementSync

import glob
import hashlib
import json
import os
import shutil
import subprocess
import sys
import zipfile


from urllib.request import urlopen as urllib_urlopen
from urllib.request import urlretrieve as urllib_urlretrieve
from urllib.error import URLError

LOGGER = None


def safe_mkdir(d):
    try:
        os.makedirs(d)
    except EnvironmentError:
        pass


def run(argv):
    dest = os.environ.get("SYSTEMPRODUCT_TEST_INPUT", default="/tmp/system-product-test-inputs")

    try:
        if argv[1]:
            dest = argv[1]
    except IndexError:
        pass

    builder = dest + "/sdds3/sdds3-builder"
    assert os.path.isfile(builder)

    sdds3_temp_dir = os.path.join(dest, "sdds3_temp")
    safe_mkdir(sdds3_temp_dir)

    datasetA_zip = SDDS3supplementSync.sync_sdds3_supplement_name(
        "sdds3.DataSetA", builder, sdds3_temp_dir, "LATEST_CLOUD_ENDPOINT", "GranularInitial")
    assert datasetA_zip is not None, "Failed to download DataSet-A supplement"

    mlmodel_zip  = SDDS3supplementSync.sync_sdds3_supplement_name(
        "sdds3.ML_MODEL3_LINUX_X86_64", builder, sdds3_temp_dir, "LINUX_SCAN")
    localrep_zip = SDDS3supplementSync.sync_sdds3_supplement_name(
        "sdds3.LocalRepData", builder, sdds3_temp_dir, "LATEST_CLOUD_ENDPOINT")

    SDDS3supplementSync.unpack(builder, datasetA_zip, os.path.join(dest, "vdl"))
    SDDS3supplementSync.unpack(builder, mlmodel_zip, os.path.join(dest, "ml_model"))
    SDDS3supplementSync.unpack(builder, localrep_zip, os.path.join(dest, "local_rep"))

    shutil.rmtree(sdds3_temp_dir)


def main(argv):
    run(argv)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
