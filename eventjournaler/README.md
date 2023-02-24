# Event Journaler

Derived from Template Plugin:

Description of Example Plugin and Architecture: https://wiki.sophos.net/display/SAVLU/Example+Plugin+Architecture

Ensure you have setup SSH keys in Stash, and have permission to clone this repo and the Everest-SystemProductTests repo.
Email ethan.vince-urwin@sophos.com to get repository access.

----

This is the component that records published events into Event Journals.


### Unified Pipeline Operations
1) See all tap build options:

       tap ls

2) Fetch build inputs as specified in `build-files/release-package.xml`:

       tap fetch event_journaler.build.{option}

3) Build event journaler on local machine:

       tap build event_journaler.build.{option}

4) Build base on local machine and run tests:

       tap run event_journaler.integration.{option} --build-backend=local

5) Run tests on branch with unified pipeline build:

       tap run event_journaler.integration.{option}

6) To specify your build mode for tests (options for modes are release and coverage):

       TAP_PARAMETER_MODE={mode} tap run event_journaler.integration.{option}