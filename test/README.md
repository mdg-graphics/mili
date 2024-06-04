# Mili and Xmilics Testing

The testing setup for Mili and Xmilics is as follows:

### Test Definitions

The tests are defined in the following files:
- `mili_test_definitions.py`
- `xmilics_test_definitions.py`

These files contain a dictionary called `mapping` which contains all of the
information for each suite of tests. The files for each suite can be found in various directories
under `mili/test/mili` and `mili/test/xmilics`.

### Testing Scripts

The testing scripts are written in Python and are in the git submodule `mdgtest` in the directory
`mili/test`. When first cloning the Mili git repository the `mdgtest` directory is empty
and users will need to run the command `git submodule update --init` to get the testing scripts.

The main test driver is `RunTest.py`. There are many command line arguments that can be provided
to the script to change its behavior. These can be seen by running `mdgtest/RunTest.py --help`.

### Running Tests

You shouldn't run the test suite on a login node, so you will have to get access to a debug/batch node. This can be done by running the command:
```
# on RZWhippet
sxterm 1 56 60 -p pdebug -A wbronze
```

You can now run the test suite as shown below:

### Mili Library
```
# All Suites/Tests
mdgtest/RunTest.py -c <path-to-build-dir> -s all

# One or More suites
mdgtest/RunTest.py -c <path-to-build-dir> -s <suite-1> -s <suite-2>

# One or More tests
mdgtest/RunTest.py -c <path-to-build-dir> -t <test-1> -t <test-2>
```

### Xmilics
```
# All Suites/Tests
mdgtest/RunTest.py -c <path-to-code> -s all

# One or More Suites
mdgtest/RunTest.py -c <path-to-code> -s <suite-1> -s <suite-2>

# One or More Tests
mdgtest/RunTest.py -c <path-to-code> -t <test-1> -t <test-2>
```

### Log files

The testing scripts will generate log files to store the results of testing (They are also written to the screen). These
log files have the format `log.<date>_<time>`. Additionally the scripts generate 2 additional logs `log.latest` and `log.previous`
which are links to the most recent log file and the previous log file.

### Testing files

The tests are by default run in a directory with the name `<sys_type>.mili` or `<sys_type>.xmilics`. This directory contains
sub directories for each of the suites and tests that are run.

### Rebaselining tests

It is sometimes necessary to re-baseline failing tests. This can be done easily using the flags `-b` or `--rebaseline`. These flags
tell the testing scripts to run the tests and overwrite the current baseline files with the newly generated results. Be sure to only run
this for the specific tests that require re-baselining.