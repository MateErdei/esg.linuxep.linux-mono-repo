# Event Journaler

Derived from Template Plugin:

Description of Example Plugin and Architecture: https://wiki.sophos.net/display/SAVLU/Example+Plugin+Architecture

Ensure you have setup SSH keys in Stash, and have permission to clone this repo and the Everest-SystemProductTests repo.
Email ethan.vince-urwin@sophos.com to get repository access.

----

This is the component that records published events into Event Journals.



For setting up Bullseye coverage on another plugin:
* Alter build-files/release-package.xml coverage mode to have the new plugin name, and also the publish section to get the outputs to the correct path
* Alter build/bullseye/uploadResults.sh to have the new plugin name in COV_HTML_BASE
* Alter pipeline/pipeline.py to:
    - have the correct plugin name for COVFILE, 
    - add a new jenkins job (copy it from the template's own job) and replace the SYSTEM_TEST_BULLSEYE_JENKINS_JOB_URL, 
    - replace all variables that have "plugin-template" with the new plugin,
    - change the get_inputs function to match what is in release-package.xml
* Alter the TA directory to add/change robot tests
* Alter build.sh to use the plugin's name instead of "plugin-template" for COVFILE, COVHTML, COV_HTML_BAS
