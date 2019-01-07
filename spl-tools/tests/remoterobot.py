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

def get_repos():
    gitRepos = os.path.join(VAGRANTROOT, "setup/gitRepos.txt")

    if not os.path.exists(gitRepos):
        return []

    repos = []
    with open(gitRepos, "r") as f:
        for line in f.readlines():
            line = line.strip()
            if line:
                directoryName = line.split("/")[-1][:-4]
                repos.append(directoryName)
    return repos

def newer_folder(folder_one, folder_two):
    newest_timestamp_folder_one = max([os.path.getctime(file) for file in glob.glob(os.path.join(folder_one, "*"))])
    newest_timestamp_folder_two = max([os.path.getctime(file) for file in glob.glob(os.path.join(folder_two, "*"))])

    if newest_timestamp_folder_one >= newest_timestamp_folder_two:
        return folder_one
    else:
        return folder_two

def find_and_verify_build_folders(repo, build64_candidates, debug_candidates):
    for build64candidate in build64_candidates:
        build64path = os.path.join(VAGRANTROOT, repo, build64candidate)
        if os.path.exists(build64path):
            for debugcandidate in debug_candidates:
                debugpath = os.path.join(VAGRANTROOT, repo, debugcandidate)
                if os.path.exists(debugpath):
                    if newer_folder(build64path, debugpath) == debugpath:
                        print("Warning: Debug build of {} more recent than build64 folder. Please run build.sh.".format(repo))
                        return False
    return True


def check_build_folder_errors():
    build64_candidates = ["build64/distribution", "build64/sdds"]
    debug_candidates = ["cmake-build-debug/distribution", "cmake-build-debug/sdds"]

    gitRepos = get_repos()

    build_errors = False
    for repo in gitRepos:
        if not find_and_verify_build_folders(repo, build64_candidates, debug_candidates):
            build_errors = True

    return build_errors


currdir = os.path.abspath(os.getcwd())

VAGRANTROOT = find_vagrant_root(currdir)

# Ensure that the build.sh build of a component is newer than the debug build
if check_build_folder_errors():
    exit(1)

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
