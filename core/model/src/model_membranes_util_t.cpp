#include "catch_wrapper.hpp"
#include "sme/model_membranes_util.hpp"
#include <QImage>

using namespace sme;

TEST_CASE("SBML membranes utils",
          "[core/model/membranes][core/model][core][model][membranes]") {
  SECTION("OrderedIntPairIndex") {
    // max key value 1: 1 possible ordered pair: (0,1)
    model::OrderedIntPairIndex ix(1);
    REQUIRE(ix.size() == 0);
    REQUIRE(ix.find(0, 1).has_value() == false);
    REQUIRE(ix.findOrInsert(0, 1) == 0);
    REQUIRE(ix.find(0, 1).value() == 0);
    REQUIRE(ix.size() == 1);
    REQUIRE_THROWS(ix.findOrInsert(1, 0));
    REQUIRE_THROWS(ix.findOrInsert(0, 0));
    REQUIRE(ix.size() == 1);
    REQUIRE(ix.findOrInsert(0, 1) == 0);
    REQUIRE(ix.size() == 1);
    REQUIRE_THROWS(ix.find(1, 0));
    REQUIRE_THROWS(ix.find(0, 0));
    REQUIRE_THROWS(ix.find(0, 2));

    // max 3: 6 possible pairs
    ix = model::OrderedIntPairIndex{3};
    REQUIRE(ix.size() == 0);
    REQUIRE(ix.find(0, 1).has_value() == false);
    REQUIRE(ix.findOrInsert(0, 1) == 0);
    REQUIRE(ix.size() == 1);
    REQUIRE(ix.find(2, 3).has_value() == false);
    REQUIRE(ix.findOrInsert(2, 3) == 1);
    REQUIRE(ix.size() == 2);
    REQUIRE(ix.find(1, 3).has_value() == false);
    REQUIRE(ix.findOrInsert(1, 3) == 2);
    REQUIRE(ix.size() == 3);
    REQUIRE(ix.find(0, 1).value() == 0);
    REQUIRE(ix.findOrInsert(0, 1) == 0);
    REQUIRE(ix.size() == 3);
    REQUIRE(ix.find(0, 2).has_value() == false);
    REQUIRE(ix.findOrInsert(0, 2) == 3);
    REQUIRE(ix.size() == 4);
    REQUIRE(ix.find(0, 3).has_value() == false);
    REQUIRE(ix.findOrInsert(0, 3) == 4);
    REQUIRE(ix.size() == 5);
    REQUIRE(ix.find(1, 2).has_value() == false);
    REQUIRE(ix.findOrInsert(1, 2) == 5);
    REQUIRE(ix.size() == 6);
    REQUIRE_THROWS(ix.findOrInsert(1, 0));
    REQUIRE_THROWS(ix.findOrInsert(0, 4));
    REQUIRE(ix.size() == 6);
    REQUIRE_THROWS(ix.find(1, 0));
    REQUIRE_THROWS(ix.find(2, 2));
    REQUIRE_THROWS(ix.find(0, 4));
    REQUIRE(ix.find(0, 1).value() == 0);
    REQUIRE(ix.find(0, 2).value() == 3);
    REQUIRE(ix.find(0, 3).value() == 4);
    REQUIRE(ix.find(1, 2).value() == 5);
    REQUIRE(ix.find(1, 3).value() == 2);
    REQUIRE(ix.find(2, 3).value() == 1);
  }
  SECTION("ImageMembranePixels") {
    SECTION("No image") {
      model::ImageMembranePixels imp;
      REQUIRE(imp.getImageSize() == sme::common::Volume{0, 0, 0});
    }
    SECTION("Single color") {
      // 5x4 image with 1 color
      QImage img(3, 3, QImage::Format_RGB32);
      QRgb col0 = qRgb(0, 0, 0);
      img.fill(col0);
      common::ImageStack images({img});
      images.convertToIndexed();
      model::ImageMembranePixels imp(images);
      REQUIRE(imp.getImageSize().width() == img.width());
      REQUIRE(imp.getImageSize().height() == img.height());
      REQUIRE(imp.getImageSize().depth() == 1);
      REQUIRE(imp.getColorIndex(col0) == 0);
    }
    SECTION("4 colors") {
      QImage img(3, 3, QImage::Format_RGB32);
      QRgb col0 = qRgb(0, 0, 0);
      QRgb col1 = qRgb(123, 0, 0);
      QRgb col2 = qRgb(0, 55, 0);
      QRgb col3 = qRgb(0, 0, 99);
      img.fill(col0);
      img.setPixel(0, 0, col1);
      img.setPixel(0, 1, col2);
      img.setPixel(1, 2, col3);
      // 1 0 0
      // 2 0 0
      // 0 3 0
      model::ImageMembranePixels imp;
      common::ImageStack images({img});
      images.convertToIndexed();
      imp.setImages(images);
      REQUIRE(imp.getImageSize().width() == img.width());
      REQUIRE(imp.getImageSize().height() == img.height());
      REQUIRE(imp.getImageSize().depth() == 1);
      auto i0 = imp.getColorIndex(col0);
      auto i1 = imp.getColorIndex(col1);
      auto i2 = imp.getColorIndex(col2);
      auto i3 = imp.getColorIndex(col3);
      const auto *p01 = imp.getVoxels(i1, i0);
      REQUIRE_THROWS(imp.getVoxels(i0, i1));
      REQUIRE(p01->size() == 1);
      REQUIRE(p01->front() == std::pair{sme::common::Voxel(0, 0, 0),
                                        sme::common::Voxel(1, 0, 0)});
      const auto *p02 = imp.getVoxels(i0, i2);
      auto p02correct =
          std::vector<std::pair<sme::common::Voxel, sme::common::Voxel>>{
              {{1, 1, 0}, {0, 1, 0}}, {{0, 2, 0}, {0, 1, 0}}};
      REQUIRE(p02->size() == 2);
      REQUIRE(
          std::is_permutation(p02->cbegin(), p02->cend(), p02correct.cbegin()));
      const auto *p03 = imp.getVoxels(i0, i3);
      auto p03correct =
          std::vector<std::pair<sme::common::Voxel, sme::common::Voxel>>{
              {{0, 2, 0}, {1, 2, 0}},
              {{1, 1, 0}, {1, 2, 0}},
              {{2, 2, 0}, {1, 2, 0}}};
      REQUIRE(p03->size() == 3);
      REQUIRE(
          std::is_permutation(p03->cbegin(), p03->cend(), p03correct.cbegin()));
      const auto *p12 = imp.getVoxels(i1, i2);
      REQUIRE(p12->size() == 1);
      REQUIRE(p12->front() == std::pair{sme::common::Voxel(0, 0, 0),
                                        sme::common::Voxel(0, 1, 0)});
      REQUIRE(imp.getVoxels(i1, i3) == nullptr);
      REQUIRE_THROWS(imp.getVoxels(i3, i1));
      REQUIRE(imp.getVoxels(i2, i3) == nullptr);
      REQUIRE_THROWS(imp.getVoxels(i3, i2));
    }
  }
}
