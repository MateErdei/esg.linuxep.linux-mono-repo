#!/usr/bin/env python3
import sys
import os
import subprocess as sp
import glob

BASE_REPO_DIR_NAME = "esg.linuxep.everest-base"
SSPL_TOOLS_REPO_DIR_NAME = "esg.linuxep.sspl-tools"
AV_REPO_DIR_NAME = "esg.linuxep.sspl-plugin-anti-virus"
EDR_REPO_DIR_NAME = "esg.linuxep.sspl-plugin-edr-component"
EVENT_JOURNALER_REPO_DIR_NAME = "esg.linuxep.sspl-plugin-event-journaler"


def add_quote(input_arg: str) -> str:
    if input_arg[0] == '"' or input_arg[0] == "'":
        return input_arg
    else:
        return '"' + input_arg + '"'


# Find the VagrantRoot folder (where the Vagrantfile is)
def find_vagrant_root(start_from_dir: str) -> str:
    curr = start_from_dir
    while not os.path.exists(os.path.join(curr, 'Vagrantfile')):
        curr = os.path.split(curr)[0]
        if curr in ['/', '/home']:
            raise AssertionError('Failed to find Vagrantfile in directory or its parents: {}'.format(start_from_dir))
    return curr


def check_vagrant_up_and_running(vagrant_wrapper_dir: str):
    vagrant_wrapper = os.path.join(vagrant_wrapper_dir, 'vagrant')
    output = sp.check_output([vagrant_wrapper, 'status'])
    if b'running' not in output:
        print('starting up vagrant')
        sp.call([vagrant_wrapper, 'up', 'ubuntu'])


def vagrant_rsync(vagrant_wrapper_dir: str):
    vagrant_wrapper = os.path.join(vagrant_wrapper_dir, 'vagrant')
    sp.check_call([vagrant_wrapper, 'rsync'])


def get_plugin_names(vagrant_root_dir: str):
    repos = []
    with open(os.path.join(vagrant_root_dir, "setup/gitRepos.txt")) as git_repos_file:
        for line in git_repos_file.readlines():
            plugin = {}
            line = line.strip()
            if "plugin" in line.lower():
                plugin_name = line.split("/")[-1].replace(".git", "").split(".")[-1]
                plugin["dir"] = line.split("/")[-1].replace(".git", "")
                plugin["name"] = plugin_name
                plugin["envName"] = plugin_name.upper().replace("-", "_") + "_SDDS"
                repos.append(plugin)
    return repos


def newer_folder(folder_one, folder_two):
    if os.path.isdir(folder_one) and os.path.isdir(folder_two):
        newest_timestamp_folder_one = max([os.path.getctime(file) for file in glob.glob(os.path.join(folder_one, "*"))])
        newest_timestamp_folder_two = max([os.path.getctime(file) for file in glob.glob(os.path.join(folder_two, "*"))])

        if newest_timestamp_folder_one >= newest_timestamp_folder_two:
            return folder_one
        else:
            return folder_two
    elif os.path.isdir(folder_one):
        return folder_one
    elif os.path.isdir(folder_two):
        return folder_two
    else:
        return


def newest_dir(dirs: [str]):
    assert len(dirs) > 0
    newest = dirs[0]
    for dir in dirs:
        if newer_folder(newest, dir) == dir:
            newest = dir
    return newest


def get_base_folder(repos_root_dir):
    old_base_debug_build = os.path.join(repos_root_dir, "everest-base/cmake-build-debug/distribution")
    old_base_release_build = os.path.join(repos_root_dir, "everest-base/cmake-build-release/distribution")
    base_debug_build = os.path.join(repos_root_dir, f"{BASE_REPO_DIR_NAME}/cmake-build-debug/distribution")
    base_release_build = os.path.join(repos_root_dir, f"{BASE_REPO_DIR_NAME}/cmake-build-release/distribution")
    return newest_dir([old_base_debug_build, old_base_release_build, base_debug_build, base_release_build])


def update_plugin_info_with_dirs(plugin_list, repos_root_dir):
    for p in plugin_list:
        plugin_build = os.path.join(repos_root_dir, p["dir"], "build64/sdds")
        plugin_debug_build = os.path.join(repos_root_dir, p["dir"], "cmake-build-debug/sdds")
        av_variant_plugin_debug_build = os.path.join(repos_root_dir, p["dir"], "build64-Debug/sdds")
        av_variant_plugin_release_build = os.path.join(repos_root_dir, p["dir"], "build64-RelWithDebInfo/sdds")
        p["build_dir"] = newest_dir(
            [plugin_build, plugin_debug_build, av_variant_plugin_debug_build, av_variant_plugin_release_build])


