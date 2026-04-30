#include "catch_wrapper.hpp"
#include "sme/feature_eval.hpp"
#include "sme/feature_options.hpp"
#include "sme/geometry.hpp"
#include <QImage>
#include <algorithm>

using namespace sme;

static geometry::Compartment make5x5Compartment() {
  QImage img(5, 5, QImage::Format_RGB32);
  const auto col = qRgb(100, 100, 100);
  img.fill(col);
  common::ImageStack imgs(std::vector<QImage>{img});
  return geometry::Compartment("comp", imgs, col);
}

static geometry::Compartment make3x3CrossCompartment() {
  QImage img(3, 3, QImage::Format_RGB32);
  const auto col = qRgb(200, 200, 200);
  const auto bg = qRgb(0, 0, 0);
  img.fill(bg);
  img.setPixel(1, 0, col);
  img.setPixel(0, 1, col);
  img.setPixel(1, 1, col);
  img.setPixel(2, 1, col);
  img.setPixel(1, 2, col);
  common::ImageStack imgs(std::vector<QImage>{img});
  return geometry::Compartment("cross", imgs, col);
}

static geometry::Compartment make2x2x3Compartment() {
  const auto col = qRgb(150, 150, 150);
  std::vector<QImage> images;
  for (int z = 0; z < 3; ++z) {
    QImage img(2, 2, QImage::Format_RGB32);
    img.fill(col);
    images.push_back(img);
  }
  common::ImageStack imgs(images);
  return geometry::Compartment("comp3d", imgs, col);
}

