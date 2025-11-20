#!/bin/bash
# norns C/C++ test runner – builds and runs C/C++ unit tests
#
# usage:
#   ./test.sh                    run all tests
#   ./test.sh --test-dry-run     preview discovered tests without building
#   ./test.sh --skip-self-test   skip test runner validation (faster)
#   ./test.sh -j 8               run tests in parallel with 8 jobs
#   ./test.sh --targets=test_matron_args               run single target
#   ./test.sh --targets=test_matron_args,test_crone    run multiple targets

echo -e "\033[1mrunning norns C/C++ tests\033[0m"
echo "-"

# run unit tests (--alltests forces execution even if cached)
./waf --alltests test "$@"

