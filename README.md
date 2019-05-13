# spatial-model-editor [![Build Status](https://travis-ci.org/lkeegan/spatial-model-editor.svg?branch=master)](https://travis-ci.org/lkeegan/spatial-model-editor) [![Build status](https://ci.appveyor.com/api/projects/status/0m87yyaalrrj5ndn?svg=true)](https://ci.appveyor.com/project/lkeegan/spatial-model-editor)

GUI spatial model editor prototype.

The initial goal is a partial re-implementation of the spatial model editing part of https://github.com/fbergmann/edit-spatial using C++/Qt5, with all dependencies statically linked so that it can be supplied as a stand-alone compiled executable for linux, windows and osx.

## WP1a
Allow the user to describe a spatial model:

  - include statically linked libSBML library including spatial extension
    - [x] linux: https://github.com/lkeegan/libsbml-static-linux
    - [x] osx: https://github.com/lkeegan/libsbml-static-osx
    - [ ] windows
  - include statically linked QT libraries (or bundled Framework for osx)
    - [ ] linux
    - [ ] osx
    - [ ] windows
  - [ ] open SBML file (including spatial information)
  - [ ] check it is valid
  - [ ] display species & compartments data
  - [ ] display geometry
  - [ ] edit species & compartments data
  - [ ] edit geometry
  - [ ] save to SBML file

## WP1b
Translate the spatial model to a system of PDEs:

  - [ ] PDE construction
  - [ ] PDE discretization
  - DUNE integration
    - [ ] generate C++ code to be compiled and ran
    - [ ] precompiled version if possible

## Continuous Integration & Testing

When the code is changed, it is automatically compiled and tested on each supported OS:

  - linux & osx: https://travis-ci.org/lkeegan/spatial-model-editor
  - windows: https://ci.appveyor.com/project/lkeegan/spatial-model-editor

The compiled binary releases will hopefully also be generated in the same way.