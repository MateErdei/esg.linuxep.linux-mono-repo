#!/usr/bin/env python3
import sys
import os
import subprocess as sp
import glob

MONO_REPO = "esg.linuxep.linux-mono-repo"
BASE_DIR_NAME = "base"
COMMON_DIR_NAME = "common"
SPL_TOOLS_DIR_NAME = "spl-tools"
AV_DIR_NAME = "av"
EDR_DIR_NAME = "edr"
EVENT_JOURNALER_DIR_NAME = "eventjournaler"
DEVICE_ISOLATION_DIR_NAME = "deviceisolation"
LIVETERMINAL_DIR_NAME = "liveterminal"

BASE_ON_VAGRANT=os.path.join("/vagrant", MONO_REPO, BASE_DIR_NAME)
COMMON_ON_VAGRANT=os.path.join("/vagrant", MONO_REPO, COMMON_DIR_NAME)
AV_ON_VAGRANT=os.path.join("/vagrant", MONO_REPO, AV_DIR_NAME)
EDR_ON_VAGRANT=os.path.join("/vagrant", MONO_REPO, EDR_DIR_NAME)
EJ_ON_VAGRANT=os.path.join("/vagrant", MONO_REPO, EVENT_JOURNALER_DIR_NAME)
DI_ON_VAGRANT=os.path.join("/vagrant", MONO_REPO, DEVICE_ISOLATION_DIR_NAME)
LT_ON_VAGRANT=os.path.join("/vagrant", MONO_REPO, LIVETERMINAL_DIR_NAME)
TOOLS_ON_VAGRANT=os.path.join("/vagrant", MONO_REPO, SPL_TOOLS_DIR_NAME)


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


def get_plugin_names():
    plugins = [
        {
            'name': 'sspl-plugin-anti-virus',
            'dir': 'av',
            'envName': 'SSPL_PLUGIN_ANTI_VIRUS_SDDS'
        },
        {
            'name': 'sspl-plugin-edr-component',
            'dir': 'edr',
            'envName': 'SSPL_PLUGIN_EDR_COMPONENT_SDDS'
        },
        {
            'name': 'sspl-plugin-event-journaler',
            'dir': 'eventjournaler',
            'envName': 'SSPL_PLUGIN_EVENT_JOURNALER_SDDS'
        },
        {
            'name': 'sspl-plugin-device-isolation',
            'dir': 'deviceisolation',
            'envName': 'SSPL_PLUGIN_DEVICE_ISOLATION_SDDS'
        },
        {
            'name': 'sspl-plugin-liveterminal',
            'dir': 'liveterminal',
            'envName': 'SSPL_PLUGIN_LIVETERMINAL_SDDS'
        },
    ]
    return plugins



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


def get_base_folder(root_dir):
    base_dir = os.path.join(root_dir, BASE_DIR_NAME)
    base_debug_build = os.path.join(base_dir, "cmake-build-debug/distribution")
    base_release_build = os.path.join(base_dir, "cmake-build-release/distribution")
    return newest_dir([base_debug_build, base_release_build])

def update_plugin_info_with_dirs(plugin_list, mono_repo_dir):
    for p in plugin_list:
        plugin_build = os.path.join(mono_repo_dir, p["dir"], "build64/sdds")
        plugin_debug_build = os.path.join(mono_repo_dir, p["dir"], "cmake-build-debug/sdds")
        av_variant_plugin_debug_build = os.path.join(mono_repo_dir, p["dir"], "build64-Debug/sdds")
        av_variant_plugin_release_build = os.path.join(mono_repo_dir, p["dir"], "build64-RelWithDebInfo/sdds")
        p["build_dir"] = newest_dir(
            [plugin_build, plugin_debug_build, av_variant_plugin_debug_build, av_variant_plugin_release_build])


def matching_starting_chars(string1, string2):
    shortest_len = min(len(string1), len(string2))
    matching_chars = 0
    for i in range(shortest_len):
        if string1[i] == string2[i]:
            matching_chars += 1
    return matching_chars


