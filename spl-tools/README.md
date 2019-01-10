General Tools for SSPL
======================

Setting up Developer Environment for SSPL
=========================================
#### Prerequisites: 
* Install an Ubuntu x64 machine, 64GB HDD, 4GB RAM
* Install git
* Setup SSH keys for Stash account
* Clone this repository

#### Setup
*./setup/Setup.sh* : Setup an Ubuntu machine ready for SSPL development. Performs the following 
steps:
1. Update / Upgrade the OS
2. Install required packages for SSPL
3. Clone all git repositories required for SSPL development. (As listed in *./gitRepos.txt*)
4. Setup NFS mounts - filers, redist etc.
5. Install vagrant
6. Setup robot test symlink in the everest-systemproducttests repo

#### Build / Install
*./setup/Build.sh* : Build all products listed in *gitRepos.txt*.

*./setup/Build.sh --debug* : Perform a clion debug build of all products listed in *gitRepos.txt*.

*./setup/Install.py* : Install Base and Plugins to Ubuntu vagrant VM. Once installed, use ```vagrant ssh ubuntu``` to 
interact with the product. 
```commandline
usage: Install.py [-h] [--mcsurl MCSURL] [--mcstoken MCSTOKEN]
                  [--exampleplugin]

Install Base and Plugins to an Ubuntu Vagrant VM

optional arguments:
  -h, --help           show this help message and exit
  --mcsurl MCSURL      MCS URL for Central (No Central connection if unset)
  --mcstoken MCSTOKEN  MCS Token for Central (No Central connection if unset)
  --exampleplugin      Install exampleplugin

```
 
Rapid Prototyping of New Plugin
=========================================
Follow these instructions if you are new to SSPL Plugin development to quickly have a new plugin up and running.
In this tutorial we will create a new plugin called 'TestScanner'.

##### Create a copy of exampleplugin
*./setup/copyPlugin.sh* will copy and rename the Example plugin. It will also remove the git history from the copy.
```commandline
$ ./setup/copyPlugin.sh --help
Usage: ./setup/copyPlugin.sh --name <plugin_name> [--gitrepo <git_repo>]

$ ./setup/copyPlugin.sh --name TestScanner

$ ls sspl-plugin-testscanner
build  
buildconfig  
build-files  
build.sh  
CMakeLists.txt  
Jenkinsfile  
localjenkins  
modules  
PLUGINNAME  
products  
README.md  
tests  
tools
```

##### Build the TestScanner plugin
Use *build.sh* to build the new plugin and run any unit tests.
```commandline
$ cd sspl-plugin-testscanner
$ ./build.sh
```

##### Install the TestScanner plugin with Base (without Central connection)
```commandline
$ ./setup/Install.py
Installing Base on Vagrant
MCSURL=None MCSTOKEN=None
Connecting to 127.0.0.1 (vagrant)
Connection to 127.0.0.1 closed.
Running ./sspl-plugin-testscanner/build64/sdds/install.sh on Vagrant
Connecting to 127.0.0.1 (vagrant)
Connection to 127.0.0.1 closed.
Running ./sspl-plugin-audit/build64/sdds/install.sh on Vagrant
Connecting to 127.0.0.1 (vagrant)
Connection to 127.0.0.1 closed.
```

