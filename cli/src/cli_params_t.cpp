#include "catch_wrapper.hpp"
#include "cli_params.hpp"
#include "model_test_utils.hpp"
#include "sme/optimize_options.hpp"

using namespace sme;

TEST_CASE("CLI Params", "[cli][params]") {
  REQUIRE(cli::toString(simulate::SimulatorType::DUNE) == "DUNE");
  REQUIRE(cli::toString(simulate::SimulatorType::Pixel) == "Pixel");

  REQUIRE_NOTHROW(cli::printParams(cli::Params{}));

  CLI::App a;
  auto params = cli::setupCLI(a);
  REQUIRE(a.get_description().substr(0, 24) == "Spatial Model Editor CLI");
  REQUIRE(a.get_groups().size() == 1);
  REQUIRE(a.get_options().size() == 4);

  test::createBinaryFile("smefiles/very-simple-model-v3.sme", "x.sme");
  for (std::size_t i = 0; i < simulate::optAlgorithmTypes.size(); ++i) {
    REQUIRE_NOTHROW(a.parse(fmt::format("fit x.sme -a{}", i)));
    REQUIRE(params.fit.algorithm == simulate::optAlgorithmTypes[i]);
  }
}
