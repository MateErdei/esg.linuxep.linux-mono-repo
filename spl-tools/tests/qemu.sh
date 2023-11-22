#!/usr/bin/env bash

cd "${0%/*}" || exit

RESET_VM=0

while [[ $# -ge 1 ]]
do
    case $1 in
        --reset)
            RESET_VM=1
            ;;
        *)
            echo "unknown argument $1"
            exit 1
            ;;
    esac
    shift
done

if ! ( groups | grep -qw kvm )
then
  sudo usermod -aG kvm "$USER"
  echo Added user "$USER" to group kvm, reboot WSL to finalise change
  exit 1
fi

# Generate key used to SSH onto test VM
vm_ssh_key="/home/$USER/.ssh/spl-qemu"
if [[ ! -f "$vm_ssh_key"  ]]
then
    ssh-keygen -b 2048 -t rsa -f "$vm_ssh_key" -q -N ""
fi

sudo apt-get install -y cloud-image-utils qemu qemu-kvm

# 20.04
img=focal-server-cloudimg-amd64.img
user_data="user-data.img"

if [[ $RESET_VM == 1 ]]
then
    echo "Deleting VM"
    rm "$user_data"
    rm "${img}"
    ssh-keygen -f "/home/$USER/.ssh/known_hosts" -R "[localhost]:2222"
fi

if [ ! -f "${img}.template" ]
then
  wget -O "${img}.template" "https://cloud-images.ubuntu.com/focal/current/${img}"
fi

if [ ! -f "${img}" ]
then
  cp "${img}.template" "${img}"
  # sparse resize
  qemu-img resize "${img}" +128G
fi

if [ ! -f "$user_data" ]; then
  cat >user-data <<EOF
#cloud-config
password: ubuntu
chpasswd: { expire: False }
ssh_authorized_keys:
  - $(cat "${vm_ssh_key}.pub")
EOF
  cloud-localds "$user_data" user-data
  rm user-data
fi

echo To SSH: ssh -p 2222 -i "$vm_ssh_key" ubuntu@localhost

qemu-system-x86_64 \
  -enable-kvm \
  -drive "file=${img},format=qcow2" \
  -drive "file=${user_data},format=raw" \
  -smp 4 \
  -m 8G \
  -net nic \
  -net user,hostfwd=tcp::2222-:22 \
;
