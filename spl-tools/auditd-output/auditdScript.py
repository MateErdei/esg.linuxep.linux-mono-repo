#!/usr/bin/env python3

import os
import re
import subprocess as sp
import shutil
import sys

platforms = ['centos', 'amazon_linux', 'rhel', 'ubuntu']
#platforms = ['amazon_linux']

########################################################################################################################
#
#   Payloads (Including Auditd Rules)
#
########################################################################################################################

add_user = """
auditctl -w /etc/group -p wa -k CFG_group
auditctl -w /etc/passwd -p wa -k CFG_passwd
auditctl -w /etc/gshadow -p rwxa -k CFG_gshadow
auditctl -w /etc/shadow -p rwxa -k CFG_shadow
auditctl -w /etc/security/opasswd -p rwxa -k CFG_opasswd
clearLogs
useradd testuser
"""

add_user_without_home_directory = """
auditctl -w /etc/group -p wa -k CFG_group
auditctl -w /etc/passwd -p wa -k CFG_passwd
auditctl -w /etc/gshadow -p rwxa -k CFG_gshadow
auditctl -w /etc/shadow -p rwxa -k CFG_shadow
auditctl -w /etc/security/opasswd -p rwxa -k CFG_opasswd
clearLogs
useradd -M testuser_no_home
"""

add_user_with_quote = r"""
auditctl -w /etc/group -p wa -k CFG_group
auditctl -w /etc/passwd -p wa -k CFG_passwd
auditctl -w /etc/gshadow -p rwxa -k CFG_gshadow
auditctl -w /etc/shadow -p rwxa -k CFG_shadow
auditctl -w /etc/security/opasswd -p rwxa -k CFG_opasswd
clearLogs
useradd test\"user
"""

add_user_with_single_quote = r"""
auditctl -w /etc/group -p wa -k CFG_group
auditctl -w /etc/passwd -p wa -k CFG_passwd
auditctl -w /etc/gshadow -p rwxa -k CFG_gshadow
auditctl -w /etc/shadow -p rwxa -k CFG_shadow
auditctl -w /etc/security/opasswd -p rwxa -k CFG_opasswd
clearLogs
useradd test\'user
"""

delete_user = """
auditctl -w /etc/group -p wa -k CFG_group
auditctl -w /etc/passwd -p wa -k CFG_passwd
auditctl -w /etc/gshadow -p rwxa -k CFG_gshadow
auditctl -w /etc/shadow -p rwxa -k CFG_shadow
auditctl -w /etc/security/opasswd -p rwxa -k CFG_opasswd

useradd testuser
clearLogs
userdel -r testuser &> /dev/null
"""

change_password = """
useradd testuser
clearLogs
echo -e "linuxpassword\nlinuxpassword" | passwd testuser
"""

change_password_with_quote = r"""
useradd testuser
clearLogs
echo -e "linux\"password\nlinux\"password" | passwd testuser
"""

change_password_with_single_quote = r"""
useradd testuser
clearLogs
echo -e "linux\'password\nlinux\'password" | passwd testuser
"""

change_password_with_japanese_chars = u"""
useradd testuser
clearLogs
echo -e "テストファイル\nテストファイル" | passwd testuser
"""


add_user_to_group = """
useradd testuser
groupadd testgrp
sleep 5
clearLogs
usermod -a -G testgrp testuser
"""

group_membership_change = """
useradd testuser
groupadd testgrp
sleep 5
groupadd test2grp
sleep 5
usermod -a -G testgrp testuser
sleep 5
clearLogs
usermod -a -G test2grp testuser
"""

remove_user_from_group = """
useradd testuser
groupadd testgrp
sleep 5
usermod -a -G testgrp testuser
clearLogs
usermod -G "" testuser
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

successful_ssh_multi_attempt = """
useradd testuser
echo -e "linuxpassword\nlinuxpassword" | passwd testuser
sleep 5
clearLogs
sshpass -p linuxpassword ssh -o StrictHostKeyChecking=no testuser@127.0.0.1 'echo successfully sshd'
sshpass -p linuxpassword ssh -o StrictHostKeyChecking=no testuser@127.0.0.1 'echo successfully sshd'
sshpass -p linuxpassword ssh -o StrictHostKeyChecking=no testuser@127.0.0.1 'echo successfully sshd'
sshpass -p linuxpassword ssh -o StrictHostKeyChecking=no testuser@127.0.0.1 'echo successfully sshd'
"""

failed_ssh_multi_attempt = """
useradd testuser
echo -e "linuxpassword\nlinuxpassword" | passwd testuser
sleep 5
clearLogs
sshpass -p badpassword ssh -o StrictHostKeyChecking=no testuser@127.0.0.1 'echo succesfully sshd'
sshpass -p badpassword ssh -o StrictHostKeyChecking=no testuser@127.0.0.1 'echo succesfully sshd'
sshpass -p badpassword ssh -o StrictHostKeyChecking=no testuser@127.0.0.1 'echo succesfully sshd'
sshpass -p badpassword ssh -o StrictHostKeyChecking=no testuser@127.0.0.1 'echo succesfully sshd'
"""

execute_file = """
auditctl -a exit,always -F arch=b64 -S execve -k procmon
auditctl -a exit,always -F arch=b32 -S execve -k procmon

