General Tools For sspl
======================


Setting up developer environment for sspl
-----------------------------------------

Run the sspl-repositories.sh:   this will clone the repositories of sspl putting them as children of the sspl-tools which allow them to share the resources and enable the tests to run easily. 

Install vagrant: ./setup/installvagrant.sh

Setup the vagrant machine: 
Run: vagrant up 



Helper scripts
--------------
./setup/clionbuildall.sh : build the products as clion debug does which can be handy for updating all the install/dist products. 


Running Robot Tests
-------------------

create a link to the remoterobot.py inside the everest-systemproducttests (ln -s ../tests/remoterobot.py robot)
Run tests as: 
./robot [any argument you would want to give robot tests]
For example: 
./robot --help






