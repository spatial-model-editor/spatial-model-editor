#include "catch_wrapper.hpp"
#include "model.hpp"
#include "model_test_utils.hpp"
#include "qlabelmousetracker.hpp"
#include "qt_test_utils.hpp"
#include "tabgeometry.hpp"
#include <QDoubleSpinBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QTabWidget>

using namespace sme::test;

SCENARIO("Geometry Tab", "[gui/tabs/geometry][gui/tabs][gui][geometry]") {
  sme::model::Model model;
  QLabelMouseTracker mouseTracker;
  mouseTracker.show();
  waitFor(&mouseTracker);
  QLabel statusBarMsg;
  auto tab = TabGeometry(model, &mouseTracker);
  tab.show();
  waitFor(&tab);
  ModalWidgetTimer mwt;
  // get pointers to widgets within tab
  auto *listCompartments{tab.findChild<QListWidget *>("listCompartments")};
  REQUIRE(listCompartments != nullptr);
  auto *listMembranes{tab.findChild<QListWidget *>("listMembranes")};
  REQUIRE(listMembranes != nullptr);
  auto *btnAddCompartment{tab.findChild<QPushButton *>("btnAddCompartment")};
  REQUIRE(btnAddCompartment != nullptr);
  auto *btnRemoveCompartment{
      tab.findChild<QPushButton *>("btnRemoveCompartment")};
  REQUIRE(btnRemoveCompartment != nullptr);
  auto *txtCompartmentName{tab.findChild<QLineEdit *>("txtCompartmentName")};
  REQUIRE(txtCompartmentName != nullptr);
  auto *tabCompartmentGeometry{
      tab.findChild<QTabWidget *>("tabCompartmentGeometry")};
  REQUIRE(tabCompartmentGeometry != nullptr);
  auto *lblCompShape{tab.findChild<QLabelMouseTracker *>("lblCompShape")};
  REQUIRE(lblCompShape != nullptr);
  auto *lblCompSize{tab.findChild<QLabel *>("lblCompSize")};
  REQUIRE(lblCompSize != nullptr);
  auto *lblCompBoundary{tab.findChild<QLabelMouseTracker *>("lblCompBoundary")};
  REQUIRE(lblCompBoundary != nullptr);
  auto *spinBoundaryIndex{tab.findChild<QSpinBox *>("spinBoundaryIndex")};
  REQUIRE(spinBoundaryIndex != nullptr);
  auto *spinMaxBoundaryPoints{
      tab.findChild<QSpinBox *>("spinMaxBoundaryPoints")};
  REQUIRE(spinMaxBoundaryPoints != nullptr);
  auto *spinBoundaryZoom{tab.findChild<QSpinBox *>("spinBoundaryZoom")};
  REQUIRE(spinBoundaryZoom != nullptr);
  auto *lblCompMesh{tab.findChild<QLabelMouseTracker *>("lblCompMesh")};
  REQUIRE(lblCompMesh != nullptr);
  auto *spinMaxTriangleArea{tab.findChild<QSpinBox *>("spinMaxTriangleArea")};
  REQUIRE(spinMaxTriangleArea != nullptr);
  auto *spinMeshZoom{tab.findChild<QSpinBox *>("spinMeshZoom")};
  REQUIRE(spinMeshZoom != nullptr);
  WHEN("very-simple-model loaded") {
    model = getExampleModel(Mod::VerySimpleModel);
    tab.loadModelData();
    WHEN("select some stuff") {
      REQUIRE(listMembranes->count() == 2);
      REQUIRE(listCompartments->count() == 3);

      // select compartments
      REQUIRE(listCompartments->currentItem()->text() == "Outside");
      REQUIRE(txtCompartmentName->text() == "Outside");
      REQUIRE(lblCompSize->text() == "Volume: 5441000 L (5441 pixels)");
      REQUIRE(tabCompartmentGeometry->currentIndex() == 0);
      REQUIRE(lblCompShape->getImage().pixel(1, 1) == 0xff000200);
      // change compartment name
      txtCompartmentName->setFocus();
      sendKeyEvents(txtCompartmentName, {"X", "Enter"});
      REQUIRE(txtCompartmentName->text() == "OutsideX");
      REQUIRE(listCompartments->currentItem()->text() == "OutsideX");
      txtCompartmentName->setFocus();
      sendKeyEvents(txtCompartmentName, {"Backspace", "Enter"});
      REQUIRE(txtCompartmentName->isEnabled() == true);
      REQUIRE(txtCompartmentName->text() == "Outside");
      REQUIRE(listCompartments->currentItem()->text() == "Outside");
      // select Cell compartment
      listCompartments->setFocus();
      sendKeyEvents(listCompartments, {"Down"});
      REQUIRE(listCompartments->currentItem()->text() == "Cell");
      REQUIRE(lblCompSize->text() == "Volume: 4034000 L (4034 pixels)");
      REQUIRE(lblCompShape->getImage().pixel(20, 20) == 0xff9061c1);
      REQUIRE(txtCompartmentName->isEnabled() == true);
      REQUIRE(txtCompartmentName->text() == "Cell");
      // select first membrane
      listMembranes->setFocus();
      sendKeyEvents(listMembranes, {" "});
      REQUIRE(txtCompartmentName->isEnabled() == true);
      REQUIRE(listMembranes->currentItem()->text() == "Outside <-> Cell");
      REQUIRE(lblCompSize->text() == "Area: 338 m^2 (338 pixels)");
      REQUIRE(txtCompartmentName->text() == "Outside <-> Cell");
      // change membrane name
      txtCompartmentName->setFocus();
      sendKeyEvents(txtCompartmentName, {"X", "Enter"});
      REQUIRE(txtCompartmentName->text() == "Outside <-> CellX");
      REQUIRE(listMembranes->currentItem()->text() == "Outside <-> CellX");
      txtCompartmentName->setFocus();
      sendKeyEvents(txtCompartmentName, {"Backspace", "Enter"});
      REQUIRE(txtCompartmentName->isEnabled() == true);
      REQUIRE(txtCompartmentName->text() == "Outside <-> Cell");
      REQUIRE(listMembranes->currentItem()->text() == "Outside <-> Cell");
      // select Nucleus compartment
      listCompartments->setFocus();
      sendKeyEvents(listCompartments, {"Down"});
      REQUIRE(listCompartments->currentItem()->text() == "Nucleus");
      REQUIRE(lblCompSize->text() == "Volume: 525000 L (525 pixels)");
      REQUIRE(txtCompartmentName->isEnabled() == true);
      REQUIRE(txtCompartmentName->text() == "Nucleus");
      REQUIRE(lblCompShape->getImage().pixel(50, 50) == 0xffc58560);

      // boundary tab
      tabCompartmentGeometry->setFocus();
      sendKeyEvents(tabCompartmentGeometry, {"Ctrl+Tab"});
      REQUIRE(tabCompartmentGeometry->currentIndex() == 1);
      spinBoundaryIndex->setFocus();
      REQUIRE(spinBoundaryIndex->value() == 0);
      REQUIRE(spinMaxBoundaryPoints->value() == 12);
      // set outer boundary to 3 points: results in invalid mesh due to boundary
      // line intersection
      sendKeyEvents(spinMaxBoundaryPoints, {"Ctrl+A", "3"});
      REQUIRE(spinMaxBoundaryPoints->value() == 3);
      // go to mesh tab: check invalid mesh error message is displayed
      tabCompartmentGeometry->setFocus();
      sendKeyEvents(tabCompartmentGeometry, {"Ctrl+Tab"});
      REQUIRE(lblCompMesh->text().right(41) ==
              "Unauthorized intersections of constraints");
      // go back to boundary tab, reset max number of boundary points
      tabCompartmentGeometry->setFocus();
      sendKeyEvents(tabCompartmentGeometry, {"Ctrl+Shift+Tab"});
      sendKeyEvents(spinMaxBoundaryPoints, {"Ctrl+A", "1", "2"});
      REQUIRE(spinMaxBoundaryPoints->value() == 12);

      // change selected boundary
      sendKeyEvents(spinBoundaryIndex, {"Up"});
      REQUIRE(spinBoundaryIndex->value() == 1);
      REQUIRE(spinMaxBoundaryPoints->value() == 31);
      // change max number of points using spin box
      sendKeyEvents(spinMaxBoundaryPoints, {"Up"});
      REQUIRE(spinMaxBoundaryPoints->value() == 32);
      sendKeyEvents(spinMaxBoundaryPoints, {"Up"});
      REQUIRE(spinMaxBoundaryPoints->value() == 33);
      sendKeyEvents(spinMaxBoundaryPoints, {"Up"});
      REQUIRE(spinMaxBoundaryPoints->value() == 34);
      sendKeyEvents(spinBoundaryIndex, {"Up"});
      REQUIRE(spinBoundaryIndex->value() == 2);
      REQUIRE(spinMaxBoundaryPoints->value() == 13);
      sendKeyEvents(spinMaxBoundaryPoints, {"Down"});
      REQUIRE(spinMaxBoundaryPoints->value() == 12);
      sendKeyEvents(spinBoundaryIndex, {"Up"});
      REQUIRE(spinBoundaryIndex->value() == 0);
      REQUIRE(spinMaxBoundaryPoints->value() == 12);
      // change max number of points using mouse scroll wheel
      lblCompBoundary->setFocus();
      sendMouseWheel(lblCompBoundary, +1);
      REQUIRE(spinMaxBoundaryPoints->value() == 13);
      sendMouseWheel(lblCompBoundary, +1);
      REQUIRE(spinMaxBoundaryPoints->value() == 14);
      sendMouseWheel(lblCompBoundary, -1);
      REQUIRE(spinMaxBoundaryPoints->value() == 13);
      // zoom in and out using spin box
      auto b0{lblCompBoundary->size().width()};
      sendKeyEvents(spinBoundaryZoom, {"Up"});
      REQUIRE(spinBoundaryZoom->value() == 1);
      REQUIRE(lblCompBoundary->size().width() > b0);
      auto b1{lblCompBoundary->size().width()};
      sendKeyEvents(spinBoundaryZoom, {"Up"});
      REQUIRE(spinBoundaryZoom->value() == 2);
      REQUIRE(lblCompBoundary->size().width() > b1);
      sendKeyEvents(spinBoundaryZoom, {"Down"});
      REQUIRE(spinBoundaryZoom->value() == 1);
      REQUIRE(lblCompBoundary->size().width() == b1);
      sendKeyEvents(spinBoundaryZoom, {"Down"});
      REQUIRE(spinBoundaryZoom->value() == 0);
      REQUIRE(lblCompBoundary->size().width() == b0);
      // zoom in and out using mouse scroll wheel
      lblCompBoundary->setFocus();
      sendMouseWheel(lblCompBoundary, +1, Qt::KeyboardModifier::ShiftModifier);
      REQUIRE(spinBoundaryZoom->value() == 1);
      REQUIRE(lblCompBoundary->size().width() == b1);
      sendMouseWheel(lblCompBoundary, +1, Qt::KeyboardModifier::ShiftModifier);
      REQUIRE(spinBoundaryZoom->value() == 2);
      REQUIRE(lblCompBoundary->size().width() > b1);
      sendMouseWheel(lblCompBoundary, -1, Qt::KeyboardModifier::ShiftModifier);
      REQUIRE(spinBoundaryZoom->value() == 1);
      REQUIRE(lblCompBoundary->size().width() == b1);
      sendMouseWheel(lblCompBoundary, -1, Qt::KeyboardModifier::ShiftModifier);
      REQUIRE(spinBoundaryZoom->value() == 0);
      REQUIRE(lblCompBoundary->size().width() == b0);

      // mesh tab
      tabCompartmentGeometry->setFocus();
      sendKeyEvents(tabCompartmentGeometry, {"Ctrl+Tab"});
      REQUIRE(tabCompartmentGeometry->currentIndex() == 2);
      REQUIRE(spinMaxTriangleArea->value() == 30);
      // change max area using spinbox
      sendKeyEvents(spinMaxTriangleArea,
                    {"End", "Backspace", "Backspace", "Backspace", "3"});
      REQUIRE(spinMaxTriangleArea->value() == 3);
      sendKeyEvents(spinMaxTriangleArea, {"Up"});
      REQUIRE(spinMaxTriangleArea->value() == 4);
      // change max area using mouse scroll wheel
      lblCompMesh->setFocus();
      sendMouseWheel(lblCompMesh, +1);
      REQUIRE(spinMaxTriangleArea->value() == 5);
      sendMouseWheel(lblCompMesh, -1);
      REQUIRE(spinMaxTriangleArea->value() == 4);

      // zoom in and out using spinbox
      auto w0{lblCompMesh->size().width()};
      sendKeyEvents(spinMeshZoom, {"Up"});
      REQUIRE(spinMeshZoom->value() == 1);
      REQUIRE(lblCompMesh->size().width() > w0);
      auto w1{lblCompMesh->size().width()};
      sendKeyEvents(spinMeshZoom, {"Up"});
      REQUIRE(spinMeshZoom->value() == 2);
      REQUIRE(lblCompMesh->size().width() > w1);
      sendKeyEvents(spinMeshZoom, {"Down"});
      REQUIRE(spinMeshZoom->value() == 1);
      REQUIRE(lblCompMesh->size().width() == w1);
      sendKeyEvents(spinMeshZoom, {"Down"});
      REQUIRE(spinMeshZoom->value() == 0);
      REQUIRE(lblCompMesh->size().width() == w0);
      // zoom in and out using mouse scroll wheel
      lblCompMesh->setFocus();
      sendMouseWheel(lblCompMesh, +1, Qt::KeyboardModifier::ShiftModifier);
      REQUIRE(spinMeshZoom->value() == 1);
      REQUIRE(lblCompMesh->size().width() == w1);
      sendMouseWheel(lblCompMesh, +1, Qt::KeyboardModifier::ShiftModifier);
      REQUIRE(spinMeshZoom->value() == 2);
      REQUIRE(lblCompMesh->size().width() > w1);
      sendMouseWheel(lblCompMesh, -1, Qt::KeyboardModifier::ShiftModifier);
      REQUIRE(spinMeshZoom->value() == 1);
      REQUIRE(lblCompMesh->size().width() == w1);
      sendMouseWheel(lblCompMesh, -1, Qt::KeyboardModifier::ShiftModifier);
      REQUIRE(spinMeshZoom->value() == 0);
      REQUIRE(lblCompMesh->size().width() == w0);
      // image tab
      tabCompartmentGeometry->setFocus();
      sendKeyEvents(tabCompartmentGeometry, {"Ctrl+Tab"});
      REQUIRE(tabCompartmentGeometry->currentIndex() == 0);
    }
    WHEN("add/remove compartments") {
      // add compartment
      mwt.addUserAction({"C", "o", "M", "p", "!"});
      mwt.start();
      sendMouseClick(btnAddCompartment);
      REQUIRE(listCompartments->count() == 4);
      REQUIRE(listCompartments->currentItem()->text() == "CoMp!");
      // click remove compartment, then "no" to cancel
      mwt.addUserAction({"Esc"});
      mwt.start();
      sendMouseClick(btnRemoveCompartment);
      REQUIRE(listCompartments->count() == 4);
      // click remove compartment, then confirm
      mwt.addUserAction({"Enter"});
      mwt.start();
      sendMouseClick(btnRemoveCompartment);
      REQUIRE(listCompartments->count() == 3);
      REQUIRE(listCompartments->currentItem()->text() == "Outside");
      mwt.addUserAction({"Enter"});
      mwt.start();
      sendMouseClick(btnRemoveCompartment);
      REQUIRE(listCompartments->count() == 2);
      REQUIRE(listCompartments->currentItem()->text() == "Cell");
      mwt.addUserAction({"Enter"});
      mwt.start();
      sendMouseClick(btnRemoveCompartment);
      REQUIRE(listCompartments->count() == 1);
      REQUIRE(listCompartments->currentItem()->text() == "Nucleus");
      mwt.addUserAction({"Enter"});
      mwt.start();
      sendMouseClick(btnRemoveCompartment);
      REQUIRE(listCompartments->count() == 0);
      REQUIRE(spinMeshZoom->isEnabled() == false);
      REQUIRE(spinBoundaryZoom->isEnabled() == false);
    }
  }
  WHEN("single pixel model loaded") {
    model = getTestModel("single-pixel-meshing");
    tab.loadModelData();
    // boundary tab
    tabCompartmentGeometry->setFocus();
    sendKeyEvents(tabCompartmentGeometry, {"Ctrl+Tab"});
    REQUIRE(tabCompartmentGeometry->currentIndex() == 1);
    spinBoundaryIndex->setFocus();
    REQUIRE(spinBoundaryIndex->value() == 0);
    REQUIRE(spinMaxBoundaryPoints->value() == 4);
    // set outer boundary to 3 points: results in invalid mesh due to interior
    // point lying outside of boundary lines
    sendKeyEvents(spinMaxBoundaryPoints, {"Down"});
    REQUIRE(spinMaxBoundaryPoints->value() == 3);
    // go to mesh tab: check invalid mesh error message is displayed
    tabCompartmentGeometry->setFocus();
    sendKeyEvents(tabCompartmentGeometry, {"Ctrl+Tab"});
    REQUIRE(lblCompMesh->text().right(41) ==
            "Triangle is outside of the boundary lines");
    // go back to boundary tab, reset max number of boundary points
    tabCompartmentGeometry->setFocus();
    sendKeyEvents(tabCompartmentGeometry, {"Ctrl+Shift+Tab"});
    sendKeyEvents(spinMaxBoundaryPoints, {"Up"});
    REQUIRE(spinMaxBoundaryPoints->value() == 4);
    // go back to mesh tab: check error message is gone
    tabCompartmentGeometry->setFocus();
    sendKeyEvents(tabCompartmentGeometry, {"Ctrl+Tab"});
    REQUIRE(lblCompMesh->text().isEmpty());
  }
}
