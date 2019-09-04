// SBML document wrapper
// - uses libSBML to read/write SBML document
// - provides the contents in Qt containers for display
// - augments the model with spatial information
// - keeps track of geometry, membranes, mesh, colours, etc

#pragma once

#include <QColor>
#include <QImage>
#include <QStringList>

#include "sbml/SBMLTypes.h"
#include "sbml/extension/SBMLDocumentPlugin.h"
#include "sbml/packages/spatial/common/SpatialExtensionTypes.h"
#include "sbml/packages/spatial/extension/SpatialExtension.h"

#include "geometry.hpp"
#include "mesh.hpp"

namespace sbml {

class SbmlDocWrapper {
 private:
  // the SBML document
  std::unique_ptr<libsbml::SBMLDocument> doc;
  // names of additional custom annotations that we add
  // to the ParametricGeometry object
  inline static const std::string annotationURI =
      "https://github.com/lkeegan/spatial-model-editor";
  inline static const std::string annotationPrefix = "SpatialModelEditor";

  // some useful pointers
  libsbml::Model *model = nullptr;
  libsbml::SpatialModelPlugin *plugin = nullptr;
  libsbml::Geometry *geom = nullptr;
  libsbml::SampledFieldGeometry *sfgeom = nullptr;
  libsbml::ParametricGeometry *parageom = nullptr;

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

  // call before importing new SBML model
  void clearAllModelData();
  // call before importing new compartment geometry image
  void clearAllGeometryData();

  // import existing (non-spatial) model information from SBML
  void initModelData();
  // import existing spatial information (image/mesh) from SBML
  void importSpatialData();
  void importGeometryDimensions();
  void importSampledFieldGeometry();
  void importParametricGeometry();
  // add default 2d Parametric & SampledField geometry to SBML
  void writeDefaultGeometryToSBML();

  void initMembraneColourPairs();
  void updateMembraneList();
  void updateReactionList();

  // update mesh object
  void updateMesh();
  // update SBML doc with mesh
  libsbml::ParametricObject *getParametricObject(
      const std::string &compartmentID) const;
  void writeMeshParamsAnnotation(libsbml::ParametricGeometry *parageom);
  void writeGeometryMeshToSBML();

  void writeGeometryImageToSBML();

  // return supplied math expression as string with any Function calls inlined
  // e.g. given mathExpression = "z*f(x,y)"
  // where the SBML model contains a function "f(a,b) = a*b-2"
  // it returns "z*(x*y-2)"
  std::string inlineFunctions(const std::string &mathExpression) const;

  // return supplied math expression as string with any Assignment rules
  // inlined
  std::string inlineAssignments(const std::string &mathExpression) const;

  // todo: remove this?
  double pixelWidth = 1.0;
  QPointF origin = QPointF(0, 0);

 public:
  const std::size_t nDimensions = 2;
  QString currentFilename;
  bool isValid = false;
  bool hasGeometry = false;

  // Qt data structures containing model data to pass to UI widgets
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
  mesh::Mesh mesh;

  void importSBMLFile(const std::string &filename);
  void importSBMLString(const std::string &xml);
  void exportSBMLFile(const std::string &filename);
  QString getXml();

  // compartment geometry: pixel-based image
  void importGeometryFromImage(const QImage &img, bool updateSBML = true);
  void importGeometryFromImage(const QString &filename, bool updateSBML = true);
  QString getCompartmentID(QRgb colour) const;
  QRgb getCompartmentColour(const QString &compartmentID) const;
  void setCompartmentColour(const QString &compartmentID, QRgb colour,
                            bool updateSBML = true);
  const QImage &getCompartmentImage() const;

  double getCompartmentSize(const QString &compartmentID) const;
  double getSpeciesCompartmentSize(const QString &speciesID) const;

  // compartment geometry: interiorPoints - used for mesh generation
  QPointF getCompartmentInteriorPoint(const QString &compartmentID) const;
  void setCompartmentInteriorPoint(const QString &compartmentID,
                                   const QPointF &point);

  // inter-compartment membranes
  const QImage &getMembraneImage(const QString &membraneID) const;

  // species concentrations
  void importConcentrationFromImage(const QString &speciesID,
                                    const QString &filename);
  QImage getConcentrationImage(const QString &speciesID) const;

  // species isSpatial flag
  void setIsSpatial(const QString &speciesID, bool isSpatial);
  bool getIsSpatial(const QString &speciesID) const;

  // species Diffusion constant
  void setDiffusionConstant(const QString &speciesID, double diffusionConstant);
  double getDiffusionConstant(const QString &speciesID) const;

  // species (non-spatially varying) initial concentration
  void setInitialConcentration(const QString &speciesID, double concentration);
  double getInitialConcentration(const QString &speciesID) const;

  // species Colour (not currently stored in SBML)
  void setSpeciesColour(const QString &speciesID, const QColor &colour);
  const QColor &getSpeciesColour(const QString &speciesID) const;

  // true if species is fixed throughout the simulation
  void setIsSpeciesConstant(const std::string &speciesID, bool constant);
  bool getIsSpeciesConstant(const std::string &speciesID) const;

  // true if species should be altered by Reactions
  bool isSpeciesReactive(const std::string &speciesID) const;

  // get map of name->value for all global constants
  // (this includes any constant species)
  std::map<std::string, double> getGlobalConstants() const;

  double getPixelWidth() const;
  void setPixelWidth(double width, bool resizeCompartments);

  std::string inlineExpr(const std::string &mathExpression) const;

  // temporary functions exposing SBML doc directly: to be refactored
  const libsbml::Reaction *getReaction(const QString &reactionID) const {
    return model->getReaction(reactionID.toStdString());
  }
  const libsbml::FunctionDefinition *getFunctionDefinition(
      const QString &functionID) const {
    return model->getFunctionDefinition(functionID.toStdString());
  }
  const libsbml::RateRule *getRateRule(const std::string &speciesID) const {
    return model->getRateRule(speciesID);
  }
  const libsbml::AssignmentRule *getAssignmentRule(
      const std::string &paramID) const {
    return model->getAssignmentRule(paramID);
  }
};

}  // namespace sbml
