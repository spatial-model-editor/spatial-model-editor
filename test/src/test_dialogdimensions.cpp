#include "dialogdimensions.hpp"

#include <QtTest>

#include "catch.hpp"
#include "qt_test_utils.hpp"

#include "logger.hpp"

constexpr int mouseDelay = 50;

// osx CI tests have issues with key presses & modal dialogs
// for now commenting out this test on osx
#ifndef Q_OS_MACOS
SCENARIO("set dimensions", "[dialogdimensions][gui]") {
  GIVEN("100x50 image, initial pixel size 1") {
    DialogDimensions dim(QSize(100, 50), 1.0);
    REQUIRE(dim.getPixelWidth() == dbl_approx(1.0));
    ModalWidgetTimer mwt;
    WHEN("user sets width to 1, pixel size -> 0.01") {
      mwt.setMessage("1");
      mwt.start();
      dim.exec();
      REQUIRE(dim.getPixelWidth() == dbl_approx(0.01));
      REQUIRE(dim.resizeCompartments() == true);
    }
    WHEN("user sets width to 1e-8, pixel size -> 1e-10") {
      mwt.setMessage("1e-8");
      mwt.start();
      dim.exec();
      REQUIRE(dim.getPixelWidth() == dbl_approx(1e-10));
      REQUIRE(dim.resizeCompartments() == true);
    }
    WHEN("user sets height to 10, pixel size -> 0.2") {
      mwt.setKeySeq({"Tab", "1", "0"});
      mwt.start();
      dim.exec();
      REQUIRE(dim.getPixelWidth() == dbl_approx(0.2));
      REQUIRE(dim.resizeCompartments() == true);
    }
    WHEN("user toggles resizeCompartments checkbox") {
      mwt.setKeySeq({"Tab", "Tab", "Tab", "Tab", "Space"});
      mwt.start();
      dim.exec();
      REQUIRE(dim.getPixelWidth() == dbl_approx(1));
      REQUIRE(dim.resizeCompartments() == false);
    }
  }
}
#endif
