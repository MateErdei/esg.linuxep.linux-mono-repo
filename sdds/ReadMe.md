Warehouse Generation
====================

This repo contains scripts to generate an SDDS warehouse file set and matching customer files. They need to be run on a machine that has the SDDS back-end installed.

Build initiation
----------------

The warehouse build is controlled by the \[Name of new build system\] build system, and is initiated according to the instructions in _Jenkinsfile_.

Build specification
----

The build inputs, outputs and commands are defined in _build/release-package.xml_.

Scripts
----

### locations.py

This script defines some directory paths and the final URL that the warehouse will be served from.

The URL is needed to create customer files and \[ is it used in the SDDS merkle trees at all? \].

### prepare.py

This script creates empty directories for the other scripts to write files to.

Depends on _locations.py_

### collate.py

This script packages the HIPS rules files into an SDDS ready package.


### GenerateWarehouse.py

Driver script that generates the final warehouse definition file, runs SDDSImport.exe to build the warehouse file set and creates the customer file.
 
#### warehouse-def.py

Generates the warehouse definition file that is consumed by SDDSImport.exe.

#### customerFile.py

Creates a customer file that points at the warehouse.
