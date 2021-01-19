#include "catch_wrapper.hpp"
#include "model.hpp"
#include "qlabelmousetracker.hpp"
#include "qt_test_utils.hpp"
#include "tabgeometry.hpp"
#include <QDoubleSpinBox>
#include <QFile>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QSpinBox>
#include <QTabWidget>

SCENARIO("Geometry Tab", "[gui/tabs/geometry][gui/tabs][gui][geometry]") {
  sme::model::Model model;
  QLabelMouseTracker mouseTracker;
  mouseTracker.show();
  waitFor(&mouseTracker);
  QLabel statusBarMsg;
  auto tab = TabGeometry(model, &mouseTracker, &statusBarMsg);
  tab.show();
  waitFor(&tab);
  ModalWidgetTimer mwt;
  // get pointers to widgets within tab
  auto *listCompartments = tab.findChild<QListWidget *>("listCompartments");
  REQUIRE(listCompartments != nullptr);
  auto *listMembranes = tab.findChild<QListWidget *>("listMembranes");
  REQUIRE(listMembranes != nullptr);
  auto *btnAddCompartment = tab.findChild<QPushButton *>("btnAddCompartment");
  REQUIRE(btnAddCompartment != nullptr);
  auto *btnRemoveCompartment =
      tab.findChild<QPushButton *>("btnRemoveCompartment");
  REQUIRE(btnRemoveCompartment != nullptr);
  auto *txtCompartmentName = tab.findChild<QLineEdit *>("txtCompartmentName");
  REQUIRE(txtCompartmentName != nullptr);
  auto *tabCompartmentGeometry =
      tab.findChild<QTabWidget *>("tabCompartmentGeometry");
  REQUIRE(tabCompartmentGeometry != nullptr);
  auto *lblCompShape = tab.findChild<QLabelMouseTracker *>("lblCompShape");
  REQUIRE(lblCompShape != nullptr);
  auto *lblCompSize = tab.findChild<QLabel *>("lblCompSize");
  REQUIRE(lblCompSize != nullptr);
  auto *spinBoundaryIndex = tab.findChild<QSpinBox *>("spinBoundaryIndex");
  REQUIRE(spinBoundaryIndex != nullptr);
  auto *spinMaxBoundaryPoints =
      tab.findChild<QSpinBox *>("spinMaxBoundaryPoints");
  REQUIRE(spinMaxBoundaryPoints != nullptr);
  auto *spinMaxTriangleArea = tab.findChild<QSpinBox *>("spinMaxTriangleArea");
  REQUIRE(spinMaxTriangleArea != nullptr);
  WHEN("very-simple-model loaded") {
    if (QFile f(":/models/very-simple-model.xml");
        f.open(QIODevice::ReadOnly)) {
      model.importSBMLString(f.readAll().toStdString());
    }
    tab.loadModelData();
    WHEN("select some stuff") {
      REQUIRE(listMembranes->count() == 2);
      REQUIRE(listCompartments->count() == 3);

      // select compartments
      REQUIRE(listCompartments->currentItem()->text() == "Outside");
      REQUIRE(txtCompartmentName->text() == "Outside");
      REQUIRE(lblCompSize->text() == "Area: 5441 m^2 (5441 pixels)");
      REQUIRE(tabCompartmentGeometry->currentIndex() == 0);
      REQUIRE(lblCompShape->getImage().pixel(1, 1) == 0xff000200);
      // change compartment name
      txtCompartmentName->setFocus();
      sendKeyEvents(txtCompartmentName, {"X", "Enter"});
      REQUIRE(txtCompartmentName->text() == "OutsideX");
      REQUIRE(listCompartments->currentItem()->text() == "OutsideX");
      txtCompartmentName->setFocus();
      sendKeyEvents(txtCompartmentName, {"Backspace", "Enter"});
      REQUIRE(txtCompartmentName->text() == "Outside");
      REQUIRE(listCompartments->currentItem()->text() == "Outside");
      // select Cell compartment
      listCompartments->setFocus();
      sendKeyEvents(listCompartments, {"Down"});
      REQUIRE(listCompartments->currentItem()->text() == "Cell");
      REQUIRE(lblCompSize->text() == "Area: 4034 m^2 (4034 pixels)");
      REQUIRE(lblCompShape->getImage().pixel(20, 20) == 0xff9061c1);
      REQUIRE(txtCompartmentName->text() == "Cell");
      // select first membrane
      listMembranes->setFocus();
      sendKeyEvents(listMembranes, {" "});
      REQUIRE(listMembranes->currentItem()->text() == "Outside <-> Cell");
      REQUIRE(lblCompSize->text() == "Length: 338 m (338 pixels)");
      REQUIRE(txtCompartmentName->text() == "Outside <-> Cell");
      // select Nucleus compartment
      listCompartments->setFocus();
      sendKeyEvents(listCompartments, {"Down"});
      REQUIRE(listCompartments->currentItem()->text() == "Nucleus");
      REQUIRE(lblCompSize->text() == "Area: 525 m^2 (525 pixels)");
      REQUIRE(txtCompartmentName->text() == "Nucleus");
      REQUIRE(lblCompShape->getImage().pixel(50, 50) == 0xffc58560);

      // boundary tab
      tabCompartmentGeometry->setFocus();
      sendKeyEvents(tabCompartmentGeometry, {"Ctrl+Tab"});
      REQUIRE(tabCompartmentGeometry->currentIndex() == 1);
      spinBoundaryIndex->setFocus();
      sendKeyEvents(spinBoundaryIndex, {"Up"});
      sendKeyEvents(spinMaxBoundaryPoints, {"Up"});
      sendKeyEvents(spinMaxBoundaryPoints, {"Up"});
      sendKeyEvents(spinMaxBoundaryPoints, {"Up"});
      sendKeyEvents(spinBoundaryIndex, {"Up"});
      sendKeyEvents(spinMaxBoundaryPoints, {"Down"});
      sendKeyEvents(spinBoundaryIndex, {"Up"});

      // mesh tab
      tabCompartmentGeometry->setFocus();
      sendKeyEvents(tabCompartmentGeometry, {"Ctrl+Tab"});
      REQUIRE(tabCompartmentGeometry->currentIndex() == 2);
      sendKeyEvents(spinMaxTriangleArea,
                    {"End", "Backspace", "Backspace", "Backspace", "3"});

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
      sendMouseClick(btnRemoveCompartment);
      sendKeyEventsToNextQDialog({"Esc"});
      REQUIRE(listCompartments->count() == 4);
      // click remove compartment, then confirm
      sendMouseClick(btnRemoveCompartment);
      sendKeyEventsToNextQDialog({"Enter"});
      REQUIRE(listCompartments->count() == 3);
      REQUIRE(listCompartments->currentItem()->text() == "Outside");
      sendMouseClick(btnRemoveCompartment);
      sendKeyEventsToNextQDialog({"Enter"});
      REQUIRE(listCompartments->count() == 2);
      REQUIRE(listCompartments->currentItem()->text() == "Cell");
      sendMouseClick(btnRemoveCompartment);
      sendKeyEventsToNextQDialog({"Enter"});
      REQUIRE(listCompartments->count() == 1);
      REQUIRE(listCompartments->currentItem()->text() == "Nucleus");
      sendMouseClick(btnRemoveCompartment);
      sendKeyEventsToNextQDialog({"Enter"});
      REQUIRE(listCompartments->count() == 0);
    }
  }
}
