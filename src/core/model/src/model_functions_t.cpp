#include "catch_wrapper.hpp"
#include "model.hpp"
#include "model_functions.hpp"
#include "model_test_utils.hpp"

using namespace sme;
using namespace sme::test;

TEST_CASE("SBML functions",
          "[core/model/functions][core/model][core][model][functions]") {
  SECTION("SBML: yeast-glycolysis") {
    auto s{getTestModel("yeast-glycolysis")};
    REQUIRE(s.getHasUnsavedChanges() == false);
    REQUIRE(s.getCompartments().getIds().size() == 1);
    REQUIRE(s.getCompartments().getIds()[0] == "compartment");
    REQUIRE(s.getSpecies().getIds("compartment").size() == 25);
    auto &funcs{s.getFunctions()};
    REQUIRE(funcs.getIds().size() == 17);
    REQUIRE(funcs.getIds()[0] == "HK_kinetics");
    REQUIRE(funcs.getName("HK_kinetics") == "HK kinetics");
    REQUIRE(funcs.getArguments("HK_kinetics").size() == 10);
    REQUIRE(funcs.getArguments("HK_kinetics")[0] == "A");
    REQUIRE(funcs.getExpression("HK_kinetics") ==
            "Vmax * (A * B / (Kglc * Katp) - P * Q / (Kglc * Katp * Keq)) "
            "/ ((1 + A / Kglc + P / Kg6p) * (1 + B / Katp + Q / Kadp))");
    SECTION("inline fn: Glycogen_synthesis_kinetics") {
      std::string expr = "Glycogen_synthesis_kinetics(abc)";
      std::string inlined = "(abc)";
      REQUIRE(s.inlineExpr(expr) == inlined);
    }
    SECTION("inline fn: ATPase_0") {
      std::string expr = "ATPase_0( a,b)";
      std::string inlined = "(b * a)";
      REQUIRE(s.inlineExpr(expr) == inlined);
    }
    SECTION("inline fn: PDC_kinetics") {
      std::string expr = "PDC_kinetics(a,V,k,n)";
      std::string inlined = "(V * (a / k)^n / (1 + (a / k)^n))";
      REQUIRE(s.inlineExpr(expr) == inlined);
    }
    SECTION("edit function: PDC_kinetics") {
      REQUIRE(funcs.getArguments("PDC_kinetics").size() == 4);
      REQUIRE(funcs.getArguments("PDC_kinetics")[0] == "A");
      REQUIRE(funcs.getArguments("PDC_kinetics")[1] == "Vmax");
      REQUIRE(funcs.getArguments("PDC_kinetics")[2] == "Kpyr");
      REQUIRE(funcs.getArguments("PDC_kinetics")[3] == "nH");
      REQUIRE(s.getHasUnsavedChanges() == false);
      REQUIRE(funcs.getHasUnsavedChanges() == false);
      auto arg{funcs.addArgument("PDC_kinetics", "x")};
      REQUIRE(s.getHasUnsavedChanges() == true);
      REQUIRE(funcs.getHasUnsavedChanges() == true);
      REQUIRE(arg == "x");
      funcs.setName("PDC_kinetics", "newName!");
      funcs.setExpression("PDC_kinetics", "(V*(x/k)^n/(1+(a/k)^n))");
      REQUIRE(funcs.getName("PDC_kinetics") == "newName!");
      REQUIRE(funcs.getArguments("PDC_kinetics").size() == 5);
      REQUIRE(funcs.getArguments("PDC_kinetics")[0] == "A");
      REQUIRE(funcs.getArguments("PDC_kinetics")[1] == "Vmax");
      REQUIRE(funcs.getArguments("PDC_kinetics")[2] == "Kpyr");
      REQUIRE(funcs.getArguments("PDC_kinetics")[3] == "nH");
      REQUIRE(funcs.getArguments("PDC_kinetics")[4] == "x");
      REQUIRE(funcs.getExpression("PDC_kinetics") ==
              "V * (x / k)^n / (1 + (a / k)^n)");
      std::string expr{"PDC_kinetics(a,V,k,n,Q)"};
      std::string inlined{"(V * (Q / k)^n / (1 + (a / k)^n))"};
      REQUIRE(s.inlineExpr(expr) == inlined);
      funcs.remove("PDC_kinetics");
      REQUIRE(funcs.getIds().size() == 16);
    }
    SECTION("add function") {
      REQUIRE(s.getHasUnsavedChanges() == false);
      REQUIRE(funcs.getHasUnsavedChanges() == false);
      funcs.add("func N~!me");
      REQUIRE(s.getHasUnsavedChanges() == true);
      REQUIRE(funcs.getHasUnsavedChanges() == true);
      REQUIRE(funcs.getName("func_Nme") == "func N~!me");
      REQUIRE(funcs.getArguments("func_Nme").isEmpty());
      REQUIRE(funcs.getExpression("func_Nme") == "0");
    }
  }
}
