#!/usr/bin/env bash

PathOfWarehouseDirectory=$1

if [ ! -d "$PathOfWarehouseDirectory" ]; then
  echo "Please, provide the directory of the files to be included in the warehouse"
  echo "Usage: startFakeWarehouse pathOfDirectory"
  exit 1
fi

if [[ ${PathOfWarehouseDirectory:0:1} !=  / ]]; then
  PathOfWarehouseDirectory="$(pwd)/$PathOfWarehouseDirectory"
fi

DIRECTORYOFSCRIPT="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

SYSTEMPRODUCTTESTSPATH="${DIRECTORYOFSCRIPT}/../.."

pushd ${SYSTEMPRODUCTTESTSPATH}

mkdir -p /tmp/sophos-root/etc/certificates
mkdir -p /tmp/sophos-root/install
mkdir -p /tmp/sophos-root/install/update/cache/primarywarehouse
mkdir -p /tmp/sophos-root/install/update/cache/primary

echo '
{
 "sophosURLs": [
  "https://localhost:1233"
 ],
 "credential": {
  "username": "regruser",
  "password": "regrABC123pass"
 },
 "proxy": {
  "url": "noproxy:",
  "credential": {
   "username": "",
   "password": ""
  }
 },
 "releaseTag": "RECOMMENDED",
 "baseVersion": "0",
 "primary": "ExamplePlugin",
 "logLevel": "VERBOSE"
}
' > /tmp/sophos-root/suldownloader_inputconf.json

PYTHONPATH=${SYSTEMPRODUCTTESTSPATH}/libs/:$PYTHONPATH  FAKEWAREHOUSE="$PathOfWarehouseDirectory" HOME_FOLDER="ExamplePlugin" python -c '
import UpdateServer
import WarehouseGenerator
import os
import time
import shutil

fakeserverpath = "/tmp/fakewarehouse_tmp"
try:
  shutil.rmtree(fakeserverpath)
except :
  pass
fakeWarehouse = os.environ["FAKEWAREHOUSE"]

updateServer = UpdateServer.UpdateServer()

warehouseGenerator=WarehouseGenerator.WarehouseGenerator()

print "Generate warehouse"

warehouseGenerator.generate_warehouse(fakeWarehouse, fakeserverpath, "ExamplePlugin")

print "Copy certificate to /tmp/sophos-root"
shutil.copyfile("SupportFiles/sophos_certs/rootca.crt", "/tmp/sophos-root/etc/certificates/rootca.crt")
shutil.copyfile("SupportFiles/sophos_certs/ps_rootca.crt", "/tmp/sophos-root/etc/certificates/ps_rootca.crt")
shutil.copyfile("SupportFiles/https/ca/root-ca.crt.pem", "/tmp/sophos-root/etc/certificates/root-ca.crt")


updateServer.start_update_server(1233, os.path.join(fakeserverpath, "customer_files"))
updateServer.start_update_server(1234, os.path.join(fakeserverpath, "warehouses/sophosmain"))
time.sleep(1000)
'
popd