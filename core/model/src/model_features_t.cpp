#include "catch_wrapper.hpp"
#include "model_test_utils.hpp"
#include "sme/model.hpp"
#include "sme/model_features.hpp"

using namespace sme;
using Mod = test::Mod;

namespace {

void removeAllFeatures(model::Model &m) {
  auto &features = m.getFeatures();
  while (features.size() > 0) {
    features.remove(0);
  }
}

} // namespace

TEST_CASE("ModelFeatures",
          "[core/model/model_features][core/model][core][model_features]") {
  SECTION("default construction") {
    model::ModelFeatures mf;
    REQUIRE(mf.size() == 0);
    REQUIRE(mf.getHasUnsavedChanges() == false);
  }
  SECTION("CRUD on VerySimpleModel") {
    auto m{test::getExampleModel(Mod::VerySimpleModel)};
    removeAllFeatures(m);
    auto &mf{m.getFeatures()};
    REQUIRE(mf.size() == 0);

    // add a feature
    simulate::RoiSettings roi;
    roi.roiType = simulate::RoiType::Analytic;
    auto idx =
        mf.add("avg_A_c1", "c1", "A_c1", roi, simulate::ReductionOp::Average);
    REQUIRE(idx == 0);
    REQUIRE(mf.size() == 1);
    REQUIRE(mf.getFeatures()[0].id == "avg_A_c1");
    REQUIRE(mf.getFeatures()[0].name == "avg_A_c1");
    REQUIRE(mf.getFeatures()[0].compartmentId == "c1");
    REQUIRE(mf.getFeatures()[0].speciesId == "A_c1");
    REQUIRE(mf.isValid(0));
    REQUIRE(mf.nRegions(0) == 1);
    REQUIRE(mf.getHasUnsavedChanges());

    // add another feature
    simulate::RoiSettings roi2;
    roi2.roiType = simulate::RoiType::Depth;
    roi2.numRegions = 3;
    auto idx2 =
        mf.add("depth_B_c2", "c2", "B_c2", roi2, simulate::ReductionOp::Max);
    REQUIRE(idx2 == 1);
    REQUIRE(mf.size() == 2);
    REQUIRE(mf.getFeatures()[1].id == "depth_B_c2");
    REQUIRE(mf.getFeatures()[0].id != mf.getFeatures()[1].id);
    REQUIRE(mf.nRegions(1) == 3);

    // rename
    auto oldId = mf.getFeatures()[0].id;
    REQUIRE(mf.setName(0, "renamed") == "renamed");
    REQUIRE(mf.getFeatures()[0].id == oldId);
    REQUIRE(mf.getFeatures()[0].name == "renamed");

    // remove first
    mf.remove(0);
    REQUIRE(mf.size() == 1);
    REQUIRE(mf.getFeatures()[0].name == "depth_B_c2");
  }
  SECTION("isValid with invalid references") {
    auto m{test::getExampleModel(Mod::VerySimpleModel)};
    removeAllFeatures(m);
    auto &mf{m.getFeatures()};

    simulate::RoiSettings roi;
    roi.roiType = simulate::RoiType::Analytic;

    // invalid compartment
    mf.add("bad_comp", "nonexistent", "A_c1", roi,
           simulate::ReductionOp::Average);
    REQUIRE(!mf.isValid(0));

    // invalid species
    mf.add("bad_spec", "c1", "nonexistent", roi,
           simulate::ReductionOp::Average);
    REQUIRE(!mf.isValid(1));

    // valid
    mf.add("good", "c1", "A_c1", roi, simulate::ReductionOp::Average);
    REQUIRE(mf.isValid(2));
  }
  SECTION("getIndexFromId") {
    auto m{test::getExampleModel(Mod::VerySimpleModel)};
    removeAllFeatures(m);
    auto &mf{m.getFeatures()};
    simulate::RoiSettings roi;
    roi.roiType = simulate::RoiType::Analytic;
    mf.add("feat1", "c1", "A_c1", roi, simulate::ReductionOp::Average);
    mf.add("feat1", "c1", "A_c1", roi, simulate::ReductionOp::Average);
    REQUIRE(mf.getFeatures()[0].id == "feat1");
    REQUIRE(mf.getFeatures()[0].name == "feat1");
    REQUIRE(mf.getFeatures()[1].id == "feat1_");
    REQUIRE(mf.getFeatures()[1].name == "feat1_");
    REQUIRE(mf.setName(1, "feat1") == "feat1_");
    REQUIRE(mf.getFeatures()[1].id == "feat1_");
    REQUIRE(mf.getFeatures()[1].name == "feat1_");
    REQUIRE(mf.getIndexFromId("feat1") == 0);
    REQUIRE(mf.getIndexFromId("feat1_") == 1);
    REQUIRE(mf.getIndexFromId("missing") == mf.size());
  }
  SECTION("voxel regions computed on add") {
    auto m{test::getExampleModel(Mod::VerySimpleModel)};
    removeAllFeatures(m);
    auto &mf{m.getFeatures()};

    simulate::RoiSettings roi;
    roi.roiType = simulate::RoiType::Analytic;
    const auto featureIndex =
        mf.add("feat", "c1", "A_c1", roi, simulate::ReductionOp::Average);
    REQUIRE(mf.isValid(featureIndex));
    // voxel regions should be non-empty for a valid feature
    const auto &regions = mf.getVoxelRegions(featureIndex);
    REQUIRE(!regions.empty());
    // default analytic expression "1" assigns every voxel to region 1
    for (auto region : regions) {
      REQUIRE(region == 1);
    }
  }
}
