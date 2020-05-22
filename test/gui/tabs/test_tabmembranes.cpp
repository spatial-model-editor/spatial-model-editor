#include <QFile>
#include <QLabel>
#include <QListWidget>

#include "catch_wrapper.hpp"
#include "qt_test_utils.hpp"
#include "sbml.hpp"
#include "tabmembranes.hpp"

SCENARIO("Membranes Tab", "[gui][tabs][membranes]") {
  sbml::SbmlDocWrapper sbmlDoc;
  auto tab = TabMembranes(sbmlDoc);
  tab.show();
  waitFor(&tab);
  // get pointers to widgets within tab
  auto *listMembranes = tab.findChild<QListWidget *>("listMembranes");
  auto *lblMembraneShape = tab.findChild<QLabel *>("lblMembraneShape");
  auto *lblMembraneLength = tab.findChild<QLabel *>("lblMembraneLength");
  auto *lblMembraneLengthUnits =
      tab.findChild<QLabel *>("lblMembraneLengthUnits");
  WHEN("very-simple-model loaded") {
    if (QFile f(":/models/very-simple-model.xml");
        f.open(QIODevice::ReadOnly)) {
      sbmlDoc.importSBMLString(f.readAll().toStdString());
    }
    tab.loadModelData();
    REQUIRE(listMembranes->count() == 2);
    REQUIRE(listMembranes->currentRow() == 0);
    // with qt5.15 do lblMembraneShape->pixmap(Qt::ReturnByValue).size() instead
    REQUIRE(lblMembraneShape->pixmap()->size() == QSize(100, 100));
    REQUIRE(lblMembraneLength->text() == "338");
    REQUIRE(lblMembraneLengthUnits->text() == "m");
    listMembranes->setFocus();
    sendKeyEvents(listMembranes, {"Down"});
    REQUIRE(listMembranes->currentRow() == 1);
    REQUIRE(lblMembraneShape->pixmap()->size() == QSize(100, 100));
    REQUIRE(lblMembraneLength->text() == "108");
    REQUIRE(lblMembraneLengthUnits->text() == "m");
  }
}
