#!/usr/bin/env python

import os
import subprocess as sp
import shutil
import sys

platforms = ['centos', 'amazon_linux', 'rhel', 'ubuntu']

########################################################################################################################
#
#   Payloads
#
########################################################################################################################

add_user = """
useradd testuser
"""
delete_user = """
useradd testuser
clearLogs
userdel -r testuser &> /dev/null
"""
change_password = """
useradd testuser
clearLogs
echo -e "linuxpassword\nlinuxpassword" | passwd testuser
"""
group_membership_change = """
useradd testuser
groupadd testgrp
sleep 5
clearLogs
usermod -a -G testgrp testuser
"""
failed_ssh_attempt = """
useradd testuser
echo -e "linuxpassword\nlinuxpassword" | passwd testuser
sleep 5
clearLogs
sshpass -p badpassword ssh -o StrictHostKeyChecking=no testuser@127.0.0.1 'echo succesfully sshd'
"""
successful_ssh_attempt = """
useradd testuser
echo -e "linuxpassword\nlinuxpassword" | passwd testuser
sleep 5
clearLogs
sshpass -p linuxpassword ssh -o StrictHostKeyChecking=no testuser@127.0.0.1 'echo successfully sshd'
"""

payloads = {'add_user': add_user,
            'delete_user': delete_user,
            'change_password': change_password,
            'group_membership_change': group_membership_change,
            'failed_ssh_attempt': failed_ssh_attempt,
            'successful_ssh_attempt': successful_ssh_attempt}

########################################################################################################################

########################################################################################################################
#
#   Script Templates
#
########################################################################################################################

payloadRunnerScript = """
#!/bin/bash

export REMOTE_DIR="/vagrant/auditd-output/{platform}"

function clearLogs()
{{
    > /var/log/audit/audit.log
    > /root/AuditEvents.bin.tmp
}}

pushd "{remotedir}"
echo "Running {filePrefix} script on {platform}!..."

clearLogs

{payload}

cp /root/AuditEvents.bin.tmp ${{REMOTE_DIR}}/{filePrefix}AuditEvents.bin
cat /var/log/audit/audit.log > ${{REMOTE_DIR}}/{filePrefix}AuditEvents.log
ausearch -i > ${{REMOTE_DIR}}/{filePrefix}AuditEventsReport.log

#Clean-up
userdel -r testuser &> /dev/null
groupdel testgrp &> /dev/null

popd
"""

platformSetupScript = """
#!/bin/bash

export REMOTE_DIR="/vagrant/auditd-output/{platform}"

cat > /root/record_bin_events << EOF
#!/bin/bash
cat >> /root/AuditEvents.bin.tmp
EOF

chmod +x /root/record_bin_events

cat > /etc/audisp/plugins.d/record_bin_events << EOF
active = yes
direction = out
path = /root/record_bin_events
type = always
format = binary
EOF
service auditd restart

echo "\nMatch LocalAddress 127.0.0.1\n\tPasswordAuthentication yes" >> /etc/ssh/sshd_config
service sshd restart

setenforce Permissive
"""

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


def vagrant_rsync(platform):
    sp.call(['/usr/bin/vagrant', 'rsync', platform])


def vagrant_pull_data(platform, localPath):
    sp.call(['/usr/bin/vagrant', 'scp',  f"{platform}:/vagrant/auditd-output/{platform}/*", localPath], stdout=sp.PIPE)


def vagrant_run(platform, bashString):
    hostfile = os.path.join(VAGRANTROOT, 'tmpscript.sh')
    remotefile = os.path.join('/vagrant', 'tmpscript.sh')

    with open(hostfile, 'w') as f:
        f.write(bashString)

    if platform == "amazon_linux":
        vagrant_rsync(platform)

    vagrant_cmd = ['/usr/bin/vagrant', 'ssh', platform, '-c', 'sudo bash %s' % remotefile]
    sp.call(vagrant_cmd)


def run_payload(platform, filePrefix, payload, remotedir):
    tempfileContent = payloadRunnerScript.format(filePrefix=filePrefix,
                                                 payload=payload,
                                                 platform=platform,
                                                 remotedir=remotedir)
    vagrant_run(platform, tempfileContent)


def main():
    # define where the current directory will be in the vagrant machine
    if VAGRANTROOT in currdir:
        remotedir = currdir.replace(VAGRANTROOT, '/vagrant')
    else:
        # change directory to the vagrant root otherwise 'vagrant ssh' will not work
        os.chdir(VAGRANTROOT)
        remotedir = os.path.join('/vagrant', currdir[1:])

    sp.call(['vagrant', 'destroy', '-f'])
    check_vagrant_up_and_running()

    for platform in platforms:

        vagrant_run(platform, platformSetupScript.format(platform=platform))

        newPath = os.path.join(currdir, platform)
        if os.path.isdir(newPath):
            shutil.rmtree(newPath)
        os.mkdir(newPath)

        for filePrefix, payload in payloads.items():
            run_payload(platform, filePrefix, payload, remotedir)

            if platform == "amazon_linux":
                vagrant_pull_data(platform, newPath)

        os.chdir(currdir)


if __name__ == "__main__":
    currdir = os.path.abspath(os.getcwd())
    VAGRANTROOT = os.path.join(find_vagrant_root(currdir))
    sys.exit(main())
