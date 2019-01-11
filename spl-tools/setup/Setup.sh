#!/bin/bash

# Ensure we run script from within sspl-tools directory
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" > /dev/null && pwd )"
pushd "${SCRIPT_DIR}/../" &> /dev/null

TOTAL_STEPS=$(grep -c [e]choProgress ${SCRIPT_DIR}/Setup.sh)
TOTAL_STEPS=$(( TOTAL_STEPS - 1))
CURRENT_STEP=1

function error()
{
	printf '\033[0;31m'
	echo "ERROR: $1"
	printf '\033[0m'
	exit 1
}

function warning()
{
    printf '\033[01;33m'
    echo "Warning: $1"
    printf '\033[0m'
}

function echoProgress()
{
    printf '\033[1;34m'
    echo "[Step $CURRENT_STEP / $TOTAL_STEPS]: $1"
    printf '\033[0m'
    CURRENT_STEP=$(( CURRENT_STEP + 1 ))
}

function cloneSSPLRepos()
{
    [[ -f ${SCRIPT_DIR}/gitRepos.txt ]] || error "Cannot find gitRepos.txt"

	for repo in $(cat ${SCRIPT_DIR}/gitRepos.txt)
	do
        repoName=$(echo $repo | awk '{n=split($0, a, "/"); print a[n]}' | sed -n "s_\([^\.]*\).*_\1_p")
        if [[ -d $repoName ]]
        then
            warning "Repo $repoName already exists!"
            continue
        fi
		git clone $repo || error "Failed to clone $repo with code $?"
	done
}

function mountShare()
{
    localMountDir="$1"
    fstabString="$2"

    if grep -iq "$localMountDir" /etc/fstab
    then
        warning "$localMountDir already in /etc/fstab"
        return
    fi

    if echo $fstabString | grep -q USERNAME
    then
        read -p "Enter username for $localMountDir: " username
        read -sp "Enter password for $localMountDir (This will be written in plaintext to /etc/fstab): " password
        echo ""

        fstabString="${fstabString/USERNAME/$username}"
        fstabString="${fstabString/PASSWORD/$password}"
    fi

    sudo cp /etc/fstab /etc/fstab.bk
    echo "$fstabString" | sudo tee --append /etc/fstab &> /dev/null

    sudo mkdir -p $localMountDir
    sudo umount $localMountDir &> /dev/null
    if ! sudo mount $localMountDir
    then
        sudo mv /etc/fstab.bk /etc/fstab
        error "Failed to mount $localMountDir"
    fi
}

function mountFiler5()
{
    filer5Loc="/uk-filer5/prodro"
    filer5mount="//UK-FILER5.PROD.SOPHOS/PRODRO $filer5Loc    cifs   users,noauto,user=USERNAME,domain=PRODUCTION,vers=2.0,password=PASSWORD 0 0"
    mountShare "${filer5Loc}" "${filer5mount}"
}

function mountFiler6()
{

    filer6Loc="/mnt/filer6/bfr"
    filer6mount="//UK-FILER6.ENG.SOPHOS/BFR  $filer6Loc cifs   noauto,users,rw,domain=GREEN,username=USERNAME,password=PASSWORD,vers=2.0   0   0"
    mountShare "${filer6Loc}" "${filer6mount}"
    filer6Loc="/mnt/filer6/linux"
    filer6mount="//UK-FILER6.ENG.SOPHOS/LINUX  $filer6Loc cifs   noauto,users,rw,domain=GREEN,username=USERNAME,password=PASSWORD,vers=2.0   0   0"
    mountShare "${filer6Loc}" "${filer6mount}"
}

function mountAllegro()
{
    allegroLoc="/redist"
    allegromount="allegro:/redist $allegroLoc nfs ro,vers=3,tcp,exec 0 0"
    mountShare "${allegroLoc}" "${allegromount}"
}

function setupMounts()
{
    mountFiler5
    mountFiler6
    mountAllegro
}

# Do not run as root - we do not want the repos to be root owned
[[ $EUID -ne 0 ]] || error "Please do not run the script as root"

# Update and Upgrade OS
echoProgress "Updating and Upgrading OS"
sudo apt-get update || error "Failed to update"
sudo apt-get -y upgrade || error "Failed to upgrade"

# Install required packages
echoProgress "Installing Required Packages"
sudo apt-get -y install zip openssh-server cmake make nfs-common cifs-utils gcc python-pip awscli virtualbox \
                        virtualbox-dkms python-devel\
	|| error "Failed to install required packages"

#Gives the ability to run robot tests locally but none of these scripts will do this themselves
sudo pip install robotframework pyzmq watchdog protobuf

sudo dpkg-reconfigure virtualbox-dkms

# Clone all SSPL Repos
echoProgress "Cloning SSPL Repos"
cloneSSPLRepos

# Setup Mounts
echoProgress "Setting up mounts"
setupMounts

# Install Vagrant
echoProgress "Setting up vagrant"
installVagrantScript=${SCRIPT_DIR}/installvagrant.sh
if [[ -x ${installVagrantScript} ]]
then
    ${installVagrantScript}
else
    error "Cannot execute ${installVagrantScript}"
fi

# Setup robot test symlink
echoProgress "Creating robot test Symlink"
if [[ -L ./everest-systemproducttests/robot ]]
then
    warning "Symlink already exists"
else
    ln -s ../tests/remoterobot.py everest-systemproducttests/robot
    chmod +x tests/remoterobot.py
fi

popd &> /dev/null
