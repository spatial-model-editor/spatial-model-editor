# Spatial Model Editor [![linux/osx build status](https://travis-ci.org/lkeegan/spatial-model-editor.svg?branch=master)](https://travis-ci.org/lkeegan/spatial-model-editor) [![windows build status](https://ci.appveyor.com/api/projects/status/0m87yyaalrrj5ndn?svg=true)](https://ci.appveyor.com/project/lkeegan/spatial-model-editor) [![codecov](https://codecov.io/gh/lkeegan/spatial-model-editor/branch/master/graph/badge.svg)](https://codecov.io/gh/lkeegan/spatial-model-editor) [![documentation status](https://readthedocs.org/projects/spatial-model-editor/badge/)](https://spatial-model-editor.readthedocs.io/en/latest/)

GUI spatial model editor prototype, download the latest executables here:

  - [linux](https://github.com/lkeegan/spatial-model-editor/releases/latest/download/spatial-model-editor)
  - [mac](https://github.com/lkeegan/spatial-model-editor/releases/latest/download/spatial-model-editor.dmg)
  - [windows](https://github.com/lkeegan/spatial-model-editor/releases/latest/download/spatial-model-editor.exe)

![screenshot](docs/img/geometry.png)

## WP1a
Allow the user to describe a spatial model: [WP1a status](https://github.com/lkeegan/spatial-model-editor/projects/1)

(a partial re-implementation of the spatial model editing part of https://github.com/fbergmann/edit-spatial using C++/Qt5, with all dependencies statically linked so that it can be supplied as a stand-alone compiled executable for linux, windows and osx.)

## WP1b
Translate the spatial model to a system of PDEs: [WP1b status](https://github.com/lkeegan/spatial-model-editor/projects/2)

## Implementation details

  - CI: each commit is automatically compiled and tested on each supported OS:
    - linux & osx: https://travis-ci.org/lkeegan/spatial-model-editor
    - windows: https://ci.appveyor.com/project/lkeegan/spatial-model-editor
  - Deployment: tagged commits also result in a binary executable for each OS which are added to the release
  - Dependencies: the result is a standalone GUI executable that includes these statically linked libraries:
    - libSBML: https://github.com/lkeegan/libsbml-static [license: LGPL]
    - Qt5: https://github.com/lkeegan/qt5-static [license: LGPL]
    - QCustomPlot 2.0.1 https://www.qcustomplot.com [license: GPL]
    - exprtk math parsing/evaluation library: https://github.com/ArashPartow/exprtk [license: MIT]
  - Testing: the tests also depend on:
    - catch2 testing framework: https://github.com/catchorg/Catch2/ [license: BSL-1.0]
