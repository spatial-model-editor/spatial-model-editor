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

static const char *getExampleFilename(Mod exampleModel) {
  switch (exampleModel) {
  case Mod::ABtoC:
    return ":/models/ABtoC.xml";
  case Mod::Brusselator:
    return ":/models/brusselator-model.xml";
  case Mod::CircadianClock:
    return ":/models/circadian-clock.xml";
  case Mod::GrayScott:
    return ":/models/gray-scott.xml";
  case Mod::LiverSimplified:
    return ":/models/liver-simplified.xml";
  case Mod::LiverCells:
    return ":/models/liver-cells.xml";
  case Mod::SingleCompartmentDiffusion:
    return ":/models/single-compartment-diffusion.xml";
  case Mod::SingleCompartmentDiffusion3D:
    return ":/models/single-compartment-diffusion-3d.xml";
  case Mod::VerySimpleModel:
    return ":/models/very-simple-model.xml";
  default:
    throw std::invalid_argument("This filename needs to be added to "
                                "model_test_utils.getExampleFilename()");
  }
}

model::Model getExampleModel(Mod exampleModel) {
  return getModel(getExampleFilename(exampleModel));
}

std::unique_ptr<libsbml::SBMLDocument> getExampleSbmlDoc(Mod exampleModel) {
  return getSbmlDoc(getExampleFilename(exampleModel));
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

void createBinaryFile(const QString &test_resource_filename,
                      const QString &output_filename) {
  QFile fIn(QString(":/test/%1").arg(test_resource_filename));
  fIn.open(QIODevice::ReadOnly);
  auto data{fIn.readAll()};
  QFile fOut(output_filename);
  fOut.open(QIODevice::WriteOnly);
  fOut.write(data);
}

} // namespace sme::test