createExecutingFile auditd_test_file1.sh
clearLogs
~/auditd_test_file1.sh
"""

execute_file_with_quote = r"""
auditctl -a exit,always -F arch=b64 -S execve -k procmon
auditctl -a exit,always -F arch=b32 -S execve -k procmon

createExecutingFile "auditd_test_file1.sh"
mv ~/auditd_test_file1.sh ~/auditd_\"test_file1.sh
clearLogs
~/auditd_\"test_file1.sh
"""

execute_file_with_single_quote = r"""
auditctl -a exit,always -F arch=b64 -S execve -k procmon
auditctl -a exit,always -F arch=b32 -S execve -k procmon

createExecutingFile auditd_test_file1.sh
mv ~/auditd_test_file1.sh ~/auditd_\'test_file1.sh
clearLogs
~/auditd_\'test_file1.sh
"""

execute_file_jp_characters = u"""
auditctl -a exit,always -F arch=b64 -S execve -k procmon
auditctl -a exit,always -F arch=b32 -S execve -k procmon
createExecutingFile テストファイル.sh
clearLogs
~/テストファイル.sh
"""

watch_directory_for_file_changes_create_file = """
auditctl -w ~/test_watch_dir/ -k test_watch
mkdir -p ~/test_watch_dir
clearLogs
createConfigFile test_watch_dir configfile1.conf
"""

watch_directory_for_file_changes_modify_file = """
auditctl -w ~/test_watch_dir/ -k test_watch
mkdir -p ~/test_watch_dir
createConfigFile test_watch_dir configfile1.conf
clearLogs
echo "hello" >> ~/test_watch_dir/configfile1.conf
"""

watch_directory_for_file_changes_create_file_with_quote = r"""
auditctl -w ~/test_watch_dir/ -k test_watch
mkdir -p ~/test_watch_dir
clearLogs
createConfigFile test_watch_dir config\"file1.conf
"""

watch_directory_for_file_changes_modify_file_with_quote = r"""
auditctl -w ~/test_watch_dir/ -k test_watch
mkdir -p ~/test_watch_dir
createConfigFile test_watch_dir config\"file1.conf
clearLogs
echo "hello" >> ~/test_watch_dir/config\"file1.conf
"""

watch_directory_for_file_changes_create_file_with_single_quote = r"""
auditctl -w ~/test_watch_dir/ -k test_watch
mkdir -p ~/test_watch_dir
clearLogs
createConfigFile test_watch_dir config\'file1.conf
"""

watch_directory_for_file_changes_modify_file_with_single_quote = r"""
auditctl -w ~/test_watch_dir/ -k test_watch
mkdir -p ~/test_watch_dir
createConfigFile test_watch_dir config\'file1.conf
clearLogs
echo "hello" >> ~/test_watch_dir/config\'file1.conf
"""

watch_directory_for_file_changes_create_jp_file = u"""
auditctl -w ~/test_watch_dir/ -k test_watch
mkdir -p ~/test_watch_dir
clearLogs
createConfigFile test_watch_dir テストファイル.conf
"""

watch_directory_for_file_changes_modify_jp_file = u"""
auditctl -w ~/test_watch_dir/ -k test_watch
mkdir -p ~/test_watch_dir
createConfigFile test_watch_dir テストファイル.conf
clearLogs
echo "hello" >> ~/test_watch_dir/テストファイル.conf
"""

execute_32_bit_file = """
auditctl -a exit,always -F arch=b64 -S execve -k archFileExe
auditctl -a exit,always -F arch=b32 -S execve -k archFileExe
clearLogs
/vagrant/payload32 payload32_ouput
"""

execute_64_bit_file = """
auditctl -a exit,always -F arch=b64 -S execve -k archFileExe
auditctl -a exit,always -F arch=b32 -S execve -k archFileExe

