# CI notes

## To release a new version

- tag a commit on master with the version number `x.y.z`

- [travis](https://travis-ci.org/spatial-model-editor/spatial-model-editor) builds will generate new [Github](https://github.com/spatial-model-editor/spatial-model-editor/releases) & [PyPI](https://pypi.org/project/sme/) releases

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

  - <https://github.com/spatial-model-editor/sme_deps_common>

      - this repo in turn includes: <https://github.com/spatial-model-editor/sme_deps_llvm>

  - <https://github.com/spatial-model-editor/sme_deps_dune>

  - <https://github.com/spatial-model-editor/sme_deps_qt5>


- the linux Python Wheel builds use these custom docker containers:

  - <https://github.com/spatial-model-editor/sme_manylinux1_x86_64>

  - <https://github.com/spatial-model-editor/sme_manylinux2010_x86_64>
