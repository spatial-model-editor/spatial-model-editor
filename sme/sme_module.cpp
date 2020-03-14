#include <pybind11/pybind11.h>

#include "version.hpp"

namespace sme {

void pybindModule(pybind11::module& m) {
  m.doc() = R"(
          Spatial Model Editor

          Python bindings to a subset of the functionality
          available in the full GUI Spatial Model Editor

          Online docs:
          https://spatial-model-editor.readthedocs.io/

          Source code:
          https://www.github.com/lkeegan/spatial-model-editor
      )";
  m.attr("__version__") = SPATIAL_MODEL_EDITOR_VERSION;
}

}  // namespace sme
