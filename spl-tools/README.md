General Tools For sspl
======================


Setting up developer environment for sspl
=========================================

Run the sspl-repositories.sh:   this will clone the repositories of sspl putting them as children of the sspl-tools
which allow them to share the resources and enable the tests to run easily.

Install vagrant: ./setup/installvagrant.sh

Setup the vagrant machine: 
Run: vagrant up 

Install sav *./setup/installsav.sh*

Helper scripts
--------------
./setup/clionbuildall.sh : build the products as clion debug does which can be handy for updating all the install/dist products. 


Running Robot Tests
===================

create a link to the tests/remoterobot.py inside the everest-systemproducttests (For example: ln -s ../tests/remoterobot.py robot)
Run tests as: 
./robot [any argument you would want to give robot tests]
For example: 
./robot --help

This script will check if the virtual machine is running and if it is not, it will bring up and setup the virtual machine.
After this, it will translate the arguments to the target path as in the virtual machine and write to the file tmpscript.sh
Finally, it will call: * vagrant ssh -c tmpcript.sh *
This call will ssh into the virtual box machine and run the robot test and bring back the results.

If you want to ssh into the machine and re-run the tests, for example, for debug, do:
** vagrant ssh **
** cd /vagrant **
** ./tmpscript.sh **

After running the first test the virtual machine will be running. You may shut it down by running:
** vagrant halt **

You may also completely remove the virtual machine by:
** vagrant destroy **









