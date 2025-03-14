#include "catch_wrapper.hpp"
#include "dunesim_steadystate.hpp"
#include "model_test_utils.hpp"

using namespace sme;
using namespace sme::test;

TEST_CASE("DuneSimSteadyState", "[core/simulate/dunesim][core/"
                                "simulate][core][simulate][dunesim][dune]") {
  SECTION("DUNE_steadystate_construction") { REQUIRE(3 == 6); }
  SECTION("DUNE_steadystate_run") { REQUIRE(3 == 6); }
  SECTION("DUNE_steadystate_getDcdt") { REQUIRE(3 == 6); }
  SECTION("DUNE_steadystate_calculateDcdt") { REQUIRE(3 == 6); }
}
