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

if [ -z "$failed" ]; then
    echo "All tests passed!"
fi

# Print summary
echo "========= +SUMMARY+ =========="
echo "Tests ran: $(for t in $tests; do echo -n $(basename $t); echo -n ' '; done)"

if [[ "$failed" ]]; then
echo "Test failed: $failed"
fi

echo "========= -SUMMARY- =========="

