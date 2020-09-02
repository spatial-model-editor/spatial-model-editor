# Contributing

## Getting started

```
git clone --recursive https://github.com/lkeegan/spatial-model-editor.git
cd spatial-model-editor
mkdir build
cd build
cmake ..
```
Note: there are many dependencies, some of which are currently not very convenient to install. For some you may find the pre-compiled static libraries used by the CI builds useful - see [ci/README](../ci/README.md) and [.travis.yml](../.travis.yml) for more details.

## Style guide

- C++17

- use `clang-format` to format the code

- each component `X` has a `X.hpp`, `X.cpp` and `X_t.cpp`, where the latter contains the tests

- follow [sonarcloud](https://sonarcloud.io/dashboard?id=lkeegan_spatial-model-editor) static analysis suggestions where possible

- avoid platform-dependent code: we support linux / macOS / windows

## Pull requests

- pull requests are built and tested on [travis](https://travis-ci.org/lkeegan/spatial-model-editor)

- static analysis report from [sonarcloud](https://sonarcloud.io/dashboard?id=lkeegan_spatial-model-editor)

- test coverage report from [codecov](https://codecov.io/gh/lkeegan/spatial-model-editor)

- documentation will also be built, see [docs/README](../docs/README.md) for more information

- all must pass before a pull request can be merged

## pre-commit hooks

- [pre-commit](https://pre-commit.com/) hooks to check & fix code formatting are available

- they are optional, i.e. their use is not (yet) enforced by CI

- to use them, first install pre-commit:

  - `pip install pre-commit`

- then in the directory where you cloned this repo run:

  - `pre-commit install`

- every subsquent git commit will check for (and fix)

  - trailing whitespace

  - end of file newline

  - yaml errors

- and run these tools:

  - [cmake-format](https://cmake-format.readthedocs.io/)

  - [black](https://black.readthedocs.io/)
