#!/bin/bash

# Ubuntu ASAN build script

set -e -x

pip install pre-commit==2.14.0

# use clang-format 12
sudo update-alternatives --remove-all clang-format || echo "nothing to remove"
sudo update-alternatives --install /usr/bin/clang-format clang-format /usr/bin/clang-format-12 100

pre-commit run --all-files || echo "formatting changes required"
