// Python.h (#included by nanobind.h) must come first
// https://docs.python.org/3.2/c-api/intro.html#include-files
#include <nanobind/nanobind.h>

#include "sme/model.hpp"
#include "sme_common.hpp"
#include "sme_species.hpp"
#include <nanobind/stl/string.h>

namespace pysme {

void bindSpecies(nanobind::module_ &m) {
  bindList<Species>(m, "Species");
  nanobind::enum_<::sme::model::ConcentrationType>(m, "ConcentrationType")
      .value("Uniform", ::sme::model::ConcentrationType::Uniform)
      .value("Analytic", ::sme::model::ConcentrationType::Analytic)
      .value("Image", ::sme::model::ConcentrationType::Image);
  nanobind::class_<Species>(m, "Species",
                            R"(
                            a species that lives in a compartment
                            )")
      .def_prop_rw("name", &Species::getName, &Species::setName,
                   R"(
                    str: the name of this species
                    )")
      .def_prop_rw("diffusion_constant", &Species::getDiffusionConstant,
                   &Species::setDiffusionConstant,
                   R"(
                    float: the diffusion constant of this species
                    )")
      .def_prop_ro("concentration_type", &Species::getInitialConcentrationType,
                   R"(
                    Species.ConcentrationType: the type of initial concentration of this species (Uniform, Analytic or Image)
                    )")
      .def_prop_rw("uniform_concentration",
                   &Species::getUniformInitialConcentration,
                   &Species::setUniformInitialConcentration,
                   R"(
                    float: the uniform initial concentration of this species as a float
                    )")
      .def_prop_rw("analytic_concentration",
                   &Species::getAnalyticInitialConcentration,
                   &Species::setAnalyticInitialConcentration,
                   R"(
                    str: the initial concentration of this species as an analytic_2d expression
                    )")
      .def_prop_rw("concentration_image",
                   &Species::getImageInitialConcentration,
                   &Species::setImageInitialConcentration,
                   nanobind::rv_policy::take_ownership,
                   R"(
                    np.ndarray(float): the initial concentration of this species as a 3d array of floats, one for each voxel in the geometry image
                    )")
      .def("__repr__",
           [](const Species &a) {
             return fmt::format("<sme.Species named '{}'>", a.getName());
           })
      .def("__str__", &Species::getStr);
}

Species::Species(::sme::model::Model *sbmlDocWrapper, const std::string &sId)
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

[[nodiscard]] ::sme::model::ConcentrationType
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

[[nodiscard]] nanobind::ndarray<nanobind::numpy, double>
Species::getImageInitialConcentration() const {
  auto shape{s->getGeometry().getImages().volume()};
  return as_ndarray(
      s->getSpecies().getSampledFieldConcentration(id.c_str(), true), shape);
}

void Species::setImageInitialConcentration(
    nanobind::ndarray<nanobind::numpy, double> array) {
  const auto size{s->getGeometry().getImages().volume()};
  auto h = static_cast<std::size_t>(size.height());
  auto w = static_cast<std::size_t>(size.width());
  auto d = size.depth();
  std::string err{"Invalid concentration image array"};
  if (array.ndim() != 3) {
    throw std::invalid_argument(fmt::format(
        "{}: is {}-dimensional, should be 3-dimensional", err, array.ndim()));
  }
  auto v = array.view<const double, nanobind::ndim<3>>();
  if (v.shape(0) != d) {
    throw std::invalid_argument(
        fmt::format("{}: depth is {}, should be {}", err, v.shape(0), d));
  }
  if (v.shape(1) != h) {
    throw std::invalid_argument(
        fmt::format("{}: height is {}, should be {}", err, v.shape(1), h));
  }
  if (v.shape(2) != w) {
    throw std::invalid_argument(
        fmt::format("{}: width is {}, should be {}", err, v.shape(2), w));
  }
  std::vector<double> sampledFieldConcentration(h * w * d, 0.0);
  for (std::size_t z = 0; z < v.shape(0); z++) {
    for (std::size_t y = 0; y < v.shape(1); y++) {
      for (std::size_t x = 0; x < v.shape(2); x++) {
        auto sampledFieldIndex = x + w * (h - 1 - y) + w * h * z;
        sampledFieldConcentration[sampledFieldIndex] = v(z, y, x);
      }
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

} // namespace pysme