clearLogs
/vagrant/payload64 payload64_ouput
"""

payloads = {'add_user': add_user,
            'add_user_without_home_directory': add_user_without_home_directory,
            'add_user_with_quote': add_user_with_quote,
            'add_user_with_single_quote': add_user_with_single_quote,
            'delete_user': delete_user,
            'change_password': change_password,
            'change_password_with_quote': change_password_with_quote,
            'change_password_with_single_quote': change_password_with_single_quote,
            'change_password_with_japanese_chars': change_password_with_japanese_chars,
            'add_user_to_group': add_user_to_group,
            'remove_user_from_group': remove_user_from_group,
            'group_membership_change': group_membership_change,
            'failed_ssh_attempt': failed_ssh_attempt,
            'successful_ssh_attempt': successful_ssh_attempt,
            'failed_ssh_multi_attempt': failed_ssh_multi_attempt,
            'successful_ssh_multi_attempt': successful_ssh_multi_attempt,
            'execute_file': execute_file,
            'execute_file_with_quote': execute_file_with_quote,
            'execute_file_with_single_quote': execute_file_with_single_quote,
            'execute_file_jp_characters': execute_file_jp_characters,
            'watch_directory_for_file_changes_create_file': watch_directory_for_file_changes_create_file,
            'watch_directory_for_file_changes_modify_file': watch_directory_for_file_changes_modify_file,
            'watch_directory_for_file_changes_create_file_with_quote': watch_directory_for_file_changes_create_file_with_quote,
            'watch_directory_for_file_changes_modify_file_with_quote': watch_directory_for_file_changes_modify_file_with_quote,
            'watch_directory_for_file_changes_create_file_with_single_quote': watch_directory_for_file_changes_create_file_with_single_quote,
            'watch_directory_for_file_changes_modify_file_with_single_quote': watch_directory_for_file_changes_modify_file_with_single_quote,
            'watch_directory_for_file_changes_create_jp_file': watch_directory_for_file_changes_create_jp_file,
            'watch_directory_for_file_changes_modify_jp_file': watch_directory_for_file_changes_modify_jp_file,
            'execute_32_bit_file': execute_32_bit_file,
            'execute_64_bit_file': execute_64_bit_file}

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

function createExecutingFile()
{{
    cat > ~/$1 << EOF
    #!/bin/bash
    echo "hello"
EOF

    chmod +x ~/$1
}}

function createConfigFile()
{{
    cat > ~/$1/$2 << EOF
    some_config_item some_config_value
EOF
}}

pushd "{remotedir}"
echo "Running {filePrefix} script on {platform}!..."

clearLogs

{payload}

cp /root/AuditEvents.bin.tmp ${{REMOTE_DIR}}/{filePrefix}AuditEvents.bin
cat /var/log/audit/audit.log > ${{REMOTE_DIR}}/{filePrefix}AuditEvents.log
ausearch -i > ${{REMOTE_DIR}}/{filePrefix}AuditEventsReport.log

# Clean-up - Delete users/groups and remove auditctl rules
userdel -r testuser &> /dev/null
groupdel testgrp &> /dev/null
auditctl -D &> /dev/null

popd
"""

platformSetupScript = """
#!/bin/bash

export REMOTE_DIR="/vagrant/auditd-output/{platform}"

mkdir -p $REMOTE_DIR

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

echo "\nMatch LocalAddress 127.0.0.1\n\tPasswordAuthentication yes" >> /etc/ssh/sshd_config
service sshd restart

setenforce Permissive &> /dev/null
sleep 5
service auditd restart
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


def check_vagrant_up_and_running(platform):
    output = sp.check_output(['/usr/bin/vagrant', 'status'])
    regex = platform + '[ ]*running'

    if not re.search(regex, str(output)):
        print('starting up vagrant')
        sp.call(['/usr/bin/vagrant', 'up', platform])

def vagrant_rsync(platform):
    sp.call(['/usr/bin/vagrant', 'rsync', platform])

def vagrant_push_data(platform, local_path):
    filename = os.path.basename(local_path)
    destination = f"{platform}:/vagrant/{filename}"
    sp.call(['/usr/bin/vagrant', 'scp',  local_path, destination], stdout=sp.PIPE)


def vagrant_pull_data(platform, localPath, filePrefix):
    sp.call(['/usr/bin/vagrant', 'scp',  f"{platform}:/vagrant/auditd-output/{platform}/{filePrefix}*", localPath], stdout=sp.PIPE)


def vagrant_run(platform, bashString):
    hostfile = os.path.join(VAGRANTROOT, 'tmpscript.sh')
    remotefile = os.path.join('/vagrant', 'tmpscript.sh')

    with open(hostfile, 'w') as f:
        f.write(bashString)

    exe32file = "/redist/binaries/linux11/input/auditDTestFiles/payload32"
    exe64file = "/redist/binaries/linux11/input/auditDTestFiles/payload64"

    vagrant_push_data(platform, exe32file)
    vagrant_push_data(platform, exe64file)

    if platform == "amazon_linux":
        vagrant_cmd = ['/usr/bin/vagrant', 'ssh', platform, '-c', 'sudo mkdir /vagrant']
        sp.call(vagrant_cmd)
        vagrant_cmd = ['/usr/bin/vagrant', 'ssh', platform, '-c', 'sudo chown ec2-user:ec2-user /vagrant']
        sp.call(vagrant_cmd)
        vagrant_push_data(platform, hostfile)

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

    for platform in platforms:
        check_vagrant_up_and_running(platform)

    for platform in platforms:
        vagrant_run(platform, platformSetupScript.format(platform=platform))

        newPath = os.path.join(currdir, platform)
        if os.path.isdir(newPath):
            shutil.rmtree(newPath)
        os.mkdir(newPath)

        for filePrefix, payload in payloads.items():
            run_payload(platform, filePrefix, payload, remotedir)

            if platform == "amazon_linux":
                vagrant_pull_data(platform, newPath, filePrefix)

        os.chdir(currdir)

    sp.call(['vagrant', 'destroy', '-f'])


if __name__ == "__main__":
    currdir = os.path.abspath(os.getcwd())
    VAGRANTROOT = os.path.join(find_vagrant_root(currdir))
    sys.exit(main())
