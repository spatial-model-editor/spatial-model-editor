# Spatial Model Editor
[![github releases](https://img.shields.io/github/release/lkeegan/spatial-model-editor.svg)](https://github.com/lkeegan/spatial-model-editor/releases)
[![linux/osx build status](https://travis-ci.org/lkeegan/spatial-model-editor.svg?branch=master)](https://travis-ci.org/lkeegan/spatial-model-editor)
[![windows build status](https://ci.appveyor.com/api/projects/status/0m87yyaalrrj5ndn?svg=true)](https://ci.appveyor.com/project/lkeegan/spatial-model-editor)
[![codecov](https://codecov.io/gh/lkeegan/spatial-model-editor/branch/master/graph/badge.svg)](https://codecov.io/gh/lkeegan/spatial-model-editor)
[![sonarcloud quality gate status](https://sonarcloud.io/api/project_badges/measure?project=lkeegan_spatial-model-editor&metric=alert_status)](https://sonarcloud.io/dashboard?id=lkeegan_spatial-model-editor)
[![lgtm status](https://img.shields.io/lgtm/grade/cpp/g/lkeegan/spatial-model-editor.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/lkeegan/spatial-model-editor/context:cpp)
[![codacy status](https://api.codacy.com/project/badge/Grade/2cc27d99b42041668944f41d88abeef0)](https://www.codacy.com/app/lkeegan/spatial-model-editor?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=lkeegan/spatial-model-editor&amp;utm_campaign=Badge_Grade)
[![documentation status](https://readthedocs.org/projects/spatial-model-editor/badge/)](https://spatial-model-editor.readthedocs.io/en/latest/)

A GUI to convert non-spatial SBML models of bio-chemical reactions into 2d spatial models, and to simulate them.

[<img src="docs/img/icon-linux.png" width="32"> linux executable](https://github.com/lkeegan/spatial-model-editor/releases/latest/download/spatial-model-editor) |
[<img src="docs/img/icon-osx.png" width="32"> mac executable](https://github.com/lkeegan/spatial-model-editor/releases/latest/download/spatial-model-editor.dmg) |
[<img src="docs/img/icon-windows.png" width="32"> windows executable](https://github.com/lkeegan/spatial-model-editor/releases/latest/download/spatial-model-editor.exe) |
[<img src="docs/img/icon-docs.png" width="32"> documentation](https://spatial-model-editor.readthedocs.io/en/latest/)
---|---|---|---

![screenshot](docs/img/geometry.png)

## Project status

### WP1a
Allow the user to describe a spatial model: [WP1a status](https://github.com/lkeegan/spatial-model-editor/projects/1)

(a partial re-implementation of the spatial model editing part of <https://github.com/fbergmann/edit-spatial> using C++/Qt5, with all dependencies statically linked so that it can be supplied as a stand-alone compiled executable for linux, windows and osx.)

### WP1b
Translate the spatial model to a system of PDEs and simulate it: [WP1b status](https://github.com/lkeegan/spatial-model-editor/projects/2)

## Implementation details

-   _CI_: each commit is automatically compiled and tested on each supported OS:

    -   linux & osx: <https://travis-ci.org/lkeegan/spatial-model-editor>

    -   windows: <https://ci.appveyor.com/project/lkeegan/spatial-model-editor>

    -   test coverage report: <https://codecov.io/gh/lkeegan/spatial-model-editor>

    -   static analysis reports:

        -   <https://sonarcloud.io/dashboard?id=lkeegan_spatial-model-editor>
        -   <https://lgtm.com/projects/g/lkeegan/spatial-model-editor/context:cpp>
        -   <https://www.codacy.com/app/lkeegan/spatial-model-editor>

-   _Deployment_: tagged commits result in github release with a binary executable for each OS

-   _Documentation_: documentation is at <https://spatial-model-editor.readthedocs.io> and is compiled with each commit

-   _Dependencies_: the result is a standalone GUI executable that includes these statically linked libraries:

    -   libSBML <http://sbml.org/Software/libSBML>

        -   license: [LGPL](http://sbml.org/Software/libSBML/LibSBML_License)
        -   using pre-compiled binaries from <https://github.com/lkeegan/libsbml-static>

    -   Expat XML library (included in libSBML) <https://github.com/libexpat/libexpat>

        -   license: [MIT](https://github.com/libexpat/libexpat/blob/master/expat/COPYING)

    -   Qt 5.12 LTS <https://www.qt.io/qt-5-12>

        -   license: [LGPL](https://doc.qt.io/qt-5/lgpl.html)
        -   using pre-compiled binaries from <https://github.com/lkeegan/qt5-static>

    -   QCustomPlot 2.0.1 <https://www.qcustomplot.com>

        -   license: GPL
        -   included in source code

    -   ExprTK math parsing/evaluation library: <https://github.com/ArashPartow/exprtk>

        -   license: MIT
        -   included in source code

    -   spdlog logging library: <https://github.com/gabime/spdlog>

        -   license: MIT
        -   included in source code

    -   Catch2 testing framework: <https://github.com/catchorg/Catch2>

        -   license: BSL-1.0
        -   included in tests source code
