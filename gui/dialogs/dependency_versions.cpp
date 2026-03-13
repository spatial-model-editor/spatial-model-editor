#include "dependency_versions.hpp"
#include <qcustomplot.h>
#include <vtkVersion.h>

namespace sme::gui {

std::vector<sme::common::DependencyVersion> getGuiDependencyVersions() {
  return {
      {"QCustomPlot", "https://www.qcustomplot.com", QCUSTOMPLOT_VERSION_STR},
      {"VTK", "https://vtk.org/", vtkVersion::GetVTKVersion()}};
}

} // namespace sme::gui
