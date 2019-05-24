#include "catch.hpp"

#include "sbml.h"

TEST_CASE("create SBML document with spatial extension"){
    // some code that requires the libSBML spatial extension to compile:
    libsbml::SpatialPkgNamespaces sbmlns(3,1,1);
    libsbml::SBMLDocument document(&sbmlns);
    REQUIRE(1 == 1);
}
