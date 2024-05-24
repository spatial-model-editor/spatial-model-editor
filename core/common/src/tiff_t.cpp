#include <QDir>
#include <QImage>
#include <QRgb>
#include <list>
#include <set>
#include <vector>

#include "catch_wrapper.hpp"
#include "sme/tiff.hpp"
#include "sme/utils.hpp"

using namespace sme;

TEST_CASE("TiffReader", "[core/common/tiff][core/common][core][tiff]") {
  SECTION("1 16bit grayscale tiff") {
    QFile::copy(":/test/16bit_gray.tif", "tmp16bit_gray.tif");
    common::TiffReader tiffReader(
        QDir::current().filePath("tmp16bit_gray.tif").toStdString());
    REQUIRE(tiffReader.empty() == false);
    REQUIRE(tiffReader.getErrorMessage().isEmpty());
    auto imgs = tiffReader.getImages();
    REQUIRE(imgs.volume().width() == 100);
    REQUIRE(imgs.volume().height() == 100);
    REQUIRE(imgs.volume().depth() == 1);
    REQUIRE(imgs[0].size() == QSize(100, 100));
    REQUIRE(imgs[0].pixel(0, 0) == 0xff000000);
  }
  SECTION("3x 8bit grayscale tiff") {
    QFile::copy(":/test/8bit_gray_3x.tif", "tmp8bit_gray_3x.tif");
    common::TiffReader tiffReader(
        QDir::current().filePath("tmp8bit_gray_3x.tif").toStdString());
    REQUIRE(tiffReader.empty() == false);
    REQUIRE(tiffReader.getErrorMessage().isEmpty());
    auto imgs = tiffReader.getImages();
    REQUIRE(imgs.volume().width() == 5);
    REQUIRE(imgs.volume().height() == 5);
    REQUIRE(imgs.volume().depth() == 3);
    REQUIRE(imgs[0].size() == QSize(5, 5));
    REQUIRE(imgs[0].pixel(0, 0) == 0xff000000);
    REQUIRE(imgs[1].size() == QSize(5, 5));
    REQUIRE(imgs[1].pixel(0, 0) == 0xff000000);
    REQUIRE(imgs[2].size() == QSize(5, 5));
    REQUIRE(imgs[2].pixel(0, 0) == 0xff000000);
  }
  SECTION("1 RGBA tiff") {
    QFile::copy(":/test/rgba_green.tif", "tmp_rgba_green.tif");
    common::TiffReader tiffReader(
        QDir::current().filePath("tmp_rgba_green.tif").toStdString());
    REQUIRE(tiffReader.empty() == false);
    REQUIRE(tiffReader.getErrorMessage().isEmpty());
    auto imgs = tiffReader.getImages();
    REQUIRE(imgs.volume().width() == 40);
    REQUIRE(imgs.volume().height() == 53);
    REQUIRE(imgs.volume().depth() == 1);
    REQUIRE(imgs[0].pixel(0, 0) == 0xff010101);
    REQUIRE(imgs[0].pixel(32, 47) == 0xff37ff01);
    auto colorTable{imgs.colorTable()};
    for (auto color : {0xff000000, 0xff010101, 0xff37ff01}) {
      CAPTURE(color);
      REQUIRE(colorTable.contains(color));
    }
    REQUIRE(colorTable.size() == 3);
  }
}
