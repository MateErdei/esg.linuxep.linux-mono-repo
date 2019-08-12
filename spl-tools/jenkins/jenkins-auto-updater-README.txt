#
# This is a readme file for the jenkins template updater job
# scripts referenced have paths relative to the sspl-tools repo, they will likely be in the same folder as this file
#

#
# If you are trying to restore the job, you may find the xml for it in the jenkins backup tar on filer6 at:
#   /mnt/filer6/linux/JenkinsBackup/
#

1.

have the job check out sspl-tools

2.

set up a configuration matrix with:

VM_NAME as a user defined axis, as a list of machine
you want to update (their vsphere names).

a Slaves axis with only master selected as an individual node (this enforces that the job
runs on master, which has the certs to ssh onto the slaves)

3.

use the vsphere plugin to revert $(VM_NAME) to the "[auto] clean" snapshot

4.

use the vsphere plugin to power on ${VM_NAME}. This will set $VSPHERE_IP to the ip of
the machine

5.

execute jenkins/doUpdate.sh with a shell command build step:

"""
#!/bin/bash -xe
jenkins/doUpdate.sh
"""
This will update the template and then power it off gracefully after about 2 minutes

6.

use the vsphere plugin to power on ${VM_NAME} again.

7.

execute jenkins/setUbuntuUpForCloning.sh with a shell command build step:

"""
#!/bin/bash -xe
jenkins/setUbuntuUpForCloning.sh
"""

8.

Use the vsphere plugin to delete the "[auto] clean-old" snapshot of $VM_NAME

9.

Use the vsphere plugin to rename the "[auto] clean" snapshot of $VM_NAME to "[auto] clean-old"

10.

use the vsphere plugin to take a snapshot of $VM_NAME and name it "[auto] clean"

11.

Set a post build action to send an email for unstable builds to yourself and other people you wish to annoy/alert
with failures of this job


DONE