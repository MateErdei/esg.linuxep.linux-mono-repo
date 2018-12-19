#!/bin/bash

# Ensure we run script from within sspl-tools directory
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" > /dev/null && pwd )"
TOTAL_STEPS=$(grep -c [e]choProgress ${SCRIPT_DIR}/Setup.sh)
pushd "${SCRIPT_DIR}../" &> /dev/null

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

function mountFiler5()
{
    filer5Loc="/uk-filer5/prodro"
    if grep -iq $filer5Loc /etc/fstab 
    then
        warning "$filer5Loc already in /etc/fstab"
        return
    fi
    read -p "Enter username for filer5: " filer5username
    read -sp "Enter password for filer5 (This will be written in plaintext to /etc/fstab): " filer5password
    echo ""

    filer5mount="//UK-FILER5.PROD.SOPHOS/PRODRO $filer5Loc    cifs   users,noauto,user=${filer5username},domain=PRODUCTION,vers=2.0,password=${filer5password} 0 0"
    sudo cp /etc/fstab /etc/fstab.bk
    echo "$filer5mount" | sudo tee --append /etc/fstab &> /dev/null
    
    sudo mkdir -p $filer5Loc
    sudo umount $filer5Loc &> /dev/null
    if ! sudo mount $filer5Loc
    then
        sudo mv /etc/fstab.bk /etc/fstab
        error "Failed to mount filer5"
    fi
}

function mountFiler6()
{
    filer6Loc="/mnt/filer6/bfr"
    if grep -iq "$filer6Loc" /etc/fstab 
    then
        warning "$filer6Loc already in /etc/fstab"
        return
    fi
    read -p "Enter username for filer6: " filer6username
    read -sp "Enter password for filer6 (This will be written in plaintext to /etc/fstab): " filer6password
    echo ""

    filer6mount="//UK-FILER6.ENG.SOPHOS/BFR  $filer6Loc cifs   noauto,users,rw,domain=GREEN,username=${filer6username},password=${filer6password},vers=2.0   0   0"
    sudo cp /etc/fstab /etc/fstab.bk
    echo "$filer6mount" | sudo tee --append /etc/fstab &> /dev/null
    
    sudo mkdir -p $filer6Loc 
    sudo umount $filer6Loc &> /dev/null
    if ! sudo mount $filer6Loc
    then
        sudo mv /etc/fstab.bk /etc/fstab
        error "Failed to mount filer6"
    fi
}

function mountAllegro()
{
    allegroLoc="/redist"
    if grep -iq "$allegroLoc" /etc/fstab 
    then
        warning "$allegroLoc already in /etc/fstab"
        return
    fi

    allegromount="allegro:/redist $allegroLoc nfs ro,vers=3,tcp,exec 0 0"
    sudo cp /etc/fstab /etc/fstab.bk
    echo "$allegromount" | sudo tee --append /etc/fstab &> /dev/null
    
    sudo mkdir -p $allegroLoc
    sudo umount $allegroLoc &> /dev/null
    if ! sudo mount $allegroLoc
    then
        sudo mv /etc/fstab.bk /etc/fstab
        error "Failed to mount allegro"
    fi
    
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
#sudo apt-get update || error "Failed to update"
#sudo apt-get -y upgrade || error "Failed to upgrade"

# Install required packages
echoProgress "Installing Required Packages"
sudo apt-get -y install zip openssh-server cmake make nfs-common cifs-utils gcc python-pip \
	|| error "Failed to install required packages"

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
