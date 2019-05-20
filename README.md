# spatial-model-editor [![linux/osx build status](https://travis-ci.org/lkeegan/spatial-model-editor.svg?branch=master)](https://travis-ci.org/lkeegan/spatial-model-editor) [![windows build status](https://ci.appveyor.com/api/projects/status/0m87yyaalrrj5ndn?svg=true)](https://ci.appveyor.com/project/lkeegan/spatial-model-editor) [![documentation status](https://readthedocs.org/projects/spatial-model-editor/badge/)](https://spatial-model-editor.readthedocs.io/en/latest/)

GUI spatial model editor prototype, download the latest executables here:

  - [linux](https://github.com/lkeegan/spatial-model-editor/releases/latest/download/spatial-model-editor)
  - [mac](https://github.com/lkeegan/spatial-model-editor/releases/latest/download/spatial-model-editor.dmg)
  - [windows](https://github.com/lkeegan/spatial-model-editor/releases/latest/download/spatial-model-editor.exe)

The initial goal is a partial re-implementation of the spatial model editing part of https://github.com/fbergmann/edit-spatial using C++/Qt5, with all dependencies statically linked so that it can be supplied as a stand-alone compiled executable for linux, windows and osx.

## WP1a
Allow the user to describe a spatial model: [WP1a status](https://github.com/lkeegan/spatial-model-editor/projects/1)

## WP1b
Translate the spatial model to a system of PDEs: [WP1b status](https://github.com/lkeegan/spatial-model-editor/projects/2)

## Implementation details

  - each commit is automatically compiled and tested on each supported OS:
    - linux & osx: https://travis-ci.org/lkeegan/spatial-model-editor
    - windows: https://ci.appveyor.com/project/lkeegan/spatial-model-editor
  - tagged commits result in a binary executable for each OS which are added to the release
  - GUI is a standalone executable that includes these statically linked libraries:
    - libSBML: https://github.com/lkeegan/libsbml-static
    - Qt5: https://github.com/lkeegan/qt5-static