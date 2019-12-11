# Modular Anti-Virus Plugin for SSPL

Simple plugin to do AV for SSPL.

----

Implement command-line AV, and possibly other sorts of AV.
Will use SUSI libraries to implement detection.

----


TODO:
This is a base for developing plugins.

Fork this repo for each new plugin then in the new repo:

* Alter Jenkinsfile to the new name
* Alter localjenkins/Jenkinsfile to the new name
* Alter build-files/release-package.xml to the new name
* Alter CMakeLists.txt to the new project name
* Add to Local Jenkins builds
* Add to Artisan CI builds

Note: the 4 first steps can be simplified by running from sspl-tools


```bash
# ./setup/setPluginName.sh <path:sspl-plugin-template> --name <newname> 
#example 
./setup/setPluginName.sh sspl-plugin-audit --name sspl-audit --name
```


The project name etc, can be customised by the build.sh and CMakeLists.txt
but a default is passed from release-package.xml

