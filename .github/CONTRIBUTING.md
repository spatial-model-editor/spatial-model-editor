# Contributing

## Getting started

```
git clone --recursive https://github.com/spatial-model-editor/spatial-model-editor.git
cd spatial-model-editor
mkdir build
cd build
cmake ..
```
Note: there are many dependencies, some of which are currently not
very convenient to install.
For some you may find the pre-compiled static libraries used by
the CI builds useful - see [ci/README](../ci/README.md) for more details.

## Style guide

- C++17

- each component `X` has a `X.hpp`, `X.cpp` and `X_t.cpp`, where the latter contains the tests

- some components also have benchmarks in `X_bench.cpp`

- follow [sonarcloud](https://sonarcloud.io/dashboard?id=spatial-model-editor_spatial-model-editor) static analysis suggestions where possible

- avoid platform-dependent code: we support linux / macOS / windows

## Pull requests

- pull requests are built and tested on [github actions](https://github.com/spatial-model-editor/spatial-model-editor/actions)

- static analysis report from [sonarcloud](https://sonarcloud.io/dashboard?id=spatial-model-editor_spatial-model-editor)

- test coverage report from [codecov](https://codecov.io/gh/spatial-model-editor/spatial-model-editor)

- documentation will also be built, see [docs/README](../docs/README.md) for more information

- code formatting will be checked and fixed if necessary (see pre-commit hooks below)

- all must pass before a pull request can be merged

- see [ci/README](../ci/README.md) for more information on how this works

## pre-commit hooks

- [pre-commit](https://pre-commit.com/) hooks to check & fix code formatting are available

- to use them, first install pre-commit:

  - `pip install pre-commit`

- then in the directory where you cloned this repo run:

  - `pre-commit install`

- every subsequent git commit will check for (and fix)

  - trailing whitespace

  - end of file newline

  - yaml errors

- and run these tools:

  - [cmake-format](https://cmake-format.readthedocs.io/)

  - [black](https://black.readthedocs.io/)

  - [nbstripout](https://pypi.org/project/nbstripout/)

  - [clang-format](https://clang.llvm.org/docs/ClangFormat.html)

- a CI job also runs these checks, and will automatically add a commit if any formatting changes are required
