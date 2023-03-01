export PATH="$PATH:/mnt/c/Program Files/Oracle/VirtualBox"
export VAGRANT_WSL_ENABLE_WINDOWS_ACCESS="1"
if  [[ -z "$WINDOWS_USER" ]]
then
  export WINDOWS_USER=$(powershell.exe '$env:UserName' | sed -e 's/\r//g')
fi
export VAGRANT_WSL_WINDOWS_ACCESS_USER_HOME_PATH="/mnt/c/Users/$WINDOWS_USER/"
