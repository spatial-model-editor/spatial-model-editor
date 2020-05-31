// Python.h must come first:
#include <pybind11/pybind11.h>

// other headers
#include "sme_module.hpp"
#include "version.hpp"

namespace sme {

void pybindModule(pybind11::module &m) {
  m.doc() = R"(
          Spatial Model Editor

          Python bindings to a subset of the functionality
          available in the full GUI Spatial Model Editor

          Online docs:
          https://spatial-model-editor.readthedocs.io/

          Source code:
          https://www.github.com/lkeegan/spatial-model-editor
      )";
  m.def("open_sbml_file", openSbmlFile, pybind11::arg("filename"),
        "Open an SBML file");
  m.def("open_example_model", openExampleModel,
        "Open a built-in example model");
  m.attr("__version__") = SPATIAL_MODEL_EDITOR_VERSION;
}

Model openSbmlFile(const std::string &filename) { return Model(filename); }

Model openExampleModel() { return Model(":/models/very-simple-model.xml"); }

} // namespace sme
