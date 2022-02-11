#include "catch_wrapper.hpp"
#include "model_test_utils.hpp"
#include "ode_import_wizard.hpp"
#include <QLabel>
#include <QLineEdit>
#include <QTextEdit>
#include <QWizardPage>

using namespace sme;
using namespace sme::test;

TEST_CASE("OdeImportWizard",
          "[gui/wizards/ode_import_wizard][gui/wizards][gui]") {
  auto m{getExampleModel(Mod::VerySimpleModel)};
  OdeImportWizard wiz(m);
  // first page
  auto *wizPrerequisites{wiz.findChild<QWizardPage *>("wizPrerequisites")};
  REQUIRE(wizPrerequisites != nullptr);
  auto *lblPrerequisites{
      wizPrerequisites->findChild<QLabel *>("lblPrerequisites")};
  REQUIRE(lblPrerequisites != nullptr);
  REQUIRE(lblPrerequisites->text().size() > 100);
  // second page
  auto *wizDepth{wiz.findChild<QWizardPage *>("wizDepth")};
  REQUIRE(wizDepth != nullptr);
  auto *txtDepthValue{wizDepth->findChild<QLineEdit *>("txtDepthValue")};
  REQUIRE(txtDepthValue != nullptr);
  auto *lblDepthUnits{wizDepth->findChild<QLabel *>("lblDepthUnits")};
  REQUIRE(lblDepthUnits != nullptr);
  auto *htmlDepthVolumes{wizDepth->findChild<QTextEdit *>("htmlDepthVolumes")};
  REQUIRE(htmlDepthVolumes != nullptr);
  // last page
  auto *wizReactions{wiz.findChild<QWizardPage *>("wizReactions")};
  REQUIRE(wizReactions != nullptr);
  auto *htmlReactionsRescalings{
      wizReactions->findChild<QTextEdit *>("htmlReactionsRescalings")};
  REQUIRE(htmlReactionsRescalings != nullptr);
}
