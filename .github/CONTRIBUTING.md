# Contributing

## Getting started

There are many dependencies, some of which are currently not
very convenient to install, so the easiest way to build locally
is probably to use the pre-compiled static libraries used by
the CI builds - see [ci/README](../ci/README.md) for more details.

### Linux local build

Clone the repo including submodules:

```
git clone --recursive https://github.com/spatial-model-editor/spatial-model-editor.git
cd spatial-model-editor
```

(If you already cloned without `--recursive` you can run `git submodule update --init --recursive`)

Run the getdeps script to download the latest static libs and copy them to `/opt/smelibs`:

```
sudo ./ext/getdeps.sh
```

Make sure you have the system libraries required by Qt6 installed, for example on Ubuntu:

```
sudo apt-get install \
    libglu1-mesa-dev \
    libx11-dev \
    libx11-xcb-dev \
    libxext-dev \
    libxfixes-dev \
    libxi-dev \
    libxrender-dev \
    libxkbcommon-dev \
    libxkbcommon-x11-dev \
    libfreetype-dev \
    libfontconfig-dev \
    '^libxcb.*-dev'
```

Build using CMake, e.g.

```
mkdir build
cd build
cmake .. -GNinja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="/opt/smelibs;/opt/smelibs/lib/cmake"
ninja
```

Now you can run the tests, see [test/README.md](https://github.com/spatial-model-editor/spatial-model-editor/blob/main/test/README.md) for more information:

```
ninja test
```

The python bindings can also be independently built & installed using pip, where any required CMake arguments can be supplied
via the `CMAKE_ARGS` env var, separated by spaces:

```
CMAKE_ARGS="-DCMAKE_PREFIX_PATH='/opt/smelibs;/opt/smelibs/lib/cmake'" pip install -v .
pytest sme
```

## Style guide

- C++20

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

- pre-commit.ci will automatically add a commit to a PR with any required fixes
