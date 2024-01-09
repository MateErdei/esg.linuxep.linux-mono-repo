import argparse
import datetime
import json
import os
import shutil
import sys

from artifactory import ArtifactoryPath
from build_scripts.artisan_fetch import artisan_fetch

warehouse_repo_url = "https://artifactory.sophos-ops.com/artifactory/esg-build-candidate/linuxep.linux-mono-repo"
no_static_suite = ['release--2023-37', 'release--2023-40', 'release--2023.4-non-arm64-thin-installer']

def get_warehouse_artifact(release_branch, output_dir,build_id=None):

    release_package_path = os.path.join(output_dir, "release-package.xml")
    id_code = ""
    if build_id:
        id_code = f'build-id="{build_id}"'
    with open(release_package_path, 'w') as release_package_file:
        contents = f"""<?xml version="1.0" encoding="utf-8"?>
    <package name="system-product-tests">
        <inputs>
            <workingdir>.</workingdir>
            <build-asset project="linuxep" repo="linux-mono-repo">
                <development-version branch="{release_branch}" {id_code}/>
                <include artifact-path="prod-sdds3-repo" dest-dir="{output_dir}/repo" />
                <include artifact-path="prod-sdds3-launchdarkly" dest-dir="{output_dir}/launchdarkly" />
            </build-asset>
        </inputs>
        <publish>
            <workingdir>sspl-base-build</workingdir>
            <destdir>sspl-base</destdir>
            <buildoutputs>
                <output name="output" srcfolder="output" artifactpath="output"> </output>
            </buildoutputs>
            <publishbranches>release</publishbranches>
        </publish>
    </package>
"""
        release_package_file.write(contents)

    artisan_fetch(release_package_path, build_mode="not_used", production_build=False)


def get_warehouse_artifact_static_suite(release_branch, output_dir):

    release_package_path = os.path.join(output_dir, "release-suite.xml")
    with open(release_package_path, 'w') as release_package_file:
        contents = f"""<?xml version="1.0" encoding="utf-8"?>
    <package name="system-product-tests">
        <inputs>
            <workingdir>.</workingdir>
            <build-asset project="linuxep" repo="linux-mono-repo">
                <development-version branch="{release_branch}" />
                <include artifact-path="prod-sdds3-static-suites" dest-dir="{output_dir}/static-suites" />
           </build-asset>
        </inputs>
        <publish>
            <workingdir>sspl-base-build</workingdir>
            <destdir>sspl-base</destdir>
            <buildoutputs>
                <output name="output" srcfolder="output" artifactpath="output"> </output>
            </buildoutputs>
            <publishbranches>release</publishbranches>
        </publish>
    </package>
"""
        release_package_file.write(contents)

    artisan_fetch(release_package_path, build_mode="not_used", production_build=False)


def get_warehouse_branches(branch_filter, url, type, version_separator, with_static_suites=False):
    release_branches = []
    all_paths = []
    for path in ArtifactoryPath(url):
        branch_name = os.path.basename(path)
        if branch_name.startswith(branch_filter):
            if with_static_suites and branch_name in no_static_suite:
                continue
            all_paths.append(branch_name)

    # Remove sprint branches
    if type == "current_shipping":
        for path in all_paths:
            try:
                if int(path.replace(branch_filter, "").split(version_separator)[0]) <= 4:
                    release_branches.append(path)
            except ValueError:
                pass
    else:
        release_branches = all_paths
    return release_branches


def get_warehouse_branch(release_type):
    version_separator = ""

    current_year = datetime.date.today().year

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

    # Handle branch format 2023-16 and also 2023-16_comments_like_this
    cleaned_branches = [branch.split("_")[0] for branch in release_branches]

    return sorted(cleaned_branches, key=lambda x: float(x[len(branch_filter):].split(version_separator)[0]),
                  reverse=True)[0]


def setup_release_warehouse(dest, release_type, override_branch):
    output_dir = os.path.join(dest, f"sdds3-{release_type}")
    if not os.path.isdir(output_dir):
        os.mkdir(output_dir)

    try:
        release_branch = override_branch if override_branch else get_warehouse_branch(release_type)
        print(f"Using {release_branch} for {release_type} warehouse")
        get_warehouse_artifact(release_branch, output_dir)
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
                                   "ML_MODEL3_LINUX_ARM64",
                                   "RuntimeDetectionRules",
                                   "ScheduledQueryPack",
                                   "SSPLFLAGS")):
                if not os.path.isfile(os.path.join(release_sdds3_package_path, package)):
                    shutil.copy(os.path.join(vut_sdds3_package_path, package),
                                os.path.join(release_sdds3_package_path, package))


