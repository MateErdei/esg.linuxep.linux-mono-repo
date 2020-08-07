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