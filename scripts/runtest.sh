#!/bin/bash
#  
# Run a specific selection of tests, or all of them.
# Stop on the first failed test.

testCount=$(find ../src -type f -name '*_test.cpp' | wc -l)
realCount=$(find . -type f -name '*_test' | wc -l)

if [[ $testCount -ne $realCount ]]; then
    echo
    echo "Not all tests were compiled into binaries."
    echo "Found: ${realCount}"
    echo "Test sources: ${testCount}"

    echo 'Run `make all` to recompile.'
    echo
    exit 1
fi

tests=$(find . -type f -name '*_test')
failed=

for _test in $tests; do
    ./${_test}
    ret=$?
    if [[ $ret -ne 0 ]]; then
        failed=$_test
        echo "Test failed: $(basename $_test)"
        break
    fi
done

# Print summary
echo "========= + SUMMARY + =========="
echo
echo "Tests ran:"

for t in $tests; do
    echo "    $(basename $t)"
done

if [[ "$failed" ]]; then
    echo
    echo "Test failed: $failed"
    echo
else
    echo
    echo "All tests passed!"
    echo
fi

echo "========= - SUMMARY - =========="

