# spatial-model-editor [![Build Status](https://travis-ci.org/lkeegan/spatial-model-editor.svg?branch=master)](https://travis-ci.org/lkeegan/spatial-model-editor) [![Build status](https://ci.appveyor.com/api/projects/status/0m87yyaalrrj5ndn?svg=true)](https://ci.appveyor.com/project/lkeegan/spatial-model-editor)

GUI spatial model editor prototype.

This will initially be a partial re-implementation of https://github.com/fbergmann/edit-spatial using C++/Qt5.

## Current features

  - [x] link libSBML library
  - [ ] open SBML file
  - [ ] display species & compartments data
  - [ ] edit species & compartments data
  - [ ] save to SBML file
  - [ ] display & edit geometry
  - [ ] DUNE integration

The end goal is to have a GUI that can setup and then simulate a spatial model, which is supplied as a compiled executable for linux, windows and macos.

## CI

When the code is changed, it is automatically compiled and tested:

  - linux & macos(todo): [![Build Status](https://travis-ci.org/lkeegan/spatial-model-editor.svg?branch=master)](https://travis-ci.org/lkeegan/spatial-model-editor)
  - windows: [![Build status](https://ci.appveyor.com/api/projects/status/0m87yyaalrrj5ndn?svg=true)](https://ci.appveyor.com/project/lkeegan/spatial-model-editor)

Ideally the compiled releases will also be generated in a similar way.