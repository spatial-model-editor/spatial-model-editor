<img align="left" width="64" height="64" src="https://raw.githubusercontent.com/lkeegan/spatial-model-editor/master/src/core/resources/icon64.png" alt="icon">

# Spatial Model Editor

[![github releases](https://img.shields.io/github/v/release/lkeegan/spatial-model-editor?sort=semver)](https://github.com/lkeegan/spatial-model-editor/releases)
[![pypi releases](https://img.shields.io/pypi/v/sme.svg)](https://pypi.org/project/sme)
[![open in colab](https://colab.research.google.com/assets/colab-badge.svg)](https://colab.research.google.com/github/lkeegan/spatial-model-editor/blob/master/sme/sme_getting_started.ipynb)
[![documentation](https://readthedocs.org/projects/spatial-model-editor/badge/)](https://spatial-model-editor.readthedocs.io/en/latest/)
[![build status](https://travis-ci.org/lkeegan/spatial-model-editor.svg?branch=master)](https://travis-ci.org/lkeegan/spatial-model-editor)
[![codecov](https://codecov.io/gh/lkeegan/spatial-model-editor/branch/master/graph/badge.svg)](https://codecov.io/gh/lkeegan/spatial-model-editor)
[![sonarcloud quality gate status](https://sonarcloud.io/api/project_badges/measure?project=lkeegan_spatial-model-editor&metric=alert_status)](https://sonarcloud.io/dashboard?id=lkeegan_spatial-model-editor)

A GUI editor to create and edit 2d spatial SBML models of bio-chemical reactions and simulate them using the
[dune-copasi](https://dune-copasi.netlify.app/) solver for reaction-diffusion systems.

To get started, download and run the GUI for your operating system

- [![linux](docs/img/icon-linux.png) linux](../../releases/latest/download/spatial-model-editor)

- [![macOS](docs/img/icon-osx.png) macOS](../../releases/latest/download/spatial-model-editor.dmg)

- [![linux](docs/img/icon-windows.png) windows](../../releases/latest/download/spatial-model-editor.exe)

Or take a look at the [documentation](https://spatial-model-editor.readthedocs.io/)

![screenshot](docs/img/mesh.png)

## Contributing

[Bug reports](https://github.com/lkeegan/spatial-model-editor/issues/new?assignees=&labels=&template=bug_report.md) and [feature requests](https://github.com/lkeegan/spatial-model-editor/issues/new?assignees=&labels=&template=feature_request.md) are very welcome, as are fixes or improvements to the [documentation](https://spatial-model-editor.readthedocs.io/) (to edit a page in the documentation, click the 'Edit on GitHub' button in the top right).

If you are interesting in contributing code, please see the [contributing guidelines](.github/CONTRIBUTING.md).

## Dependencies

Spatial Model Editor makes use of the following open source libraries:

- [dune-copasi](https://gitlab.dune-project.org/copasi/dune-copasi) - license: [GPL2 + runtime exception](https://dune-project.org/about/license/)

- [libSBML](http://sbml.org/Software/libSBML) - license: [LGPL](http://sbml.org/Software/libSBML/LibSBML_License)

- [Qt5](https://www.qt.io/) - license: [LGPL](https://doc.qt.io/qt-5/lgpl.html)

- [QCustomPlot](https://www.qcustomplot.com) - license: [GPL](https://www.gnu.org/licenses/gpl-3.0.html)

- [spdlog](https://github.com/gabime/spdlog) - license: [MIT](https://github.com/gabime/spdlog/blob/v1.x/LICENSE)

- [fmt](https://github.com/fmtlib/fmt) - license: [MIT](https://github.com/fmtlib/fmt/blob/master/LICENSE.rst)

- [SymEngine](https://github.com/symengine/symengine) - license: [MIT](https://github.com/symengine/symengine/blob/master/LICENSE)

- [LLVM Core](https://llvm.org/) - license: [Apache 2.0 License with LLVM exceptions](https://llvm.org/docs/DeveloperPolicy.html#copyright-license-and-patents)

- [GNU Multiple Precision Arithmetic Library](https://gmplib.org/) - license: [LGPL](https://www.gnu.org/licenses/lgpl-3.0.html)

- [Triangle](http://www.cs.cmu.edu/~quake/triangle.html) - license: "Private, research, and institutional use is free."

- [muParser](https://github.com/beltoforion/muparser) - license: [MIT](https://github.com/beltoforion/muparser/blob/master/License.txt)

- [libTIFF](http://www.libtiff.org/) - license: [MIT](http://www.libtiff.org/misc.html)

- [Expat](https://github.com/libexpat/libexpat) - license: [MIT](https://github.com/libexpat/libexpat/blob/master/expat/COPYING)

- [Threading Building Blocks](https://github.com/intel/tbb) - license: [Apache-2.0](https://github.com/intel/tbb/blob/tbb_2020/LICENSE)

- [Open Source Computer Vision Library](https://github.com/opencv/opencv) - license: [Apache-2.0](https://github.com/opencv/opencv/blob/master/LICENSE)

- [pybind11](https://github.com/pybind/pybind11) - license: [BSD-style](https://github.com/pybind/pybind11/blob/master/LICENSE)

- [Catch2](https://github.com/catchorg/Catch2) - license: [BSL-1.0](https://github.com/catchorg/Catch2/blob/master/LICENSE.txt)
