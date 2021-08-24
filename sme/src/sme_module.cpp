// Python.h (included by pybind11.h) must come first
// https://docs.python.org/3.2/c-api/intro.html#include-files
#include <pybind11/pybind11.h>

#include "sme_module.hpp"
#include "version.hpp"
#include <QFile>

namespace sme {

void pybindModule(pybind11::module &m) {
  m.doc() = R"(
            Spatial Model Editor Python interface

            Python bindings to a subset of the functionality
            available in the full GUI Spatial Model Editor

            https://spatial-model-editor.readthedocs.io/
            )";
  m.def("open_file", openFile, pybind11::arg("filename"),
        R"(
        opens a sme or SBML file containing a spatial model

        Args:
            filename (str): the sme or SBML file to open

        Returns:
            Model: the spatial model
        )");
  m.def("open_sbml_file", openSbmlFile, pybind11::arg("filename"),
        R"(
        opens an SBML file containing a spatial model

        Args:
            filename (str): the SBML file to open

        Returns:
            Model: the spatial model
        )");
  m.def("open_example_model", openExampleModel,
        pybind11::arg("name") = "very-simple-model",
        R"(
        opens a built in example spatial model

        The model name can optionally be specified to one of the following built
        in models:

        * "ABtoC"
        * "brusselator-model"
        * "circadian-clock"
        * "gray-scott"
        * "liver-simplified"
        * "liver-cells"
        * "single-compartment-diffusion"
        * "very-simple-model"

        If the name is not specified the the default
        example model is "very-simple-model".

        Args:
            name (str, optional): name of the example model to open, default: "very-simple-model"

        Returns:
            Model: the example spatial model

        Examples:
          >>> import sme
          >>> model = sme.open_example_model()
          >>> repr(model)
          "<sme.Model named 'Very Simple Model'>"
          >>> print(model)
          <sme.Model>
            - name: 'Very Simple Model'
            - compartments:
               - Outside
               - Cell
               - Nucleus
            - membranes:
               - Outside <-> Cell
               - Cell <-> Nucleus
        )");
  m.attr("__version__") = common::SPATIAL_MODEL_EDITOR_VERSION;
}

Model openFile(const std::string &filename) { return Model(filename); }

Model openSbmlFile(const std::string &filename) { return Model(filename); }

Model openExampleModel(const std::string &name) {
  Model m;
  std::string xml;
  if (QFile f(QString(":/models/%1.xml").arg(name.c_str()));
      f.open(QIODevice::ReadOnly | QIODevice::Text)) {
    xml = f.readAll().toStdString();
  } else {
    throw SmeInvalidArgument("Failed to open example model '" + name +
                             "'. Type help(sme.open_example_model) to see the "
                             "available built-in models.");
  }
  m.importSbmlString(xml);
  return m;
}

} // namespace sme

//

//

//

//

//

//

//

//

//

//

//

//

//
