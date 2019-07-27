#!/bin/bash
#  
# Run a specific selection of tests, or all of them.
# Stop on the first failed test.

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

