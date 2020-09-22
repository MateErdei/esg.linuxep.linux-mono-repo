#coverage build with bullseye
1. Trigger CI coverage build with bullseye:
    * on build pipeline build base with mode=coverage
    * static anaylsis is also done whenever coverage is build is run, this so we save on spinning an extra machine 
       and we publish the results together in one place

    * RESULTS for both static analysis and coverage are published to:
      * filer6 path: _output/analysis_
      * artifactory path: analysis.zip

# static anaylsis with cppcheck
1. Trigger CI static analysis with cppcheck only:
    * on build pipeline build base with mode=analysis
    * RESULTS published to folder "analysis" both on filer6 & artifactory
        * filer6 path: _output/analysis_
        * artifactory path: analysis.zip
 
2. Trigger local analysis with cppcheck only:
    * ./build.sh --analysis --no-build
    * RESULTS in output/analysis
    
3. View the html report:
    * open index.html
    
# Unified Pipeline Operations
1) Fetch build inputs as specified in release xml
   tap fetch sspl_base
2) build ever-base on local machine
   tap run sspl_base.build --build-backend=local
3) build base on local machine and run tests
   export TAP_PARAMETER_MODE=mode -> one of "release|analysis"
   tap run sspl_base --build-backend=local
4) run tests on branch with unified pipelie build
   export TAP_PARAMETER_MODE=mode -> one of "release|coverage" - 'release' is the default so no need to export
   
   tap run sspl_base