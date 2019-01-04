#!/usr/bin/env python
import sys
import os
import subprocess as sp
import glob

REQUIRED_BUILDS=["everest-base", "exampleplugin"]

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
    newest_timestamp_folder_one = max([os.path.getctime(file) for file in glob.glob(os.path.join(folder_one, "*"))])
    newest_timestamp_folder_two = max([os.path.getctime(file) for file in glob.glob(os.path.join(folder_two, "*"))])

    if newest_timestamp_folder_one >= newest_timestamp_folder_two:
        return folder_one
    else:
        return folder_two

def check_build_folders():
    base_build_folder = os.path.join(VAGRANTROOT, "everest-base/build64/distribution")
    base_debug_folder = os.path.join(VAGRANTROOT, "everest-base/cmake-build-debug/distribution")
    example_plugin_build_folder = os.path.join(VAGRANTROOT, "exampleplugin/build64/sdds")
    example_plugin_debug_folder = os.path.join(VAGRANTROOT, "exampleplugin/cmake-build-debug/sdds")
    if os.path.exists(base_build_folder):
        if newer_folder(base_build_folder, base_debug_folder) == base_debug_folder:
            print("Warning: Debug build of Everest Base more recent than build64 folder")
            exit(1)
    else:
        print("Warning: build64 folder does not exist in Everest Base")
        exit(1)

    if os.path.exists(example_plugin_build_folder):
        if newer_folder(example_plugin_build_folder, example_plugin_debug_folder) == example_plugin_debug_folder:
            print("Warning: Debug build of ExamplePlugin more recent than build64 folder")
            exit(1)
    else:
        print("Warning: build64 folder does not exist in ExamplePlugin")
        exit(1)

currdir = os.path.abspath(os.getcwd())

VAGRANTROOT = find_vagrant_root(currdir)

check_build_folders()

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

# create the temp file that will be executed inside the vagrant machine
tempfileContent = """#!/bin/bash
pushd "%s"
sudo /usr/bin/python -m robot %s
popd
""" % (remotedir, ' '.join(quoted_args))

hostfile = os.path.join(VAGRANTROOT, 'tmpscript.sh')
remotefile = os.path.join('/vagrant', 'tmpscript.sh')

with open(hostfile, 'w') as f:
    f.write(tempfileContent)

# run the command in the vagrant machine
vagrant_cmd = ['/usr/bin/vagrant', 'ssh', 'ubuntu', '-c', 'bash %s' % remotefile]

sp.call(vagrant_cmd)
os.chdir(currdir)
