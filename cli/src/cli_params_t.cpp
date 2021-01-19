#include "catch_wrapper.hpp"
#include "cli_params.hpp"

using namespace sme;

SCENARIO("CLI Params", "[cli][params]") {
  REQUIRE(cli::toString(simulate::SimulatorType::DUNE) == "DUNE");
  REQUIRE(cli::toString(simulate::SimulatorType::Pixel) == "Pixel");

  REQUIRE_NOTHROW(cli::printParams(cli::Params{}));

  CLI::App a;
  cli::setupCLI(a);
  REQUIRE(a.get_description().substr(0, 24) == "Spatial Model Editor CLI");
  REQUIRE(a.get_groups().size() == 1);
  REQUIRE(a.get_options().size() == 9);
  REQUIRE(a.get_option("file")->get_required() == true);
}
