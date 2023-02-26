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

Note: the 4 first steps can be simplified by running from sspl-tools


```bash
# ./setup/setPluginName.sh <path:sspl-plugin-template> --name <newname> 
#example 
./setup/setPluginName.sh sspl-plugin-audit --name sspl-audit --name
```


The project name etc, can be customised by the build.sh and CMakeLists.txt
but a default is passed from release-package.xml

#coverage build with bullseye and static anaylsis with cppcheck
1. Trigger CI coverage build with bullseye:
    * on build pipeline build edr with mode=coverage
    * on the test pipeline build edr and select coverage=yes the link above will be updated with the latest results
    * static anaylsis is also done whenever coverage is build is run, this so we save on spinning an extra machine 
       and we publish the results together in one place

    * RESULTS for both static analysis and coverage are published to:
      * output/coverage or artificatory and filer6
      
1. Trigger CI static analysis with cppcheck only:
    * on build pipeline build edr with mode=analysis
    * RESULTS published to folder "analysis" both on filer6 & artifactory


### Unified Pipeline Operations
1) See all tap build options:

       tap ls

2) Fetch build inputs as specified in `build-files/release-package.xml`:

       tap fetch edr_plugin.build.{option}

3) Build on local machine:

       tap build edr_plugin.build.{option}

4) Build on local machine and run tests:

       tap run edr_plugin.{integration|component}.{option} --build-backend=local

5) Run tests on branch with unified pipeline build:

       tap run edr_plugin.{integration|component}.{option}

6) To specify your build mode for tests (options for modes are release and coverage):

       TAP_PARAMETER_MODE={mode} tap run edr_plugin.{integration|component}.{option}

