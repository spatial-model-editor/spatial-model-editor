# spatial-model-editor [![linux/osx build status](https://travis-ci.org/lkeegan/spatial-model-editor.svg?branch=master)](https://travis-ci.org/lkeegan/spatial-model-editor) [![windows build status](https://ci.appveyor.com/api/projects/status/0m87yyaalrrj5ndn?svg=true)](https://ci.appveyor.com/project/lkeegan/spatial-model-editor) [![documentation status](https://readthedocs.org/projects/spatial-model-editor/badge/)](https://spatial-model-editor.readthedocs.io/en/latest/)

GUI spatial model editor prototype, download the latest executables here:

  - [linux](https://github.com/lkeegan/spatial-model-editor/releases/latest/download/spatial-model-editor)
  - [mac](https://github.com/lkeegan/spatial-model-editor/releases/latest/download/spatial-model-editor.dmg)
  - [windows](https://github.com/lkeegan/spatial-model-editor/releases/latest/download/spatial-model-editor.exe)

The initial goal is a partial re-implementation of the spatial model editing part of https://github.com/fbergmann/edit-spatial using C++/Qt5, with all dependencies statically linked so that it can be supplied as a stand-alone compiled executable for linux, windows and osx.

## WP1a
Allow the user to describe a spatial model: [project status](https://github.com/lkeegan/spatial-model-editor/projects/1)

## WP1b
Translate the spatial model to a system of PDEs:

  - [ ] PDE construction
  - [ ] PDE discretization
  - DUNE integration
    - [ ] generate C++ code to be compiled and ran
    - [ ] precompiled version if possible

## Implementation details

  - each commit is automatically compiled and tested on each supported OS:
    - linux & osx: https://travis-ci.org/lkeegan/spatial-model-editor
    - windows: https://ci.appveyor.com/project/lkeegan/spatial-model-editor
  - for tagged commits this results in a release containing binary executables
  - includes statically linked libraries:
    - https://github.com/lkeegan/libsbml-static
    - https://github.com/lkeegan/qt5-static

  - [ ] catch2 unit tests
  - [ ] qttest GUI tests
  - [ ] code coverage
  - [ ] static analysis
  - [x] releases
