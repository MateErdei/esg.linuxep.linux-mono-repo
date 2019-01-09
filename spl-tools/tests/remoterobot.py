#!/usr/bin/env python
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
    if 'running' not in output:
        print ('starting up vagrant')
        sp.call(['/usr/bin/vagrant', 'up', 'ubuntu'])


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

def get_example_folder():
    exampleBuild = os.path.join(VAGRANTROOT, "exampleplugin/build64/sdds")
    exampleDebugBuild = os.path.join(VAGRANTROOT, "exampleplugin/cmake-build-debug/sdds")
    return newer_folder(exampleBuild, exampleDebugBuild)

currdir = os.path.abspath(os.getcwd())

VAGRANTROOT = find_vagrant_root(currdir)

"""
Find latest build directory for base and example plugin, out of build64 (generated by ./build.sh) and cmake-build-debug
(generated by CLion / running 'make install dist' inside the cmake-build-debug folder).
    
    To add new repo:
        1) Ensure that the robot framework has an environment variable that can be used to specify where the build folder 
           of the new repo is.
        2) Create a new function get_<repo>_folder() by copying the get_base_folder() function. 
            i)  Replace baseBuild and baseDebugBuild with the folder names of new candidates for the new repo
        3) Call the function below, and add a print statement to tell the user which folder it is using
        4) When creating the script to be run on vagrant (tempfileContent), export the variable that robot framework 
           expects with the folder.
"""

base_folder = get_base_folder()
example_folder = get_example_folder()

if base_folder:
    print("Using {} folder for base".format(base_folder))
else:
    print("Cannot find folder for everest-base")

if example_folder:
    print("Using {} folder for exampleplugin".format(example_folder))
else:
    print("Cannot find folder for exampleplugin")

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
remote_args = ['--exclude=manual', '--exclude=weekly'] + remote_args

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

base_folder = "/vagrant" + base_folder[len(VAGRANTROOT):]
example_folder = "/vagrant" + example_folder[len(VAGRANTROOT):]

# create the temp file that will be executed inside the vagrant machine
tempfileContent = """#!/bin/bash
pushd "%s"
export BASE_DIST=%s
export EXAMPLE_PLUGIN_SDDS=%s
sudo -E /usr/bin/python -m robot %s
popd
""" % (remotedir, base_folder, example_folder,  ' '.join(quoted_args))

hostfile = os.path.join(VAGRANTROOT, 'tmpscript.sh')
remotefile = os.path.join('/vagrant', 'tmpscript.sh')

with open(hostfile, 'w') as f:
    f.write(tempfileContent)

# run the command in the vagrant machine
vagrant_cmd = ['/usr/bin/vagrant', 'ssh', 'ubuntu', '-c', 'bash %s' % remotefile]

sp.call(vagrant_cmd)
os.chdir(currdir)
