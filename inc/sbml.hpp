// SBML document wrapper
// - uses libSBML to read/write SBML document
// - provides the contents in Qt containers for display
// - augments the model with spatial information
// - keeps track of geometry, membranes, mesh, colours, etc

#pragma once

#include <QColor>
#include <QImage>
#include <QStringList>
#include <memory>

#include "geometry.hpp"
#include "units.hpp"

// SBML forward declarations
namespace libsbml {
class SBMLDocument;
class Model;
class SpatialModelPlugin;
class Geometry;
class SampledFieldGeometry;
class ParametricGeometry;
class ParametricObject;
}  // namespace libsbml

namespace mesh {
class Mesh;
}

namespace sbml {

using NameDoublePair = std::pair<std::string, double>;

class Reac {
 public:
  std::string ID;
  std::string expression;
  std::vector<NameDoublePair> products;
  std::vector<NameDoublePair> reactants;
  std::vector<NameDoublePair> constants;
};

class Func {
 public:
  std::string ID;
  std::string expression;
  std::vector<std::string> arguments;
};

class SbmlDocWrapper {
 private:
  // the SBML document
  std::shared_ptr<libsbml::SBMLDocument> doc;
  // names of additional custom annotations that we add
  // to the ParametricGeometry object
  inline static const std::string annotationURI =
      "https://github.com/lkeegan/spatial-model-editor";
  inline static const std::string annotationPrefix = "spatialModelEditor";

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
  std::map<QString, QColor> mapSpeciesIdToColour;

  // membrane maps
  // for each pair of different colours: map to a continuous index
  // NOTE: colours ordered by ascending numerical value
  std::map<std::pair<QRgb, QRgb>, std::size_t> mapColPairToIndex;
  std::vector<std::pair<std::string, std::pair<QRgb, QRgb>>>
      vecMembraneColourPairs;
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
  void importTimeUnitsFromSBML(int defaultUnitIndex = 0);
  void importLengthUnitsFromSBML(int defaultUnitIndex = 0);
  void importVolumeUnitsFromSBML(int defaultUnitIndex = 0);
  void importAmountUnitsFromSBML(int defaultUnitIndex = 0);
  // import existing spatial information (image/mesh) from SBML
  void importSpatialData();
  void importGeometryDimensions();
  void importSampledFieldGeometry();
  void importParametricGeometry();
  // add default 2d Parametric & SampledField geometry to SBML
  void writeDefaultGeometryToSBML();

  void setFieldConcAnalytic(geometry::Field &field, const std::string &expr);

  void initMembraneColourPairs();
  void updateMembraneList();
  void updateReactionList();
  void updateFunctionList();

  // remove initialAssignment and related things from SBML
  // (sampledField, spatialref, etc)
  void removeInitialAssignment(const std::string &speciesID);

  // update mesh object
  void updateMesh();
  // update SBML doc with mesh
  libsbml::ParametricObject *getParametricObject(
      const std::string &compartmentID) const;
  void writeMeshParamsAnnotation(libsbml::ParametricGeometry *parageom);
  void writeGeometryMeshToSBML();

  void writeGeometryImageToSBML();

  void setSBMLCompartmentSizeUnit(int metreExponent);

  // return supplied math expression as string with any Function calls inlined
  // e.g. given mathExpression = "z*f(x,y)"
  // where the SBML model contains a function "f(a,b) = a*b-2"
  // it returns "z*(x*y-2)"
  std::string inlineFunctions(const std::string &mathExpression) const;

  // return supplied math expression as string with any Assignment rules
  // inlined
  std::string inlineAssignments(const std::string &mathExpression) const;

  double pixelWidth = 1.0;
  QPointF origin = QPointF(0, 0);

 public:
  std::size_t nDimensions = 2;
  units::ModelUnits modelUnits;

  QString currentFilename;
  bool isValid = false;
  bool hasGeometryImage = false;
  bool hasValidGeometry = false;

  void setUnitsTimeIndex(int index);
  void setUnitsLengthIndex(int index);
  void setUnitsVolumeIndex(int index);
  void setUnitsAmountIndex(int index);

  // Qt data structures containing model data to pass to UI widgets
  QStringList compartments;
  QStringList membranes;
  // <compartment ID, list of species ID in this compartment>
  std::map<QString, QStringList> species;
  std::map<QString, QStringList> reactions;
  QStringList functions;

  // spatial information
  std::map<QString, geometry::Compartment> mapCompIdToGeometry;
  std::map<QString, geometry::Field> mapSpeciesIdToField;
  std::vector<geometry::Membrane> membraneVec;
  std::shared_ptr<mesh::Mesh> mesh;

  explicit SbmlDocWrapper();

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
  QString getCompartmentSizeUnits(const QString &compartmentID) const;

  // compartment geometry: interiorPoints - used for mesh generation
  QPointF getCompartmentInteriorPoint(const QString &compartmentID) const;
  void setCompartmentInteriorPoint(const QString &compartmentID,
                                   const QPointF &point);

  // inter-compartment membranes
  const QImage &getMembraneImage(const QString &membraneID) const;

  // species concentrations
  void setAnalyticConcentration(const QString &speciesID,
                                const QString &analyticExpression);
  QString getAnalyticConcentration(const QString &speciesID) const;

  std::string getSpeciesSampledFieldInitialAssignment(
      const std::string &speciesID) const;
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
  void setPixelWidth(double width);
  void setCompartmentSizeFromImage(const std::string &compartmentID);

  std::string inlineExpr(const std::string &mathExpression) const;

  std::string getRateRule(const std::string &speciesID) const;
  Reac getReaction(const QString &reactionID) const;
  Func getFunctionDefinition(const QString &functionID) const;
};

}  // namespace sbml
