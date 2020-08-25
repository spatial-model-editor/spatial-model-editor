<img align="left" width="64" height="64" src="https://raw.githubusercontent.com/lkeegan/spatial-model-editor/master/src/core/resources/icon64.png" alt="icon">

# Spatial Model Editor

[![github releases](https://img.shields.io/github/release/lkeegan/spatial-model-editor.svg)](https://github.com/lkeegan/spatial-model-editor/releases)
[![pypi releases](https://img.shields.io/pypi/v/sme.svg)](https://pypi.org/project/sme)
[![open in colab](https://colab.research.google.com/assets/colab-badge.svg)](https://colab.research.google.com/github/lkeegan/spatial-model-editor/blob/master/sme/sme_getting_started.ipynb)
[![documentation](https://readthedocs.org/projects/spatial-model-editor/badge/)](https://spatial-model-editor.readthedocs.io/en/latest/)
[![build status](https://travis-ci.org/lkeegan/spatial-model-editor.svg?branch=master)](https://travis-ci.org/lkeegan/spatial-model-editor)
[![codecov](https://codecov.io/gh/lkeegan/spatial-model-editor/branch/master/graph/badge.svg)](https://codecov.io/gh/lkeegan/spatial-model-editor)
[![sonarcloud quality gate status](https://sonarcloud.io/api/project_badges/measure?project=lkeegan_spatial-model-editor&metric=alert_status)](https://sonarcloud.io/dashboard?id=lkeegan_spatial-model-editor)

A GUI editor to convert non-spatial SBML models of bio-chemical reactions
into 2d spatial models and simulate them using the
[dune-copasi](https://gitlab.dune-project.org/copasi/dune-copasi) solver
for reaction-diffusion systems.

To get started, download and run the GUI for your operating system

- [![linux](docs/quickstart/img/icon-linux.png) linux](../../releases/latest/download/spatial-model-editor)
- [![macOS](docs/quickstart/img/icon-osx.png) macOS](../../releases/latest/download/spatial-model-editor.dmg)
- [![linux](docs/quickstart/img/icon-windows.png) windows](../../releases/latest/download/spatial-model-editor.exe)

Or take a look at the [documentation](https://spatial-model-editor.readthedocs.io/)

![screenshot](docs/img/mesh.png)

## Project status

### [WP1a](https://github.com/lkeegan/spatial-model-editor/projects/1)

Allow the user to describe a spatial model:

- import/export SBML
- add geometry
- diffusion coefficients
- spatially varying initial concentrations
- etc.

### [WP1b](https://github.com/lkeegan/spatial-model-editor/projects/2)

Translate the spatial model to a system of PDEs and simulate it:

- generate 2d triangular mesh
- construct PDEs and Jacobians
- simulate using DUNE
- visualize simulation results, etc.

## Implementation details

- _CI_: each commit is automatically compiled and tested on each supported OS:

  - builds: <https://travis-ci.org/lkeegan/spatial-model-editor>

  - test coverage report: <https://codecov.io/gh/lkeegan/spatial-model-editor>

  - static analysis reports: <https://sonarcloud.io/dashboard?id=lkeegan_spatial-model-editor>

- _Deployment_: tagged commits result in

  - GUI/CLI executables: github release <https://github.com/lkeegan/spatial-model-editor/releases>

  - Python library: PyPI release <https://pypi.org/project/sme/>

- _Documentation_: <https://spatial-model-editor.readthedocs.io>

- _Dependencies_:

  - dune-copasi Biochemical System Simulator <https://gitlab.dune-project.org/copasi/dune-copasi>

    - license: [GPL2 + runtime exception](https://dune-project.org/about/license/)
    - using pre-compiled binaries from <https://github.com/lkeegan/dune-copasi-static>

  - libSBML <http://sbml.org/Software/libSBML>

    - license: [LGPL](http://sbml.org/Software/libSBML/LibSBML_License)
    - using pre-compiled binaries from <https://github.com/lkeegan/libsbml-static>

  - Qt5 <https://www.qt.io/>

    - license: [LGPL](https://doc.qt.io/qt-5/lgpl.html)
    - using pre-compiled binaries from <https://github.com/lkeegan/qt5-static>

  - QCustomPlot <https://www.qcustomplot.com>

    - license: GPL
    - included in source code

  - spdlog logging library: <https://github.com/gabime/spdlog>

    - license: [MIT](https://github.com/gabime/spdlog/blob/v1.x/LICENSE)
    - using pre-compiled binaries from <https://github.com/lkeegan/libsbml-static>

  - fmt formatting library: <https://github.com/fmtlib/fmt>

    - license: [MIT](https://github.com/fmtlib/fmt/blob/master/LICENSE.rst)
    - using pre-compiled binaries from <https://github.com/lkeegan/libsbml-static>

  - SymEngine symbolic manipulation library: <https://github.com/symengine/symengine>

    - license: [MIT](https://github.com/symengine/symengine/blob/master/LICENSE)
    - using pre-compiled binaries from <https://github.com/lkeegan/libsbml-static>

  - LLVM Core libraries: <https://llvm.org/>

    - license: University of Illinois/NCSA Open Source License
    - using pre-compiled binaries from <https://github.com/lkeegan/llvm-static>

  - GNU Multiple Precision Arithmetic Library: <https://gmplib.org/>

    - license: LGPL
    - using pre-compiled binaries from <https://github.com/lkeegan/libsbml-static>

  - Triangle: <http://www.cs.cmu.edu/~quake/triangle.html>

    - license: "Private, research, and institutional use is free."
    - included in source code

  - muParser: <https://github.com/beltoforion/muparser>

    - license: [MIT](https://github.com/beltoforion/muparser/blob/master/License.txt)
    - using pre-compiled binaries from <https://github.com/lkeegan/libsbml-static>

  - libTIFF: <http://www.libtiff.org/>

    - license: [MIT](http://www.libtiff.org/misc.html)
    - using pre-compiled binaries from <https://github.com/lkeegan/libsbml-static>

  - Expat XML library: <https://github.com/libexpat/libexpat>

    - license: [MIT](https://github.com/libexpat/libexpat/blob/master/expat/COPYING)
    - using pre-compiled binaries from <https://github.com/lkeegan/libsbml-static>

  - Threading Building Blocks (TBB): <https://github.com/intel/tbb>

    - license: [Apache-2.0](https://github.com/intel/tbb/blob/tbb_2020/LICENSE)
    - using pre-compiled binaries from <https://github.com/lkeegan/libsbml-static>

  - Open Source Computer Vision Library (OpenCV): <https://github.com/opencv/opencv>

    - license: [BSD](https://github.com/opencv/opencv/blob/master/LICENSE)
    - using pre-compiled binaries from <https://github.com/lkeegan/libsbml-static>

  - pybind11 (for optional python bindings): <https://github.com/pybind/pybind11>

    - license: [BSD-style](https://github.com/pybind/pybind11/blob/master/LICENSE)
    - included in source code

  - Catch2 (for tests): <https://github.com/catchorg/Catch2>

    - license: [BSL-1.0](https://github.com/catchorg/Catch2/blob/master/LICENSE.txt)
    - included in tests source code
