_________________________________________________________________________
DEPENDENCIES:

This tool relies on being able to reach artifactory (for the warehouse files) and the sophos pypi repository (for pip dependencies).
EMS machines should have no trouble with this.
Your personal machines might not be able to reach these services.

If Running on a machine which cannot reach these services, you may still run this tool in offline mode. See section below.
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

for 999 warehouse:
username: av_user_999
password: password
address: https://localhost:8001

_________________________________________________________________________
OFFLINE MODE

running wrapper.sh with --offline-mode allows you to serve warehouses without relying on external dependencies.
However, you must provided the required artifacts via additional arguments:

You must provide:

--support-files: path to SupportFiles.zip artifact from warehouse build

And at least one pair of:
    --vut-customer: develop/customer.zip
    --vut-warehouse: develop/warehouse.zip
    OR
    --nine-nine-nine-customer: edr-mdr-999/customer.zip
    --nine-nine-nine-warehouse: edr-mdr-999/warehouse.zip
    artifacts from the warehouse build

These can be found at:
    https://artifactory.sophos-ops.com/ui/repos/tree/General/esg-dev-build-tested%2Flinuxep.sspl-warehouse

_________________________________________________________________________
TO CHECK PROCESSES RUNNING:

you can check the processes are still running with:
"""
ps -ef | grep cid
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