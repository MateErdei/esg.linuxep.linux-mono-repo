#!/usr/bin/env bash

# This is a helper script that will copy the needed performance
# scripts from filer6 to the performance machine. This is to be executed as root and used in conjunction
# with the publish_scripts_to_filer.sh script.

function failure()
{
  echo "$1"
  exit 1
}

[[ -d /mnt/filer6/linux/ ]] || failure "Could not access filer6 linux dir"

# prepare gcc build area
if [[ -d /root/gcc-build-test ]]
then
  echo "gcc build area already exists, skipping setting it up"
else
  echo "Setting up gcc build area"
  rm -rf /root/gcc-build-test
  mkdir -p /root/gcc-build-test/build || failure "Could not make gcc build dir"
  cp /mnt/filer6/linux/SSPL/tools/gcc-releases-gcc-8.5.0.tar.gz /root/gcc-build-test/ || failure "Could not copy gcc source tar"
  pushd /root/gcc-build-test || failure "Could not pushd into gcc build dir to untar"
    tar -xzf gcc-releases-gcc-8.5.0.tar.gz || failure "Could not untar gcc source"
  popd
  pushd /root/gcc-build-test/build
    ../gcc-releases-gcc-8.5.0/configure --enable-languages=c,c++ --disable-multilib  || echo "Could not configure gcc build will patch and try running /root/gcc-build-test/gcc-releases-gcc-8.5.0/contrib/download_prerequisites"
    sleep 3

    echo "Patching ftp to http as the files are not available via ftp"
    sed -i 's/ftp:/http:/g' /root/gcc-build-test/gcc-releases-gcc-8.5.0/contrib/download_prerequisites || failure "Could not patch download_prerequisites"
    sleep 3

    pushd /root/gcc-build-test/gcc-releases-gcc-8.5.0
      ./contrib/download_prerequisites || failure "Could not download prerequisites for gcc"
    popd
    ../gcc-releases-gcc-8.5.0/configure --enable-languages=c,c++ --disable-multilib  || failure "Could not configure gcc build"
  popd
fi

# Copy perf scripts
echo "Copying performance scripts into /root"
cp -r /mnt/filer6/linux/SSPL/performance /root/

echo "Copying MCS certs to /root"
cp -r /mnt/filer6/linux/SSPL/tools/setup_sspl/certs /root/

echo "Installing needed python3 modules"
python3 -m pip install requests || failure "Could not install requests"

echo ""
echo "Some example crontab entries:"
echo ""
echo "# Run gcc build timed test every 2 hours"
echo "0 */2 * * * python3 /root/performance/RunPerfTests.py -s gcc"
echo ""
echo "# Install EDR at  8am, run tests and then remove it at 4pm"
echo "0 8 * * * /root/performance/install-edr.sh <CENTRAL_ACCOUNT_PASSWORD>"
echo "*/30 08-15  * * *  python3 /root/performance/RunPerfTests.py -s central-livequery -r p -i <CENTRAL_ACCOUNT_ID> -p <CENTRAL_ACCOUNT_SECRET>"
echo "*/30 08-15  * * *  python3 /root/performance/RunPerfTests.py -s central-livequery -r p -e LicenceDarwin+2@gmail.com -p <CENTRAL_ACCOUNT_PASSWORD>"
echo "0 16 * * * /root/ssplDogfoodFeedback.sh && /opt/sophos-spl/bin/uninstall.sh --force"
echo ""
echo "# Install EDR and MTR at  4pm, run tests and then remove it at 12am"
echo "0 16 * * * /root/performance/install-edr-mtr.sh <CENTRAL_ACCOUNT_PASSWORD>"
echo "*/30 16-21  * * *  python3 /root/performance/RunPerfTests.py -s central-livequery -r p -i <CENTRAL_ACCOUNT_ID> -p <CENTRAL_ACCOUNT_SECRET>"
echo "*/30 16-21  * * *  python3 /root/performance/RunPerfTests.py -s central-livequery -r p -e LicenceDarwin@gmail.com -p <CENTRAL_ACCOUNT_PASSWORD>"
echo "0 0 * * * /root/ssplDogfoodFeedback.sh && /opt/sophos-spl/bin/uninstall.sh --force"
echo ""
echo "# Run local live queries at 10am"
echo "0 10 * * *  python3 /root/performance/RunPerfTests.py -s local-livequery"
echo ""
echo "# Measuring osquery DB files every 5 mins"
echo "*/5 * * * *  bash /root/performance/save-osquery-db-file-count.sh"
echo ""

echo "Done"


