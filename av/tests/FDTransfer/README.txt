
Commands:

make fd_transfer_server fd_transfer_client
sudo setcap cap_sys_chroot=ep tests/FDTransfer/fd_transfer_server
sudo setcap cap_dac_read_search=ep tests/FDTransfer/fd_transfer_client
tests/FDTransfer/fd_transfer_server
tests/FDTransfer/fd_transfer_client /etc/shadow