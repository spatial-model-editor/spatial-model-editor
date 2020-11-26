# CI notes

## To release a new version

- Tag a commit on master with the version number `x.y.z`

- Github Actions will run and generate [Github](https://github.com/spatial-model-editor/spatial-model-editor/releases) & [PyPI](https://pypi.org/project/sme/) releases

- Documentation at [readthedocs](https://spatial-model-editor.readthedocs.io) will be updated

## To update the "latest" binaries

- We provide a `latest` Github release for beta testing / live-at-head users

- To update this release to the current commit on master, move the `latest` tag:

```
git push origin :refs/tags/latest
git tag -f latest
git push origin latest
```

- This will trigger Github Action builds that update the `latest` Github release binaries

## Dependencies

- The CI builds use statically compiled versions of all dependencies

- These are provided as binary releases from these repos:

  - <https://github.com/spatial-model-editor/sme_deps_common>

      - This repo in turn includes: <https://github.com/spatial-model-editor/sme_deps_llvm>

  - <https://github.com/spatial-model-editor/sme_deps_dune>

  - <https://github.com/spatial-model-editor/sme_deps_qt5>


- The linux Python Wheel builds use these custom docker containers:

  - <https://github.com/spatial-model-editor/sme_manylinux2010_x86_64>

  - <https://github.com/spatial-model-editor/sme_manylinux2010-pypy_x86_64>