TEST_CASE("FeatureEval",
          "[core/simulate/feature_eval][core/simulate][core][feature_eval]") {
  SECTION("getNumRegions") {
    simulate::RoiSettings roi;
    REQUIRE(simulate::getNumRegions(roi) == 1);

    roi.roiType = simulate::RoiType::Image;
    roi.numRegions = 5;
    REQUIRE(simulate::getNumRegions(roi) == 5);

    roi.roiType = simulate::RoiType::Depth;
    roi.numRegions = 3;
    REQUIRE(simulate::getNumRegions(roi) == 3);

    roi.roiType = simulate::RoiType::AxisSlices;
    roi.numRegions = 4;
    REQUIRE(simulate::getNumRegions(roi) == 4);
  }

  SECTION("applyReduction: Average") {
    std::vector<double> concs = {2.0, 4.0, 6.0, 8.0};
    std::vector<std::size_t> regions = {1, 1, 2, 1};
    REQUIRE(simulate::applyReduction(simulate::ReductionOp::Average, concs,
                                     regions,
                                     1) == dbl_approx((2.0 + 4.0 + 8.0) / 3.0));
    REQUIRE(simulate::applyReduction(simulate::ReductionOp::Average, concs,
                                     regions, 2) == dbl_approx(6.0));
  }

  SECTION("applyReduction: Sum") {
    std::vector<double> concs = {1.0, 2.0, 3.0};
    std::vector<std::size_t> regions = {1, 1, 1};
    REQUIRE(simulate::applyReduction(simulate::ReductionOp::Sum, concs, regions,
                                     1) == dbl_approx(6.0));
  }

  SECTION("applyReduction: Min/Max") {
    std::vector<double> concs = {5.0, 1.0, 3.0, 7.0};
    std::vector<std::size_t> regions = {1, 1, 2, 1};
    REQUIRE(simulate::applyReduction(simulate::ReductionOp::Min, concs, regions,
                                     1) == dbl_approx(1.0));
    REQUIRE(simulate::applyReduction(simulate::ReductionOp::Max, concs, regions,
                                     1) == dbl_approx(7.0));
    REQUIRE(simulate::applyReduction(simulate::ReductionOp::Min, concs, regions,
                                     2) == dbl_approx(3.0));
  }

  SECTION("applyReduction: empty region") {
    std::vector<double> concs = {1.0, 2.0};
    std::vector<std::size_t> regions = {1, 1};
    REQUIRE(simulate::applyReduction(simulate::ReductionOp::Average, concs,
                                     regions, 5) == dbl_approx(0.0));
    REQUIRE(simulate::applyReduction(simulate::ReductionOp::Sum, concs, regions,
                                     5) == dbl_approx(0.0));
    REQUIRE(simulate::applyReduction(simulate::ReductionOp::Min, concs, regions,
                                     5) == dbl_approx(0.0));
    REQUIRE(simulate::applyReduction(simulate::ReductionOp::Max, concs, regions,
                                     5) == dbl_approx(0.0));
    REQUIRE(simulate::applyReduction(simulate::ReductionOp::Median, concs,
                                     regions, 5) == dbl_approx(0.0));
  }

  SECTION("applyReduction: quantiles") {
    std::vector<double> concs = {8.0, 2.0, 4.0, 10.0, 6.0};
    std::vector<std::size_t> regions = {1, 1, 1, 1, 1};
    REQUIRE(simulate::applyReduction(simulate::ReductionOp::FirstQuartile,
                                     concs, regions, 1) == dbl_approx(4.0));
    REQUIRE(simulate::applyReduction(simulate::ReductionOp::Median, concs,
                                     regions, 1) == dbl_approx(6.0));
    REQUIRE(simulate::applyReduction(simulate::ReductionOp::ThirdQuartile,
                                     concs, regions, 1) == dbl_approx(8.0));
  }

  SECTION("computeDepthRegions: 5x5 compartment") {
    auto comp = make5x5Compartment();
    REQUIRE(comp.nVoxels() == 25);
    auto regions = simulate::computeDepthRegions(comp, 3);
    REQUIRE(regions.size() == 25);
    std::size_t count1 = 0, count2 = 0, count3 = 0;
    for (auto region : regions) {
      if (region == 1) {
        ++count1;
      } else if (region == 2) {
        ++count2;
      } else if (region == 3) {
        ++count3;
      }
    }
    REQUIRE(count1 == 16);
    REQUIRE(count2 == 8);
    REQUIRE(count3 == 1);
  }

  SECTION(
      "computeDepthRegions: deeper voxels excluded beyond requested depth") {
    auto comp = make5x5Compartment();
    REQUIRE(comp.nVoxels() == 25);

    auto regions1 = simulate::computeDepthRegions(comp, 1);
    std::size_t count0 = 0, count1 = 0;
    for (auto region : regions1) {
      if (region == 0) {
        ++count0;
      } else if (region == 1) {
        ++count1;
      }
    }
    REQUIRE(count0 == 9);
    REQUIRE(count1 == 16);

    auto regions2 = simulate::computeDepthRegions(comp, 2);
    std::size_t count00 = 0, count11 = 0, count22 = 0;
    for (auto region : regions2) {
      if (region == 0) {
        ++count00;
      } else if (region == 1) {
        ++count11;
      } else if (region == 2) {
        ++count22;
      }
    }
    REQUIRE(count00 == 1);
    REQUIRE(count11 == 16);
    REQUIRE(count22 == 8);
  }

  SECTION("computeDepthRegions: groups voxel layers by thickness") {
    auto comp = make5x5Compartment();
    REQUIRE(comp.nVoxels() == 25);

    auto regions1 = simulate::computeDepthRegions(comp, 1, 2);
    std::size_t count0 = 0, count1 = 0;
    for (auto region : regions1) {
      if (region == 0) {
        ++count0;
      } else if (region == 1) {
        ++count1;
      }
    }
    REQUIRE(count0 == 1);
    REQUIRE(count1 == 24);

    auto regions2 = simulate::computeDepthRegions(comp, 2, 2);
    std::size_t count00 = 0, count11 = 0, count22 = 0;
    for (auto region : regions2) {
      if (region == 0) {
        ++count00;
      } else if (region == 1) {
        ++count11;
      } else if (region == 2) {
        ++count22;
      }
    }
    REQUIRE(count00 == 0);
    REQUIRE(count11 == 24);
    REQUIRE(count22 == 1);

    simulate::RoiSettings roi;
    roi.roiType = simulate::RoiType::Depth;
    roi.numRegions = 1;
    simulate::setRoiParameterInt(roi, simulate::roi_param::depthThicknessVoxels,
                                 2);
    auto voxelRegions = simulate::computeVoxelRegions(roi, comp, {}, {});
    REQUIRE(std::ranges::count(voxelRegions, 1) == 24);
    REQUIRE(std::ranges::count(voxelRegions, 0) == 1);
  }

  SECTION("computeDepthRegions: single voxel") {
    QImage img(1, 1, QImage::Format_RGB32);
    const auto col = qRgb(50, 50, 50);
    img.setPixel(0, 0, col);
    common::ImageStack imgs(std::vector<QImage>{img});
    geometry::Compartment comp("single", imgs, col);
    auto regions = simulate::computeDepthRegions(comp, 3);
    REQUIRE(regions.size() == 1);
    REQUIRE(regions[0] == 1);
  }

  SECTION("computeDepthRegions: empty compartment") {
    QImage img(1, 1, QImage::Format_RGB32);
    img.fill(qRgb(0, 0, 0));
    common::ImageStack imgs(std::vector<QImage>{img});
    geometry::Compartment comp("empty", imgs, qRgb(255, 255, 255));
    auto regions = simulate::computeDepthRegions(comp, 2);
    REQUIRE(regions.empty());
  }

  SECTION("computeDepthRegions: cross-shaped compartment") {
    auto comp = make3x3CrossCompartment();
    REQUIRE(comp.nVoxels() == 5);
    auto regions = simulate::computeDepthRegions(comp, 2);
    REQUIRE(regions.size() == 5);
    std::size_t count1 = 0, count2 = 0;
    for (auto region : regions) {
      if (region == 1) {
        ++count1;
      } else if (region == 2) {
        ++count2;
      }
    }
    REQUIRE(count1 == 4);
    REQUIRE(count2 == 1);
  }

  SECTION("computeAxisSliceRegions: x bands split compartment extent") {
    auto comp = make5x5Compartment();
    REQUIRE(comp.nVoxels() == 25);
    auto regions = simulate::computeAxisSliceRegions(comp, 2, 0);
    REQUIRE(regions.size() == 25);
    REQUIRE(std::ranges::count(regions, 1) == 15);
    REQUIRE(std::ranges::count(regions, 2) == 10);
  }

  SECTION("computeAxisSliceRegions: y bands use physical image orientation") {
    auto comp = make5x5Compartment();
    auto regions = simulate::computeAxisSliceRegions(comp, 5, 1);
    REQUIRE(regions.size() == comp.nVoxels());
    for (std::size_t i = 0; i < comp.nVoxels(); ++i) {
      const auto &voxel = comp.getVoxel(i);
      if (voxel.p.y() == 0) {
        REQUIRE(regions[i] == 5);
      } else if (voxel.p.y() == 4) {
        REQUIRE(regions[i] == 1);
      }
    }
  }

  SECTION("computeAxisSliceRegions: z bands split 3d compartment") {
    auto comp = make2x2x3Compartment();
    auto regions = simulate::computeAxisSliceRegions(comp, 3, 2);
    REQUIRE(regions.size() == 12);
    REQUIRE(std::ranges::count(regions, 1) == 4);
    REQUIRE(std::ranges::count(regions, 2) == 4);
    REQUIRE(std::ranges::count(regions, 3) == 4);
  }

  SECTION("computeVoxelRegions: axis slices use axis parameter") {
    auto comp = make5x5Compartment();
    simulate::RoiSettings roi;
    roi.roiType = simulate::RoiType::AxisSlices;
    roi.numRegions = 5;
    simulate::setRoiParameterInt(roi, simulate::roi_param::axis, 1);
    auto regions = simulate::computeVoxelRegions(roi, comp, {}, {});
    for (std::size_t i = 0; i < comp.nVoxels(); ++i) {
      const auto &voxel = comp.getVoxel(i);
      if (voxel.p.y() == 0) {
        REQUIRE(regions[i] == 5);
      } else if (voxel.p.y() == 4) {
        REQUIRE(regions[i] == 1);
      }
    }
  }

  SECTION("computeVoxelRegions: analytic expression uses integer cast") {
    auto comp = make5x5Compartment();
    simulate::RoiSettings roi;
    roi.roiType = simulate::RoiType::Analytic;
    roi.expression = "x";
    roi.numRegions = 3;
    common::VoxelF origin(0.0, 0.0, 0.0);
    common::VolumeF voxelSize(1.0, 1.0, 1.0);
    auto regions = simulate::computeVoxelRegions(roi, comp, origin, voxelSize);
    REQUIRE(regions.size() == 25);
    std::size_t count0 = 0, count1 = 0, count2 = 0, count3 = 0;
    for (auto region : regions) {
      if (region == 0) {
        ++count0;
      } else if (region == 1) {
        ++count1;
      } else if (region == 2) {
        ++count2;
      } else if (region == 3) {
        ++count3;
      }
    }
    REQUIRE(count0 == 10);
    REQUIRE(count1 == 5);
    REQUIRE(count2 == 5);
    REQUIRE(count3 == 5);
  }

  SECTION("computeVoxelRegions: analytic y coordinate matches physical image "
          "orientation") {
    auto comp = make5x5Compartment();
    simulate::RoiSettings roi;
    roi.roiType = simulate::RoiType::Analytic;
    roi.expression = "y";
    roi.numRegions = 4;
    common::VoxelF origin(0.0, 0.0, 0.0);
    common::VolumeF voxelSize(1.0, 1.0, 1.0);
    auto regions = simulate::computeVoxelRegions(roi, comp, origin, voxelSize);
    REQUIRE(regions.size() == comp.nVoxels());

    for (std::size_t i = 0; i < comp.nVoxels(); ++i) {
      const auto &voxel = comp.getVoxel(i);
      if (voxel.p.y() == 0) {
        REQUIRE(regions[i] == 4);
      } else if (voxel.p.y() == 4) {
        REQUIRE(regions[i] == 0);
      }
    }
  }

  SECTION("computeVoxelRegions: image regions use direct values") {
    auto comp = make5x5Compartment();
    simulate::RoiSettings roi;
    roi.roiType = simulate::RoiType::Image;
    roi.numRegions = 2;
    roi.regionImage.reserve(25);
    for (int y = 0; y < 5; ++y) {
      for (int x = 0; x < 5; ++x) {
        roi.regionImage.push_back(static_cast<std::size_t>(x));
      }
    }
    common::VoxelF origin(0.0, 0.0, 0.0);
    common::VolumeF voxelSize(1.0, 1.0, 1.0);
    auto regions = simulate::computeVoxelRegions(roi, comp, origin, voxelSize);
    REQUIRE(regions.size() == 25);
    std::size_t count0 = 0, count1 = 0, count2 = 0;
    for (auto region : regions) {
      if (region == 0) {
        ++count0;
      } else if (region == 1) {
        ++count1;
      } else if (region == 2) {
        ++count2;
      }
    }
    REQUIRE(count0 == 15);
    REQUIRE(count1 == 5);
    REQUIRE(count2 == 5);
  }

  SECTION("evaluateFeature: one region -> one scalar") {
    simulate::FeatureDefinition feat;
    feat.name = "avg_conc";
    feat.compartmentId = "comp";
    feat.speciesId = "A";
    feat.roi.roiType = simulate::RoiType::Analytic;
    feat.roi.numRegions = 1;
    feat.reduction = simulate::ReductionOp::Average;

    std::vector<double> concs = {2.0, 4.0, 6.0, 8.0, 10.0};
    std::vector<std::size_t> regions = {1, 1, 1, 1, 1};
    auto results = simulate::evaluateFeature(feat, concs, regions);
    REQUIRE(results.size() == 1);
    REQUIRE(results[0] == dbl_approx(6.0));
  }

  SECTION("evaluateFeature: 3 regions -> 3 scalars") {
    simulate::FeatureDefinition feat;
    feat.name = "regioned";
    feat.compartmentId = "comp";
    feat.speciesId = "A";
    feat.roi.roiType = simulate::RoiType::Image;
    feat.roi.numRegions = 3;
    feat.reduction = simulate::ReductionOp::Sum;

    std::vector<double> concs = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0};
    std::vector<std::size_t> regions = {1, 2, 3, 1, 2, 3};
    auto results = simulate::evaluateFeature(feat, concs, regions);
    REQUIRE(results.size() == 3);
    REQUIRE(results[0] == dbl_approx(5.0));
    REQUIRE(results[1] == dbl_approx(7.0));
    REQUIRE(results[2] == dbl_approx(9.0));
  }

  SECTION("evaluateFeature: excluded voxels use region 0") {
    simulate::FeatureDefinition feat;
    feat.name = "partial";
    feat.compartmentId = "comp";
    feat.speciesId = "A";
    feat.roi.roiType = simulate::RoiType::Analytic;
    feat.roi.numRegions = 1;
    feat.reduction = simulate::ReductionOp::Average;

    std::vector<double> concs = {10.0, 20.0, 30.0, 40.0};
    std::vector<std::size_t> regions = {1, 0, 1, 0};
    auto results = simulate::evaluateFeature(feat, concs, regions);
    REQUIRE(results.size() == 1);
    REQUIRE(results[0] == dbl_approx(20.0));
  }

  SECTION("evaluateFeature: depth regions with reduction") {
    simulate::FeatureDefinition feat;
    feat.name = "depth_max";
    feat.compartmentId = "comp";
    feat.speciesId = "A";
    feat.roi.roiType = simulate::RoiType::Depth;
    feat.roi.numRegions = 2;
    feat.reduction = simulate::ReductionOp::Max;

    std::vector<double> concs = {1.0, 5.0, 3.0, 7.0, 2.0};
    std::vector<std::size_t> regions = {1, 1, 1, 2, 2};
    auto results = simulate::evaluateFeature(feat, concs, regions);
    REQUIRE(results.size() == 2);
    REQUIRE(results[0] == dbl_approx(5.0));
    REQUIRE(results[1] == dbl_approx(7.0));
  }

  SECTION("evaluateFeature: one region with quantile reductions") {
    simulate::FeatureDefinition feat;
    feat.name = "quantiles";
    feat.compartmentId = "comp";
    feat.speciesId = "A";
    feat.roi.roiType = simulate::RoiType::Analytic;
    feat.roi.numRegions = 1;

    std::vector<double> concs = {8.0, 2.0, 4.0, 10.0, 6.0};
    std::vector<std::size_t> regions = {1, 1, 1, 1, 1};

    feat.reduction = simulate::ReductionOp::FirstQuartile;
    auto results = simulate::evaluateFeature(feat, concs, regions);
    REQUIRE(results.size() == 1);
    REQUIRE(results[0] == dbl_approx(4.0));

    feat.reduction = simulate::ReductionOp::Median;
    results = simulate::evaluateFeature(feat, concs, regions);
    REQUIRE(results.size() == 1);
    REQUIRE(results[0] == dbl_approx(6.0));

    feat.reduction = simulate::ReductionOp::ThirdQuartile;
    results = simulate::evaluateFeature(feat, concs, regions);
    REQUIRE(results.size() == 1);
    REQUIRE(results[0] == dbl_approx(8.0));
  }

  SECTION("evaluateFeature: quantiles are computed per region") {
    simulate::FeatureDefinition feat;
    feat.name = "region_quantiles";
    feat.compartmentId = "comp";
    feat.speciesId = "A";
    feat.roi.roiType = simulate::RoiType::Image;
    feat.roi.numRegions = 2;
    feat.reduction = simulate::ReductionOp::Median;

    std::vector<double> concs = {1.0, 100.0, 5.0, 200.0, 9.0, 300.0};
    std::vector<std::size_t> regions = {1, 2, 1, 2, 1, 2};
    auto results = simulate::evaluateFeature(feat, concs, regions);
    REQUIRE(results.size() == 2);
    REQUIRE(results[0] == dbl_approx(5.0));
    REQUIRE(results[1] == dbl_approx(200.0));
  }

  SECTION(
      "evaluateFeature: quantiles ignore excluded and out of range regions") {
    simulate::FeatureDefinition feat;
    feat.name = "partial_quantile";
    feat.compartmentId = "comp";
    feat.speciesId = "A";
    feat.roi.roiType = simulate::RoiType::Analytic;
    feat.roi.numRegions = 1;
    feat.reduction = simulate::ReductionOp::Median;

    std::vector<double> concs = {1.0, 1000.0, 5.0, 2000.0, 9.0};
    std::vector<std::size_t> regions = {1, 0, 1, 2, 1};
    auto results = simulate::evaluateFeature(feat, concs, regions);
    REQUIRE(results.size() == 1);
    REQUIRE(results[0] == dbl_approx(5.0));
  }

  SECTION("evaluateFeature: empty quantile regions return zero") {
    simulate::FeatureDefinition feat;
    feat.name = "empty_quantile";
    feat.compartmentId = "comp";
    feat.speciesId = "A";
    feat.roi.roiType = simulate::RoiType::Image;
    feat.roi.numRegions = 2;
    feat.reduction = simulate::ReductionOp::Median;

    std::vector<double> concs = {2.0, 4.0, 6.0};
    std::vector<std::size_t> regions = {1, 1, 1};
    auto results = simulate::evaluateFeature(feat, concs, regions);
    REQUIRE(results.size() == 2);
    REQUIRE(results[0] == dbl_approx(4.0));
    REQUIRE(results[1] == dbl_approx(0.0));
  }
}
