# AGENTS.md

## Project overview

Spatial Model Editor a user friendly GUI editor to create, edit and simulate spatial SBML models of bio-chemical reactions.
Simulation can be done using the dune-copasi Finite Element Method solver on a triangular mesh, or a voxel-based finite difference solver.
It is written in c++20 and is cross platform: works on Linux, Windows and Mac. There is also a Python and command-line interface.

## Repo structure

Key folders

  - core/: the main library code, with key subfolders for mesh, model and simulate
  - gui/: the GUI code, with key subfolders for widgets, dialogs and tabs
  - sme/: the Python bindings

Other folders

  - docs/: the documentation
  - app/: the GUI app
  - benchmark/: the benchmark binaries
  - ci/: scripts for CI jobs
  - cli/: the command-line interface
  - test/: the test binaries
  - ext/: external library submodules: ignore this folder

Note that the implementation code, test code and optional benchmark code for a component are all located together.

## How to run locally

We have many dependencies, so we statically compile all of them in CI, and then provide compiled binaries built in CI for our users.
You can assume the script ci/local-linux-dev-setup.sh has been run, i.e.

  - in build-sme-release/ you can do
    - `ninja` to build
    - `ninja test` to run all non-gui tests in parallel
    - `./test/tests` with suitable tags to run a subset of the catch2 test suite.
  - in build-sme-release/sme you can do
    - `python -m pytest ../../sme/test` to run the python tests
  - if a test segfaults, in build-sme-asan/ you can do
    - `ASAN_OPTIONS="halt_on_error=0" ninja` to build
    - `./test/tests` with suitable tags to re-run a segfaulting test to see the ASAN output.
  - note that running the full test suite can take a few minutes

## Conventions and patterns

- each component X has a X.hpp, X.cpp and X_t.cpp, where the latter contains the tests
- some components also have benchmarks in X_bench.cpp

## PR expectations

Every PR should include meaningful tests that can be read as developer documentation of the functionality.
If there are any user-visible changes the user documentation and CHANGELOG.md should be updated accordingly.
Formatting should be done using the pre-commit hooks, these can be run manually with `prek run -a`.
If `prek` is not installed, install it with `pip install prek`.
