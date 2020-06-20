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

#include "model_compartments.hpp"
#include "model_functions.hpp"
#include "model_geometry.hpp"
#include "model_math.hpp"
#include "model_membranes.hpp"
#include "model_parameters.hpp"
#include "model_reactions.hpp"
#include "model_species.hpp"
#include "model_units.hpp"

// SBML forward declarations
namespace libsbml {
class SBMLDocument;
class Model;
class Compartment;
class SpatialModelPlugin;
class Geometry;
class SampledFieldGeometry;
class ParametricGeometry;
class ParametricObject;
class Reaction;
class UnitDefinition;
} // namespace libsbml

namespace mesh {
class Mesh;
}

namespace model {

struct SpeciesGeometry {
  QSize compartmentImageSize;
  const std::vector<QPoint> &compartmentPoints;
  const QPointF &physicalOrigin;
  double pixelWidth;
  const ModelUnits &modelUnits;
};

class Model {
private:
  std::unique_ptr<libsbml::SBMLDocument> doc;
  bool isValid{false};
  QString currentFilename;

  ModelCompartments modelCompartments;
  ModelGeometry modelGeometry;
  ModelMembranes modelMembranes;
  ModelSpecies modelSpecies;
  ModelReactions modelReactions;
  ModelFunctions modelFunctions;
  ModelParameters modelParameters;
  ModelUnits modelUnits;
  ModelMath modelMath;

  void initModelData();

public:
  bool getIsValid() const;
  const QString &getCurrentFilename() const;

  void setName(const QString &name);
  QString getName() const;

  ModelCompartments &getCompartments();
  const ModelCompartments &getCompartments() const;
  ModelGeometry &getGeometry();
  const ModelGeometry &getGeometry() const;
  ModelMembranes &getMembranes();
  const ModelMembranes &getMembranes() const;
  ModelSpecies &getSpecies();
  const ModelSpecies &getSpecies() const;
  ModelReactions &getReactions();
  const ModelReactions &getReactions() const;
  ModelFunctions &getFunctions();
  const ModelFunctions &getFunctions() const;
  ModelParameters &getParameters();
  const ModelParameters &getParameters() const;
  ModelUnits &getUnits();
  const ModelUnits &getUnits() const;
  ModelMath &getMath();
  const ModelMath &getMath() const;

  explicit Model();
  Model(Model &&) noexcept = default;
  Model &operator=(Model &&) noexcept = default;
  Model &operator=(const Model &) = delete;
  Model(const Model &) = delete;
  ~Model();

  void createSBMLFile(const std::string &name);
  void importSBMLFile(const std::string &filename);
  void importSBMLString(const std::string &xml);
  void exportSBMLFile(const std::string &filename);
  QString getXml();
  void clear();

  SpeciesGeometry getSpeciesGeometry(const QString &speciesID) const;

  std::string inlineExpr(const std::string &mathExpression) const;

  std::string getRateRule(const std::string &speciesID) const;
};
} // namespace model
