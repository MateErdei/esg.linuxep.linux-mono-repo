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
* Alter CMakeLists.txt to the new name and rigid ID
* Run tools/changeNamespace.sh <namespace> to change the namespace of plugin/*
* Add to Local Jenkins builds
* Add to Artisan CI builds