def find_repos_root_dir(current_dir: str) -> str:
    current_user = os.environ.get('USER')
    find_result = sp.run(["find", f"/home/{current_user}", "-name", ".git"], text=True, capture_output=True)
    stdout_str = find_result.stdout
    repos = stdout_str.splitlines()
    for repo_path in repos:
        if repo_path.endswith(SSPL_TOOLS_REPO_DIR_NAME+"/.git"):
            return str(os.path.dirname(os.path.dirname(repo_path)))


def copy_file_from_vagrant_machine_to_host(vagrant_wrapper_dir: str, src: str, dest: str):
    vagrant_wrapper = os.path.join(vagrant_wrapper_dir, 'vagrant')
    sp.check_call([vagrant_wrapper, 'scp', src, dest, ], stdout=sp.DEVNULL, stderr=sp.STDOUT)


# Create the temp file that will be executed inside the vagrant machine for Base tests
def generate_base_test_script(remote_dir: str, base_folder_vagrant: str, env_variables: str, quoted_args: [str]) -> str:
    if os.environ.get("NO_GATHER"):
        gather = ""
        av = ""
    else:
        gather = f"TEST_UTILS=/vagrant/{BASE_REPO_DIR_NAME}/testUtils  sudo -HE /vagrant/{BASE_REPO_DIR_NAME}/testUtils/SupportFiles/jenkins/gatherTestInputs.sh"
        av = f"sudo -E /vagrant/{BASE_REPO_DIR_NAME}/testUtils/libs/DownloadAVSupplements.py"
    temp_file_content = f"""#!/bin/bash
set -ex
cd {remote_dir}
export BASE_DIST={base_folder_vagrant}
# Gather command
{gather}
# AV supplements command
{av}
# Env vars
{env_variables}
# Set python path
export PYTHONPATH=/vagrant/{BASE_REPO_DIR_NAME}/testUtils/libs/:/vagrant/{BASE_REPO_DIR_NAME}/testUtils/SupportFiles/:$PYTHONPATH
# Run robot
sudo -E /usr/bin/python3 -m robot {' '.join(quoted_args)}
"""
    return temp_file_content


# Create the temp file that will be executed inside the vagrant machine for AV tests
def generate_av_test_script(remote_dir: str, av_folder_vagrant: str, env_variables: str, quoted_args: [str]) -> str:
    quoted_args.insert(0, "WUKS")
    quoted_args.insert(0, "--removekeywords")
    temp_file_content = f"""#!/bin/bash
set -ex
cd {remote_dir}
if [[ ! -f "/tmp/ran_av_setup_vagrant_script" ]]
then
    echo "Running AV vagrant setup script"
    pushd "{av_folder_vagrant}/TA"
        chmod +x bin/setupVagrant.sh
        sudo bin/setupVagrant.sh
    popd
fi
# Env vars
{env_variables}
# Run robot
sudo -E /usr/bin/python3 -m robot {' '.join(quoted_args)}
"""
    return temp_file_content


# Create the temp file that will be executed inside the vagrant machine for EDR tests
def generate_edr_test_script(remote_dir: str, edr_folder_vagrant: str, env_variables: str, quoted_args: [str]) -> str:
    temp_file_content = f"""#!/bin/bash
set -ex
cd {remote_dir}
if [[ ! -f "/tmp/ran_edr_setup_vagrant_script" ]]
then
    echo "Running EDR vagrant setup script"
    pushd "{edr_folder_vagrant}/TA"
        chmod +x bin/setupVagrant.sh
        sudo bin/setupVagrant.sh
    popd
fi
# Env vars
{env_variables}
# Run robot
sudo -E /usr/bin/python3 -m robot {' '.join(quoted_args)}
"""
    return temp_file_content

