#ifndef SBML_H
#define SBML_H

#include <QColor>
#include <QDebug>
#include <QImage>
#include <QStringList>
#include <QStringListModel>

#include <sbml/SBMLTypes.h>
#include <sbml/extension/SBMLDocumentPlugin.h>
#include <sbml/packages/spatial/common/SpatialExtensionTypes.h>
#include <sbml/packages/spatial/extension/SpatialExtension.h>

class sbmlDocWrapper {
 public:
  bool isValid = false;
  libsbml::Model *model = nullptr;
  // Qt data structures containing model data to pass to UI widgets
  QString xml;
  QStringList compartments;
  // list of species for given compartment ID
  std::map<QString, QStringList> species;
  QStringList reactions;
  QStringList functions;
  // map from speciesID to index
  std::map<std::string, std::size_t> speciesIndex;
  // vector of speciesID with above indexing
  std::vector<std::string> speciesID;

  void importSBMLFile(const std::string &filename);

  // compartment geometry
  void importGeometryFromImage(const QString &filename);
  const QImage &getCompartmentImage();
  QString getCompartmentID(QRgb colour) const;
  QRgb getCompartmentColour(const QString &compartmentID) const;
  void setCompartmentColour(const QString &compartmentID, QRgb colour);

  // return supplied expression as string with any function calls inlined
  // e.g. given mathExpression = "z*f(x,y)"
  // where the SBML model contains a function "f(a,b) = a*b-2"
  // it returns "z*(x*y-2)"
  std::string inlineFunctions(const std::string &mathExpression) const;

 private:
  std::unique_ptr<libsbml::SBMLDocument> doc;
  // map between compartment IDs and colours in compartment image
  std::map<QString, QRgb> mapCompartmentToColour;
  std::map<QRgb, QString> mapColourToCompartment;
  QImage compartmentImage;
};

#endif  // SBML_H
