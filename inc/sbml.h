// SBML document wrapper
// - uses libSBML to read an SBML document
// - provides the contents in Qt containers for display
// - augments the model with spatial information
// - keeps track of geometry, membranes, colours, etc

#pragma once

#include <QColor>
#include <QDebug>
#include <QImage>
#include <QStringList>
#include <QStringListModel>

#include <sbml/SBMLTypes.h>
#include <sbml/extension/SBMLDocumentPlugin.h>
#include <sbml/packages/spatial/common/SpatialExtensionTypes.h>
#include <sbml/packages/spatial/extension/SpatialExtension.h>

#include "geometry.h"
#include "logger.h"

namespace sbml {

class SbmlDocWrapper {
 public:
  bool isValid = false;
  bool hasGeometry = false;
  libsbml::Model *model = nullptr;

  // Qt data structures containing model data to pass to UI widgets
  // todo: when model structure more concretely defined, replace this with MVC
  QStringList compartments;
  QStringList membranes;
  // <compartment ID, list of species ID in this compartment>
  std::map<QString, QStringList> species;
  std::map<QString, QStringList> reactions;
  QStringList functions;
  std::map<QString, QColor> mapSpeciesIdToColour;

  // spatial information
  std::map<QString, geometry::Compartment> mapCompIdToGeometry;
  std::map<QString, geometry::Field> mapSpeciesIdToField;
  std::vector<geometry::Membrane> membraneVec;

  void importSBMLFile(const std::string &filename);
  void exportSBMLFile(const std::string &filename) const;

  // compartment geometry
  void importGeometryFromImage(const QString &filename);
  const QImage &getCompartmentImage();
  QString getCompartmentID(QRgb colour) const;
  QRgb getCompartmentColour(const QString &compartmentID) const;
  void setCompartmentColour(const QString &compartmentID, QRgb colour);

  // inter-compartment membranes
  void updateMembraneList();
  const QImage &getMembraneImage(const QString &membraneID);

  void updateReactionList();

  // species concentrations
  void importConcentrationFromImage(const QString &speciesID,
                                    const QString &filename);
  const QImage &getConcentrationImage(const QString &speciesID);

  // species Diffusion constant
  void setDiffusionConstant(const QString &speciesID, double diffusionConstant);
  double getDiffusionConstant(const QString &speciesID) const;

  // species Colour
  void setSpeciesColour(const QString &speciesID, const QColor &colour);
  const QColor &getSpeciesColour(const QString &speciesID) const;

  // return raw XML as QString
  QString getXml() const;

  // returns true if species is fixed throughout the simulation
  bool isSpeciesConstant(const std::string &speciesID) const;

  // returns true if species should be altered by Reactions
  bool isSpeciesReactive(const std::string &speciesID) const;

  // return supplied math expression as string with any Function calls inlined
  // e.g. given mathExpression = "z*f(x,y)"
  // where the SBML model contains a function "f(a,b) = a*b-2"
  // it returns "z*(x*y-2)"
  std::string inlineFunctions(const std::string &mathExpression) const;

  // return supplied math expression as string with any Assignment rules inlined
  std::string inlineAssignments(const std::string &mathExpression) const;

 private:
  std::unique_ptr<libsbml::SBMLDocument> doc;
  // map between compartment IDs and colours in compartment image
  std::map<QString, QRgb> mapCompartmentToColour;
  std::map<QRgb, QString> mapColourToCompartment;
  QImage compartmentImage;

  // membrane maps
  // for each pair of different colours: map to a continuous index
  // NOTE: colours ordered by ascending numerical value
  std::map<std::pair<QRgb, QRgb>, std::size_t> mapColPairToIndex;
  // for each pair of adjacent pixels of different colour,
  // add the pair of QPoints to the vector for this pair of colours,
  // i.e. the membrane between compartments of these two colours
  // NOTE: colour pairs are ordered in ascending order
  std::vector<std::vector<std::pair<QPoint, QPoint>>> membranePairs;
  std::map<QString, std::size_t> mapMembraneToIndex;
  std::map<QString, QImage> mapMembraneToImage;
};

// a set of default colours for display purposes
class defaultSpeciesColours {
 private:
  const std::vector<QColor> colours{
      {230, 25, 75},  {60, 180, 75},   {255, 225, 25}, {0, 130, 200},
      {245, 130, 48}, {145, 30, 180},  {70, 240, 240}, {240, 50, 230},
      {210, 245, 60}, {250, 190, 190}, {0, 128, 128},  {230, 190, 255},
      {170, 110, 40}, {255, 250, 200}, {128, 0, 0},    {170, 255, 195},
      {128, 128, 0},  {255, 215, 180}, {0, 0, 128},    {128, 128, 128}};

 public:
  const QColor &operator[](std::size_t i) const {
    return colours[i % colours.size()];
  }
};

}  // namespace sbml
