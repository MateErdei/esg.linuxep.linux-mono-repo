# Base
### Getting Started
Once this repo is cloned, run the `setup_build_tools.sh` script to setup the needed pre-reqs for building. 

You're now ready to build base by running `build.sh`.

If you're using an IDE then make sure to set the toolchain environment to evaluate `setup_env_vars.sh`, in CLion this is done from: `File | Settings | Build, Execution, Deployment | Toolchains`, and then setting the environment file.

Built artifacts are gathered into the `output` directory.

### Coverage build with bullseye
1. Trigger CI coverage build with bullseye:
    * on build pipeline build base with `mode=coverage`
    * static analysis is also done whenever coverage is build is run, this is so we save on spinning an extra machine, and we publish the results together in one place 
2. Results for both static analysis and coverage are published to:
   * filer6 path: `output/analysis`
   * artifactory path: `analysis.zip`

### Static analysis with cppcheck
1. Trigger CI static analysis with cppcheck only:
    * on build pipeline build base with `mode=analysis`
2. Trigger local analysis with cppcheck only:

       ./build.sh --analysis --no-build

   Results are in `output/analysis`
3. View the html report in `index.html`
    
### Unified Pipeline Operations
1) Fetch build inputs as specified in `build/release-package.xml`:

       tap fetch sspl_base

2) Build everest-base on local machine:

       tap run sspl_base.build --build-backend=local

3) Build base on local machine and run tests:

       export TAP_PARAMETER_MODE=mode -> one of "release|analysis"
       tap run sspl_base --build-backend=local

4) Run tests on branch with unified pipeline build:

       export TAP_PARAMETER_MODE=mode -> one of "release|coverage" - 'release' is the default so no need to export
       tap run sspl_base