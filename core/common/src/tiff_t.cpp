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
    REQUIRE(tiffReader.size() == 1);
    REQUIRE(tiffReader.getErrorMessage().isEmpty());
    auto imgs = tiffReader.getImages();
    REQUIRE(imgs.volume().depth() == 1);
    REQUIRE(imgs[0].size() == QSize(100, 100));
    REQUIRE(imgs[0].pixel(0, 0) == 0xff000000);
  }
  SECTION("3x 8bit grayscale tiff") {
    QFile::copy(":/test/8bit_gray_3x.tif", "tmp8bit_gray_3x.tif");
    common::TiffReader tiffReader(
        QDir::current().filePath("tmp8bit_gray_3x.tif").toStdString());
    REQUIRE(tiffReader.size() == 3);
    REQUIRE(tiffReader.getErrorMessage().isEmpty());
    auto imgs = tiffReader.getImages();
    REQUIRE(imgs.volume().depth() == 3);
    REQUIRE(imgs[0].size() == QSize(5, 5));
    REQUIRE(imgs[0].pixel(0, 0) == 0xff000000);
    REQUIRE(imgs[1].size() == QSize(5, 5));
    REQUIRE(imgs[1].pixel(0, 0) == 0xff000000);
    REQUIRE(imgs[2].size() == QSize(5, 5));
    REQUIRE(imgs[2].pixel(0, 0) == 0xff000000);
  }
}
