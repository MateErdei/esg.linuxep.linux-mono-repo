# Base
### Getting Started
Once this repo is cloned, run the `setup_build_tools.sh` script to setup the needed pre-reqs for building. 

You're now ready to build base by running `build.sh`.

If you're using an IDE then make sure to set the toolchain environment to evaluate `setup_env_vars.sh`, in CLion this is done from: `File | Settings | Build, Execution, Deployment | Toolchains`, and then setting the environment file.

Built artifacts are gathered into the `output` directory.

### Coverage build with bullseye
1. Trigger CI coverage build with bullseye:
    * on build pipeline build base with `mode=coverage`
2. Results for both static analysis and coverage are published to:
   * filer6 path: `output/analysis`
   * artifactory path: `analysis.zip`

### Static analysis with cppcheck
This runs automatically during pipeline builds. However, it's also possible to run it manually, either: 
* Trigger CI static analysis with cppcheck only:
    * on build pipeline build base with `mode=analysis`
* Trigger local analysis with cppcheck only:

      ./build.sh --analysis --no-build

   Results are placed in `output/analysis`.

Once complete, you can view the html report in `index.html`.
    
### Unified Pipeline Operations
1) See all tap build options:

       tap ls

2) Fetch build inputs as specified in `build/release-package.xml`:

       tap fetch sspl_base.build.{option}

3) Build everest-base on local machine:

       tap build sspl_base.build.{option}

4) Build base on local machine and run tests:

       tap run sspl_base.integration.{option} --build-backend=local

5) Run tests on branch with unified pipeline build:

       tap run sspl_base.integration.{option}

6) To specify your build mode for tests (options are debug, release, and coverage):

       TAP_PARAMETER_MODE={mode} tap run sspl_base.integration.{option}