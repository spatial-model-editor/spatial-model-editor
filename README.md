# spatial-model-editor [![Build Status](https://travis-ci.org/lkeegan/spatial-model-editor.svg?branch=master)](https://travis-ci.org/lkeegan/spatial-model-editor) [![Build status](https://ci.appveyor.com/api/projects/status/0m87yyaalrrj5ndn?svg=true)](https://ci.appveyor.com/project/lkeegan/spatial-model-editor)

GUI spatial model editor prototype.

This will initially be a partial re-implementation of https://github.com/fbergmann/edit-spatial using C++/Qt5, supplied as a compiled executable for linux, windows and mac.

## WP1a
Allow the user to describe a spatial model:

  - statically linked libSBML library including spatial extension
    - [x] linux
    - [ ] osx
    - [ ] windows
  - [ ] open SBML file
  - [ ] display species & compartments data
  - [ ] display geometry
  - [ ] edit species & compartments data
  - [ ] save to SBML file

## WP1b
Translate the spatial model to a system of PDEs:

  - [ ] DUNE integration
  - [ ] Discretization

## Continuous Integration & Testing

When the code is changed, it is automatically compiled and tested on each supported OS:

  - linux & osx: https://travis-ci.org/lkeegan/spatial-model-editor
  - windows: https://ci.appveyor.com/project/lkeegan/spatial-model-editor

Ideally the compiled binary releases will also be generated in a similar way.

## Dependencies

All dependencies should be statically linked to be able to provide the user with a single independent executable to run.
  - Qt5
    - [ ] linux
    - [ ] osx
    - [ ] windows
  - libSBML (including spatial extension)
    - [x] linux: https://github.com/lkeegan/libsbml-static-linux
    - [ ] osx
    - [ ] windows
    - todo: check libxml etc deps
  - ...