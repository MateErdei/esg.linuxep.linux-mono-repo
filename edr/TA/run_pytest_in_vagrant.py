import os
import subprocess

def find_vagrant_root():
    curr = os.path.abspath(os.getcwd())
    while not os.path.exists(os.path.join(curr, 'Vagrantfile')):
        curr = os.path.split(curr)[0]
        if curr in ['/', '/home']:
            raise AssertionError('Failed to find Vagrantfile in directory or its parents')
    return curr

tempfileContent = """#!/bin/bash
mkdir -p /opt/test/inputs/edr
if [[ ! -L /opt/test/inputs/test_scripts ]]
then
    echo 'creating symlink'
    ln -s /vagrant/sspl-plugin-edr-component/TA  /opt/test/inputs/test_scripts
fi

if [[ ! -L /opt/test/inputs/edr/SDDS-COMPONENT ]]
then
    echo 'creating symlink'
    ln -s /vagrant/sspl-plugin-edr-component/output/SDDS-COMPONENT/  /opt/test/inputs/edr/SDDS-COMPONENT
fi

if [[ ! -L /opt/test/inputs/edr/componenttests ]]
then
    echo 'creating symlink'
    ln -s /vagrant/sspl-plugin-edr-component/output/componenttests/  /opt/test/inputs/edr/componenttests
fi

pushd /vagrant/sspl-plugin-edr-component/TA
sudo -E python3 -u -m pytest
popd
"""
VAGRANTROOT = find_vagrant_root()
hostfile = os.path.join(VAGRANTROOT, 'tmpscript.sh')
remotefile = os.path.join('/vagrant', 'tmpscript.sh')

with open(hostfile, 'w') as f:
    f.write(tempfileContent)

# run the command in the vagrant machine
vagrant_cmd = ['/usr/bin/vagrant', 'ssh', 'ubuntu', '-c', 'bash %s' % remotefile]

subprocess.call(vagrant_cmd)