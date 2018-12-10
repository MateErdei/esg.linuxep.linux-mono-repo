#!/usr/bin/env python

import os
import subprocess as sp
import shutil

platforms = ['centos', 'amazon_linux', 'rhel', 'ubuntu']

########################################################################################################################
#
#   Payloads
#
########################################################################################################################

add_user="""
useradd testuser
"""
delete_user="""
useradd testuser
clearLogs
userdel -r testuser &> /dev/null
"""
change_password="""
useradd testuser
clearLogs
echo -e "linuxpassword\nlinuxpassword" | passwd testuser
"""
group_membership_change="""
useradd testuser
groupadd testgrp
sleep 5
clearLogs
usermod -a -G testgrp testuser
"""
failed_ssh_attempt="""
HOSTNAME=$(hostname)
sshpass -p badpassword ssh -o StrictHostKeyChecking=no vagrant@127.0.0.1 'echo succesfully sshd'
"""
successful_ssh_attempt="""
sshpass -p vagrant ssh -o StrictHostKeyChecking=no vagrant@127.0.0.1 'echo successfully sshd'
"""

payloads={'add_user': add_user,
          'delete_user':delete_user, 
          'change_password':change_password,
          'group_membership_change':group_membership_change,
          'failed_ssh_attempt':failed_ssh_attempt,
          'successful_ssh_attempt':successful_ssh_attempt}

########################################################################################################################

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
        sp.call(['/usr/bin/vagrant', 'up'])

currdir = os.path.abspath(os.getcwd())
VAGRANTROOT = os.path.join(find_vagrant_root(currdir))

# define where the current directory will be in the vagrant machine
if VAGRANTROOT in currdir:
    remotedir = currdir.replace(VAGRANTROOT, '/vagrant')
else:
    # change directory to the vagrant root otherwise 'vagrant ssh' will not work
    os.chdir(VAGRANTROOT)
    remotedir = os.path.join('/vagrant', currdir[1:])


print(VAGRANTROOT)
print(currdir)
print(remotedir)

#sp.call(['vagrant', 'destroy', '-f'])
check_vagrant_up_and_running()

platformSetup = f"""
cat > /vagrant/record_bin_events << EOF
#!/bin/bash
cat >> /vagrant/AuditEvents.bin.tmp
EOF

chmod +x /vagrant/record_bin_events

cat > /etc/audisp/plugins.d/record_bin_events << EOF
active = yes
direction = out
path = /bin/bash
type = always
format = binary
args = /vagrant/record_bin_events
EOF
service auditd restart
"""

for platform in platforms:

    newPath = os.path.join(currdir, platform)
    if os.path.isdir(newPath):
        shutil.rmtree(newPath)
    os.mkdir(newPath)

    for filePrefix, payload in payloads.items():
        # create the temp file that will be executed inside the vagrant machine
        tempfileContent = f"""
#!/bin/bash

{platformSetup}

function clearLogs()
{{
    > /var/log/audit/audit.log
    > ./{platform}/AuditEvents.bin.tmp
}}

pushd "{remotedir}"
echo "Running {filePrefix} script on {platform}!..."

clearLogs

{payload}

cp ./{platform}/AuditEvents.bin.tmp ./{platform}/{filePrefix}AuditEvents.bin
cat /var/log/audit/audit.log > ./{platform}/{filePrefix}AuditEvents.log
ausearch -i > ./{platform}/{filePrefix}AuditEventsReport.log

#Clean-up
userdel -r testuser &> /dev/null
groupdel testgrp &> /dev/null

popd
"""

        hostfile = os.path.join(VAGRANTROOT, 'tmpscript.sh')
        remotefile = os.path.join('/vagrant', 'tmpscript.sh')

        with open(hostfile, 'w') as f:
            f.write(tempfileContent)

        # run the command in the vagrant machine
        vagrant_cmd = ['/usr/bin/vagrant', 'ssh', platform, '-c', 'sudo bash %s' % remotefile]

        sp.call(vagrant_cmd)
    os.chdir(currdir)
