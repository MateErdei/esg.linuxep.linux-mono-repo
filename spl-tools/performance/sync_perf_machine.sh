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

echo "Setting up machine"
yum upgrade
yum -y install python3

# prepare gcc build area
if [[ -d /root/gcc-build-test ]]
then
  echo "gcc build area already exists, skipping setting it up"
else
  echo "Setting up gcc build area"
  rm -rf /root/gcc-build-test
  mkdir -p /root/gcc-build-test/build || failure "Could not make gcc build dir"
  cp /mnt/filer6/linux/SSPL/tools/gcc-11.2.0.tar.gz /root/gcc-build-test/ || failure "Could not copy gcc source tar"
  pushd /root/gcc-build-test || failure "Could not pushd into gcc build dir to untar"
    tar -xzf gcc-11.2.0.tar.gz || failure "Could not untar gcc source"
  popd
  pushd /root/gcc-build-test/build
    ../gcc-11.2.0/configure --enable-languages=c,c++ --disable-multilib  || echo "Could not configure gcc build will patch and try running /root/gcc-build-test/gcc-11.2.0/contrib/download_prerequisites"
    sleep 3

    echo "Patching ftp to http as the files are not available via ftp"
    sed -i 's/ftp:/http:/g' /root/gcc-build-test/gcc-11.2.0/contrib/download_prerequisites || failure "Could not patch download_prerequisites"
    sleep 3

    pushd /root/gcc-build-test/gcc-11.2.0
      ./contrib/download_prerequisites || failure "Could not download prerequisites for gcc"
    popd
    ../gcc-11.2.0/configure --enable-languages=c,c++ --disable-multilib  || failure "Could not configure gcc build"
  popd
fi

# Copy perf scripts
echo "Copying performance scripts into /root"
cp -r /mnt/filer6/linux/SSPL/performance /root/

echo "Copying MCS certs to /root"
cp -r /mnt/filer6/linux/SSPL/tools/setup_sspl/certs /root/

echo "Installing needed python3 modules"
python3 -m pip install requests watchdog websockets nest_asyncio || failure "Could not install requests"

echo ""
echo "Add the following crontab and then reboot the machine to complete sync:"
echo ""
echo "@reboot /root/metricbeat-7.1.1-linux-x86_64/metricbeat -c /root/metricbeat-7.1.1-linux-x86_64/metricbeat.yml"
echo "0 16 * * * /root/performance/ssplDogfoodFeedback.sh"
echo ""

echo "The performance machine will also require python 3.7 to be installed for some tests to run"

echo "Done"


