//
// Created by acaramizaru on 7/25/23.
//

#include "catch_wrapper.hpp"
#include "qt_test_utils.hpp"

#include "qopenglmousetracker.hpp"
#include "sme/logger.hpp"
#include <sme/mesh3d.hpp>

#include "model_test_utils.hpp"
#include "qvoxelrenderer.hpp"
#include "sme/image_stack.hpp"
#include "sme/tiff.hpp"
#include "sme/utils.hpp"
#include <QDir>
#include <QImage>
#include <QPoint>

using namespace sme::test;

static const char *tags{"[gui/widgets/QOpenGLMouseTracker][gui][opengl]"};

/********************************************************************************
** w generated from reading UI file 'designercDBdml.ui'
**
** Created by: Qt User Interface Compiler version 5.15.3
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QWidget>

TEST_CASE("QOpenGLMouseTracker: OpenGL", tags) {

  QWidget *w = new QWidget();

  QHBoxLayout *horizontalLayout;
  QStackedWidget *stackedWidget;
  QWidget *page;
  QVoxelRenderer *pushButton;
  QWidget *page_2;
  QOpenGLMouseTracker *pushButton_2;

  if (w->objectName().isEmpty())
    w->setObjectName(QString::fromUtf8("w"));
  w->resize(400, 300);
  horizontalLayout = new QHBoxLayout(w);
  horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
  stackedWidget = new QStackedWidget(w);
  stackedWidget->setObjectName(QString::fromUtf8("stackedWidget"));
  page = new QWidget();
  page->setObjectName(QString::fromUtf8("page"));
  pushButton = new QVoxelRenderer(page);
  pushButton->setObjectName(QString::fromUtf8("pushButton"));
  pushButton->setGeometry(QRect(110, 70, 397, 330));
  stackedWidget->addWidget(page);
  page_2 = new QWidget();
  page_2->setObjectName(QString::fromUtf8("page_2"));
  pushButton_2 = new QOpenGLMouseTracker(page_2);
  pushButton_2->setObjectName(QString::fromUtf8("pushButton_2"));
  pushButton_2->setGeometry(QRect(90, 140, 397, 330));
  stackedWidget->addWidget(page_2);

  horizontalLayout->addWidget(stackedWidget);

  w->show();
  sme::test::createBinaryFile("geometry/3d_two_eggs_disconnected.tiff",
                              "tmp_two_eggs.tif");
  sme::common::TiffReader tiffReader(
      QDir::current().filePath("tmp_two_eggs.tif").toStdString());
  REQUIRE(tiffReader.empty() == false);
  REQUIRE(tiffReader.getErrorMessage().isEmpty());
  auto imageStack = tiffReader.getImages();
  imageStack.convertToIndexed();
  sme::common::VolumeF voxelSize(1.0, 1.0, 1.0);
  sme::common::VoxelF originPoint(0.0, 0.0, 0.0);
  auto colours = imageStack[0].colorTable();
  REQUIRE(colours.size() == 3);
  QRgb colOutside{0xff000000};
  QRgb colCell{0xff7f7f7f};
  QRgb colNucleus{0xffffffff};
  std::vector<std::size_t> maxCellVolume{3};

  sme::mesh::Mesh3d mesh3d(imageStack, maxCellVolume, voxelSize, originPoint,
                           sme::common::toStdVec(colours));

  QColor redColor = QColor(255, 0, 0);
  QColor blueColor = QColor(0, 0, 255);
  QColor blackColor = QColor(0, 0, 0);
  QColor greenColor = QColor(0, 255, 0);
  QColor yelloColor = QColor(255, 255, 0);
  pushButton->setImage(imageStack);
  wait(100);
  stackedWidget->setCurrentIndex(0);
  wait(100);
  stackedWidget->setCurrentIndex(1);
  wait(100);
  pushButton_2->SetSubMeshes(mesh3d);
  wait(5000);
  stackedWidget->setCurrentIndex(0);
  wait(5000);
  stackedWidget->setCurrentIndex(1);
  wait(5000);
  stackedWidget->setCurrentIndex(0);
  wait(5000);
  stackedWidget->setCurrentIndex(1);
  wait(50000000);

  QVoxelRenderer voxelRenderer;

  voxelRenderer.show();

  QOpenGLMouseTracker test = QOpenGLMouseTracker();

  test.setBackground(blackColor);
  test.show();

  // camera position
  test.SetCameraPosition(0, 0, -70);

  SECTION("Two disconnected eggs") {
    sme::test::createBinaryFile("geometry/3d_two_eggs_disconnected.tiff",
                                "tmp_two_eggs.tif");
    sme::common::TiffReader tiffReader(
        QDir::current().filePath("tmp_two_eggs.tif").toStdString());
    REQUIRE(tiffReader.empty() == false);
    REQUIRE(tiffReader.getErrorMessage().isEmpty());
    auto imageStack = tiffReader.getImages();
    imageStack.convertToIndexed();
    sme::common::VolumeF voxelSize(1.0, 1.0, 1.0);
    sme::common::VoxelF originPoint(0.0, 0.0, 0.0);
    voxelRenderer.setImage(imageStack);
    auto colours = imageStack[0].colorTable();
    REQUIRE(colours.size() == 3);
    QRgb colOutside{0xff000000};
    QRgb colCell{0xff7f7f7f};
    QRgb colNucleus{0xffffffff};
    std::vector<std::size_t> maxCellVolume{3};

    SECTION("All three compartments") {
      sme::mesh::Mesh3d mesh3d(imageStack, maxCellVolume, voxelSize,
                               originPoint, sme::common::toStdVec(colours));
      REQUIRE(mesh3d.isValid() == true);
      REQUIRE(mesh3d.getErrorMessage().empty());
      REQUIRE(mesh3d.getTetrahedronIndices().size() == 3);

      test.SetSubMeshes(mesh3d, {redColor, blueColor, greenColor});

      wait(100000000000);

      auto QcolorSelection = QColor(test.getColour());

      // forced windows resize and forced repainting
      test.resize(500, 500);
      // test.repaint();

      // the corner initial color should be black.
      REQUIRE(blackColor == QcolorSelection);

      // visibility test
      test.setSubmeshVisibility(0, false);
      test.repaint();
      sendMouseClick(&test, {253, 235});
      QcolorSelection = QColor(test.getColour());
      REQUIRE(blackColor == QcolorSelection);

      wait(100);

      test.setSubmeshVisibility(0, true);
      test.repaint();
      sendMouseClick(&test, {253, 235});
      QcolorSelection = QColor(test.getColour());
      REQUIRE(blackColor != QcolorSelection);

      wait(100);

      // zoom
      sendMouseWheel(&test, 1);

      // move mouse over image
      sendMouseMove(&test, {10, 44});

      // click on image
      sendMouseClick(&test, {40, 40});
      // test.repaint();

      QcolorSelection = QColor(test.getColour());

      REQUIRE(blueColor != QcolorSelection);

      wait(100);

      sendMouseClick(&test, {0, 0});

      QcolorSelection = QColor(test.getColour());

      REQUIRE(redColor == QcolorSelection);

      wait(100);

      sendMouseClick(&test, {126, 319});

      QcolorSelection = QColor(test.getColour());

      REQUIRE(yelloColor == QcolorSelection);

      wait(100);

      // reset
      sendMouseClick(&test, {412, 445});

      QcolorSelection = QColor(test.getColour());

      REQUIRE(blackColor == QcolorSelection);

      wait(100);

      // reset
      sendMouseClick(&test, {412, 445});

      QcolorSelection = QColor(test.getColour());

      REQUIRE(blackColor == QcolorSelection);

      wait(100);

      sendMouseClick(&test, {0, 0});

      QcolorSelection = QColor(test.getColour());

      REQUIRE(redColor == QcolorSelection);

      wait(100);

      sendMouseClick(&test, {331, 322});

      QcolorSelection = QColor(test.getColour());

      REQUIRE(yelloColor == QcolorSelection);

      wait(100);

      // reset
      sendMouseClick(&test, {412, 445});

      QcolorSelection = QColor(test.getColour());

      REQUIRE(blackColor == QcolorSelection);
    }
  }
}
