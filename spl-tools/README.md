General Tools For sspl
======================


Setting up developer environment for sspl
=========================================

Run the sspl-repositories.sh. This will clone the repositories of sspl putting them as children of the sspl-tools
  which allow them to share the resources and enable the tests to run easily.

Install vagrant: *./setup/installvagrant.sh*

Setup the vagrant with *vagrant up*

Helper scripts
--------------
*./setup/clionbuildall.sh* : build the products as clion debug does which can be handy for updating all the install/dist products. 


Running Robot Tests
===================

create a link to the tests/remoterobot.py inside the everest-systemproducttests (For example: *ln -s ../tests/remoterobot.py robot*)
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