def find_mono_repo_root_dir(current_dir: str) -> str:
    this_file = os.path.realpath(__file__)
    this_dir = os.path.dirname(this_file)
    current_user = os.environ.get('USER')
    find_result = sp.run(["find", f"/home/{current_user}", "-name", ".git"], text=True, capture_output=True)
    stdout_str = find_result.stdout
    repos = stdout_str.splitlines()
    closest_path = ""
    best_match = 0
    for repo_path in repos:
        if repo_path.endswith(MONO_REPO + "/.git"):
            matching_start_of_path = matching_starting_chars(repo_path, this_dir)
            if matching_start_of_path > best_match:
                best_match = matching_start_of_path
                closest_path = os.path.dirname(repo_path)
    return closest_path


def copy_file_from_vagrant_machine_to_host(vagrant_wrapper_dir: str, src: str, dest: str):
    vagrant_wrapper = os.path.join(vagrant_wrapper_dir, 'vagrant')
    sp.check_call([vagrant_wrapper, 'scp', src, dest, ], stdout=sp.DEVNULL, stderr=sp.STDOUT)


# Create the temp file that will be executed inside the vagrant machine for Base tests
def generate_base_test_script(remote_dir: str, base_folder_vagrant: str, common_folder_vagrant: str, env_variables: str, quoted_args: [str]) -> str:
    if os.environ.get("NO_GATHER"):
        gather = ""
        av = ""
    else:
        gather = f"TEST_UTILS={base_folder_vagrant}/testUtils  sudo -HE {base_folder_vagrant}/testUtils/SupportFiles/jenkins/gatherTestInputs.sh"
        av = f"sudo -E {common_folder_vagrant}/TA/libs/DownloadAVSupplements.py"
    temp_file_content = f"""#!/bin/bash
set -ex
cd {remote_dir}
export BASE_DIST={base_folder_vagrant}
sudo ln -snf {base_folder_vagrant}/testUtils /opt/test/inputs/testUtils
sudo ln -snf {base_folder_vagrant}/testUtils/SupportFiles /opt/test/inputs/SupportFiles
# Gather command
{gather}
# AV supplements command
{av}
# Env vars
{env_variables}
# Create symlinks
sudo ln -snf "{common_folder_vagrant}/TA/libs" /opt/test/inputs/common_test_libs
sudo ln -snf "{common_folder_vagrant}/TA/robot" /opt/test/inputs/common_test_robot
sudo ln -snf "{base_folder_vagrant}/testUtils/SupportFiles" /opt/test/inputs/SupportFiles
# Create testUtilsMarker
sudo touch /opt/test/inputs/testUtilsMarker
# Run robot
sudo -E /usr/bin/python3 -m robot --pythonpath=/opt/test/inputs/common_test_libs {' '.join(quoted_args)}
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
# Set python path
export PYTHONPATH=/opt/test/inputs/common_test_libs
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
# Set python path
export PYTHONPATH=/opt/test/inputs/common_test_libs
# Run robot
sudo -E /usr/bin/python3 -m robot {' '.join(quoted_args)}
"""
    return temp_file_content


# Create the temp file that will be executed inside the vagrant machine for Event Journaler tests
def generate_event_journaler_test_script(remote_dir: str, journaler_folder_vagrant: str, env_variables: str,
                                         quoted_args: [str]) -> str:
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
# Set python path
export PYTHONPATH=/opt/test/inputs/common_test_libs
# Run robot
sudo -E /usr/bin/python3 -m robot {' '.join(quoted_args)}
"""
    return temp_file_content


# Create the temp file that will be executed inside the vagrant machine for Event Journaler tests
def generate_device_isolation_test_script(remote_dir: str, device_isolation_folder_vagrant: str, env_variables: str,
                                         quoted_args: [str]) -> str:
    temp_file_content = f"""#!/bin/bash
