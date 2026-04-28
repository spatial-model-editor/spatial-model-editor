#include "catch_wrapper.hpp"
#include "sme/feature_options.hpp"
#include <cereal/archives/json.hpp>
#include <sstream>

using namespace sme;

TEST_CASE("FeatureOptions",
          "[core/simulate/feature_options][core/simulate][core][feature_"
          "options]") {
  SECTION("toString helpers") {
    REQUIRE(simulate::toString(simulate::RoiType::Analytic) == "Analytic");
    REQUIRE(simulate::toString(simulate::RoiType::Image) == "Image");
    REQUIRE(simulate::toString(simulate::RoiType::Depth) == "Depth");
    REQUIRE(simulate::toString(simulate::RoiType::AxisSlices) == "Axis slices");
    REQUIRE(simulate::toString(simulate::ReductionOp::Average) == "Average");
    REQUIRE(simulate::toString(simulate::ReductionOp::Sum) == "Sum");
    REQUIRE(simulate::toString(simulate::ReductionOp::Min) == "Min");
    REQUIRE(simulate::toString(simulate::ReductionOp::Max) == "Max");
    REQUIRE(simulate::toString(simulate::ReductionOp::FirstQuantile) ==
            "First quantile");
    REQUIRE(simulate::toString(simulate::ReductionOp::Median) == "Median");
    REQUIRE(simulate::toString(simulate::ReductionOp::ThirdQuantile) ==
            "Third quantile");
  }
  SECTION("RoiSettings serialization round-trip") {
    simulate::RoiSettings roi;
    roi.roiType = simulate::RoiType::Image;
    roi.expression = "int(x > 1)";
    roi.numRegions = 5;
    roi.regionImage = {0, 1, 2, 3, 0, 1};
    simulate::setRoiParameterInt(roi, simulate::roi_param::axis, 2);
    simulate::setRoiParameterInt(roi, simulate::roi_param::depthThicknessVoxels,
                                 3);
    simulate::setRoiParameterDouble(roi, "double_param", 1.5);
    simulate::setRoiParameterBool(roi, "bool_param", true);

    std::stringstream ss;
    {
      cereal::JSONOutputArchive oar(ss);
      oar(roi);
    }
    std::string json = ss.str();
    simulate::RoiSettings roi2;
    {
      std::istringstream iss(json);
      cereal::JSONInputArchive iar(iss);
      iar(roi2);
    }
    REQUIRE(roi2.roiType == simulate::RoiType::Image);
    REQUIRE(roi2.expression == "int(x > 1)");
    REQUIRE(roi2.numRegions == 5);
    REQUIRE(roi2.regionImage == std::vector<std::size_t>{0, 1, 2, 3, 0, 1});
    REQUIRE(simulate::getRoiParameterInt(roi2, simulate::roi_param::axis, 0) ==
            2);
    REQUIRE(simulate::getRoiParameterInt(
                roi2, simulate::roi_param::depthThicknessVoxels, 1) == 3);
    REQUIRE(simulate::getRoiParameterDouble(roi2, "double_param", 0.0) ==
            dbl_approx(1.5));
    REQUIRE(simulate::getRoiParameterBool(roi2, "bool_param", false) == true);
  }
  SECTION("FeatureDefinition serialization round-trip") {
    simulate::FeatureDefinition feat;
    feat.id = "core_concentration";
    feat.name = "core_concentration";
    feat.compartmentId = "comp1";
    feat.speciesId = "specA";
    feat.roi.roiType = simulate::RoiType::AxisSlices;
    feat.roi.numRegions = 4;
    simulate::setRoiParameterInt(feat.roi, simulate::roi_param::axis, 2);
    feat.reduction = simulate::ReductionOp::Median;

    std::stringstream ss;
    {
      cereal::JSONOutputArchive oar(ss);
      oar(feat);
    }
    std::string json = ss.str();
    simulate::FeatureDefinition feat2;
    {
      std::istringstream iss(json);
      cereal::JSONInputArchive iar(iss);
      iar(feat2);
    }
    REQUIRE(feat2.id == "core_concentration");
    REQUIRE(feat2.name == "core_concentration");
    REQUIRE(feat2.compartmentId == "comp1");
    REQUIRE(feat2.speciesId == "specA");
    REQUIRE(feat2.roi.roiType == simulate::RoiType::AxisSlices);
    REQUIRE(feat2.roi.numRegions == 4);
    REQUIRE(simulate::getRoiParameterInt(feat2.roi, simulate::roi_param::axis,
                                         0) == 2);
    REQUIRE(feat2.reduction == simulate::ReductionOp::Median);
  }
  SECTION("FeatureResult serialization round-trip") {
    simulate::FeatureResult res;
    res.values = {{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}};

    std::stringstream ss;
    {
      cereal::JSONOutputArchive oar(ss);
      oar(res);
    }
    std::string json = ss.str();
    simulate::FeatureResult res2;
    {
      std::istringstream iss(json);
      cereal::JSONInputArchive iar(iss);
      iar(res2);
    }
    REQUIRE(res2.values.size() == 2);
    REQUIRE(res2.values[0] == std::vector<double>{1.0, 2.0, 3.0});
    REQUIRE(res2.values[1] == std::vector<double>{4.0, 5.0, 6.0});
  }
  SECTION("default values") {
    simulate::RoiSettings roi;
    REQUIRE(roi.roiType == simulate::RoiType::Analytic);
    REQUIRE(roi.expression == "1");
    REQUIRE(roi.numRegions == 1);
    REQUIRE(roi.regionImage.empty());
    REQUIRE(roi.parameters.empty());
    REQUIRE(simulate::getRoiParameterInt(roi, "missing", 3) == 3);
    REQUIRE(simulate::getRoiParameterDouble(roi, "missing", 2.0) ==
            dbl_approx(2.0));
    REQUIRE(simulate::getRoiParameterBool(roi, "missing", true) == true);

    simulate::FeatureDefinition feat;
    REQUIRE(feat.id.empty());
    REQUIRE(feat.name.empty());
    REQUIRE(feat.compartmentId.empty());
    REQUIRE(feat.speciesId.empty());
    REQUIRE(feat.reduction == simulate::ReductionOp::Average);

    simulate::FeatureResult res;
    REQUIRE(res.values.empty());
  }
  SECTION("makeUniqueFeatureId") {
    std::vector<simulate::FeatureDefinition> features{
        {.id = "feature_a", .name = "feature a"},
        {.id = "feature_a_", .name = "feature a_"}};
    REQUIRE(simulate::makeUniqueFeatureId("feature a", features) ==
            "feature_a__");
    REQUIRE(simulate::makeUniqueFeatureId("123", {}) == "_123");
    REQUIRE(simulate::makeUniqueFeatureId("!@#", {}) == "feature");
  }
}
