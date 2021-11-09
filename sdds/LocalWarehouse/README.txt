_________________________________________________________________________
DEPENDENCIES:

This tool relies on being able to reach artifactory (for the warehouse files) and the sophos pypi repository (for pip dependencies).
EMS machines should have no trouble with this.
Your personal machines might not be able to reach these services.
_________________________________________________________________________
USAGE:
"""
. ./wrapper --branch <branch of sspl-warehouse you want, defaults to develop>
"""

IN CENTRAL:
Set your update credentials to:

username: av_user_vut

password: password
address: https://localhost:8000

You should now be able to update/install against the locally served warehouse.
Rerun wrapper.sh to relaunch the processes if needed
_________________________________________________________________________
TO CHECK PROCESSES RUNNING:

you can check the processes are still running with:
"""
root@7007-jak:/opt/sophos-spl# ps -ef | grep cid
"""

which should show:
"""
root     11468     1  0 Nov03 pts/0    00:00:16 python3 /tmp/ostiant/utils/cidServer.py 443 /tmp/ostiant/local_warehouses  --secure=/tmp/ostiant/utils/server-private.pem
root     11469     1  0 Nov03 pts/0    00:00:00 python3 /tmp/ostiant/utils/cidServer.py 8000 /tmp/ostiant/local_warehouses/dev/sspl-warehouse/vut/warehouse/customer/  --secure=/tmp/ostiant/utils/server-private.pem
"""
_________________________________________________________________________

wrapper.sh gathers the few dependencies needed, calls generate_local_warehouse.py and exports OVERRIDE_SOPHOS_LOCATION

ALTERNATE USAGE:
"""
./generate_local_warehouse.py
export OVERRIDE_SOPHOS_LOCATION=https://localhost:8000
"""