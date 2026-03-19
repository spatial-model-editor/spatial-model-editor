# CLAUDE.md

## Project overview

Spatial Model Editor is a GUI editor to create, edit and simulate spatial SBML models of bio-chemical reactions. Simulation uses the dune-copasi FEM solver on a triangular mesh, or a voxel-based finite difference solver. Written in C++20, cross-platform (Linux, Windows, Mac). Also has Python and CLI interfaces.

## Repo structure

Key folders:
- `core/` — main library code (mesh, model, simulate)
- `gui/` — GUI code (widgets, dialogs, tabs)
- `sme/` — Python bindings
- `app/` — GUI app binary
- `cli/` — command-line interface
- `test/` — test binaries
- `docs/` — documentation
- `ext/` — external library submodules (ignore this folder)

Implementation, test, and benchmark code for a component are co-located: `X.hpp`, `X.cpp`, `X_t.cpp`, and optionally `X_bench.cpp`.

## Building and testing

You can assume the script `ci/local-dev-setup.sh` has been run.
All commands run from `build-release/`:

```bash
ninja                                           # build
ninja test                                      # run all non-gui tests in parallel
./test/tests "[tagname]"                        # run a subset of catch2 tests
xvfb-run -a bash -c 'jwm & sleep 1 && ./test/tests "[gui]"'   # run GUI tests headlessly on Linux
xvfb-run -a bash -c 'jwm & sleep 1 && ./test/tests "TestName"' # run a specific GUI test
```

Python tests from `build-release/sme`:
```bash
python -m pytest ../../sme/test
```

For segfault debugging, use `build-asan/`:
```bash
ASAN_OPTIONS="halt_on_error=0" ninja            # build with ASAN
./test/tests "[tagname]"                        # re-run to see ASAN output
```

## Code style and conventions

- C++20
- Formatting: run `prek run -a` (install with `pip install prek` if needed)
- Tests use Catch2
- Every PR should include meaningful tests
- Update `CHANGELOG.md` and user docs for user-visible changes
