# CI notes

## Pull requests

- Pull requests are built and tested on travis: <https://travis-ci.org/lkeegan/spatial-model-editor>

- Static analysis: <https://sonarcloud.io/dashboard?id=lkeegan_spatial-model-editor>

- Test coverage: <https://codecov.io/gh/lkeegan/spatial-model-editor>

- Both need to pass before a pull request can be merged

## pre-commit hooks

- Pre-commit hooks to check code formatting are available

- They are optional, i.e. their use is not (yet) enforced by CI

- To use them, first install pre-commit:

  - `pip install pre-commit`

- Then in the directory where you cloned this repo run:

  - `pre-commit install`

- Every subsquent git commit will check for (and fix)

  - trailing whitespace

  - end of file newline

  - yaml errors

- and run these tools:

  - cmake-format

  - black

  - markdownlint

## Deploy new version

- tag a commit on master with the version number `x.y.z`

- travis builds will generate new Github & PyPI releases

- <https://github.com/lkeegan/spatial-model-editor/releases>

- <https://pypi.org/project/sme/>

- documentation is also updated: <https://spatial-model-editor.readthedocs.io>

## Update latest binaries

- we also provide a `latest` Github release for beta testing / live-at-head users

- to update this release to the current commit on master, move the `latest` tag:

```
git push origin :refs/tags/latest
git tag -f latest
git push origin latest
```

- this will trigger travis builds that update the `latest` Github release binaries

## Dependencies

- the CI builds use statically compiled versions of all dependencies

- these are provided as binary releases from these repos:

  - <https://github.com/lkeegan/dune-copasi-static>

  - <https://github.com/lkeegan/libsbml-static>

  - <https://github.com/lkeegan/qt5-static>

  - <https://github.com/lkeegan/llvm-static>
