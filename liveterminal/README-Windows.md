# Development instructions

## Continuous Integration dashboard

ESG-CI: <https://portal.sophos-ops.com/component/winep/liveterminal>

## Building

Update TAP tools:

```sh
$ python -m pip install tap --upgrade
```

### List builds, fetch inputs and run a build

```sh
$ tap ls
$ git clean -dxf
$ tap fetch liveterminal.build.windows
$ tap build liveterminal.build.windows
```

## Coverage build

Every ESG-CI build currently generates coverage.

### Bullseye/C++ coverage

After successful completion, the unit test coverage file can be found in the build outputs and the functional test coverage files in the test outputs.

Merge .cov files by opening CMD, navigating to the folder that contains the .cov files and running (note: no space after -f):

    covmerge -c -fMerged.cov *.cov

### Rust coverage
Coverage builds generate Rust code coverage as well. As with C++ coverage, unit test coverage files can be found on uk-filer6 and
functional test coverage files will be uploaded to artifactory. However, as the process to generate Rust-coverage reports is quite long, 
you will need to use the rt.py script:

    $ ./coverage/rt/rt.py rc -c liveterminal -br develop -grc ./source/_input/rust/installer/Rust-1.68.2/cargo/bin/grcov.exe

Running ```rt.py rust-coverage -h``` will display more documentation and sample commands.

The script will generate HTML coverage reports.
