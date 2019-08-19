#!/bin/bash

# Clean up
rm -rf report
rm -f cov.info

# Generate coverage report
lcov -c --directory . --output-file cov.info
genhtml cov.info --output-directory report

