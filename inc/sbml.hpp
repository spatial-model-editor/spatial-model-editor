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
#include <optional>

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
class Reaction;
class UnitDefinition;
}  // namespace libsbml

namespace mesh {
class Mesh;
}

namespace sbml {

class SbmlDocWrapper;

struct IdNameValue {
  std::string id;
  std::string name;
  double value;
};

struct Reac {
  std::string id;
  std::string name;
  std::string locationId;
  std::vector<IdNameValue> species;
  std::vector<IdNameValue> constants;
  std::string expression;
  std::vector<std::string> compartments;
};

struct Func {
  std::string id;
  std::string name;
  std::string expression;
  std::vector<std::string> arguments;
};

struct SpeciesGeometry {
  QSize compartmentImageSize;
  const std::vector<QPoint> &compartmentPoints;
  const QPointF &physicalOrigin;
  double pixelWidth;
  const units::ModelUnits &modelUnits;
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

  // map between pair of compartment Ids to corresponding Membrane Id
  std::map<std::pair<std::string, std::string>, std::string>
      mapCompartmentPairToMembrane;

  // map from reaction Ids to Reac objects
  std::map<QString, Reac> mapReactionIdToReac;

  // call before importing new compartment geometry image
  void clearAllGeometryData();

  // import existing SBML Model
  void initModelData();
  units::ModelUnits modelUnits;
  void importTimeUnitsFromSBML(int defaultUnitIndex);
  void importLengthUnitsFromSBML(int defaultUnitIndex);
  void importVolumeUnitsFromSBML(int defaultUnitIndex);
  void importAmountUnitsFromSBML(int defaultUnitIndex);
  // import existing spatial information (image/mesh) from SBML
  void importSpatialData();
  void importGeometryDimensions();
  void importSampledFieldGeometry();
  void importParametricGeometry();
  // add default 2d Parametric & SampledField geometry to SBML
  void writeDefaultGeometryToSBML();
  void createField(const QString &speciesID, const QString &compartmentID);
  void setFieldConcAnalytic(geometry::Field &field, const std::string &expr);
  void initMembraneColourPairs();
  void updateMembraneList();
  void updateReactionList();
  void updateFunctionList();
  void importCompartmentsAndSpeciesFromSBML();
  void validateAndUpgradeSBMLDoc();

  // remove initialAssignment and related things from SBML
  // (sampledField, spatialref, etc)
  void removeInitialAssignment(const std::string &speciesID);

  std::vector<QPointF> getInteriorPixelPoints() const;
  // update mesh object
  void updateMesh();
  // update SBML doc with mesh
  libsbml::ParametricObject *getParametricObject(
      const std::string &compartmentID) const;
  void writeMeshParamsAnnotation(libsbml::ParametricGeometry *parageom);
  void writeGeometryMeshToSBML();
  void writeGeometryImageToSBML();

  bool isSIdAvailable(const std::string &id) const;
  bool isSpatialIdAvailable(const std::string &id) const;

  libsbml::UnitDefinition *getOrCreateUnitDef(const std::string &Id,
                                              const std::string &defaultId);
  // return supplied math expression as string with any Function calls inlined
  // e.g. given mathExpression = "z*f(x,y)"
  // where the SBML model contains a function "f(a,b) = a*b-2"
  // it returns "z*(x*y-2)"
  std::string inlineFunctions(const std::string &mathExpression) const;

  // return supplied math expression as string with any Assignment rules
  // inlined
  std::string inlineAssignments(const std::string &mathExpression) const;

  double pixelWidth = 1.0;
  QPointF physicalOrigin = QPointF(0, 0);
  QSizeF physicalSize = QSizeF(0, 0);

 public:
  std::size_t nDimensions = 2;

  QString currentFilename;
  bool isValid = false;
  bool hasGeometryImage = false;
  bool hasValidGeometry = false;

  void setUnitsTimeIndex(int index);
  void setUnitsLengthIndex(int index);
  void setUnitsVolumeIndex(int index);
  void setUnitsAmountIndex(int index);
  const units::ModelUnits &getModelUnits() const;

  // Qt data structures containing model data to pass to UI widgets
  QStringList compartments;
  QStringList compartmentNames;
  QStringList membranes;
  QStringList membraneNames;
  // <compartment ID, list of species ID in this compartment>
  std::map<QString, QStringList> species;
  QString currentSpecies;
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
  QString getCompartmentName(const QString &compartmentID) const;
  QRgb getCompartmentColour(const QString &compartmentID) const;
  void setCompartmentColour(const QString &compartmentID, QRgb colour,
                            bool updateSBML = true);
  const QImage &getCompartmentImage() const;

  double getCompartmentSize(const QString &compartmentID) const;
  SpeciesGeometry getSpeciesGeometry(const QString &speciesID) const;
  QString getCompartmentSizeUnits(const QString &compartmentID) const;

  QString getSpeciesCompartment(const QString &speciesID) const;
  void setSpeciesCompartment(const QString &speciesID,
                             const QString &compartmentID);
  void addSpecies(const QString &speciesID, const QString &compartmentID);
  void removeSpecies(const QString &speciesID);

  // compartment geometry: interiorPoints - used for mesh generation
  std::optional<QPointF> getCompartmentInteriorPoint(
      const QString &compartmentID) const;
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
  void setSampledFieldConcentration(
      const QString &speciesID, const std::vector<double> &concentrationArray);
  std::vector<double> getSampledFieldConcentration(
      const QString &speciesID) const;

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

  void setSpeciesName(const QString &speciesID, const QString &name);
  QString getSpeciesName(const QString &speciesID) const;
  QString getReactionName(const QString &reactionID) const;

  // true if species should be altered by Reactions
  bool isSpeciesReactive(const std::string &speciesID) const;

  std::vector<IdNameValue> getGlobalConstants() const;
  // convert name to a unique alphanumeric SId
  QString nameToSId(const QString &name) const;

  double getPixelWidth() const;
  void setPixelWidth(double width);
  const QPointF &getOrigin() const;
  void setCompartmentSizeFromImage(const std::string &compartmentID);

  std::string inlineExpr(const std::string &mathExpression) const;

  std::string getRateRule(const std::string &speciesID) const;
  const Reac &getReaction(const QString &reactionID) const;
  void setReaction(const Reac &reac);
  void setReactionLocation(const QString &reactionId,
                           const QString &locationId);
  void addReaction(const QString &reactionName, const QString &locationId);
  void removeReaction(const QString &reactionID);

  Func getFunctionDefinition(const QString &functionID) const;
  void setFunctionDefinition(const Func &func);
  void addFunction(const QString &functionName);
  void removeFunction(const QString &functionID);
};

}  // namespace sbml