# Create the temp file that will be executed inside the vagrant machine for Event Journaler tests
def generate_event_journaler_test_script(remote_dir: str, journaler_folder_vagrant: str, env_variables: str, quoted_args: [str]) -> str:
    temp_file_content = f"""#!/bin/bash
set -ex
cd {remote_dir}
if [[ ! -f "/tmp/ran_event_journaler_setup_vagrant_script" ]]
then
    echo "Running Event Journaler vagrant setup script"
    pushd "{journaler_folder_vagrant}/TA"
        chmod +x bin/setupVagrant.sh
        sudo bin/setupVagrant.sh
    popd
fi
# Env vars
{env_variables}
# Run robot
sudo -E /usr/bin/python3 -m robot {' '.join(quoted_args)}
"""
    return temp_file_content


def main():
    current_dir = os.path.abspath(os.getcwd())
    print(f"Current dir: {current_dir}")

    # Where are we running this script from, different repos have different test setup steps.
    current_repo = BASE_REPO_DIR_NAME
    if BASE_REPO_DIR_NAME in current_dir:
        current_repo = BASE_REPO_DIR_NAME
    elif AV_REPO_DIR_NAME in current_dir:
        current_repo = AV_REPO_DIR_NAME
    elif EDR_REPO_DIR_NAME in current_dir:
        current_repo = EDR_REPO_DIR_NAME
    elif EVENT_JOURNALER_REPO_DIR_NAME in current_dir:
        current_repo = EVENT_JOURNALER_REPO_DIR_NAME

    # repos_root is where all SPL repos are located, usually /home/$USER/gitrepos/
    repos_root = find_repos_root_dir(current_dir)
    print(f"Repos root dir: {repos_root}")

    sspl_tools_repo_dir = os.path.join(repos_root, SSPL_TOOLS_REPO_DIR_NAME)

    # vagrant_root is where the .vagrant file is located
    vagrant_root = find_vagrant_root(sspl_tools_repo_dir)
    print(f"Vagrant root dir: {vagrant_root}")

    vagrant_shared_dir = repos_root
    print(f"Vagrant shared dir: {vagrant_shared_dir}")

    if current_repo == BASE_REPO_DIR_NAME:
        # Find the latest build directory for base
        base_folder = get_base_folder(repos_root)
        if base_folder:
            print("Using {} folder for base".format(base_folder))
        else:
            print("Cannot find folder for everest-base")
            exit(1)
        base_folder_vagrant = "/vagrant" + base_folder[len(repos_root):]
        print(f"Base folder = {base_folder}")
        print(f"Base folder on vagrant = {base_folder_vagrant}")

    def get_plugin_info():
        plugins_info = get_plugin_names(vagrant_root)
        update_plugin_info_with_dirs(plugins_info, repos_root)
        return plugins_info

    plugins = get_plugin_info()

    env_variables = ""

    for plugin in plugins:
        local_folder = plugin["build_dir"]
        if os.path.isdir(local_folder):
            print(f'Using {local_folder} folder for {plugin["name"]}')
            plugin["build_dir_from_vagrant"] = "/vagrant" + local_folder[len(repos_root):]
            env_variables += "export " + plugin["envName"] + "=" + plugin["build_dir_from_vagrant"] + "\n"
        else:
            print(f'Cannot find folder for {plugin["name"]}, looked here: {plugin["build_dir"]}')

    # Translate the arguments from the host machine to the vagrant machine
    remote_args = []
    exclude_manual = True
    exclude_slow = True
    for arg in sys.argv[1:]:
        if os.path.exists(arg) and os.path.isabs(arg):
            if vagrant_shared_dir in arg:
                remote_args.append(arg.replace(vagrant_shared_dir, '/vagrant'))
            raise AttributeError("Path parameter not in the vagrantroot is not supported: Arg=" + arg)
        elif arg == "--no-exclude-manual":
            exclude_manual = False
        elif arg == "--no-exclude-slow":
            exclude_slow = False
        else:
            remote_args.append(arg)

    # Add common parameters used most of the time
    if exclude_manual and "--include=manual" not in remote_args:
        remote_args.insert(0, '--exclude=manual')
    if exclude_slow and "--include=slow" not in remote_args:
        remote_args.insert(0, '--exclude=slow')

    # Add quotes to args
    quoted_args = [add_quote(a) for a in remote_args]

    # Find where the current directory (where someone ran ./robot -t "test" . from) will be
    # when mapped to the vagrant machine
    if vagrant_shared_dir in current_dir:
        remote_dir = current_dir.replace(vagrant_shared_dir, '/vagrant')
        print(f"remote dir = {remote_dir}")
    else:
        # Change directory to the vagrant root
        os.chdir(vagrant_shared_dir)
        print(f"Changed dir to {vagrant_shared_dir}")
        remote_dir = os.path.join('/vagrant', current_dir[1:])
        print(remote_dir)

    print("Checking vagrant machine status...")
    check_vagrant_up_and_running(vagrant_root)

    # If we're running base tests then we need filer6 for the SDDS specs
    if current_repo == BASE_REPO_DIR_NAME:
        print("Based on cwd assuming we're running base tests")
        sdds_specs_dir_source = "/mnt/filer6/linux/SSPL/testautomation/sdds-specs"
        if not os.path.isdir(sdds_specs_dir_source):
            print("Mounting filer6 for SDDS specs")
            sp.check_call(["mount", "/mnt/filer6/linux/"])
            if not os.path.isdir(sdds_specs_dir_source):
                print(f"Make sure filer6 is mounted on WSL so the SDDS specs are accessible: {sdds_specs_dir_source}")
                return

    # Create the temp file that will be executed inside the vagrant machine
    if current_repo == BASE_REPO_DIR_NAME:
        temp_file_content = generate_base_test_script(remote_dir, base_folder_vagrant, env_variables, quoted_args)
    elif current_repo == AV_REPO_DIR_NAME:
        av_dir_on_vagrant = f"/vagrant/{AV_REPO_DIR_NAME}"
        # AV needs to run tests from TA dir so hard code the remote_dir
        # remap test location arg, assuming last arg is the dir of the tests we want to run
        test_dir_absolute = os.path.join(remote_dir, quoted_args[-1])
        quoted_args[-1] = add_quote(test_dir_absolute)
        remote_dir = f"{av_dir_on_vagrant}/TA"
        temp_file_content = generate_av_test_script(remote_dir, av_dir_on_vagrant, env_variables, quoted_args)
    elif current_repo == EDR_REPO_DIR_NAME:
        edr_dir_on_vagrant = f"/vagrant/{EDR_REPO_DIR_NAME}"
        # EDR also needs to run tests from TA dir so hard code the remote_dir
        # remap test location arg, assuming last arg is the dir of the tests we want to run
        test_dir_absolute = os.path.join(remote_dir, quoted_args[-1])
        quoted_args[-1] = add_quote(test_dir_absolute)
        remote_dir = f"{edr_dir_on_vagrant}/TA"
        temp_file_content = generate_edr_test_script(remote_dir, edr_dir_on_vagrant, env_variables, quoted_args)
    elif current_repo == EVENT_JOURNALER_REPO_DIR_NAME:
        journaler_dir_on_vagrant = f"/vagrant/{EDR_REPO_DIR_NAME}"
        # EDR also needs to run tests from TA dir so hard code the remote_dir
        # remap test location arg, assuming last arg is the dir of the tests we want to run
        test_dir_absolute = os.path.join(remote_dir, quoted_args[-1])
        quoted_args[-1] = add_quote(test_dir_absolute)
        remote_dir = f"{journaler_dir_on_vagrant}/TA"
        temp_file_content = generate_edr_test_script(remote_dir, journaler_dir_on_vagrant, env_variables, quoted_args)

    tmp_file_on_host = os.path.join(vagrant_root, 'tmpscript.sh')
    tmp_file_on_guest = os.path.join('/vagrant/', SSPL_TOOLS_REPO_DIR_NAME, 'tmpscript.sh')

    with open(tmp_file_on_host, 'w') as f:
        f.write(temp_file_content)
        print(f"Wrote test execution file {tmp_file_on_host}")

    vagrant_rsync(vagrant_root)

    # Run the command in the vagrant machine
    print("Running test script on Vagrant...")
    vagrant_wrapper = os.path.join(vagrant_root, "vagrant")
    vagrant_cmd = [vagrant_wrapper, 'ssh', 'ubuntu', '-c', f"bash {tmp_file_on_guest}"]
    sp.call(vagrant_cmd)
    os.chdir(current_dir)

    # Copy log back to dev machine, so it can be viewed
    copy_file_from_vagrant_machine_to_host(vagrant_root, f"ubuntu:{remote_dir}/log.html", current_dir)
    print("View log file by running:")
    # From WSL open in default Windows browser
    print("Explorer.exe log.html")


if __name__ == '__main__':
    main()
