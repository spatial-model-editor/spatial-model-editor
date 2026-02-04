// todo: re-enable tiff tests once
// https://gitlab.dune-project.org/copasi/dune-copasi/-/issues/80 is resolved

#include "catch_wrapper.hpp"
#include "dune_headers.hpp"
#include "dunefunctorfactory.hpp"
#include "model_test_utils.hpp"
#include "sme/duneconverter.hpp"
#include "sme/model.hpp"
#include <QDir>
#include <QFile>
#include <cmath>
#include <locale>

using namespace sme;
using namespace sme::test;

static Dune::ParameterTree getConfig(const simulate::DuneConverter &dc) {
  Dune::ParameterTree config;
  std::stringstream ssIni(dc.getIniFile().toStdString());
  Dune::ParameterTreeParser::readINITree(ssIni, config);
  return config;
}

static QString getFirstNonConstantSpeciesId(const model::Model &m) {
  for (const auto &compartmentId : m.getCompartments().getIds()) {
    for (const auto &speciesId : m.getSpecies().getIds(compartmentId)) {
      if (!m.getSpecies().getIsConstant(speciesId)) {
        return speciesId;
      }
    }
  }
  return {};
}

TEST_CASE("DUNE: SmeFunctorFactory",
          "[core/simulate/dunefunctorfactory][core/"
          "simulate][core][dunefunctorfactory][dune]") {
  SECTION("constructs for 2d and 3d") {
    auto m2d{getExampleModel(Mod::VerySimpleModel)};
    auto m3d{getExampleModel(Mod::VerySimpleModel3D)};
    simulate::DuneConverter dc2d(m2d);
    simulate::DuneConverter dc3d(m3d);
    simulate::SmeFunctorFactory<2> factory2d(dc2d);
    simulate::SmeFunctorFactory<3> factory3d(dc3d);
  }

  SECTION("uses sampled diffusion array for 2d scalar and tensor functors") {
    auto m{getExampleModel(Mod::VerySimpleModel)};
    const auto vol = m.getGeometry().getImages().volume();
    const auto arraySize =
        static_cast<std::size_t>(vol.width()) * vol.height() * vol.depth();
    std::vector<double> diffusion(arraySize);
    for (std::size_t i = 0; i < diffusion.size(); ++i) {
      diffusion[i] = static_cast<double>(i);
    }
    const auto speciesId = getFirstNonConstantSpeciesId(m);
    REQUIRE(!speciesId.isEmpty());
    m.getSpecies().setSampledFieldDiffusionConstant(speciesId, diffusion);

    simulate::DuneConverter dc(m);
    simulate::SmeFunctorFactory<2> factory(dc);
    const auto &arrays = dc.getDiffusionConstantArrays();
    REQUIRE(!arrays.empty());
    const auto &arrayName = arrays.begin()->first;
    const auto &array = arrays.begin()->second;

    Dune::ParameterTree cfg;
    cfg["parser_type"] = "sme";
    cfg["expression"] = arrayName;

    Dune::Copasi::LocalDomain<2> local_domain;
    const auto origin = dc.getOrigin();
    const auto voxel = dc.getVoxelSize();
    const auto image = dc.getImageSize();
    const int ix = image.width() / 2;
    const int iy = image.height() / 2;
    local_domain.position = {origin.p.x() + (ix + 0.5) * voxel.width(),
                             origin.p.y() + (iy + 0.5) * voxel.height()};

    const auto index = static_cast<std::size_t>(ix + image.width() * iy);
    auto scalar = factory.make_scalar("", cfg, local_domain, 0);
    REQUIRE(scalar()[0] == Catch::Approx(array[index]));

    auto tensorApply = factory.make_tensor_apply("", cfg, local_domain, 0);
    auto vec = Dune::FieldVector<double, 2>{2.0, -3.0};
    auto result = tensorApply(vec);
    REQUIRE(result[0] == Catch::Approx(array[index] * vec[0]));
    REQUIRE(result[1] == Catch::Approx(array[index] * vec[1]));
  }

  SECTION("uses sampled diffusion array for 3d scalar functor and clamps") {
    auto m{getExampleModel(Mod::SingleCompartmentDiffusion3D)};
    const auto vol = m.getGeometry().getImages().volume();
    const auto arraySize =
        static_cast<std::size_t>(vol.width()) * vol.height() * vol.depth();
    std::vector<double> diffusion(arraySize);
    for (std::size_t i = 0; i < diffusion.size(); ++i) {
      diffusion[i] = static_cast<double>(i % 97);
    }
    const auto speciesId = getFirstNonConstantSpeciesId(m);
    REQUIRE(!speciesId.isEmpty());
    m.getSpecies().setSampledFieldDiffusionConstant(speciesId, diffusion);

    simulate::DuneConverter dc(m);
    simulate::SmeFunctorFactory<3> factory(dc);
    const auto &arrays = dc.getDiffusionConstantArrays();
    REQUIRE(!arrays.empty());
    const auto &arrayName = arrays.begin()->first;
    const auto &array = arrays.begin()->second;

    Dune::ParameterTree cfg;
    cfg["parser_type"] = "sme";
    cfg["expression"] = arrayName;

    Dune::Copasi::LocalDomain<3> local_domain;
    const auto origin = dc.getOrigin();
    const auto voxel = dc.getVoxelSize();
    const auto image = dc.getImageSize();
    const int ix = image.width() - 1;
    const int iy = image.height() - 1;
    const int iz = image.depth() - 1;
    local_domain.position = {origin.p.x() + image.width() * voxel.width(),
                             origin.p.y() + image.height() * voxel.height(),
                             origin.z + image.depth() * voxel.depth()};

    const auto index = static_cast<std::size_t>(
        ix + image.width() * iy + image.width() * image.height() * iz);
    auto scalar = factory.make_scalar("", cfg, local_domain, 0);
    REQUIRE(scalar()[0] == Catch::Approx(array[index]));
  }

  SECTION("throws for missing sme diffusion array and unsupported vector") {
    auto m{getExampleModel(Mod::VerySimpleModel)};
    simulate::DuneConverter dc(m);
    simulate::SmeFunctorFactory<2> factory(dc);

    Dune::Copasi::LocalDomain<2> local_domain;
    local_domain.position = {0.0, 0.0};

    Dune::ParameterTree missingArrayCfg;
    missingArrayCfg["parser_type"] = "sme";
    missingArrayCfg["expression"] = "does_not_exist";
    REQUIRE_THROWS(factory.make_scalar("", missingArrayCfg, local_domain, 0));

    Dune::ParameterTree vectorCfg;
    vectorCfg["parser_type"] = "sme";
    vectorCfg["expression"] = "anything";
    REQUIRE_THROWS(factory.make_vector("", vectorCfg, local_domain, 0));
  }
}
