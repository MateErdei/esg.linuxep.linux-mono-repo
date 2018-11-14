# Template Plugin for Everest

Derived from Example Plugin:

Description of Example Plugin and Architecture: https://wiki.sophos.net/display/SAVLU/Example+Plugin+Architecture

Ensure you have setup SSH keys in Stash, and have permission to clone this repo and the Everest-SystemProductTests repo.
Email douglas.leeder@sophos.com to get repository access.

----

This is a base for developing plugins.

Fork this repo for each new plugin then in the new repo:

* Alter Jenkinsfile to the new name
* Alter localjenkins/Jenkinsfile to the new name
* Alter build-files/release-package.xml to the new name
* Alter CMakeLists.txt to the new project name
* Add to Local Jenkins builds
* Add to Artisan CI builds

The project name etc, can be customised by the build.sh and CMakeLists.txt
but a default is passed from release-package.xml