set -ex
cd {remote_dir}
if [[ ! -f "/tmp/ran_device_isolation_setup_vagrant_script" ]]
then
    echo "Running Event Journaler vagrant setup script"
    pushd "{device_isolation_folder_vagrant}/TA"
        chmod +x bin/setupVagrant.sh
        sudo bin/setupVagrant.sh
    popd
fi
# Env vars
{env_variables}
# Set python path
export PYTHONPATH=/opt/test/inputs/common_test_libs
# Run robot
sudo -E /usr/bin/python3 -m robot {' '.join(quoted_args)}
"""
    return temp_file_content


def generate_liveterminal_test_script(remote_dir: str, liveterminal_folder_vagrant: str, env_variables: str,
                                         quoted_args: [str]) -> str:
    temp_file_content = f"""#!/bin/bash
set -ex
cd {remote_dir}
if [[ ! -f "/tmp/ran_liveterminal_setup_vagrant_script" ]]
then
    echo "Running Live Terminal vagrant setup script"
    pushd "{liveterminal_folder_vagrant}/TA"
        chmod +x bin/setupVagrant.sh
        sudo bin/setupVagrant.sh
    popd
fi
# Env vars
{env_variables}
# Set python path
export PYTHONPATH=/opt/test/inputs/common_test_libs
# Run robot
sudo -E /usr/bin/python3 -m robot {' '.join(quoted_args)}
"""
    return temp_file_content


def main():
    current_dir = os.path.abspath(os.getcwd())
    print(f"Current dir: {current_dir}")

    # Where are we running this script from, different components have different test setup steps.
    current_repo = BASE_DIR_NAME
    if os.path.join(MONO_REPO, BASE_DIR_NAME) in current_dir:
        current_repo = BASE_DIR_NAME
    elif os.path.join(MONO_REPO, AV_DIR_NAME) in current_dir:
        current_repo = AV_DIR_NAME
    elif os.path.join(MONO_REPO, EDR_DIR_NAME) in current_dir:
        current_repo = EDR_DIR_NAME
    elif os.path.join(MONO_REPO, EVENT_JOURNALER_DIR_NAME) in current_dir:
        current_repo = EVENT_JOURNALER_DIR_NAME
    elif os.path.join(MONO_REPO, DEVICE_ISOLATION_DIR_NAME) in current_dir:
        current_repo = DEVICE_ISOLATION_DIR_NAME
    elif os.path.join(MONO_REPO, LIVETERMINAL_DIR_NAME) in current_dir:
        current_repo = LIVETERMINAL_DIR_NAME

    mono_repo_path = find_mono_repo_root_dir(current_dir)
    print(f"Linux mono repo dir: {mono_repo_path}")

    # repos_root = find_repos_root_dir(current_dir)
    # print(f"Repos root dir: {repos_root}")

    sspl_tools_repo_dir = os.path.join(mono_repo_path, SPL_TOOLS_DIR_NAME)

    # vagrant_root is where the .vagrant file is located
    vagrant_root = find_vagrant_root(sspl_tools_repo_dir)
    print(f"Vagrant root dir: {vagrant_root}")

    vagrant_shared_dir = os.path.dirname(mono_repo_path)
    print(f"Vagrant shared dir: {vagrant_shared_dir}")

    if current_repo == BASE_DIR_NAME:
        # Find the latest build directory for base
        base_folder = get_base_folder(mono_repo_path)
        if base_folder:
            print("Using {} folder for base".format(base_folder))
        else:
            print("Cannot find folder for everest-base")
            exit(1)
        print(f"Base folder = {base_folder}")
        print(f"Base folder on vagrant = {BASE_ON_VAGRANT}")

    def get_plugin_info():
        plugins_info = get_plugin_names()
        update_plugin_info_with_dirs(plugins_info, mono_repo_path)
        return plugins_info

    plugins = get_plugin_info()

    env_variables = ""

    for plugin in plugins:
        local_folder = plugin["build_dir"]
        if os.path.isdir(local_folder):
            print(f'Using {local_folder} folder for {plugin["name"]}')
            plugin["build_dir_from_vagrant"] = os.path.join("/vagrant", MONO_REPO, local_folder[len(mono_repo_path)+1:])
            env_variables += "export " + plugin["envName"] + "=" + plugin["build_dir_from_vagrant"] + "\n"
        else:
            print(f'Cannot find folder for {plugin["name"]}, looked here: {plugin["build_dir"]}')

    # for p in plugins:
    #     print(p)



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

    # Create the temp file that will be executed inside the vagrant machine
    if current_repo == BASE_DIR_NAME:
        temp_file_content = generate_base_test_script(remote_dir, BASE_ON_VAGRANT, COMMON_ON_VAGRANT, env_variables, quoted_args)

    elif current_repo == AV_DIR_NAME:
        # AV needs to run tests from TA dir so hard code the remote_dir
        # remap test location arg, assuming last arg is the dir of the tests we want to run
        test_dir_absolute = os.path.join(remote_dir, quoted_args[-1])
        quoted_args[-1] = add_quote(test_dir_absolute)
        remote_dir = f"{AV_ON_VAGRANT}/TA"
        temp_file_content = generate_av_test_script(remote_dir, AV_ON_VAGRANT, env_variables, quoted_args)

    elif current_repo == EDR_DIR_NAME:
        # EDR also needs to run tests from TA dir so hard code the remote_dir
        # remap test location arg, assuming last arg is the dir of the tests we want to run
        test_dir_absolute = os.path.join(remote_dir, quoted_args[-1])
        quoted_args[-1] = add_quote(test_dir_absolute)
        remote_dir = f"{EDR_ON_VAGRANT}/TA"
        temp_file_content = generate_edr_test_script(remote_dir, EDR_ON_VAGRANT, env_variables, quoted_args)

    elif current_repo == EVENT_JOURNALER_DIR_NAME:
        # Event Journaler also needs to run tests from TA dir so hard code the remote_dir
        # remap test location arg, assuming last arg is the dir of the tests we want to run
        test_dir_absolute = os.path.join(remote_dir, quoted_args[-1])
        quoted_args[-1] = add_quote(test_dir_absolute)
        remote_dir = f"{EJ_ON_VAGRANT}/TA"
        temp_file_content = generate_event_journaler_test_script(remote_dir, EJ_ON_VAGRANT, env_variables, quoted_args)

    elif current_repo == DEVICE_ISOLATION_DIR_NAME:
        # Device Isolation also needs to run tests from TA dir so hard code the remote_dir
        # remap test location arg, assuming last arg is the dir of the tests we want to run
        test_dir_absolute = os.path.join(remote_dir, quoted_args[-1])
        quoted_args[-1] = add_quote(test_dir_absolute)
        remote_dir = f"{DI_ON_VAGRANT}/TA"
        temp_file_content = generate_device_isolation_test_script(remote_dir, DI_ON_VAGRANT, env_variables, quoted_args)

    elif current_repo == LIVETERMINAL_DIR_NAME:
        # Event Journaler also needs to run tests from TA dir so hard code the remote_dir
        # remap test location arg, assuming last arg is the dir of the tests we want to run
        test_dir_absolute = os.path.join(remote_dir, quoted_args[-1])
        quoted_args[-1] = add_quote(test_dir_absolute)
        remote_dir = f"{LT_ON_VAGRANT}/TA"
        temp_file_content = generate_liveterminal_test_script(remote_dir, LT_ON_VAGRANT, env_variables, quoted_args)
    else:
        raise AssertionError("Unknown Repo")

    tmp_file_on_host = os.path.join(vagrant_root, 'tmpscript.sh')
    tmp_file_on_guest = os.path.join('/vagrant/', MONO_REPO, SPL_TOOLS_DIR_NAME, 'tmpscript.sh')

    print(f"tmp_file_on_host = {tmp_file_on_host}")
    print(f"tmp_file_on_guest = {tmp_file_on_guest}")
    print(f"temp_file_content = {temp_file_content}")

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
