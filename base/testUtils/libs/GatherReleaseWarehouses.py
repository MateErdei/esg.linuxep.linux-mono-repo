import datetime
import io
import os
import shutil
import sys
import zipfile

import requests
from artifactory import ArtifactoryPath


def unpack_sdds3_artifact(build_url, artifact_name, output_dir):
    unpack_location = os.path.join(output_dir, artifact_name)

    artifact_url = os.path.join(build_url, "build", f"prod-sdds3-{artifact_name}.zip")

    r = requests.get(artifact_url)
    z = zipfile.ZipFile(io.BytesIO(r.content))
    z.extractall(unpack_location)


def get_warehouse_branches(branch_filter, url, type, version_separator):
    release_branches = []
    all_paths = []
    for path in ArtifactoryPath(url):
        branch_name = os.path.basename(path)
        if branch_name.startswith(branch_filter):
            all_paths.append(branch_name)

    # Remove sprint branches
    if type == "current_shipping":
        for path in all_paths:
            try:
                if int(path.strip(branch_filter).split(version_separator)[0]) <= 4:
                    release_branches.append(path)
            except ValueError:
                pass
    else:
        release_branches = all_paths
    return release_branches


def gather_sdds3_warehouse_files(output_dir, release_type):
    print(f"gather_sdds3_warehouse_files {output_dir} {release_type}")
    release_branches, builds = [], []
    version_separator = ""

    current_year = datetime.date.today().year
    warehouse_repo_url = "https://artifactory.sophos-ops.com/artifactory/esg-build-candidate/linuxep.sspl-warehouse"

    if release_type == "dogfood":
        version_separator = "-"
    elif release_type == "current_shipping":
        version_separator = "."
    else:
        raise AssertionError(f"Invalid argument {release_type}: use dogfood or current_shipping")

    branch_filter = f"release--{current_year}{version_separator}"
    release_branches = get_warehouse_branches(branch_filter, warehouse_repo_url, release_type, version_separator)

    if len(release_branches) == 0:
        branch_filter = f"release--{current_year - 1}{version_separator}"
        release_branches = get_warehouse_branches(branch_filter, warehouse_repo_url, release_type, version_separator)

    release_branch = sorted(release_branches,
                            key=lambda x: float(x[len(branch_filter):].split(version_separator)[0]),
                            reverse=True)[0]
    for build in ArtifactoryPath(os.path.join(warehouse_repo_url, release_branch)):
        builds.append(build)
    latest_build = sorted(builds, key=lambda x: int(os.path.basename(x).split('-')[0]), reverse=True)[0]

    unpack_sdds3_artifact(latest_build, "launchdarkly", output_dir)
    unpack_sdds3_artifact(latest_build, "repo", output_dir)


def setup_release_warehouse(dest, release_type):
    output_dir = os.path.join(dest, f"sdds3-{release_type}")
    if not os.path.isdir(output_dir):
        os.mkdir(output_dir)

    try:
        gather_sdds3_warehouse_files(output_dir, release_type)
    except Exception as ex:
        raise AssertionError(f"Failed to gather {release_type} warehouse files. Error: {ex}")

    sdds3repo_path = os.path.join(output_dir, "repo")
    vut_sdds3_repo_path = os.path.join(dest, "sdds3", "repo")

    vut_sdds3_package_path = os.path.join(vut_sdds3_repo_path, "package")
    release_sdds3_package_path = os.path.join(sdds3repo_path, "package")

    vut_sdds3_supplement_path = os.path.join(vut_sdds3_repo_path, "supplement")
    release_sdds3_supplement_path = os.path.join(sdds3repo_path, "supplement")

    if not os.path.isdir(release_sdds3_supplement_path):
        os.mkdir(release_sdds3_supplement_path)

    files = os.listdir(vut_sdds3_supplement_path)
    for f in files:
        if not os.path.isfile(os.path.join(release_sdds3_supplement_path, f)):
            print(f"Copying {f}")
            shutil.copy(os.path.join(vut_sdds3_supplement_path, f), release_sdds3_supplement_path)

    for (dirpath, dirnames, filenames) in os.walk(vut_sdds3_package_path):
        for package in filenames:
            if package.startswith(("DataSetA",
                                   "LocalRepData",
                                   "ML_MODEL3_LINUX_X86_64",
                                   "RuntimeDetectionRules",
                                   "ScheduledQueryPack",
                                   "SSPLFLAGS")):
                if not os.path.isfile(os.path.join(release_sdds3_package_path, package)):
                    shutil.copy(os.path.join(vut_sdds3_package_path, package),
                                os.path.join(release_sdds3_package_path, package))


def run(argv):
    dest = os.environ.get("SYSTEMPRODUCT_TEST_INPUT", default="/tmp/system-product-test-inputs")
    try:
        if argv[1]:
            dest = argv[1]
    except IndexError:
        pass

    setup_release_warehouse(dest, "dogfood")
    setup_release_warehouse(dest, "current_shipping")


def main(argv):
    run(argv)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
