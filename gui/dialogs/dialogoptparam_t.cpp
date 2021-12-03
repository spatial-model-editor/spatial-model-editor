#include "catch_wrapper.hpp"
#include "dialogoptparam.hpp"
#include "qt_test_utils.hpp"

using namespace sme;
using namespace sme::test;

TEST_CASE("DialogOptParam", "[gui/dialogs/optparam][gui/"
                            "dialogs][gui][optimize]") {
  std::vector<simulate::OptParam> defaultOptParams;
  defaultOptParams.push_back({simulate::OptParamType::ReactionParameter,
                              "myReacOptParam1", "k1", "reac1", 0.1, 0.2});
  defaultOptParams.push_back({simulate::OptParamType::ReactionParameter,
                              "myReacOptParam2", "k7", "reac2", 0.5, 0.9});
  defaultOptParams.push_back({simulate::OptParamType::ModelParameter,
                              "myOptParam", "p1", "", 0.0, 0.05});
  auto optParam{defaultOptParams.back()};
  ModalWidgetTimer mwt;
  SECTION("no parameters") {
    DialogOptParam dia({});
    mwt.addUserAction();
    mwt.start();
    dia.exec();
  }
  SECTION("no pre-selected parameter") {
    DialogOptParam dia(defaultOptParams);
    mwt.addUserAction();
    mwt.start();
    dia.exec();
    REQUIRE(dia.getOptParam().optParamType ==
            simulate::OptParamType::ReactionParameter);
    REQUIRE(dia.getOptParam().name == "myReacOptParam1");
  }
  SECTION("user does nothing: correct initial values") {
    DialogOptParam dia(defaultOptParams, &optParam);
    mwt.addUserAction();
    mwt.start();
    dia.exec();
    REQUIRE(dia.getOptParam().optParamType ==
            simulate::OptParamType::ModelParameter);
    REQUIRE(dia.getOptParam().name == "myOptParam");
    REQUIRE(dia.getOptParam().id == "p1");
    REQUIRE(dia.getOptParam().parentId == "");
    REQUIRE(dia.getOptParam().lowerBound == dbl_approx(0.0));
    REQUIRE(dia.getOptParam().upperBound == dbl_approx(0.05));
  }
  SECTION("user changes bounds") {
    DialogOptParam dia(defaultOptParams, &optParam);
    mwt.addUserAction({"Tab", "0", ".", "4", "Tab", "1"});
    mwt.start();
    dia.exec();
    REQUIRE(dia.getOptParam().optParamType ==
            simulate::OptParamType::ModelParameter);
    REQUIRE(dia.getOptParam().name == "myOptParam");
    REQUIRE(dia.getOptParam().id == "p1");
    REQUIRE(dia.getOptParam().parentId == "");
    REQUIRE(dia.getOptParam().lowerBound == dbl_approx(0.4));
    REQUIRE(dia.getOptParam().upperBound == dbl_approx(1));
  }
  SECTION("user changes parameter: bounds set from corresponding default") {
    DialogOptParam dia(defaultOptParams, &optParam);
    mwt.addUserAction({"Up"});
    mwt.start();
    dia.exec();
    REQUIRE(dia.getOptParam().optParamType ==
            simulate::OptParamType::ReactionParameter);
    REQUIRE(dia.getOptParam().name == "myReacOptParam2");
    REQUIRE(dia.getOptParam().id == "k7");
    REQUIRE(dia.getOptParam().parentId == "reac2");
    REQUIRE(dia.getOptParam().lowerBound == dbl_approx(0.5));
    REQUIRE(dia.getOptParam().upperBound == dbl_approx(0.9));
  }
  SECTION("user changes parameter and bounds") {
    DialogOptParam dia(defaultOptParams, &optParam);
    mwt.addUserAction({"Up", "Up", "Tab", "6", "Tab", "1", "9"});
    mwt.start();
    dia.exec();
    REQUIRE(dia.getOptParam().optParamType ==
            simulate::OptParamType::ReactionParameter);
    REQUIRE(dia.getOptParam().name == "myReacOptParam1");
    REQUIRE(dia.getOptParam().id == "k1");
    REQUIRE(dia.getOptParam().parentId == "reac1");
    REQUIRE(dia.getOptParam().lowerBound == dbl_approx(6));
    REQUIRE(dia.getOptParam().upperBound == dbl_approx(19));
  }
}
