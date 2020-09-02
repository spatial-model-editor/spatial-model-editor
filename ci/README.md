# CI notes

## To release a new version

- tag a commit on master with the version number `x.y.z`

- [travis](https://travis-ci.org/lkeegan/spatial-model-editor) builds will generate new [Github](https://github.com/lkeegan/spatial-model-editor/releases) & [PyPI](https://pypi.org/project/sme/) releases

- [readthedocs](https://spatial-model-editor.readthedocs.io) documentation will be updated

## To update the "latest" binaries

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

- the linux Python Wheel builds use these custom docker containers:

  - <https://github.com/lkeegan/sme_manylinux1_x86_64>

  - <https://github.com/lkeegan/sme_manylinux2010_x86_64>
