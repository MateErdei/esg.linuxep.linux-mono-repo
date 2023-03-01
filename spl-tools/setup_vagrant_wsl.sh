#!/usr/bin/env bash

THIS_FILE_FULL_PATH=$(readlink -f $0)
THIS_DIR=$(dirname "$THIS_FILE_FULL_PATH")

# Run this from within WSL instance
set -e
VAGRANT_VERSION="2.3.2"
rm -rf /tmp/hashicorp-archive-keyring.gpg
wget -O- https://apt.releases.hashicorp.com/gpg | gpg --dearmor --output /tmp/hashicorp-archive-keyring.gpg
sudo cp  /tmp/hashicorp-archive-keyring.gpg /usr/share/keyrings/hashicorp-archive-keyring.gpg
echo "deb [signed-by=/usr/share/keyrings/hashicorp-archive-keyring.gpg] https://apt.releases.hashicorp.com $(lsb_release -cs) main" | sudo tee /etc/apt/sources.list.d/hashicorp.list
sudo apt update && sudo apt install vagrant=$VAGRANT_VERSION
vagrant --version
vagrant plugin install virtualbox_WSL2
vagrant plugin install vagrant-vbguest
vagrant plugin install vagrant-scp

# The "./vagrant" script is a wrapper around the "vagrant" command, it sets env vars needed to run under WSL
chmod +x "$THIS_DIR/vagrant"

# Check if the vagrant box needs updating, this will show a message to the user
./vagrant box outdated

# Setup symlinks in other repos so when in the root of those repos ./vagrant works and ./robot works.
bash "$THIS_DIR/setup_robot_vagrant_symlinks.sh"

# Vagrant on WSL cannot mount WSL directories, only windows ones, so we can trick it to be able to mount a WSL dir by
# making a symlink from Windows to WSL filer6 dir. This is then a shared dif on vagrant.
# Only needed because of SDDS specs.
export WINDOWS_USER=$(powershell.exe '$env:UserName' | sed -e 's/\r//g')
if [ ! -d "/mnt/filer6/linux" ]
then
  echo "Please mount filer6 https://sophos.atlassian.net/wiki/spaces/LD/pages/226296563009/How+to+Mount+filer6"
  exit 1
fi
sudo ln -snf "/mnt/filer6/linux" "/mnt/c/Users/$WINDOWS_USER/link-filer6"

# Check vagrant is installed on both Windows and WSL, also that the versions match.
# C:\HashiCorp\Vagrant\bin
if [ -f /mnt/c/HashiCorp/Vagrant/bin/vagrant.exe ]
then
  WINDOWS_VAGRANT_VERSION=$("/mnt/c/HashiCorp/Vagrant/bin/vagrant.exe" --version | sed -e 's/\r//g')
else
  WINDOWS_VAGRANT_VERSION=""
fi

echo $WINDOWS_VAGRANT_VERSION
if [ "$WINDOWS_VAGRANT_VERSION" != "Vagrant $VAGRANT_VERSION" ]
then
  echo "Please make sure to install Vagrant to your Windows machine, you can get version $VAGRANT_VERSION from: https://releases.hashicorp.com/vagrant/${VAGRANT_VERSION}/vagrant_${VAGRANT_VERSION}_windows_amd64.msi"
else
  echo "Windows and WSL Vagrant versions match, both $VAGRANT_VERSION"
fi

echo ""
echo "Usage"
echo 'vagrant on WSL needs environment variables set, you can either use the vagrant wrapper "./vagrant" instead of the "vagrant" cmd or set the variables yourself.'
echo "./vagrant <cmd>"
echo "examples:"
echo "./vagrant status"
echo "./vagrant up ubuntu"
echo "./vagrant ssh"