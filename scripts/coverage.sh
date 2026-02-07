#!/usr/bin/env bash

set -ex
source .venv/bin/activate

cmake --workflow --preset build-and-coverage "$@"

# Check if the directory exists before creating it
if [ ! -d "coverage/html" ]; then
    mkdir -p coverage/html
fi

python -m gcovr -r . -e ".*\.h" -e src/impl/src -e build-cov/_deps/ -e test/ \
    --html-details coverage/html/index.html \
    --markdown coverage/coverage.md \
    --markdown-summary=coverage/summary.md

echo "Coverage report generated."
