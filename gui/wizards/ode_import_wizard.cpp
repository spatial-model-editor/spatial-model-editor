#include "ode_import_wizard.hpp"
#include "ui_ode_import_wizard.h"

static QString toQStringApprox(double x) { return QString::number(x, 'g', 5); }

static QString toQStringAccurate(double x) {
  return QString::number(x, 'g', 13);
}

static QString makeDepthTableRow(const QString &compartment, double odeVolume,
                                 double pdeVolume) {
  double ratio{pdeVolume / odeVolume};
  QString hexColor{"#00aa00"};
  if (ratio > 1.1 || ratio < 0.9) {
    hexColor = "#aa0000";
  }
  return QString(
             "<tr><td><p>%1</p></td><td><p>%2</p></td><td><p>%3</p></"
             "td><td><p style=\"font-weight:600; color:%4;\">%5</p></td></tr>")
      .arg(compartment.toHtmlEscaped())
      .arg(toQStringApprox(odeVolume))
      .arg(toQStringApprox(pdeVolume))
      .arg(hexColor)
      .arg(toQStringApprox(ratio));
}

static QString makeTableHeader() {
  return R"(<html><head/><body>
             <table border="1"
                    style="font-size:large; margin-top:10px; margin-bottom:2px; margin-left:2px; margin-right:2px;"
                    align="center"
                    cellspacing="0"
                    cellpadding="10">)";
}

static QString makeDepthTable(const sme::model::Model &model) {
  const auto &compartments{model.getCompartments()};
  auto table{makeTableHeader()};
  table.push_back(
      "<tr><th><p>Compartment</p></th><th><p>ODE Volume</p></th><th><p>PDE "
      "Volume</p></th><th><p>Ratio</p></th></tr>");
  const auto &odeVolumes{compartments.getInitialCompartmentSizes()};
  for (const auto &id : compartments.getIds()) {
    double oldSize{1.0};
    if (auto old{odeVolumes.find(id.toStdString())}; old != odeVolumes.end()) {
      oldSize = old->second;
    }
    table.append(makeDepthTableRow(compartments.getName(id), oldSize,
                                   compartments.getSize(id)));
  }
  table.append("</table></body></html>");
  return table;
}

void setBestDepth(sme::model::Model &model) {
  auto oldSizes{model.getCompartments().getInitialCompartmentSizes()};
  const auto &compartments{model.getCompartments()};
  auto &geometry{model.getGeometry()};
  auto voxelSize{geometry.getVoxelSize()};
  auto initialDepth{voxelSize.depth()};
  // set depth to match first compartment volume for which we have an ode volume
  for (const auto &compartmentId : compartments.getIds()) {
    if (auto iter{oldSizes.find(compartmentId.toStdString())};
        iter != oldSizes.cend()) {
      auto odeVol{iter->second};
      auto pdeVol{compartments.getSize(compartmentId)};
      auto newDepth{initialDepth * odeVol / pdeVol};
      geometry.setVoxelSize({voxelSize.width(), voxelSize.height(), newDepth});
      return;
    }
  }
}

OdeImportWizard::OdeImportWizard(sme::model::Model &smeModel, QWidget *parent)
    : QWizard(parent), ui{std::make_unique<Ui::OdeImportWizard>()},
      model{smeModel},
      initialDepth{model.getGeometry().getVoxelSize().depth()} {
  ui->setupUi(this);
  setBestDepth(model);
  ui->lblDepthUnits->setText(model.getUnits().getLength().name);
  ui->txtDepthValue->setText(
      toQStringAccurate(model.getGeometry().getVoxelSize().depth()));
  updateDepth(ui->txtDepthValue->text());
  connect(ui->txtDepthValue, &QLineEdit::textEdited, this,
          &OdeImportWizard::updateDepth);
}

double OdeImportWizard::getInitialPixelDepth() const { return initialDepth; }

const std::vector<sme::model::ReactionRescaling> &
OdeImportWizard::getReactionRescalings() const {
  return reactionRescalings;
}

static QString makeRescalingsTable(
    const std::vector<sme::model::ReactionRescaling> &reactionRescalings) {
  auto table{makeTableHeader()};
  table.push_back("<tr><th><p>Reaction</p></th><th><p>Location</p></th><th "
                  "colspan=2><p>Rate</p></th></tr>");
  for (const auto &reactionRescaling : reactionRescalings) {
    table.push_back(
        QString("<tr><td align=center valign=middle rowspan=2><p>%1</p></td>")
            .arg(reactionRescaling.reactionName.toHtmlEscaped()));
    table.push_back(
        QString("<td align=center valign=middle rowspan=2><p>%1</p></td>")
            .arg(reactionRescaling.reactionLocation.toHtmlEscaped()));
    table.push_back(
        QString("<td align=right><p>Original:</p></td><td><p>%1</p></tr>")
            .arg(reactionRescaling.originalExpression.toHtmlEscaped()));
    table.push_back(
        QString(
            "<tr><td align=right><p>Rescaled:</p></td><td><p>%1</p></td></tr>")
            .arg(reactionRescaling.rescaledExpression.toHtmlEscaped()));
  }
  return table;
}

void OdeImportWizard::updateDepth(const QString &text) {
  bool validDouble{false};
  double depth{text.toDouble(&validDouble)};
  if (validDouble) {
    const auto vs{model.getGeometry().getVoxelSize()};
    model.getGeometry().setVoxelSize({vs.width(), vs.height(), depth});
    ui->htmlDepthVolumes->setHtml(makeDepthTable(model));
    reactionRescalings = model.getReactions().getSpatialReactionRescalings();
    ui->htmlReactionsRescalings->setHtml(
        makeRescalingsTable(reactionRescalings));
  }
}

OdeImportWizard::~OdeImportWizard() = default;
