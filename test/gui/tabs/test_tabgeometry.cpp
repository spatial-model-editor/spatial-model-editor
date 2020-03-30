#include <QDoubleSpinBox>
#include <QFile>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QSpinBox>
#include <QTabWidget>

#include "catch_wrapper.hpp"
#include "qlabelmousetracker.hpp"
#include "qt_test_utils.hpp"
#include "sbml.hpp"
#include "tabgeometry.hpp"

SCENARIO("Geometry Tab", "[gui][tabs][geometry]") {
  sbml::SbmlDocWrapper sbmlDoc;
  QLabelMouseTracker mouseTracker;
  mouseTracker.show();
  waitFor(&mouseTracker);
  QLabel statusBarMsg;
  auto tab = TabGeometry(sbmlDoc, &mouseTracker, &statusBarMsg);
  tab.show();
  waitFor(&tab);
  ModalWidgetTimer mwt;
  // get pointers to widgets within tab
  auto *listCompartments = tab.findChild<QListWidget *>("listCompartments");
  auto *btnAddCompartment = tab.findChild<QPushButton *>("btnAddCompartment");
  auto *btnRemoveCompartment =
      tab.findChild<QPushButton *>("btnRemoveCompartment");
  auto *txtCompartmentName = tab.findChild<QLineEdit *>("txtCompartmentName");
  auto *tabCompartmentGeometry =
      tab.findChild<QTabWidget *>("tabCompartmentGeometry");
  auto *lblCompShape = tab.findChild<QLabelMouseTracker *>("lblCompShape");
  // auto *btnChangeCompartment = tab.findChild<QPushButton
  // *>("btnChangeCompartment");
  // auto *lblCompBoundary =
  //     tab.findChild<QLabelMouseTracker *>("lblCompBoundary");
  auto *spinBoundaryIndex = tab.findChild<QSpinBox *>("spinBoundaryIndex");
  auto *spinMaxBoundaryPoints =
      tab.findChild<QSpinBox *>("spinMaxBoundaryPoints");
  auto *spinBoundaryWidth =
      tab.findChild<QDoubleSpinBox *>("spinBoundaryWidth");
  // auto *lblCompMesh = tab.findChild<QLabelMouseTracker *>("lblCompMesh");
  auto *spinMaxTriangleArea = tab.findChild<QSpinBox *>("spinMaxTriangleArea");
  WHEN("very-simple-model loaded") {
    if (QFile f(":/models/very-simple-model.xml");
        f.open(QIODevice::ReadOnly)) {
      sbmlDoc.importSBMLString(f.readAll().toStdString());
    }
    tab.loadModelData();
    WHEN("select some stuff") {
      // select compartments
      REQUIRE(listCompartments->count() == 3);
      REQUIRE(listCompartments->currentItem()->text() == "Outside");
      REQUIRE(txtCompartmentName->text() == "Outside");
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
      listCompartments->setFocus();
      sendKeyEvents(listCompartments, {"Down"});
      REQUIRE(listCompartments->currentItem()->text() == "Cell");
      REQUIRE(lblCompShape->getImage().pixel(20, 20) == 0xff9061c1);
      REQUIRE(txtCompartmentName->text() == "Cell");
      sendKeyEvents(listCompartments, {"Down"});
      REQUIRE(listCompartments->currentItem()->text() == "Nucleus");
      REQUIRE(txtCompartmentName->text() == "Nucleus");
      REQUIRE(lblCompShape->getImage().pixel(50, 50) == 0xffc58560);

      // boundary pixels tab
      tabCompartmentGeometry->setFocus();
      sendKeyEvents(tabCompartmentGeometry, {"Ctrl+Tab"});
      REQUIRE(tabCompartmentGeometry->currentIndex() == 1);

      // boundary tab
      tabCompartmentGeometry->setFocus();
      sendKeyEvents(tabCompartmentGeometry, {"Ctrl+Tab"});
      REQUIRE(tabCompartmentGeometry->currentIndex() == 2);
      spinBoundaryIndex->setFocus();
      sendKeyEvents(spinBoundaryIndex, {"Up"});
      sendKeyEvents(spinMaxBoundaryPoints, {"Up"});
      sendKeyEvents(spinMaxBoundaryPoints, {"Up"});
      sendKeyEvents(spinMaxBoundaryPoints, {"Up"});
      sendKeyEvents(spinBoundaryIndex, {"Up"});
      sendKeyEvents(spinMaxBoundaryPoints, {"Down"});
      sendKeyEvents(spinBoundaryWidth, {"Down"});
      sendKeyEvents(spinBoundaryWidth, {"Down"});
      sendKeyEvents(spinBoundaryIndex, {"Up"});

      // mesh tab
      tabCompartmentGeometry->setFocus();
      sendKeyEvents(tabCompartmentGeometry, {"Ctrl+Tab"});
      REQUIRE(tabCompartmentGeometry->currentIndex() == 3);
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
