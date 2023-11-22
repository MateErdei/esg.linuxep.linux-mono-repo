#!/usr/bin/env python3
# Download supplements from Artifactory, select latest for LocalRep, MLData and VDL.

# https://artifactory.sophos-ops.com/api/storage/esg-tap-component-store/com.sophos/ssplav-vdl/released/
# select latest
# lr data
# ml data

import SDDS3supplementSync

import os
import shutil
import sys
import platform


LOGGER = None


def safe_mkdir(d):
    try:
        os.makedirs(d)
    except EnvironmentError:
        pass


def download_av_supplements(dest="/opt/test/inputs"):
    builder = os.getenv("SDDS3_BUILDER")
    assert os.path.isfile(builder)

    sdds3_temp_dir = os.path.join(dest, "sdds3_temp")
    safe_mkdir(sdds3_temp_dir)

    datasetA_zip = SDDS3supplementSync.sync_sdds3_supplement_name(
        "sdds3.DataSetA", builder, sdds3_temp_dir, "LATEST_CLOUD_ENDPOINT", "GranularInitial")
    assert datasetA_zip is not None, "Failed to download DataSet-A supplement"

    mlmodel_name = None
    if platform.machine() == "x86_64":
        mlmodel_name = "sdds3.ML_MODEL3_LINUX_X86_64"
    elif platform.machine() == "aarch64":
        mlmodel_name = "sdds3.ML_MODEL3_LINUX_ARM64"
    mlmodel_zip = SDDS3supplementSync.sync_sdds3_supplement_name(mlmodel_name, builder, sdds3_temp_dir, "LINUX_SCAN")

    localrep_zip = SDDS3supplementSync.sync_sdds3_supplement_name(
        "sdds3.LocalRepData", builder, sdds3_temp_dir, "LATEST_CLOUD_ENDPOINT")

    SDDS3supplementSync.unpack(builder, datasetA_zip, os.path.join(dest, "dataseta"))
    SDDS3supplementSync.unpack(builder, mlmodel_zip, os.path.join(dest, "ml_model"))
    SDDS3supplementSync.unpack(builder, localrep_zip, os.path.join(dest, "local_rep"))

    shutil.rmtree(sdds3_temp_dir)


def main(argv):
    dest = os.environ.get("SYSTEMPRODUCT_TEST_INPUT")
    if len(argv) >= 2 and argv[1]:
        dest = argv[1]

    download_av_supplements(dest)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