def setup_fixed_versions(dest, fixed_version):
    # Find the warehouse branch that matched fixed_versions and then create a symlink to it with that name
    #get year of fixed version
    year = fixed_version.split(" ")[1][:4]

    if fixed_version != "FTS 2023.4.0.14":
        branch_filter = f"release--{year}"
        release_branches = get_warehouse_branches(branch_filter, warehouse_repo_url, None, None, True)
        print(release_branches)
        for release_branch in release_branches:
            output_dir = os.path.join(dest, f"sdds3-{release_branch}")
            if not os.path.isdir(output_dir):
                os.makedirs(output_dir)
            get_warehouse_artifact_static_suite(release_branch, output_dir)

            with open(os.path.join(output_dir, "static-suites", "linuxep.json")) as f:
                release_version = json.load(f)["name"]
                # Special handling for mis-matching version numbers for LINUXDAR-8073
                if release_version == "FTS 2023.4.0.27-LINUXDAR-8073":
                    release_version = "FTS 2023.4.0.30-LINUXDAR-8073"
                print(f"Fixed version name = {release_version}")
                if release_version in fixed_version:
                    get_warehouse_artifact(release_branch, output_dir)
                    symlink_path = os.path.join(dest, release_version)
                    if not os.path.exists(symlink_path):
                        os.symlink(output_dir, symlink_path)
                    fixed_version = ""
                    print(f"Found desired fixed version {release_version}")
            if not fixed_version:
                print("Found fixed versionK")
                break
        if fixed_version:
            print(f"Failed to find fixed version: {fixed_version}")
    else:
        release_branch = "release--2023.4"
        release_version = "FTS 2023.4.0.14"
        build_id = "20231117142146-704d26b9d2f10d7bbc843d378c6c700dcf500074-1rARjl"
        output_dir = os.path.join(dest, f"sdds3-{release_branch}")
        if not os.path.isdir(output_dir):
            os.makedirs(output_dir)
        get_warehouse_artifact(release_branch, output_dir, build_id)
        symlink_path = os.path.join(dest, release_version)
        if not os.path.exists(symlink_path):
            os.symlink(output_dir, symlink_path)
    vut_sdds3_repo_path = os.path.join(dest, "repo")
    vut_sdds3_package_path = os.path.join(vut_sdds3_repo_path, "package")
    vut_sdds3_supplement_path = os.path.join(vut_sdds3_repo_path, "supplement")
    fixed_version_sdds3_repo_path = os.path.join(symlink_path, "repo")
    fixed_version_sdds3_package_path = os.path.join(fixed_version_sdds3_repo_path, "package")
    fixed_version_sdds3_supplement_path = os.path.join(fixed_version_sdds3_repo_path, "supplement")

    if not os.path.isdir(fixed_version_sdds3_supplement_path):
        os.mkdir(fixed_version_sdds3_supplement_path)

    files = os.listdir(vut_sdds3_supplement_path)
    for f in files:
        if not os.path.isfile(os.path.join(fixed_version_sdds3_supplement_path, f)):
            print(f"Copying {f}")
            shutil.copy(os.path.join(vut_sdds3_supplement_path, f), fixed_version_sdds3_supplement_path)

    for (dirpath, dirnames, filenames) in os.walk(vut_sdds3_package_path):
        for package in filenames:
            if package.startswith(("DataSetA",
                                   "LocalRepData",
                                   "ML_MODEL3_LINUX_X86_64",
                                   "ML_MODEL3_LINUX_ARM64",
                                   "RuntimeDetectionRules",
                                   "ScheduledQueryPack",
                                   "SSPLFLAGS")):
                if not os.path.isfile(os.path.join(fixed_version_sdds3_package_path, package)):
                    shutil.copy(os.path.join(vut_sdds3_package_path, package),
                                os.path.join(fixed_version_sdds3_package_path, package))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--dest",
                        default=os.environ.get("SYSTEMPRODUCT_TEST_INPUT", default="/tmp/system-product-test-inputs"))
    parser.add_argument("--dogfood-override", default=os.environ.get("SDDS3_DOGFOOD", default=None))
    parser.add_argument("--current-shipping-override", default=os.environ.get("SDDS3_CURRENT_SHIPPING", default=None))
    parser.add_argument("--fixed-version",  default=None)
    args = parser.parse_args()

    if args.fixed_version:
        setup_fixed_versions(args.dest, args.fixed_version)
    else:
        setup_release_warehouse(args.dest, "dogfood", args.dogfood_override)
        setup_release_warehouse(args.dest, "current_shipping", args.current_shipping_override)
    return 0


if __name__ == "__main__":
    sys.exit(main())
