// Python.h must come first:
#include <pybind11/pybind11.h>

// other headers
#include <pybind11/stl.h>

#include <QFile>
#include <QImage>
#include <exception>

#include "logger.hpp"
#include "sme_common.hpp"
#include "sme_model.hpp"

static std::vector<std::vector<std::vector<int>>> qImageToVec(
    const QImage& img) {
  std::size_t h = static_cast<std::size_t>(img.height());
  std::size_t w = static_cast<std::size_t>(img.width());
  std::vector<std::vector<std::vector<int>>> v(
      h, std::vector<std::vector<int>>(w, std::vector<int>(3, 0)));
  for (std::size_t y = 0; y < h; ++y) {
    for (std::size_t x = 0; x < w; ++x) {
      auto c = img.pixelColor(static_cast<int>(x), static_cast<int>(y));
      v[y][x][0] = c.red();
      v[y][x][1] = c.green();
      v[y][x][2] = c.blue();
    }
  }
  return v;
}

namespace sme {

void pybindModel(const pybind11::module& m) {
  pybind11::class_<sme::Model>(m, "Model")
      .def(pybind11::init<const std::string&>(), pybind11::arg("filename"))
      .def("export_sbml_file", &sme::Model::exportSbmlFile,
           pybind11::arg("filename"))
      .def("simulate", &sme::Model::simulate, pybind11::arg("simulation_time"),
           pybind11::arg("image_interval"))
      .def("simulation_time_points", &sme::Model::simulationTimePoints)
      .def("concentration_image", &sme::Model::concentrationImage,
           pybind11::arg("time_point_index"))
      .def("compartment_image", &sme::Model::compartmentImage)
      .def_property("name", &sme::Model::getName, &sme::Model::setName,
                    "The name of this model")
      .def_property_readonly("compartments", &sme::Model::getCompartments,
                             "The compartments in this model")
      .def("__repr__",
           [](const sme::Model& a) {
             return fmt::format("<sme.Model named '{}'>", a.getName());
           })
      .def("__str__", &sme::Model::getStr);
}

void Model::importSbmlFile(const std::string& filename) {
  s = std::make_unique<sbml::SbmlDocWrapper>();
  QFile f(filename.c_str());
  if (f.open(QIODevice::ReadOnly)) {
    s->importSBMLString(f.readAll().toStdString());
  } else {
    throw std::invalid_argument("Failed to open file: " + filename);
  }
  compartments.clear();
  compartments.reserve(static_cast<std::size_t>(s->compartments.size()));
  for (const auto& compartmentId : s->compartments) {
    compartments.emplace_back(s.get(), compartmentId.toStdString());
  }
}

Model::Model(const std::string& filename) {
  // disable logging
  spdlog::set_level(spdlog::level::off);
  importSbmlFile(filename);
}

void Model::exportSbmlFile(const std::string& filename) const {
  s->exportSBMLFile(filename);
}

void Model::simulate(double simulationTime, double imageInterval) {
  sim = std::make_unique<simulate::Simulation>(
      *s.get(), simulate::SimulatorType::Pixel, 2);
  auto options = sim->getIntegratorOptions();
  options.order = 2;
  options.maxTimestep = std::numeric_limits<double>::max();
  options.maxAbsErr = std::numeric_limits<double>::max();
  options.maxRelErr = 0.01;
  sim->setIntegratorOptions(options);
  while (sim->getTimePoints().back() < simulationTime) {
    sim->doTimestep(imageInterval);
  }
}

std::vector<double> Model::simulationTimePoints() const {
  return sim->getTimePoints();
}

std::vector<std::vector<std::vector<int>>> Model::concentrationImage(
    std::size_t timePointIndex) const {
  auto img = sim->getConcImage(timePointIndex, {}, true);
  return qImageToVec(img);
}

std::vector<std::vector<std::vector<int>>> Model::compartmentImage() const {
  return qImageToVec(s->getCompartmentImage());
}

void Model::setName(const std::string& name) { s->setName(name.c_str()); }

std::string Model::getName() const { return s->getName().toStdString(); }

std::map<std::string, Compartment*> Model::getCompartments() {
  return vecToNamePtrMap(compartments);
}

std::string Model::getStr() const {
  std::string str("<sme.Model>\n");
  str.append(fmt::format("  - name: '{}'\n", getName()));
  str.append(fmt::format("  - compartments:{}", vecToNames(compartments)));
  return str;
}

}  // namespace sme
