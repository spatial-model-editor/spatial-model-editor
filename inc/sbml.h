// SBML document wrapper
// - uses libSBML to read an SBML document
// - provides the contents in Qt containers for display
// - augments the model with spatial information
// - keeps track of geometry, membranes, colours, etc

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

#include "model.h"

class sbmlDocWrapper {
 public:
  bool isValid = false;
  bool hasGeometry = false;
  libsbml::Model *model = nullptr;
  // Qt data structures containing model data to pass to UI widgets
  // todo: when model structure more concretely defined, replace this with MVC
  // setup
  QStringList compartments;
  QStringList membranes;
  // <compartment ID, list of species ID in this compartment>
  std::map<QString, QStringList> species;
  QStringList reactions;
  QStringList functions;

  // compartment geometry and field of concentrations
  // Will eventually do this for each compartment
  // for now implicitly assuming first compartment for all of these:
  Geometry geom;
  Field field;
  // for above: map from speciesID to index
  std::map<std::string, std::size_t> speciesIndex;
  // for above: vector of speciesID with above indexing
  std::vector<std::string> speciesID;

  void importSBMLFile(const std::string &filename);

  // compartment geometry
  void importGeometryFromImage(const QString &filename);
  const QImage &getCompartmentImage();
  QString getCompartmentID(QRgb colour) const;
  QRgb getCompartmentColour(const QString &compartmentID) const;
  void setCompartmentColour(const QString &compartmentID, QRgb colour);

  // for each pair of different colours: map to a continuous index
  // NOTE: colours ordered by ascending numerical value
  std::map<std::pair<QRgb, QRgb>, std::size_t> mapColPairToIndex;

  // for each pair of adjacent pixels of different colour,
  // add the pair of QPoints to the vector for this pair of colours,
  // i.e. the membrane between compartments of these two colours
  // NOTE: colour pairs are ordered in ascending order
  // x-neighbours
  std::vector<std::vector<std::pair<QPoint, QPoint>>> membranePairs;
  const QImage &getMembraneImage(const QString &membraneID);
  void updateMembraneList();
  std::map<QString, QImage> mapMembraneToImage;
  // species concentrations
  void importConcentrationFromImage(const QString &speciesID,
                                    const QString &filename);
  const QImage &getConcentrationImage(const QString &speciesID);

  // return raw XML as QString
  QString getXml() const;

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
  std::map<QString, std::size_t> mapMembraneToIndex;
};

#endif  // SBML_H
