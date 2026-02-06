#include "catch_wrapper.hpp"
#include "dialogimportanalyticgeometry.hpp"
#include "model_test_utils.hpp"
#include "qlabelmousetracker.hpp"
#include "qt_test_utils.hpp"
#include <QDialogButtonBox>
#include <QFile>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>

using namespace sme::test;

struct DialogImportAnalyticGeometryWidgets {
  explicit DialogImportAnalyticGeometryWidgets(
      const DialogImportAnalyticGeometry *dialog) {
    GET_DIALOG_WIDGET(QLabelMouseTracker, lblPreview);
    GET_DIALOG_WIDGET(QSlider, slideZIndex);
    GET_DIALOG_WIDGET(QSpinBox, spinResolutionX);
    GET_DIALOG_WIDGET(QSpinBox, spinResolutionY);
    GET_DIALOG_WIDGET(QSpinBox, spinResolutionZ);
    GET_DIALOG_WIDGET(QPushButton, btnResetResolution);
    GET_DIALOG_WIDGET(QDialogButtonBox, buttonBox);
  }
  QLabelMouseTracker *lblPreview;
  QSlider *slideZIndex;
  QSpinBox *spinResolutionX;
  QSpinBox *spinResolutionY;
  QSpinBox *spinResolutionZ;
  QPushButton *btnResetResolution;
  QDialogButtonBox *buttonBox;
};

TEST_CASE("DialogImportAnalyticGeometry",
          "[gui/dialogs/importanalyticgeometry][gui/"
          "dialogs][gui][importanalyticgeometry]") {
  SECTION("2d model: z resolution locked to 1") {
    createBinaryFile("models/analytic_2d.xml", "tmp-analytic-2d.xml");
    const auto filename = QString("tmp-analytic-2d.xml");
    auto defaultSize =
        sme::model::ModelGeometry::getDefaultAnalyticGeometryImageSize(
            filename);
    REQUIRE(defaultSize.has_value());

    DialogImportAnalyticGeometry dialog(filename, *defaultSize);
    dialog.show();
    waitFor(&dialog);
    DialogImportAnalyticGeometryWidgets widgets(&dialog);

    REQUIRE(dialog.getImageSize().width() == 50);
    REQUIRE(dialog.getImageSize().height() == 50);
    REQUIRE(dialog.getImageSize().depth() == 1);
    REQUIRE(widgets.spinResolutionZ->isEnabled() == false);
    REQUIRE(widgets.spinResolutionZ->isVisible() == false);
    REQUIRE(widgets.slideZIndex->isVisible() == false);
    REQUIRE(widgets.lblPreview->getImage().volume().width() == 50);
    REQUIRE(widgets.lblPreview->getImage().volume().height() == 50);
    REQUIRE(widgets.lblPreview->getImage().volume().depth() == 1);

    widgets.spinResolutionX->setValue(40);
    widgets.spinResolutionY->setValue(20);
    widgets.spinResolutionZ->setValue(2);

    REQUIRE(dialog.getImageSize().width() == 40);
    REQUIRE(dialog.getImageSize().height() == 20);
    REQUIRE(dialog.getImageSize().depth() == 1);
    REQUIRE(widgets.lblPreview->getImage().volume().width() == 40);
    REQUIRE(widgets.lblPreview->getImage().volume().height() == 20);
    REQUIRE(widgets.lblPreview->getImage().volume().depth() == 1);

    sendMouseClick(widgets.btnResetResolution);
    REQUIRE(dialog.getImageSize().width() == 50);
    REQUIRE(dialog.getImageSize().height() == 50);
    REQUIRE(dialog.getImageSize().depth() == 1);

    QFile::remove("tmp-analytic-2d.xml");
  }

  SECTION("3d model: z resolution editable") {
    createBinaryFile("models/analytic_3d.xml", "tmp-analytic-3d.xml");
    const auto filename = QString("tmp-analytic-3d.xml");
    auto defaultSize =
        sme::model::ModelGeometry::getDefaultAnalyticGeometryImageSize(
            filename);
    REQUIRE(defaultSize.has_value());
    REQUIRE(defaultSize->depth() > 1);

    DialogImportAnalyticGeometry dialog(filename, *defaultSize);
    dialog.show();
    waitFor(&dialog);
    DialogImportAnalyticGeometryWidgets widgets(&dialog);

    REQUIRE(widgets.spinResolutionZ->isEnabled() == true);
    REQUIRE(widgets.spinResolutionZ->isVisible() == true);
    REQUIRE(widgets.slideZIndex->isVisible() == true);
    widgets.spinResolutionZ->setValue(2);
    REQUIRE(dialog.getImageSize().depth() == 2);
    REQUIRE(widgets.lblPreview->getImage().volume().depth() == 2);

    QFile::remove("tmp-analytic-3d.xml");
  }
}
