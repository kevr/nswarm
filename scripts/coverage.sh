#!/bin/bash

# Clean up
rm -rf report
rm -f cov.info

# Generate coverage report
lcov -c --directory . --output-file cov.info

# Filter out unwanted report sources
lcov --remove cov.info -o cov.filtered.info \
    '*json.hpp*' \
    '*boost*' \
    '/usr/include*' \
    '9.1.0*'

genhtml cov.filtered.info --output-directory report

