#!/usr/bin/env python3
import sys
import os
import subprocess as sp
import glob

# auxiliary method to add quotes to arguments
def add_quote(inp):
    if inp[0] == '"' or inp[0] == "'":
        return inp
    else:
        return '"' + inp + '"'


# auxiliary method to find the VagrantRoot folder (where the Vagrantfile is)
def find_vagrant_root(start_from_dir):
    curr = start_from_dir
    while not os.path.exists(os.path.join(curr, 'Vagrantfile')):
        curr = os.path.split(curr)[0]
        if curr in ['/', '/home']:
            raise AssertionError('Failed to find Vagrantfile in directory or its parents: {}'.format(start_from_dir))
    return curr


def check_vagrant_up_and_running():
    output = sp.check_output(['/usr/bin/vagrant', 'status'])
    if b'running' not in output:
        print ('starting up vagrant')
        sp.call(['/usr/bin/vagrant', 'up', 'ubuntu'])

def get_plugin_names():
    repos = []
    with open(os.path.join(currdir, "../../setup/gitRepos.txt")) as f:
        for line in f.readlines():
            plugin = {}
            line = line.strip()
            if "plugin" in line.lower():
                plugin_name = line.split("/")[-1].split(".")[0]
                plugin["name"] = plugin_name
                plugin["envName"] = plugin_name.upper().replace("-","_") + "_SDDS"
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

def get_base_folder():
    baseBuild = os.path.join(VAGRANTROOT, "everest-base/build64/distribution")
    baseDebugBuild = os.path.join(VAGRANTROOT, "everest-base/cmake-build-debug/distribution")
    return newer_folder(baseBuild, baseDebugBuild)

def get_plugin_folders(plugin_list):
    for plugin in plugin_list:
        pluginBuild = os.path.join(VAGRANTROOT, plugin["name"], "build64/sdds")
        pluginDebugBuild = os.path.join(VAGRANTROOT, plugin["name"], "cmake-build-debug/sdds")
        plugin["folder"] = newer_folder(pluginBuild, pluginDebugBuild)

currdir = os.path.abspath(os.getcwd())

VAGRANTROOT = find_vagrant_root(currdir)

"""
Find latest build directory for base and listed plugins, out of build64 (generated by ./build.sh) and cmake-build-debug
(generated by CLion / running 'make install dist' inside the cmake-build-debug folder).
"""

base_folder = get_base_folder()

if base_folder:
    print("Using {} folder for base".format(base_folder))
else:
    print("Cannot find folder for everest-base")
base_folder = "/vagrant" + base_folder[len(VAGRANTROOT):]

plugins = get_plugin_names()
get_plugin_folders(plugins)

env_variables = ""

for plugin in plugins:
    local_folder = plugin["folder"]
    if local_folder:
        print("Using {} folder for {}".format(local_folder, plugin["name"]))
        plugin["folder"] = "/vagrant" + local_folder[len(VAGRANTROOT):]
        env_variables += "export " + plugin["envName"] + "=" + plugin["folder"] + "\n"
    else:
        print("Cannot find folder for {}".format(plugin["name"]))

# translate the arguments from the host machine to the vagrant machine
remote_args = []
for arg in sys.argv[1:]:
    if os.path.exists(arg) and os.path.isabs(arg):
        if VAGRANTROOT in arg:
            remote_args.append(arg.replace(VAGRANTROOT, '/vagrant'))
        raise AttributeError("Path parameter not in the vagrantroot is not supported: Arg=" + arg)
    else:
        remote_args.append(arg)


# add common parameters used most of times
if "--include=manual" not in remote_args:
    remote_args.insert(0, '--exclude=manual')
if "--include=slow" not in remote_args:
    remote_args.insert(0, '--exclude=slow')


# add quotes to make it safer
quoted_args = [add_quote(a) for a in remote_args]

# define where the current directory will be in the vagrant machine
if VAGRANTROOT in currdir:
    remotedir = currdir.replace(VAGRANTROOT, '/vagrant')
else:
    # change directory to the vagrant root otherwise 'vagrant ssh' will not work
    os.chdir(VAGRANTROOT)
    remotedir = os.path.join('/vagrant', currdir[1:])

check_vagrant_up_and_running()

# create the temp file that will be executed inside the vagrant machine

if os.environ.get("NO_GATHER"):
    gather = ""
else:
    gather = "TEST_UTILS=/vagrant/everest-base/testUtils  sudo -E /vagrant/everest-base/testUtils/SupportFiles/jenkins/gatherTestInputs.sh"

tempfileContent = f"""#!/bin/bash
pushd {remotedir}
export BASE_DIST={base_folder}
{gather}
{env_variables}
export PYTHONPATH=/vagrant/everest-base/testUtils/libs/:/vagrant/everest-base/testUtils/SupportFiles/:$PYTHONPATH
sudo -E /usr/bin/python3 -m robot {' '.join(quoted_args)}
popd
"""

hostfile = os.path.join(VAGRANTROOT, 'tmpscript.sh')
remotefile = os.path.join('/vagrant', 'tmpscript.sh')

with open(hostfile, 'w') as f:
    f.write(tempfileContent)

# run the command in the vgagrant machine
vagrant_cmd = ['/usr/bin/vagrant', 'ssh', 'ubuntu', '-c', 'bash %s' % remotefile]

sp.call(vagrant_cmd)
os.chdir(currdir)
