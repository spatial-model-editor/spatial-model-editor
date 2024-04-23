# CI notes

## To release a new version

- Ensure that the [CMakeLists.txt](https://github.com/spatial-model-editor/spatial-model-editor/blob/main/CMakeLists.txt#L7) version number matches the one in [setup.py](https://github.com/spatial-model-editor/spatial-model-editor/blob/main/setup.py#L95)

- Tag this commit on main with the same version number `x.y.z`

- Github Actions will run and generate [Github](https://github.com/spatial-model-editor/spatial-model-editor/releases) & [PyPI](https://pypi.org/project/sme/) releases

- Documentation at [readthedocs](https://spatial-model-editor.readthedocs.io) will be updated

## Pre-release "latest" binaries

- The binaries in the [latest pre-release](https://github.com/spatial-model-editor/spatial-model-editor/releases/tag/latest) are automatically updated on each commit to main

## Dependencies

### Static libraries

- The CI builds use statically compiled versions of all dependencies

- These are provided as binary releases from [sme_deps](https://github.com/spatial-model-editor/sme_deps)

- To use a new release, update `SME_DEPS_VERSION` in [ci/get-libs.sh](https://github.com/spatial-model-editor/spatial-model-editor/blob/main/ci/getlibs.sh#L6)

- Note that sme_deps depends on [sme_deps_common](https://github.com/spatial-model-editor/sme_deps_common), which in turn depends on [sme_deps_qt](https://github.com/spatial-model-editor/sme_deps_qt) and [sme_deps_llvm](https://github.com/spatial-model-editor/sme_deps_llvm), so a new sme_deps release may first require a new release from these repos.

### Docker images

- The linux Python Wheel CI builds use this custom docker container:

  - <https://github.com/spatial-model-editor/sme_manylinux_x86_64>

- To use a new tag, update `CIBW_MANYLINUX_X86_64_IMAGE` in [.github/workflows/wheels.yml](https://github.com/spatial-model-editor/spatial-model-editor/blob/main/.github/workflows/wheels.yml#L30-L31)
