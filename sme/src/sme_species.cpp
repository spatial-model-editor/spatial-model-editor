// Python.h (included by pybind11.h) must come first
// https://docs.python.org/3.2/c-api/intro.html#include-files
#include <pybind11/pybind11.h>

#include "model.hpp"
#include "sme_common.hpp"
#include "sme_species.hpp"

namespace sme {

void pybindSpecies(pybind11::module &m) {
  sme::bindList<Species>(m, "Species");
  pybind11::enum_<model::ConcentrationType>(m, "ConcentrationType")
      .value("Uniform", model::ConcentrationType::Uniform)
      .value("Analytic", model::ConcentrationType::Analytic)
      .value("Image", model::ConcentrationType::Image);
  pybind11::class_<Species>(m, "Species",
                            R"(
                            a species that lives in a compartment
                            )")
      .def_property("name", &Species::getName, &Species::setName,
                    R"(
                    str: the name of this species
                    )")
      .def_property("diffusion_constant", &Species::getDiffusionConstant,
                    &Species::setDiffusionConstant,
                    R"(
                    float: the diffusion constant of this species
                    )")
      .def_property_readonly("concentration_type",
                             &Species::getInitialConcentrationType,
                             R"(
                    Species.ConcentrationType: the type of initial concentration of this species (Uniform, Analytic or Image)
                    )")
      .def_property("uniform_concentration",
                    &Species::getUniformInitialConcentration,
                    &Species::setUniformInitialConcentration,
                    R"(
                    float: the uniform initial concentration of this species as a float
                    )")
      .def_property("analytic_concentration",
                    &Species::getAnalyticInitialConcentration,
                    &Species::setAnalyticInitialConcentration,
                    R"(
                    str: the initial concentration of this species as an analytic expression
                    )")
      .def_property("concentration_image",
                    &Species::getImageInitialConcentration,
                    &Species::setImageInitialConcentration,
                    R"(
                    np.ndarray(float): the initial concentration of this species as a 2d array of floats, one for each pixel in the geometry image
                    )")
      .def("__repr__",
           [](const Species &a) {
             return fmt::format("<sme.Species named '{}'>", a.getName());
           })
      .def("__str__", &Species::getStr);
}

Species::Species(model::Model *sbmlDocWrapper, const std::string &sId)
    : s(sbmlDocWrapper), id(sId) {}

void Species::setName(const std::string &name) {
  s->getSpecies().setName(id.c_str(), name.c_str());
}

std::string Species::getName() const {
  return s->getSpecies().getName(id.c_str()).toStdString();
}

void Species::setDiffusionConstant(double diffusionConstant) {
  s->getSpecies().setDiffusionConstant(id.c_str(), diffusionConstant);
}

double Species::getDiffusionConstant() const {
  return s->getSpecies().getDiffusionConstant(id.c_str());
}

[[nodiscard]] model::ConcentrationType
Species::getInitialConcentrationType() const {
  return s->getSpecies().getInitialConcentrationType(id.c_str());
}

[[nodiscard]] double Species::getUniformInitialConcentration() const {
  return s->getSpecies().getInitialConcentration(id.c_str());
}

void Species::setUniformInitialConcentration(double value) {
  s->getSpecies().setInitialConcentration(id.c_str(), value);
}

[[nodiscard]] std::string Species::getAnalyticInitialConcentration() const {
  return s->getSpecies().getAnalyticConcentration(id.c_str()).toStdString();
}

void Species::setAnalyticInitialConcentration(const std::string &expression) {
  s->getSpecies().setAnalyticConcentration(id.c_str(), expression.c_str());
}

[[nodiscard]] pybind11::array_t<double>
Species::getImageInitialConcentration() const {
  auto size{s->getGeometry().getImage().size()};
  return as_ndarray(
      s->getSpecies().getSampledFieldConcentration(id.c_str(), true),
      {size.height(), size.width()});
}

void Species::setImageInitialConcentration(pybind11::array_t<double> array) {
  const auto size{s->getGeometry().getImage().size()};
  auto h{size.height()};
  auto w{size.width()};
  std::string err{"Invalid concentration image array"};
  if (array.ndim() != 2) {
    throw sme::SmeInvalidArgument(fmt::format(
        "{}: is {}-dimensional, should be 2-dimensional", err, array.ndim()));
  }
  if (array.shape(0) != size.height()) {
    throw sme::SmeInvalidArgument(
        fmt::format("{}: height is {}, should be {}", err, array.shape(0), h));
  }
  if (array.shape(1) != size.width()) {
    throw sme::SmeInvalidArgument(
        fmt::format("{}: width is {}, should be {}", err, array.shape(1), w));
  }
  std::vector<double> sampledFieldConcentration(static_cast<std::size_t>(h * w),
                                                0.0);
  auto r{array.unchecked<2>()};
  for (pybind11::ssize_t y = 0; y < array.shape(0); y++) {
    for (pybind11::ssize_t x = 0; x < array.shape(1); x++) {
      auto sampledFieldIndex{static_cast<std::size_t>(x + w * (h - 1 - y))};
      sampledFieldConcentration[sampledFieldIndex] = r(y, x);
    }
  }
  s->getSpecies().setSampledFieldConcentration(id.c_str(),
                                               sampledFieldConcentration);
}

std::string Species::getStr() const {
  std::string str("<sme.Species>\n");
  str.append(fmt::format("  - name: '{}'\n", getName()));
  str.append(
      fmt::format("  - diffusion_constant: {}\n", getDiffusionConstant()));
  return str;
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
