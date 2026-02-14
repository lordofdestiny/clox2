#!/usr/bin/env bash

set -ex
source .venv/bin/activate

cmake --workflow --preset dev-build-coverage "$@"

# Check if the directory exists before creating it
if [ ! -d "coverage/html" ]; then
    mkdir -p coverage/html
fi

python -m gcovr -r . -e ".*\.h" -e lib/impl/src -e coverage/_deps/ -e lib/**/test/ \
    --html-details coverage/html/index.html \
    --markdown coverage/coverage.md \

echo "Coverage report generated."
