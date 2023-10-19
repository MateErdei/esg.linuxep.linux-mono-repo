## Running Robot Tests


The robot tests require that your dev machine has been set up (https://sophos.atlassian.net/wiki/spaces/LD/pages/39042169567/How+To+Build+Base+and+setup+a+dev+machine) and that the esg.linuxep.sspl-tools/setup_vagrant_wsl.sh script has been run.
1) Activate the tap environment (if not already activated by IDE - to set this up see dev setup page) When in "edr" directory:
```sh
source ../base/tap_venv/bin/activate
```
2) Change directory to "TA"
```sh
cd TA
```
3) Run a Robot test
```sh
./robot -t <Test Name> .
# For example:
./robot -t "EDR Plugin outputs XDR results and Its Answer is available to MCSRouter" .
```

You can also run suites or pass in any other robot arguments to the ./robot wrapper:

```sh
# Run all tests
./robot .

# Run a suite of tests
./robot --suite "EDRAndBase" .

# Run a single test
./robot --test "A test name" .

# Exclude tests by tag by running the following
./robot --exclude manual .
```

#### In order to run these tests in CI/TAP:
```sh
# Run the pytests and robot tests
tap run edr_plugin --debug-loop                         
```

```sh
# Run the pytests only    
tap run edr_plugin.test.component --debug-loop                                
```

```sh
# Run the robot tests only
tap run edr_plugin.test.integration --debug-loop                                 
```

```sh
 # Run both pytests and robot tests for bullseye coverage, the branch must have a coverage build.
 # i.e. develop or branches end with 'coverage'
tap run edr_plugin.coverage --debug-loop


```
#### Working with CI portal
a) Build portal for EDR - manually produce a coverage on CI

choose -> "Build with Parameters" and set mode = coverage

b) Test Pipeline for EDR - manually run coverage on any branch with a valid build

choose -> "Build with Parameters" -> coverage = yes              

#### Unified pipeline
1) Fetch build inputs as specified in release xml
    tap fetch edr_plugin
2) build edr on local machine
    tap run edr_plugin.build --build-backend=local
3) build edr on local machine and run tests
    export TAP_PARAMETER_MODE=mode -> one of "release|analysis"
    tap run edr_plugin --build-backend=local
4) run tests on branch with unified pipelie build
    export TAP_PARAMETER_MODE=mode -> one of "release|coverage"
    tap run edr_plugin
5) Run tap test on branch with classic pipline build.
    unset TAP_PARAMETER_MODE (if its set)
    tap run edr_plugin... (usual options are optional)

For information on running outside TAP see PytestScripts/README