##### Verify that the TestScanner plugin is running
```commandline
$ vagrant ssh ubuntu

vagrant@vagrant$ ps -ef | grep [T]estScanner
sophos-+ 28144 27823  0 11:51 ?        00:00:00 /opt/sophos-spl/plugins/TestScanner/bin/testscanner

vagrant@vagrant$ sudo su
root@vagrant$ tail /opt/sophos-spl/plugins/TestScanner/log/TestScanner.log
0       [2019-01-09T11:51:24.626.289] DEBUG [2850219904] TestScanner <> Plugin Callback Started
0       [2019-01-09T11:51:24.626.551]  INFO [2850219904] pluginapi <> Plugin initialized: TestScanner
0       [2019-01-09T11:51:24.626.674]  INFO [2850219904] pluginapi <> Registering 'TestScanner' with management agent
1       [2019-01-09T11:51:24.627.126]  INFO [2850219904] TestScanner <> Request SAV policy
1       [2019-01-09T11:51:24.627.134]  INFO [2850219904] pluginapi <> Request policy message for AppId: SAV
1       [2019-01-09T11:51:24.627.358] ERROR [2850219904] pluginapi <> Protocol Serializer message error: No policy available
1       [2019-01-09T11:51:24.627.381] ERROR [2850219904] pluginapi <> Invalid reply, error: No policy available
1       [2019-01-09T11:51:24.627.407] ERROR [2850219904] TestScanner <> Request of Policies for SAV failed. Error: Invalid reply, error: No policy available
1       [2019-01-09T11:51:24.627.421]  INFO [2850219904] TestScanner <> Entering the main loop

root@vagrant$ exit

```
Note that it does not receive a policy because there is no Central connection in this scenario.


Running Robot Tests
===================

If not done already by *./setup/Setup.sh*, create a link to the tests/remoterobot.py inside the everest-systemproducttests (For example: *ln -s ../tests/remoterobot.py robot*)
Run tests as:
```python
./robot [any argument you would want to give to robot tests]
``` 

For example: 
```commandline
./robot --help
```


This script will check if the virtual machine is running and if it is not, it will bring up and setup the virtual machine.
After this, it will translate the arguments to the target path as in the virtual machine and write to the file tmpscript.sh
Finally, it will call: *vagrant ssh -c tmpcript.sh*
This call will ssh into the virtual box machine and run the robot test and bring back the results.

If you want to ssh into the machine and re-run the tests, for example, for debug, do:

```commandline
  vagrant ssh ubuntu
  cd /vagrant
  ./tmpscript.sh
```

Helper Scripts List
=========================================
*./setup/Setup.sh* : setup Ubuntu machine ready for SSPL development

*./setup/Build.sh* : build all products listed in *gitRepos.txt*

*./setup/Install.py* : install Base and Plugins to Ubuntu vagrant VM

*./setup/copyPlugin.sh* : copies and renames exampleplugin for rapid plugin prototyping  

*./setup/installvagrant.sh* : installs vagrant

*./setup/installsav.sh* : installs SAV to Ubuntu machine using dogfood account

Other Vagrant Boxes
===================

The following 3 vagrant definitions can be used for AuditD work but will not run the robot tests. Note to run the amazon_linux box you may need to run *./setup/installvagrant.sh* to install the vagrant-aws plugin.

amazon_linux
centos
rhel

To start a vagrant instance use

```commandline
vagrant up centos
```

To start all instances use
```commandline
vagrant up
```

To ssh to a vagrant instance use

```commandline
vagrant ssh rhel
```

The amazon_linux vagrant machine will start an AWS instance. To avoid wasting resources you should either halt or shutdown this box when you are finished using it
```commandline
vagrant halt amazon_linux
```
or
```commandline
vagrant destroy amazon_linux
```

You may need to run "aws configure" on your pairing station to run the amazon_linux box if you haven't already. to do this:

'''
aws configure
'''
You will then be prompted for an:
    AWS Access Key ID -> Get from password state
    AWS Secret Access Key -> Get from password state
    Default region name -> Usually this is eu-west-1
    Default output format -> Leave blank

For more information on how to create and edit boxes see the wiki: https://wiki.sophos.net/display/SAVLU/How+to+create+and+edit+a+vagrant+machine
There is also more information about the boxes setup that can be found in the Vagrantfile

Useful Vagrant Commands
=======================

After running the test suite the virtual machine will remain running.
You may shut it down by running:

```commandline
vagrant halt ubuntu
```

You may also completely remove the virtual machine by:
```commandline
vagrant destroy ubuntu
```

To find the status of installed vagrant boxes
```commandline
vagrant status
```

To list the installed vagrant boxes
```commandline
vagrant box list
```

To remove a box from vagrant
```commandline
vagrant box remove <box>
```

For further information on vagrant, check [vagrant user guide](https://www.vagrantup.com/intro/getting-started/index.html).
