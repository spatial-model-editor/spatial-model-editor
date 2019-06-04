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
 private:
  std::unique_ptr<libsbml::SBMLDocument> doc;

 public:
  bool isValid = false;

  libsbml::Model *model = nullptr;

  QString xml;

  // loop up given specified species ID, return index
  std::map<std::string, std::size_t> speciesIndex;
  // vector of speciesID with above indexing
  std::vector<std::string> speciesID;

  QStringList reactions;
  // list of species for given compartment ID
  std::map<QString, QStringList> species;
  QStringList compartments;
  QStringList functions;

  // map between compartment IDs and colours in compartment image
  std::map<QString, QRgb> compartment_to_colour;
  std::map<QRgb, QString> colour_to_compartment;

  // image of compartment geometry
  QImage compartment_image;

  void loadFile(const std::string &filename);
  void importGeometry(const QString &filename);

  // return supplied expr as string with function calls inlined
  // e.g. given expr = "z*f(x,y)"
  // where the SBML model contains a function "f(a,b) = a*b-2"
  // it returns "z*(x*y-2)"
  std::string inlineFunctions(const std::string &expr) const;
};

#endif  // SBML_H
