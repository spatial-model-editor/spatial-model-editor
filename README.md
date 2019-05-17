# spatial-model-editor [![Build Status](https://travis-ci.org/lkeegan/spatial-model-editor.svg?branch=master)](https://travis-ci.org/lkeegan/spatial-model-editor) [![Build status](https://ci.appveyor.com/api/projects/status/0m87yyaalrrj5ndn?svg=true)](https://ci.appveyor.com/project/lkeegan/spatial-model-editor)

GUI spatial model editor prototype.

  - Download the latest executables here: [linux](https://github.com/lkeegan/spatial-model-editor/releases/latest/download/spatial-model-editor), [mac](https://github.com/lkeegan/spatial-model-editor/releases/latest/download/spatial-model-editor.dmg)

The initial goal is a partial re-implementation of the spatial model editing part of https://github.com/fbergmann/edit-spatial using C++/Qt5, with all dependencies statically linked so that it can be supplied as a stand-alone compiled executable for linux, windows and osx.

## WP1a
Allow the user to describe a spatial model:

Functionality:
  - [x] open SBML file (possibly including spatial information)
  - [ ] check it is valid
  - [ ] display species & compartments data
  - [ ] display geometry
  - [ ] edit species & compartments data
  - [ ] edit geometry
  - [ ] save to SBML file

Implementation details:
  - include statically linked libSBML library including spatial extension
    - [x] linux
    - [x] osx
    - [ ] windows
    - see https://github.com/lkeegan/libsbml-static
  - include statically linked QT libraries
    - [x] linux
    - [x] osx
    - [ ] windows
    - see https://github.com/lkeegan/qt5-static
  - generate standalone GUI executable
    - [x] linux: https://github.com/lkeegan/spatial-model-editor/releases/latest/download/spatial-model-editor
    - [x] osx: https://github.com/lkeegan/spatial-model-editor/releases/latest/download/spatial-model-editor.dmg
    - [ ] windows

Development workflow:

## WP1b
Translate the spatial model to a system of PDEs:

  - [ ] PDE construction
  - [ ] PDE discretization
  - DUNE integration
    - [ ] generate C++ code to be compiled and ran
    - [ ] precompiled version if possible

## Continuous Integration

With each commit to the repository, the code is automatically compiled and tested on each supported OS:

  - linux & osx: https://travis-ci.org/lkeegan/spatial-model-editor
  - windows: https://ci.appveyor.com/project/lkeegan/spatial-model-editor

The compiled binary releases are also generated as part of this process for tagged commits.

  - [ ] catch2 unit tests
  - [ ] qttest GUI tests
  - [ ] code coverage
  - [ ] static analysis
  - [x] releases
