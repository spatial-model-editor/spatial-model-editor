#include "model_test_utils.hpp"
#include <QFile>
#include <QString>
#include <string>

namespace sme::test {

static std::string getXml(const QString &filename) {
  QFile f(filename);
  if (!f.open(QIODevice::ReadOnly)) {
    throw std::runtime_error("sme::test::getXml() :: File" +
                             filename.toStdString() + " not found");
  }
  return f.readAll().toStdString();
}

static model::Model getModel(const QString &filename) {
  model::Model m;
  m.importSBMLString(getXml(filename));
  return m;
}

static std::unique_ptr<libsbml::SBMLDocument>
getSbmlDoc(const QString &filename) {
  return std::unique_ptr<libsbml::SBMLDocument>{
      libsbml::readSBMLFromString(getXml(filename).c_str())};
}

model::Model getExampleModel(Mod exampleModel) {
  switch (exampleModel) {
  case Mod::ABtoC:
    return getModel(":/models/ABtoC.xml");
  case Mod::Brusselator:
    return getModel(":/models/brusselator-model.xml");
  case Mod::CircadianClock:
    return getModel(":/models/circadian-clock.xml");
  case Mod::GrayScott:
    return getModel(":/models/gray-scott.xml");
  case Mod::LiverSimplified:
    return getModel(":/models/liver-simplified.xml");
  case Mod::LiverCells:
    return getModel(":/models/liver-cells.xml");
  case Mod::SingleCompartmentDiffusion:
    return getModel(":/models/single-compartment-diffusion.xml");
  case Mod::VerySimpleModel:
    return getModel(":/models/very-simple-model.xml");
  }
}

model::Model getTestModel(const QString &filename) {
  return getModel(QString(":test/models/%1.xml").arg(filename));
}

std::unique_ptr<libsbml::SBMLDocument> getTestSbmlDoc(const QString &filename) {
  return getSbmlDoc(QString(":test/models/%1.xml").arg(filename));
}

std::unique_ptr<libsbml::SBMLDocument> toSbmlDoc(model::Model &model) {
  std::string xml{model.getXml().toStdString()};
  return std::unique_ptr<libsbml::SBMLDocument>{
      libsbml::readSBMLFromString(xml.c_str())};
}

} // namespace sme::test
