#!/usr/bin/env python3
import sys
import os
import subprocess as sp


SPL_TOOLS_DIR = "spl-tools"
MONO_REPO_ON_VAGRANT = "/vagrant/esg.linuxep.linux-mono-repo"
SPL_TOOLS_ON_VAGRANT = os.path.join(MONO_REPO_ON_VAGRANT, SPL_TOOLS_DIR)

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


def find_repos_root_dir(current_dir: str) -> str:
    current_user = os.environ.get('USER')
    find_result = sp.run(["find", f"/home/{current_user}", "-name", ".git"], text=True, capture_output=True)
    stdout_str = find_result.stdout
    repos = stdout_str.splitlines()
    for repo_path in repos:
        if repo_path.endswith("esg.linuxep.linux-mono-repo/.git"):
            return str(os.path.dirname(os.path.dirname(repo_path)))


def copy_file_from_vagrant_machine_to_host(vagrant_wrapper_dir: str, src: str, dest: str):
    vagrant_wrapper = os.path.join(vagrant_wrapper_dir, 'vagrant')
    sp.check_call([vagrant_wrapper, 'scp', src, dest, ], stdout=sp.DEVNULL, stderr=sp.STDOUT)


# Create the temp file that will be executed inside the vagrant machine
def generate_tap_script(remote_dir: str, env_variables: str, quoted_args: [str]) -> str:
    temp_file_content = f"""#!/bin/bash
set -ex

mkdir -p /pyvenv
if ! [[ -d /pyvenv/tapvenv ]]
then
    python3 -m venv /pyvenv/tapvenv
    cat >"/pyvenv/tapvenv/pip.conf" <<EOF
[global]
index-url = https://artifactory.sophos-ops.com/api/pypi/pypi/simple
EOF
fi
source /pyvenv/tapvenv/bin/activate
pip install pip --upgrade
pip install tap
pip install keyrings.alt
mkdir -p /opt/test
rm -rf /opt/test/inputs

cd {remote_dir}
{env_variables} tap {' '.join(quoted_args)}
"""
    return temp_file_content


def main():
    current_dir = os.path.abspath(os.getcwd())
    print(f"Current dir: {current_dir}")

    # repos_root is where all SPL repos are located, usually /home/$USER/gitrepos/
    repos_root = find_repos_root_dir(current_dir)
    print(f"Repos root dir: {repos_root}")

    sspl_tools_dir = os.path.join(repos_root, SPL_TOOLS_DIR)

    # vagrant_root is where the .vagrant file is located
    vagrant_root = find_vagrant_root(sspl_tools_dir)
    print(f"Vagrant root dir: {vagrant_root}")

    vagrant_shared_dir = repos_root
    print(f"Vagrant shared dir: {vagrant_shared_dir}")

    # Translate the arguments from the host machine to the vagrant machine
    remote_args = sys.argv[1:]

    # Add quotes to args
    quoted_args = [add_quote(a) for a in remote_args]

    # Find where the current directory will be when mapped to the vagrant machine
    if vagrant_shared_dir in current_dir:
        remote_dir = current_dir.replace(vagrant_shared_dir, '/vagrant')
        print(f"remote dir = {remote_dir}")
    else:
        # Change directory to the vagrant root
        os.chdir(vagrant_shared_dir)
        print(f"Changed dir to {vagrant_shared_dir}")
        remote_dir = os.path.join('/vagrant', current_dir[1:])
        print(remote_dir)

    env_variables = ""
    for key in os.environ:
        if key.startswith("TAP_PARAMETER_"):
            env_variables += f" {key}={add_quote(os.environ[key])}"

    temp_file_content = generate_tap_script(remote_dir, env_variables, quoted_args)

    print("Checking vagrant machine status...")
    check_vagrant_up_and_running(vagrant_root)

    tmp_file_on_host = os.path.join(vagrant_root, 'tmpscript.sh')
    tmp_file_on_guest = os.path.join(SPL_TOOLS_ON_VAGRANT, 'tmpscript.sh')

    with open(tmp_file_on_host, 'w') as f:
        f.write(temp_file_content)
        print(f"Wrote test execution file {tmp_file_on_host}")

    vagrant_rsync(vagrant_root)

    # Run the command in the vagrant machine
    print("Running test script on Vagrant...")
    vagrant_wrapper = os.path.join(vagrant_root, "vagrant")
    vagrant_cmd = [vagrant_wrapper, 'ssh', 'ubuntu', '-c', f"sudo bash {tmp_file_on_guest}"]
    sp.call(vagrant_cmd)
    os.chdir(current_dir)

    # Copy log back to dev machine, so it can be viewed
    copy_file_from_vagrant_machine_to_host(vagrant_root, f"ubuntu:/opt/test/logs/log.html", current_dir)
    print("View log file by running:")
    # From WSL open in default Windows browser
    print("Explorer.exe log.html")


if __name__ == '__main__':
    main()
